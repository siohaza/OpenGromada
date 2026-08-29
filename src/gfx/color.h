#ifndef COLOR_H
#define COLOR_H

#include "util/decomp.h"

class GAMMA;
#ifdef DECOMP_INLINE_MAKEVID_GETPIXEL_SRET
class PICTURE_MAKEVID;
#endif

class RGB555 {
public:
	unsigned short m_value; // 0x00
};

DECOMP_SIZE_ASSERT(RGB555, 0x2)

extern int RGB16_rMask;
extern int RGB16_gMask;
extern int RGB16_rShift;
extern int RGB16_gShift;

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
#ifdef DECOMP_INLINE_MAKEVID_GETPIXEL_SRET
	__forceinline COLOR(const PICTURE_MAKEVID* p_picture, int p_x, int p_y);
#endif
	COLOR AlphaAdd(COLOR p_color, unsigned int p_alpha);
};

DECOMP_SIZE_ASSERT(COLOR, 0x4)

#ifndef DECOMP_COLOR_COPY_OUT_OF_LINE
inline COLOR::COLOR(const COLOR& p_other)
{
	m_value = p_other.m_value;
}
#endif

inline COLOR::COLOR(const RGB555& p_rgb555)
{
	m_value = ((((p_rgb555.m_value & 0xff80) | 0xffff8000) << 3 | (p_rgb555.m_value & 0x3fc)) << 3
		| (p_rgb555.m_value & 0x1f))
		<< 3;
}

#ifndef DECOMP_COLOR_RGB16_CTOR_OUT_OF_LINE
inline COLOR::COLOR(const unsigned short* p_rgb16)
{
	m_value = ((*p_rgb16 << (16 - RGB16_rShift)) & 0xffff0000)
			| ((*p_rgb16 << (8 - RGB16_gShift)) & 0xff00)
			| (((*p_rgb16 & 0x1f) | 0xffe00000) << 3);
}
#endif

// FUNCTION: ALIEN 0x414fd0
inline COLOR::COLOR(int p_r, int p_g, int p_b)
{
	if (p_r < 0)
		p_r = 0;
	else if (p_r > 255)
		p_r = 255;
	if (p_g < 0)
		p_g = 0;
	else if (p_g > 255)
		p_g = 255;
	if (p_b < 0)
		p_b = 0;
	else if (p_b > 255)
		p_b = 255;
	m_value = ((((p_r | 0xffffff00) << 8) | p_g) << 8) | p_b;
}

#endif
