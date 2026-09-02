#include "platform/store_steam.h"

#ifdef ALIEN_HAVE_STEAMWORKS

#include "platform/store.h"

#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

#define STEAM_API_NODLL
#include <steam/steam_api.h>

namespace
{

constexpr int MAX_STEAM_BOARDS = 32;

class STEAM_STORE {
public:
	void InitLeaderboards(const char* const* p_names, int p_count);
	void UploadScore(const char* p_name, int p_score);
	int UploadRank() const { return m_uploadRank; }
	int DownloadEntries(const char* p_name, int p_count, int p_offset);
	const char* EntryName(int p_index);
	int EntryScore(int p_index) const;
	void CancelCalls();

private:
	struct BOARD {
		std::string m_name;
		SteamLeaderboard_t m_handle = 0;
	};

	int BoardIndex(const char* p_name) const;
	void StartFindChain(int p_from);
	void FailPendingOps(int p_board);
	void FlushPendingOps(int p_board);
	void IssueUpload(int p_board, int p_score);
	void IssueDownload(int p_board, int p_count, int p_offset);

	void OnFindLeaderboard(LeaderboardFindResult_t* p_result, bool p_ioFailure);
	void OnUploadScore(LeaderboardScoreUploaded_t* p_result, bool p_ioFailure);
	void OnDownloadEntries(LeaderboardScoresDownloaded_t* p_result, bool p_ioFailure);

	CCallResult<STEAM_STORE, LeaderboardFindResult_t> m_findCall;
	CCallResult<STEAM_STORE, LeaderboardScoreUploaded_t> m_uploadCall;
	CCallResult<STEAM_STORE, LeaderboardScoresDownloaded_t> m_downloadCall;

	BOARD m_boards[MAX_STEAM_BOARDS];
	int m_boardCount = 0;
	int m_findIndex = -1;

	int m_uploadRank = -1;
	bool m_pendingUpload = false;
	int m_pendingUploadBoard = -1;
	int m_pendingUploadScore = 0;

