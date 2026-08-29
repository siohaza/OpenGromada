#ifndef VID_SOFTWARE16_H
#define VID_SOFTWARE16_H

#include "video/vid_software.h"

inline COLOR::COLOR(const GAMMA& p_gamma, const COLOR& p_color)
{
	if (p_gamma.m_a || p_gamma.m_b) {
		unsigned int c = p_color.m_value;
		unsigned int gb = p_gamma.m_b;
		unsigned int na = ~p_gamma.m_a;
		int b = (gb & 0xff) + (((na & 0xff) + 1) * (c & 0xff) >> 8);
		int g = ((gb >> 8) & 0xff) + ((((na >> 8) & 0xff) + 1) * ((c >> 8) & 0xff) >> 8);
		int r = ((gb >> 16) & 0xff) + ((((na >> 16) & 0xff) + 1) * ((c >> 16) & 0xff) >> 8);
		COLOR t(r, g, b);
		m_value = t.m_value;
	}
	else
		m_value = p_color.m_value;
}

class TEXTURE;

class GAMMA;

// VTABLE: ALIEN 0x47a408

class VID_SOFTWARE16 : public VID_SOFTWARE {
public:
	VID_SOFTWARE16() {}
	VID_SOFTWARE16(STREAM* p_stream) : VID_SOFTWARE(p_stream) {}

	VID* CreateMirror();
	int Draw(SPRITE* p_sprite);
	void DrawToVid(const SPRITE* p_sprite, const VID_TEXCOOR* p_texCoor,
		TEXTURE* p_texture, TEXTURE* p_zTexture);

	void SetGammaToPalette(unsigned char* p_palette, const GAMMA& p_gamma);
	int PaletteSize();
};

#endif
