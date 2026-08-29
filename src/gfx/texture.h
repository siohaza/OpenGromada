#ifndef TEXTURE_H
#define TEXTURE_H

#include <windows.h>

#include "util/decomp.h"

struct IDirect3DTexture8;
struct IDirect3DSurface8;
class GAMMA;

// VTABLE: ALIEN 0x47a290

class TEXTURE {
public:
	TEXTURE(int p_width, int p_height, int p_format, unsigned int p_flags);
	TEXTURE(int p_width, int p_height, int p_format, unsigned int p_flags,
		const void* p_palette, class STREAM* p_stream);
	virtual ~TEXTURE();

	void Create(int p_width, int p_height, int p_format, unsigned int p_flags);
	int Lock(int* p_pitch, const RECT* p_rect);
	int CopyFromSurface(IDirect3DSurface8* p_surface, const RECT* p_rect, const POINT* p_point);

	int Draw(const RECT* p_dst, const RECT* p_src, const GAMMA* p_gamma);

	char* Draw_z(float p_z1, int p_z2, const int* p_dst, const int* p_src, const GAMMA* p_gamma);
	int SetPaletteEntries(void* p_entries);
	int SetPalette(const void* p_palette);

	void* m_data; // 0x04
	IDirect3DTexture8* m_texture; // 0x08
	int m_format; // 0x0c
	int m_width; // 0x10
	int m_height; // 0x14
	undefined m_unk0x18[4]; // 0x18
	unsigned int m_flags; // 0x1c
};

DECOMP_SIZE_ASSERT(TEXTURE, 0x20)

extern int TextureMemoryInUse;

// SYNTHETIC: ALIEN 0x4032c0
// TEXTURE::`scalar deleting destructor'

#endif
