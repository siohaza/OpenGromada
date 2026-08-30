#include "gfx/color.h"

#include "gfx/gamma.h"

// GLOBAL: ALIEN 0x483d30
int RGB16_rShift = 8;

// GLOBAL: ALIEN 0x483d34
int RGB16_gShift = 3;

// FUNCTION: ALIEN 0x416680
COLOR COLOR::AlphaAdd(COLOR p_color, unsigned int p_alpha)
{
	p_alpha++;
	*this = COLOR(
		(p_alpha * ((p_color.m_value >> 16) & 0xff) + (256 - p_alpha) * ((m_value >> 16) & 0xff)) >> 8,
		(p_alpha * ((p_color.m_value >> 8) & 0xff) + (256 - p_alpha) * ((m_value >> 8) & 0xff)) >> 8,
		(p_alpha * (p_color.m_value & 0xff) + (256 - p_alpha) * (m_value & 0xff)) >> 8
	);
	return *this;
}

// FUNCTION: ALIEN 0x416800
COLOR::COLOR(const unsigned short* p_rgb16)
{
	unsigned int c = *p_rgb16;
	m_value = ((c << (16 - RGB16_rShift)) & 0xffff0000) | ((c << (8 - RGB16_gShift)) & 0xff00) |
			  (((c & 0x1f) | 0xffe00000) << 3);
}

COLOR::COLOR(const GAMMA& p_gamma, const COLOR& p_color)
{
	if (p_gamma.m_a || p_gamma.m_b) {
		unsigned int c = p_color.m_value;
		unsigned int gb = p_gamma.m_b;
		unsigned int na = ~p_gamma.m_a;
		int b = (gb & 0xff) + (((na & 0xff) + 1) * (c & 0xff) >> 8);
		int g = ((gb >> 8) & 0xff) + ((((na >> 8) & 0xff) + 1) * ((c >> 8) & 0xff) >> 8);
		int r = ((gb >> 16) & 0xff) + ((((na >> 16) & 0xff) + 1) * ((c >> 16) & 0xff) >> 8);
		m_value = COLOR(r, g, b).m_value;
	}
	else {
		m_value = p_color.m_value;
	}
}