	std::string m_dlName;
	int m_dlCount = 0;
	int m_dlOffset = 0;
	int m_dlState = -2;
	bool m_dlIssued = false;
	std::vector<CSteamID> m_entryIds;
	std::vector<int> m_entryScores;
	std::string m_entryNameBuffer;
};

STEAM_STORE g_steamStore;
bool g_attached = false;

int STEAM_STORE::BoardIndex(const char* p_name) const
{
	for (int i = 0; i < m_boardCount; ++i) {
		if (m_boards[i].m_name == p_name) {
			return i;
		}
	}
	return -1;
}

void STEAM_STORE::InitLeaderboards(const char* const* p_names, int p_count)
{
	m_boardCount = p_count < MAX_STEAM_BOARDS ? p_count : MAX_STEAM_BOARDS;
	for (int i = 0; i < m_boardCount; ++i) {
		m_boards[i].m_name = p_names[i];
		m_boards[i].m_handle = 0;
	}
	StartFindChain(0);
}

void STEAM_STORE::StartFindChain(int p_from)
{
	if (m_findIndex >= 0) {
		return; // a find is already in flight
	}
	for (int i = p_from < 0 ? 0 : p_from; i < m_boardCount; ++i) {
		if (!m_boards[i].m_handle && !m_boards[i].m_name.empty()) {
			m_findIndex = i;
			SteamAPICall_t call = SteamUserStats()->FindLeaderboard(m_boards[i].m_name.c_str());
			m_findCall.Set(call, this, &STEAM_STORE::OnFindLeaderboard);
			return;
		}
	}
	m_findIndex = -1;
}

void STEAM_STORE::OnFindLeaderboard(LeaderboardFindResult_t* p_result, bool p_ioFailure)
{
	const int board = m_findIndex;
	m_findIndex = -1;
	if (board < 0 || board >= m_boardCount) {
		return;
	}
	if (!p_ioFailure && p_result->m_bLeaderboardFound) {
		m_boards[board].m_handle = p_result->m_hSteamLeaderboard;
		const char* name = SteamUserStats()->GetLeaderboardName(p_result->m_hSteamLeaderboard);
		if (name && m_boards[board].m_name != name) {
			fprintf(stderr, "[steam] leaderboard name mismatch: '%s' vs '%s'\n", m_boards[board].m_name.c_str(), name);
		}
		FlushPendingOps(board);
	}
	else {
		fprintf(stderr, "[steam] leaderboard '%s' not found\n", m_boards[board].m_name.c_str());
		FailPendingOps(board);
	}
	StartFindChain(board + 1);
}

void STEAM_STORE::FailPendingOps(int p_board)
{
	if (m_pendingUpload && m_pendingUploadBoard == p_board) {
		m_pendingUpload = false;
		m_uploadRank = -2;
	}
	if (m_dlIssued && !m_dlName.empty() && BoardIndex(m_dlName.c_str()) == p_board && m_dlState == -1) {
		m_dlState = -2;
	}
}

void STEAM_STORE::FlushPendingOps(int p_board)
{
	if (m_pendingUpload && m_pendingUploadBoard == p_board) {
		m_pendingUpload = false;
		IssueUpload(p_board, m_pendingUploadScore);
	}
	if (m_dlIssued && !m_dlName.empty() && BoardIndex(m_dlName.c_str()) == p_board && m_dlState == -1) {
		IssueDownload(p_board, m_dlCount, m_dlOffset);
	}
}

void STEAM_STORE::UploadScore(const char* p_name, int p_score)
{
	const int board = BoardIndex(p_name);
	if (board < 0) {
		m_uploadRank = -2;
		return;
	}
	m_uploadRank = -1;
	if (!m_boards[board].m_handle) {
		m_pendingUpload = true;
		m_pendingUploadBoard = board;
		m_pendingUploadScore = p_score;
		StartFindChain(board);
		return;
	}
	IssueUpload(board, p_score);
}

void STEAM_STORE::IssueUpload(int p_board, int p_score)
{
	SteamAPICall_t call = SteamUserStats()->UploadLeaderboardScore(
		m_boards[p_board].m_handle,
		k_ELeaderboardUploadScoreMethodKeepBest,
		p_score,
		nullptr,
		0
	);
	m_uploadCall.Set(call, this, &STEAM_STORE::OnUploadScore);
}

void STEAM_STORE::OnUploadScore(LeaderboardScoreUploaded_t* p_result, bool p_ioFailure)
{
	if (p_ioFailure || !p_result->m_bSuccess) {
		m_uploadRank = -2;
		return;
	}
	m_uploadRank = p_result->m_nGlobalRankNew >= 1 ? p_result->m_nGlobalRankNew : -2;
}

int STEAM_STORE::DownloadEntries(const char* p_name, int p_count, int p_offset)
{
	if (m_dlIssued && m_dlName == p_name && m_dlCount == p_count && m_dlOffset == p_offset) {
		const int state = m_dlState;
		if (state == -2) {
			m_dlIssued = false;
		}
		return state;
	}

	const int board = BoardIndex(p_name);
	if (board < 0) {
		return -2;
	}
	m_dlName = p_name;
	m_dlCount = p_count;
	m_dlOffset = p_offset < 0 ? 0 : p_offset;
	m_dlState = -1;
	m_dlIssued = true;
	m_entryIds.clear();
	m_entryScores.clear();
	if (!m_boards[board].m_handle) {
		StartFindChain(board);
		return -1;
	}
	IssueDownload(board, m_dlCount, m_dlOffset);
	return -1;
}

void STEAM_STORE::IssueDownload(int p_board, int p_count, int p_offset)
{
	SteamAPICall_t call = SteamUserStats()->DownloadLeaderboardEntries(
		m_boards[p_board].m_handle,
		k_ELeaderboardDataRequestGlobal,
		p_offset + 1,
		p_offset + p_count
	);
	m_downloadCall.Set(call, this, &STEAM_STORE::OnDownloadEntries);
}

void STEAM_STORE::OnDownloadEntries(LeaderboardScoresDownloaded_t* p_result, bool p_ioFailure)
{
	if (p_ioFailure) {
		m_dlState = -2;
		return;
	}
	const int count = p_result->m_cEntryCount < m_dlCount ? p_result->m_cEntryCount : m_dlCount;
	m_entryIds.clear();
	m_entryScores.clear();
	for (int i = 0; i < count; ++i) {
		LeaderboardEntry_t entry;
		if (!SteamUserStats()
				 ->GetDownloadedLeaderboardEntry(p_result->m_hSteamLeaderboardEntries, i, &entry, nullptr, 0)) {
			break;
		}
		m_entryIds.push_back(entry.m_steamIDUser);
		m_entryScores.push_back(entry.m_nScore);
		const char* name = SteamFriends()->GetFriendPersonaName(entry.m_steamIDUser);
		if (!name || !*name) {
			SteamFriends()->RequestUserInformation(entry.m_steamIDUser, true);
		}
	}
	m_dlState = (int) m_entryIds.size();
}

const char* STEAM_STORE::EntryName(int p_index)
{
	if (p_index < 0 || p_index >= (int) m_entryIds.size()) {
		return "";
	}
	const char* name = SteamFriends()->GetFriendPersonaName(m_entryIds[p_index]);
	m_entryNameBuffer = name && *name ? name : "...";
	return m_entryNameBuffer.c_str();
}

int STEAM_STORE::EntryScore(int p_index) const
{
	if (p_index < 0 || p_index >= (int) m_entryScores.size()) {
		return 0;
	}
	return m_entryScores[p_index];
}

void STEAM_STORE::CancelCalls()
{
	m_findCall.Cancel();
	m_uploadCall.Cancel();
	m_downloadCall.Cancel();
}

void SetAppIdEnvironment()
{
	if (getenv("SteamAppId")) {
		return;
	}
	char appId[16];
	snprintf(appId, sizeof(appId), "%d", (int) ALIEN_STEAM_APPID);
#ifdef _WIN32
	_putenv_s("SteamAppId", appId);
	_putenv_s("SteamGameId", appId);
#else
	setenv("SteamAppId", appId, 0);
	setenv("SteamGameId", appId, 0);
#endif
}

} // namespace

