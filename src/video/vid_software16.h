#ifndef VID_SOFTWARE16_H
#define VID_SOFTWARE16_H

#include "video/vid_software.h"

class TEXTURE;

class GAMMA;
class EX_SPRITE_DATA;

// VTABLE: ALIEN 0x47a408

class VID_SOFTWARE16 : public VID_SOFTWARE {
public:
	VID_SOFTWARE16() {}
	VID_SOFTWARE16(STREAM* p_stream) : VID_SOFTWARE(p_stream) {}

	VID* CreateMirror();
	int Draw(SPRITE* p_sprite);
	void DrawToVid(const SPRITE* p_sprite, const VID_TEXCOOR* p_texCoor, TEXTURE* p_texture, TEXTURE* p_zTexture);

	void SetGammaToPalette(unsigned char* p_palette, const GAMMA& p_gamma);
	int PaletteSize();

private:
	int DrawFrame(
		int p_frame,
		float p_x,
		float p_y,
		float p_z,
		float p_shiftX,
		float p_shiftY,
		unsigned int p_spriteFlags,
		EX_SPRITE_DATA* p_exData,
		SPRITE* p_gammaSprite
	);
	void DrawFrameToVid(
		int p_frame,
		float p_x,
		float p_y,
		float p_z,
		unsigned int p_spriteFlags,
		const VID_TEXCOOR* p_texCoor,
		TEXTURE* p_texture,
		TEXTURE* p_zTexture
	);

	friend struct VID_SOFTWARE16_TEST_ACCESS;
};

#endif
