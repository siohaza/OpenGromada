#include "game/settings.h"

// GLOBAL: ALIEN 0x47f4b0
SETTINGS Settings = { { 0 }, { 640, 800, 1024 }, { 480, 600, 768 }, { 16, 32 }, 0, 0, 640, 480, 32, 1 };

// FUNCTION: ALIEN 0x4348c0
int SETTINGS::CheckMode(int p_width, int p_height, int p_bpp)
{
	int i;
	for (i = 0; m_bpp[i]; ++i)
		if (p_bpp == m_bpp[i])
			break;
	if (!m_bpp[i])
		return 0;
	for (int j = 0; m_width[j]; ++j)
		if (p_width == m_width[j] && p_height == m_height[j])
			return 1;
	return 0;
}
