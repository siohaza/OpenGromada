#ifndef PLATFORM_STORE_STEAM_H
#define PLATFORM_STORE_STEAM_H

bool SteamStore_Attach();
void SteamStore_Shutdown();
void SteamStore_RunCallbacks();

bool SteamStore_GetStat(const char* p_id, int* p_out);
bool SteamStore_SetStat(const char* p_id, int p_value);
bool SteamStore_GetAchievement(const char* p_id, bool* p_achieved);
bool SteamStore_SetAchievement(const char* p_id);
bool SteamStore_ClearAchievement(const char* p_id);
bool SteamStore_ResetAllStats();
bool SteamStore_StoreStats();

void SteamStore_InitLeaderboards(const char* const* p_names, int p_count);
void SteamStore_UploadScore(const char* p_name, int p_score);
int SteamStore_UploadRank();
int SteamStore_DownloadEntries(const char* p_name, int p_count, int p_offset);
const char* SteamStore_EntryName(int p_index);
int SteamStore_EntryScore(int p_index);

#endif