namespace
{

void* g_steamApiLib = nullptr;

typedef ESteamAPIInitResult(S_CALLTYPE* InitFn)(const char*, SteamErrMsg*);
typedef void(S_CALLTYPE* VoidFn)();
typedef void(S_CALLTYPE* RegisterCallResultFn)(class CCallbackBase*, SteamAPICall_t);
typedef void*(S_CALLTYPE* FindOrCreateUserInterfaceFn)(HSteamUser, const char*);
typedef void*(S_CALLTYPE* ContextInitFn)(void*);
typedef HSteamUser(S_CALLTYPE* GetHSteamUserFn)();
typedef HSteamPipe(S_CALLTYPE* GetHSteamPipeFn)();

InitFn g_fpInit = nullptr;
VoidFn g_fpShutdown = nullptr;
VoidFn g_fpRunCallbacks = nullptr;
RegisterCallResultFn g_fpRegisterCallResult = nullptr;
RegisterCallResultFn g_fpUnregisterCallResult = nullptr;
FindOrCreateUserInterfaceFn g_fpFindOrCreateUserInterface = nullptr;
ContextInitFn g_fpContextInit = nullptr;
GetHSteamUserFn g_fpGetHSteamUser = nullptr;
GetHSteamPipeFn g_fpGetHSteamPipe = nullptr;

bool LoadSteamApiLibrary()
{
	if (g_steamApiLib) {
		return true;
	}
#if defined(_WIN32)
#if defined(_WIN64)
	const char* libName = "steam_api64.dll";
#else
	const char* libName = "steam_api.dll";
#endif
#elif defined(__APPLE__)
	const char* libName = "libsteam_api.dylib";
#else
	const char* libName = "libsteam_api.so";
#endif
	const char* base = SDL_GetBasePath();
	if (base) {
		const std::string beside = std::string(base) + libName;
		g_steamApiLib = SDL_LoadObject(beside.c_str());
#if defined(__APPLE__)
		if (!g_steamApiLib) {
			const std::string bundle = std::string(base) + "../MacOS/" + libName;
			g_steamApiLib = SDL_LoadObject(bundle.c_str());
		}
#endif
	}
	if (!g_steamApiLib) {
		g_steamApiLib = SDL_LoadObject(libName);
	}
	if (!g_steamApiLib) {
		fprintf(stderr, "[steam] %s not found; running without Steam\n", libName);
		return false;
	}

	struct EXPORT_BINDING {
		void** m_slot;
		const char* m_name;
	};
	const EXPORT_BINDING bindings[] = {
		{(void**) &g_fpInit, "SteamInternal_SteamAPI_Init"},
		{(void**) &g_fpShutdown, "SteamAPI_Shutdown"},
		{(void**) &g_fpRunCallbacks, "SteamAPI_RunCallbacks"},
		{(void**) &g_fpRegisterCallResult, "SteamAPI_RegisterCallResult"},
		{(void**) &g_fpUnregisterCallResult, "SteamAPI_UnregisterCallResult"},
		{(void**) &g_fpFindOrCreateUserInterface, "SteamInternal_FindOrCreateUserInterface"},
		{(void**) &g_fpContextInit, "SteamInternal_ContextInit"},
		{(void**) &g_fpGetHSteamUser, "SteamAPI_GetHSteamUser"},
		{(void**) &g_fpGetHSteamPipe, "SteamAPI_GetHSteamPipe"},
	};
	for (const EXPORT_BINDING& binding : bindings) {
		*binding.m_slot = (void*) SDL_LoadFunction((SDL_SharedObject*) g_steamApiLib, binding.m_name);
		if (!*binding.m_slot) {
			fprintf(stderr, "[steam] %s lacks %s; running without Steam\n", libName, binding.m_name);
			SDL_UnloadObject((SDL_SharedObject*) g_steamApiLib);
			g_steamApiLib = nullptr;
			return false;
		}
	}
	return true;
}

} // namespace

