#ifndef COLOR_H
#define COLOR_H

#include "util/decomp.h"

class GAMMA;

class RGB555 {
public:
	unsigned short m_value; // 0x00
};

static_assert(sizeof(RGB555) == 2, "RGB555 is a packed 16-bit pixel");

extern int RGB16_rMask;
extern int RGB16_gMask;
extern int RGB16_rShift;
extern int RGB16_gShift;

// Expands RGB16 by shifting, so maximum channels reach 0xf8 rather than 0xff.
inline unsigned int ExpandRGB16(unsigned int p_value)
{
	return 0xff000000u | ((p_value & 0x1f) << 3) | ((p_value << (8 - RGB16_gShift)) & 0xff00) |
		   ((p_value << (16 - RGB16_rShift)) & 0xff0000);
}

class COLOR {
public:
	undefined4 m_value; // 0x00

	COLOR() {}
	COLOR(const COLOR& p_other);

	COLOR(int p_value) { m_value = p_value; }
	COLOR(int p_r, int p_g, int p_b);

	COLOR(int p_a, int p_r, int p_g, int p_b);
	COLOR(const RGB555& p_rgb555);
	COLOR(const unsigned short* p_rgb16);
	COLOR(const GAMMA& p_gamma, const COLOR& p_color);
	COLOR AlphaAdd(COLOR p_color, unsigned int p_alpha);
};

static_assert(sizeof(COLOR) == 4, "COLOR is a packed 32-bit ARGB pixel");

inline COLOR::COLOR(const RGB555& p_rgb555)
{
	m_value = ((((p_rgb555.m_value & 0xff80) | 0xffff8000) << 3 | (p_rgb555.m_value & 0x3fc)) << 3 |
			   (p_rgb555.m_value & 0x1f))
			  << 3;
}

// FUNCTION: ALIEN 0x414fd0
inline COLOR::COLOR(int p_r, int p_g, int p_b)
{
	if (p_r < 0) {
		p_r = 0;
	}
	else if (p_r > 255) {
		p_r = 255;
	}
	if (p_g < 0) {
		p_g = 0;
	}
	else if (p_g > 255) {
		p_g = 255;
	}
	if (p_b < 0) {
		p_b = 0;
	}
	else if (p_b > 255) {
		p_b = 255;
	}
	m_value = ((((p_r | 0xffffff00) << 8) | p_g) << 8) | p_b;
}

#endif
