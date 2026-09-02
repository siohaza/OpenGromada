#ifndef PLATFORM_STORE_H
#define PLATFORM_STORE_H

enum {
	ALIEN_STEAM_APPID = 33100
};

void Platform_StoreInit();
void Platform_StoreShutdown();
void Platform_StorePump();
bool Platform_StoreAlive();

void Platform_StoreSetAchievement(const char* p_id);
int Platform_StoreGetAchievement(const char* p_id);
void Platform_StoreClearAchievement(const char* p_id);
void Platform_StoreResetAllStats();
void Platform_StoreSetStat(const char* p_id, int p_value);
int Platform_StoreGetStat(const char* p_id);
void Platform_StoreSaveStatsIfNeeded();

void Platform_StoreInitLeaderboards(const char* p_names);
void Platform_StoreUpdateLeaderboard(const char* p_name, const char* p_playerName, int p_score);
int Platform_StoreDownloadLeaderboardEntries(const char* p_name, int p_count, int p_offset);
const char* Platform_StoreLeaderboardEntryName(int p_index);
int Platform_StoreLeaderboardEntryScore(int p_index);
int Platform_StoreLastUploadRank();

#endif