S_API ESteamAPIInitResult S_CALLTYPE SteamInternal_SteamAPI_Init(const char* p_versions, SteamErrMsg* p_message)
{
	return g_fpInit ? g_fpInit(p_versions, p_message) : k_ESteamAPIInitResult_NoSteamClient;
}

S_API void S_CALLTYPE SteamAPI_Shutdown()
{
	if (g_fpShutdown) {
		g_fpShutdown();
	}
}

S_API void S_CALLTYPE SteamAPI_RunCallbacks()
{
	if (g_fpRunCallbacks) {
		g_fpRunCallbacks();
	}
}

S_API void S_CALLTYPE SteamAPI_RegisterCallResult(class CCallbackBase* p_callback, SteamAPICall_t p_call)
{
	if (g_fpRegisterCallResult) {
		g_fpRegisterCallResult(p_callback, p_call);
	}
}

S_API void S_CALLTYPE SteamAPI_UnregisterCallResult(class CCallbackBase* p_callback, SteamAPICall_t p_call)
{
	if (g_fpUnregisterCallResult) {
		g_fpUnregisterCallResult(p_callback, p_call);
	}
}

S_API void* S_CALLTYPE SteamInternal_FindOrCreateUserInterface(HSteamUser p_user, const char* p_version)
{
	return g_fpFindOrCreateUserInterface ? g_fpFindOrCreateUserInterface(p_user, p_version) : nullptr;
}

S_API void* S_CALLTYPE SteamInternal_ContextInit(void* p_contextInitData)
{
	return g_fpContextInit ? g_fpContextInit(p_contextInitData) : nullptr;
}

S_API HSteamUser S_CALLTYPE SteamAPI_GetHSteamUser()
{
	return g_fpGetHSteamUser ? g_fpGetHSteamUser() : 0;
}

S_API HSteamPipe S_CALLTYPE SteamAPI_GetHSteamPipe()
{
	return g_fpGetHSteamPipe ? g_fpGetHSteamPipe() : 0;
}

