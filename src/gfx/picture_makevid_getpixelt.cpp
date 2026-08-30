#include "gfx/picture_makevid.h"

inline static int OutOfBounds(PICTURE_BASE* p_impl, int p_x, int p_y)
{
	return p_x < 0 || p_y < 0 || p_x >= p_impl->m_width || p_y >= p_impl->m_height;
}

// FUNCTION: ALIEN 0x42c790
int PICTURE_MAKEVID::IsPixel(int p_x, int p_y)
{
	PICTURE_BASE* color = m_color.m_impl;
	if (OutOfBounds(color, p_x, p_y)) {
		return 0;
	}
	else {
		PICTURE_BASE* z = m_z.m_impl;
		if (z->m_pixels && m_z.m_format == 5) {
			if ((unsigned short) z->GetData(p_x, p_y) == 0x8000) {
				return 0;
			}
			if ((m_unk0x42c & 0x20) && (m_unk0x42c & 2) && (m_unk0x42c & 4)) {
				if (m_unk0x42c & 1) {
					return GetAlpha(p_x, p_y) >> 4;
				}
				return GetPixelT(p_x, p_y) & 0xfff;
			}
			return 1;
		}
		if ((m_unk0x42c & 2) && (m_unk0x42c & 1)) {
			return GetAlpha(p_x, p_y) >> 4;
		}
		int pixel;
		return *GetPixel(&pixel, p_x, p_y) & 0xffffff;
	}
}
