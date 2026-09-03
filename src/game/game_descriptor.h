
#pragma once

enum GAME_ID {
	GAME_AS1 = 0,
	GAME_ZS1 = 1,
};

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
};

extern const GAME_DESCRIPTOR* GameDesc;

inline bool Game_IsZS1()
{
	return GameDesc->m_id == GAME_ZS1;
}

void Game_SetCliOverride(GAME_ID p_id);

// Returns false when objects.res is a readable container of some other
// Sigma engine; the caller should abort rather than mis-load it.
bool Game_Detect();