bool SteamStore_Attach()
{
	if (g_attached) {
		return true;
	}
	if (!LoadSteamApiLibrary()) {
		return false;
	}
	SetAppIdEnvironment();
	SteamErrMsg message;
	const ESteamAPIInitResult result = SteamAPI_InitEx(&message);
	if (result != k_ESteamAPIInitResult_OK) {
		fprintf(stderr, "[steam] SteamAPI_InitEx failed (%d): %s\n", (int) result, message);
		return false;
	}
	g_attached = true;
	fprintf(stderr, "[steam] connected\n");
	return true;
}

void SteamStore_Shutdown()
{
	if (!g_attached) {
		return;
	}
	g_steamStore.CancelCalls();
	SteamAPI_Shutdown();
	g_attached = false;
}

void SteamStore_RunCallbacks()
{
	if (g_attached) {
		SteamAPI_RunCallbacks();
	}
}

bool SteamStore_GetStat(const char* p_id, int* p_out)
{
	return g_attached && SteamUserStats()->GetStat(p_id, (int32*) p_out);
}

bool SteamStore_SetStat(const char* p_id, int p_value)
{
	return g_attached && SteamUserStats()->SetStat(p_id, (int32) p_value);
}

bool SteamStore_GetAchievement(const char* p_id, bool* p_achieved)
{
	return g_attached && SteamUserStats()->GetAchievement(p_id, p_achieved);
}

bool SteamStore_SetAchievement(const char* p_id)
{
	return g_attached && SteamUserStats()->SetAchievement(p_id);
}

bool SteamStore_ClearAchievement(const char* p_id)
{
	return g_attached && SteamUserStats()->ClearAchievement(p_id);
}

bool SteamStore_ResetAllStats()
{
	return g_attached && SteamUserStats()->ResetAllStats(true);
}

bool SteamStore_StoreStats()
{
	return g_attached && SteamUserStats()->StoreStats();
}

void SteamStore_InitLeaderboards(const char* const* p_names, int p_count)
{
	if (g_attached) {
		g_steamStore.InitLeaderboards(p_names, p_count);
	}
}

void SteamStore_UploadScore(const char* p_name, int p_score)
{
	if (g_attached) {
		g_steamStore.UploadScore(p_name, p_score);
	}
}

int SteamStore_UploadRank()
{
	return g_attached ? g_steamStore.UploadRank() : -2;
}

int SteamStore_DownloadEntries(const char* p_name, int p_count, int p_offset)
{
	return g_attached ? g_steamStore.DownloadEntries(p_name, p_count, p_offset) : -2;
}

const char* SteamStore_EntryName(int p_index)
{
	return g_attached ? g_steamStore.EntryName(p_index) : "";
}

int SteamStore_EntryScore(int p_index)
{
	return g_attached ? g_steamStore.EntryScore(p_index) : 0;
}

#else // !ALIEN_HAVE_STEAMWORKS

bool SteamStore_Attach()
{
	return false;
}
void SteamStore_Shutdown()
{
}
void SteamStore_RunCallbacks()
{
}
bool SteamStore_GetStat(const char*, int*)
{
	return false;
}
bool SteamStore_SetStat(const char*, int)
{
	return false;
}
bool SteamStore_GetAchievement(const char*, bool*)
{
	return false;
}
bool SteamStore_SetAchievement(const char*)
{
	return false;
}
bool SteamStore_ClearAchievement(const char*)
{
	return false;
}
bool SteamStore_ResetAllStats()
{
	return false;
}
bool SteamStore_StoreStats()
{
	return false;
}
void SteamStore_InitLeaderboards(const char* const*, int)
{
}
void SteamStore_UploadScore(const char*, int)
{
}
int SteamStore_UploadRank()
{
	return -2;
}
int SteamStore_DownloadEntries(const char*, int, int)
{
	return -2;
}
const char* SteamStore_EntryName(int)
{
	return "";
}
int SteamStore_EntryScore(int)
{
	return 0;
}

#endif
