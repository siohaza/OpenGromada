#include "platform/store.h"

#include "game/data_version.h"
#include "platform/ini.h"
#include "platform/paths.h"
#include "platform/store_steam.h"
#include "platform/timing.h"

#include <algorithm>
#include <string>
#include <vector>

namespace
{

constexpr unsigned int STORE_FLUSH_INTERVAL_MS = 10000;

constexpr const char* KNOWN_STATS[] = {
	"StatNumKilled",
	"StatNumDestroyBox",
	"StatNumDestroyBarrel",
	"StatNumCollectMoney",
};
constexpr const char* KNOWN_ACHIEVEMENTS[] = {
	"ACH_KILL_MONSTER_0",
	"ACH_KILL_MONSTER_1",
	"ACH_KILL_MONSTER_2",
	"ACH_BOX_0",
	"ACH_BOX_1",
	"ACH_BOX_2",
	"ACH_BARRELS_0",
	"ACH_BARRELS_1",
	"ACH_BARRELS_2",
	"ACH_MONEY_0",
	"ACH_MONEY_1",
	"ACH_MONEY_2",
	"ACH_SCORE_SUR_0",
	"ACH_SCORE_SUR_1",
	"ACH_SCORE_SUR_2",
	"ACH_ALL_WEAPONS",
	"ACH_ALL_IMPLANTS",
};
constexpr int MAX_LEADERBOARDS = 32;
constexpr int MAX_LEADERBOARD_ROWS = 64;

bool g_steamAlive = false;
bool g_attachTried = false;
bool g_statsDirty = false;
bool g_mirrorDirty = false;
unsigned int g_lastFlushTicks = 0;
bool g_rankFromSteam = false;
bool g_entriesFromSteam = false;

std::vector<std::string> g_boards;
std::vector<std::string> g_entryNames;
std::vector<int> g_entryScores;
int g_lastUploadRank = -1;

INI_FILE& Mirror()
{
	static INI_FILE ini;
	static bool loaded = false;
	if (!loaded) {
		loaded = true;
		ini.Load((std::string(Platform_PrefPath()) + "saves/stats.ini").c_str());
	}
	return ini;
}

INI_FILE& Boards()
{
	static INI_FILE ini;
	static bool loaded = false;
	if (!loaded) {
		loaded = true;
		ini.Load((std::string(Platform_PrefPath()) + "saves/leaderboards.ini").c_str());
	}
	return ini;
}

void FlushMirror()
{
	if (g_mirrorDirty) {
		Mirror().Save();
		g_mirrorDirty = false;
	}
}

void FlushStats(bool p_force)
{
	const unsigned int now = Platform_Ticks();
	if (!p_force && now - g_lastFlushTicks < STORE_FLUSH_INTERVAL_MS) {
		return;
	}
	g_lastFlushTicks = now;
	FlushMirror();
	if (g_statsDirty && g_steamAlive) {
		SteamStore_StoreStats();
		g_statsDirty = false;
	}
}

void MergeStatWithSteam(const char* p_id, bool* p_pushed)
{
	int steamValue = 0;
	if (!SteamStore_GetStat(p_id, &steamValue)) {
		return;
	}
	const int local = Mirror().GetInt("stats", p_id, 0);
	if (steamValue > local) {
		Mirror().SetInt("stats", p_id, steamValue);
		g_mirrorDirty = true;
	}
	else if (local > steamValue && SteamStore_SetStat(p_id, local)) {
		*p_pushed = true;
	}
}

void MergeAchievementWithSteam(const char* p_id, bool* p_pushed)
{
	bool achieved = false;
	if (!SteamStore_GetAchievement(p_id, &achieved)) {
		return;
	}
	if (achieved) {
		if (!Mirror().GetInt("achievements", p_id, 0)) {
			Mirror().SetInt("achievements", p_id, 1);
			g_mirrorDirty = true;
		}
	}
	else if (Mirror().GetInt("achievements", p_id, 0) && SteamStore_SetAchievement(p_id)) {
		*p_pushed = true;
	}
}

// Stats and achievements are synchronously valid right after SteamAPI init,
// so the whole two-way reconciliation happens once at attach time.
void MergeWithSteam()
{
	bool pushed = false;
	for (const char* id : KNOWN_STATS) {
		MergeStatWithSteam(id, &pushed);
	}
	for (int i = 0; const char* key = Mirror().KeyAt("stats", i); ++i) {
		MergeStatWithSteam(key, &pushed);
	}
	for (const char* id : KNOWN_ACHIEVEMENTS) {
		MergeAchievementWithSteam(id, &pushed);
	}
	for (int i = 0; const char* key = Mirror().KeyAt("achievements", i); ++i) {
		MergeAchievementWithSteam(key, &pushed);
	}
	if (pushed) {
		SteamStore_StoreStats();
	}
	FlushMirror();
}

bool BoardRegistered(const char* p_name)
{
	for (const std::string& board : g_boards) {
		if (board == p_name) {
			return true;
		}
	}
	return false;
}

struct BOARD_ROW {
	std::string m_name;
	int m_score;
};

std::vector<BOARD_ROW> LoadBoard(const char* p_board)
{
	std::vector<BOARD_ROW> rows;
	for (int i = 0; i < MAX_LEADERBOARD_ROWS; ++i) {
		const std::string nameKey = "Name" + std::to_string(i);
		const char* name = Boards().Get(p_board, nameKey.c_str());
		if (!name) {
			break;
		}
		const std::string scoreKey = "Score" + std::to_string(i);
		rows.push_back(BOARD_ROW{name, Boards().GetInt(p_board, scoreKey.c_str(), 0)});
	}
	return rows;
}

void SaveBoard(const char* p_board, const std::vector<BOARD_ROW>& p_rows)
{
	for (int i = 0; i < MAX_LEADERBOARD_ROWS; ++i) {
		const std::string nameKey = "Name" + std::to_string(i);
		const std::string scoreKey = "Score" + std::to_string(i);
		if (i < (int) p_rows.size()) {
			Boards().Set(p_board, nameKey.c_str(), p_rows[i].m_name.c_str());
			Boards().SetInt(p_board, scoreKey.c_str(), p_rows[i].m_score);
		}
		else {
			Boards().Erase(p_board, nameKey.c_str());
			Boards().Erase(p_board, scoreKey.c_str());
		}
	}
	Boards().Save();
}

} // namespace

