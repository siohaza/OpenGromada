#ifndef SETTINGS_H
#define SETTINGS_H

#include "util/decomp.h"

class SETTINGS {
public:
	char m_appName[0x100]; // 0x00
	int m_width[32]; // 0x100
	int m_height[32]; // 0x180
	int m_bpp[8]; // 0x200

	unsigned int m_flag; // 0x220
	int m_device; // 0x224
	int m_screenX; // 0x228
	int m_screenY; // 0x22c
	int m_screenBpp; // 0x230
	int m_fullscreen; // 0x234

	int CheckMode(int p_width, int p_height, int p_bpp);
};

DECOMP_SIZE_ASSERT(SETTINGS, 0x238)

extern SETTINGS Settings;

#endif
