#ifndef GAME_DATA_VERSION_H
#define GAME_DATA_VERSION_H

enum GAME_DATA_VERSION {
	GAME_DATA_RETAIL_12 = 0,
	GAME_DATA_STEAM_122 = 1,
};

inline GAME_DATA_VERSION GameDataVersion = GAME_DATA_RETAIL_12;

inline bool GameData_IsSteam()
{
	return GameDataVersion == GAME_DATA_STEAM_122;
}

#endif
