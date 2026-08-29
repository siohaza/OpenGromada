#define DECOMP_STRING_COPY_CTOR_CALL_COPY_NONNULL
#define DECOMP_UNINITIALIZED_STRING_DEFAULT_CTOR
#define DECOMP_INLINE_STRING_DTOR
#define DECOMP_COLOR_COPY_OUT_OF_LINE
#define DECOMP_INLINE_MAKEVID_GETPIXEL_SRET
#include "gfx/picture_makevid.h"

static inline int OutOfBounds(PICTURE_BASE* p_impl, int p_x, int p_y)
{
	return p_x < 0 || p_y < 0 || p_x >= p_impl->m_width || p_y >= p_impl->m_height;
}

__forceinline COLOR::COLOR(const PICTURE_MAKEVID* p_picture, int p_x, int p_y)
{
	p_picture->GetPixel((int*) this, p_x, p_y);
}

// STUB: ALIEN 0x429480
inline unsigned int PICTURE_MAKEVID::GetPixelT(int p_x, int p_y)
{
	PICTURE_BASE* color = m_color.m_impl;
	if (OutOfBounds(color, p_x, p_y))
		return 0;

	if (m_unk0x42c & 0x1000) {
		if (m_unk0x42c & 8)
			return color->GetData(p_x, p_y);
		unsigned int pixel;
		GetPixel((int*) &pixel, p_x, p_y);
		int a = pixel >> 24;
		int r = (pixel >> 16) & 0xff;
		int g = (pixel >> 8) & 0xff;
		int b = pixel & 0xff;
		if (!IsPixel(p_x, p_y))
			a = 0;
		if ((m_unk0x42c & 1) && (m_unk0x42c & 2)) {
			r = 255 * r / a;
			g = 255 * g / a;
			b = 255 * b / a;
			if (r > 255)
				r = 255;
			if (g > 255)
				g = 255;
			if (b > 255)
				b = 255;
			return (a << 24) | (r << 16) | (g << 8) | b;
		}
		return ((a >> 7) << 15) | ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
	}
	else if (!(m_unk0x42c & 8)) {
		unsigned int pixel;
		GetPixel((int*) &pixel, p_x, p_y);
		unsigned char b = pixel;
		unsigned char a = pixel >> 24;
		unsigned char r = pixel >> 16;
		unsigned char g = pixel >> 8;
		if ((m_unk0x42c & 1) && (m_unk0x42c & 2)) {
			if (a >> 4) {
				r = (255 * r / a) >> 4;
				g = (255 * g / a) >> 4;
				b = (255 * b / a) >> 4;
				if (r > 15)
					r = 15;
				if (g > 15)
					g = 15;
				if (b > 15)
					b = 15;
				return ((a >> 4) << 12) | (r << 8) | (g << 4) | b;
			}
			return 0;
		}
		if ((m_unk0x42c & 4) && (m_unk0x42c & 2))
			return ((pixel & 0xff) >> 4) + (((r >> 4) << 8) | ((g >> 4) << 4));
		return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
	}
	else if (m_unk0x42c & 2) {
		return GetPaletteNumber(GetPixel(p_x, p_y));
	}
	else
		return color->GetData(p_x, p_y);
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
			if ((unsigned short) z->GetData(p_x, p_y) == 0x8000)
				return 0;
			if ((m_unk0x42c & 0x20) && (m_unk0x42c & 2) && (m_unk0x42c & 4)) {
				if (m_unk0x42c & 1)
					return GetAlpha(p_x, p_y) >> 4;
#pragma inline_depth(0)
				return GetPixelT(p_x, p_y) & 0xfff;
#pragma inline_depth(8)
			}
			return 1;
		}
		if ((m_unk0x42c & 2) && (m_unk0x42c & 1))
			return GetAlpha(p_x, p_y) >> 4;
		int pixel;
		return *GetPixel(&pixel, p_x, p_y) & 0xffffff;
	}
}
