#ifndef SETTINGS_H
#define SETTINGS_H

#include "util/decomp.h"

class STRING;

// Generated display-policy migration version.
enum {
	SETTINGS_DISPLAY_POLICY_VERSION = 4
};

class SETTINGS {
public:
	char m_appName[0x100]; // 0x00
	int m_width[32];       // 0x100
	int m_height[32];      // 0x180
	int m_bpp[8];          // 0x200

	unsigned int m_flag; // 0x220
	int m_device;        // 0x224
	int m_screenX;       // 0x228
	int m_screenY;       // 0x22c
	int m_screenBpp;     // 0x230
	int m_fullscreen;    // 0x234
	int m_renderWidth;
	int m_nativeResolution;
	int m_uiScale;
	int m_displayPolicyVersion;
	int m_desktopResolution;

	int CheckMode(int p_width, int p_height, int p_bpp);
};

extern SETTINGS Settings;

// Parses port options and preserves the legacy cfg/script argument.
void Settings_ParseCommandLine(int p_argc, char** p_argv, STRING* p_gameArgument);
bool Settings_ForceRedBlood();
bool Settings_MigrateLegacyDisplay(SETTINGS* p_settings);
bool Settings_MigrateLegacyDisplayForDesktop(SETTINGS* p_settings, int p_desktopWidth, int p_desktopHeight);
void Settings_ApplyCommandLine(SETTINGS* p_settings);

#endif
