
#pragma once

enum GAME_ID {
	GAME_AS1 = 0,
	GAME_ZS1 = 1,
	GAME_THESEUS,
	GAME_CRAZY_LUNCH,
	GAME_LAST_HOPE,
	GAME_CHACKS_TEMPLE,
	GAME_LOCOLAND,
};


enum GAME_OBJ_SCHEMA { GAME_OBJ_AS1, GAME_OBJ_ZS1, GAME_OBJ_LOCOLAND };
enum GAME_SFX_SCHEMA { GAME_SFX_AS1, GAME_SFX_THESEUS, GAME_SFX_ZS1 };
enum GAME_SCRIPT_DIALECT { GAME_SCRIPT_AS1, GAME_SCRIPT_THESEUS, GAME_SCRIPT_CRAZY_LUNCH, GAME_SCRIPT_ZS1, GAME_SCRIPT_LOCOLAND };
enum GAME_HUD_LAYOUT { GAME_HUD_AS1, GAME_HUD_ZS1 };
enum GAME_LAYER_RULES { GAME_LAYERS_AS1, GAME_LAYERS_ZS1, GAME_LAYERS_LOCOLAND };
enum GAME_MENU_RULES { GAME_MENU_AS1, GAME_MENU_ZS1 };
enum GAME_ENEMY_SEARCH_RULES { GAME_ENEMY_SEARCH_FLAGMAN, GAME_ENEMY_SEARCH_HASH };

struct GAME_DESCRIPTOR {
	GAME_ID m_id;
	const char* m_className;
	const char* m_title;
	const char* m_appMetaId;
	int m_weapRecordBytes;
	int m_legacyVidQueryBase;
	int m_uiBaseWidth;
	int m_uiBaseHeight;
	int m_fixedFrameWidth;
	int m_fixedFrameHeight;
	const char* m_profileId;
	const char* m_configName;
	const char* m_resourceName;
	int m_engineVersion;
	GAME_OBJ_SCHEMA m_objSchema;
	GAME_SFX_SCHEMA m_sfxSchema;
	GAME_SCRIPT_DIALECT m_scriptDialect;
	GAME_HUD_LAYOUT m_hudLayout;
	GAME_LAYER_RULES m_layerRules;
	const char* m_status;
	bool m_runtimeEnabled;


	int m_menuScriptWidth = 0;
	int m_menuScriptHeight = 0;
	GAME_MENU_RULES m_menuRules = GAME_MENU_AS1;


	int m_unitCountLayers = 0;


	bool m_gamebarNumbersAreWidths = true;
	const char* m_configAlias = nullptr;


	bool m_mapObjTrailingBytes = false;


	bool m_unitRecordsHaveShortList = false;
	bool m_lifetimeInWeapon = false;
	bool m_weaponHasKeyframes = true;
	bool m_rtsControls = false;
	int m_nativeMapVersion = 12;


	bool m_shortZeroSoftwareFrame = false;

	bool m_nativeScriptTextFiles = false;


	GAME_ENEMY_SEARCH_RULES m_enemySearchRules = GAME_ENEMY_SEARCH_FLAGMAN;

	bool m_nativeMoviePlayback = false;

	bool m_expandProfileReferences = false;
};

extern const GAME_DESCRIPTOR* GameDesc;

inline bool Game_IsZS1()
{
	return GameDesc->m_id == GAME_ZS1;
}

void Game_SetCliOverride(GAME_ID p_id);
bool Game_SetCliOverride(const char* p_name);
void Game_SetConfigOverride(const char* p_name);
void Game_SetProbeJson(bool p_enabled);
bool Game_WantsProbeJson();
const char* Game_ConfigName();
const char* Game_ResourceName();
const char* Game_StartMap();
const char* Game_Edition();
void Game_PrintProbeJson();



bool Game_Detect();
bool Game_RuntimeAvailable();