void Platform_StoreInit()
{
	g_lastFlushTicks = Platform_Ticks();
}

void Platform_StoreShutdown()
{
	FlushStats(true);
	SteamStore_Shutdown();
	g_steamAlive = false;
}

void Platform_StorePump()
{
	if (!g_attachTried && GameData_IsSteam()) {
		g_attachTried = true;
		g_steamAlive = SteamStore_Attach();
		if (g_steamAlive) {
			MergeWithSteam();
		}
	}
	if (g_steamAlive) {
		SteamStore_RunCallbacks();
	}
	FlushStats(false);
}

bool Platform_StoreAlive()
{
	return g_steamAlive;
}

void Platform_StoreSetAchievement(const char* p_id)
{
	Mirror().SetInt("achievements", p_id, 1);
	g_mirrorDirty = true;
	FlushMirror();
	if (g_steamAlive && SteamStore_SetAchievement(p_id)) {
		SteamStore_StoreStats();
		g_statsDirty = false;
	}
}

int Platform_StoreGetAchievement(const char* p_id)
{
	return Mirror().GetInt("achievements", p_id, 0);
}

void Platform_StoreClearAchievement(const char* p_id)
{
	Mirror().SetInt("achievements", p_id, 0);
	g_mirrorDirty = true;
	g_statsDirty = true;
	if (g_steamAlive) {
		SteamStore_ClearAchievement(p_id);
	}
}

void Platform_StoreResetAllStats()
{
	Mirror().EraseSection("stats");
	Mirror().EraseSection("achievements");
	g_mirrorDirty = true;
	if (g_steamAlive) {
		SteamStore_ResetAllStats();
	}
}

void Platform_StoreSetStat(const char* p_id, int p_value)
{
	if (p_value <= Mirror().GetInt("stats", p_id, 0)) {
		return;
	}
	Mirror().SetInt("stats", p_id, p_value);
	g_mirrorDirty = true;
	g_statsDirty = true;
	if (g_steamAlive) {
		SteamStore_SetStat(p_id, p_value);
	}
}

