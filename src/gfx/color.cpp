#define DECOMP_COLOR_RGB16_CTOR_OUT_OF_LINE

#include "gfx/color.h"

// GLOBAL: ALIEN 0x483d30
int RGB16_rShift = 8;

// GLOBAL: ALIEN 0x483d34
int RGB16_gShift = 3;

// FUNCTION: ALIEN 0x416680
COLOR COLOR::AlphaAdd(COLOR p_color, unsigned int p_alpha)
{
	p_alpha++;
	*this = COLOR((p_alpha * ((p_color.m_value >> 16) & 0xff)
					 + (256 - p_alpha) * ((m_value >> 16) & 0xff))
					>> 8,
				  (p_alpha * ((p_color.m_value >> 8) & 0xff)
					 + (256 - p_alpha) * ((m_value >> 8) & 0xff))
					>> 8,
				  (p_alpha * (p_color.m_value & 0xff)
					 + (256 - p_alpha) * (m_value & 0xff))
					>> 8);
	return *this;
}

// FUNCTION: ALIEN 0x416800
COLOR::COLOR(const unsigned short* p_rgb16)
{
	unsigned int c = *p_rgb16;
	m_value = ((c << (16 - RGB16_rShift)) & 0xffff0000)
			| ((c << (8 - RGB16_gShift)) & 0xff00)
			| (((c & 0x1f) | 0xffe00000) << 3);
}
