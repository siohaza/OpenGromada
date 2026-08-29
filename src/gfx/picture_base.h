#ifndef PICTURE_BASE_H
#define PICTURE_BASE_H

#include "gfx/color.h"
#include "util/decomp.h"
#include "util/string.h"

#include <stdio.h>

// VTABLE: ALIEN 0x47a718

class PICTURE_BASE {
public:
	PICTURE_BASE();
	PICTURE_BASE(int p_width, int p_height, int p_bpp);
	~PICTURE_BASE()
	{
		Close();
	}
	virtual void* ScalarDeletingDestructor(unsigned int p_flags); // vtable+0x00
	virtual int NextFrame(); // vtable+0x04
	virtual int Rewind(); // vtable+0x08
	virtual int Load(const STRING& p_name); // vtable+0x0c
	virtual int Close(); // vtable+0x10

	int m_noFrames; // 0x04
	int m_frame; // 0x08
	int m_unk0x0c; // 0x0c
	int m_width; // 0x10
	int m_height; // 0x14
	int m_bpp; // 0x18

	STRING m_name; // 0x1c
	FILE* m_file; // 0x20
	union {
		undefined m_unk0x24[0x400]; // 0x24
		unsigned int m_palette[256];
	};
	unsigned char* m_pixels; // 0x424
	int m_unk0x428; // 0x428

	void SetSize(int p_width, int p_height, int p_bpp);
	int GetData(int p_x, int p_y);
	void PutData(int p_x, int p_y, unsigned int p_data);
	int* GetPixel(int* p_out, int p_x, int p_y);
	int SaveTGA(const STRING& p_name, int p_x, int p_y, int p_w, int p_h);
	void PutPixel(int p_x, int p_y, COLOR p_color);
	char** GetName_impl(char** p_out);
};

DECOMP_SIZE_ASSERT(PICTURE_BASE, 0x42c)

#endif