int Platform_StoreGetStat(const char* p_id)
{
	return Mirror().GetInt("stats", p_id, 0);
}

void Platform_StoreSaveStatsIfNeeded()
{
	if (g_statsDirty || g_mirrorDirty) {
		FlushStats(false);
	}
}

void Platform_StoreInitLeaderboards(const char* p_names)
{
	g_boards.clear();
	std::string names = p_names ? p_names : "";
	size_t start = 0;
	while (start <= names.size() && (int) g_boards.size() < MAX_LEADERBOARDS) {
		size_t end = names.find('#', start);
		if (end == std::string::npos) {
			end = names.size();
		}
		if (end > start) {
			g_boards.push_back(names.substr(start, end - start));
		}
		start = end + 1;
	}
	if (g_steamAlive && !g_boards.empty()) {
		std::vector<const char*> names_c;
		for (const std::string& board : g_boards) {
			names_c.push_back(board.c_str());
		}
		SteamStore_InitLeaderboards(names_c.data(), (int) names_c.size());
	}
}

void Platform_StoreUpdateLeaderboard(const char* p_name, const char* p_playerName, int p_score)
{
	if (!BoardRegistered(p_name)) {
		g_lastUploadRank = -2;
		return;
	}
	const char* player = p_playerName && *p_playerName ? p_playerName : "Player";
	std::vector<BOARD_ROW> rows = LoadBoard(p_name);
	bool found = false;
	for (BOARD_ROW& row : rows) {
		if (row.m_name == player) {
			found = true;
			row.m_score = std::max(row.m_score, p_score);
			break;
		}
	}
	if (!found) {
		rows.push_back(BOARD_ROW{player, p_score});
	}
	std::stable_sort(rows.begin(), rows.end(), [](const BOARD_ROW& a, const BOARD_ROW& b) {
		return a.m_score > b.m_score;
	});
	if ((int) rows.size() > MAX_LEADERBOARD_ROWS) {
		rows.resize(MAX_LEADERBOARD_ROWS);
	}
	SaveBoard(p_name, rows);

	g_lastUploadRank = -2;
	for (int i = 0; i < (int) rows.size(); ++i) {
		if (rows[i].m_name == player) {
			g_lastUploadRank = i + 1;
			break;
		}
	}

	g_rankFromSteam = g_steamAlive;
	if (g_steamAlive) {
		SteamStore_UploadScore(p_name, p_score);
	}
}

int Platform_StoreDownloadLeaderboardEntries(const char* p_name, int p_count, int p_offset)
{
	if (!BoardRegistered(p_name)) {
		return -2;
	}
	g_entriesFromSteam = g_steamAlive;
	if (g_entriesFromSteam) {
		return SteamStore_DownloadEntries(p_name, p_count, p_offset);
	}
	g_entryNames.clear();
	g_entryScores.clear();
	const std::vector<BOARD_ROW> rows = LoadBoard(p_name);
	const int offset = std::max(p_offset, 0);
	for (int i = offset; i < (int) rows.size() && (int) g_entryNames.size() < p_count; ++i) {
		g_entryNames.push_back(rows[i].m_name);
		g_entryScores.push_back(rows[i].m_score);
	}
	return (int) g_entryNames.size();
}

const char* Platform_StoreLeaderboardEntryName(int p_index)
{
	if (g_entriesFromSteam) {
		return SteamStore_EntryName(p_index);
	}
	if (p_index < 0 || p_index >= (int) g_entryNames.size()) {
		return "";
	}
	return g_entryNames[p_index].c_str();
}

int Platform_StoreLeaderboardEntryScore(int p_index)
{
	if (g_entriesFromSteam) {
		return SteamStore_EntryScore(p_index);
	}
	if (p_index < 0 || p_index >= (int) g_entryScores.size()) {
		return 0;
	}
	return g_entryScores[p_index];
}

int Platform_StoreLastUploadRank()
{
	if (g_rankFromSteam) {
		return SteamStore_UploadRank();
	}
	return g_lastUploadRank;
}
