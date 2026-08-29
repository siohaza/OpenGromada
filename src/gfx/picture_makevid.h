#ifndef PICTURE_MAKEVID_H
#define PICTURE_MAKEVID_H

#include "gfx/picture_base.h"
#include "util/decomp.h"
#include "gfx/color.h"
#include "gfx/picture.h"

class RESOURCE;

// VTABLE: ALIEN 0x47a508

class PICTURE_MAKEVID {
public:
#ifdef DECOMP_INLINE_PICTURE_MAKEVID_CTOR_TAG

	enum INLINE_CTOR_TAG { INLINE_CTOR };
	PICTURE_MAKEVID(INLINE_CTOR_TAG)
		: m_paletteDecode(0), m_unk0x42c(0)
	{
	}
#endif
	PICTURE_MAKEVID();
	virtual ~PICTURE_MAKEVID() // vtable+0x00
	{
		if (m_paletteDecode)
			::operator delete(m_paletteDecode);
	}
	virtual int NextFrame(); // vtable+0x04
	virtual int Rewind(); // vtable+0x08
	virtual int Load(STRING p_name, STRING p_alpha, STRING p_z); // vtable+0x0c
	virtual int Close(); // vtable+0x10

	PICTURE m_color; // 0x04
	PICTURE m_alpha; // 0x10
	PICTURE m_z; // 0x1c
	unsigned int m_palette[256]; // 0x28
	unsigned char* m_paletteDecode; // 0x428
	int m_unk0x42c; // 0x42c

	int GetNoFrames() const { return m_color.m_impl->m_noFrames; }

	void GetRectangle(int* p_left, int* p_top, int* p_right, int* p_bottom);
	int GetPaletteNumber(COLOR p_color) const;
	void SetPaletteDecodeNumber(COLOR p_color, unsigned char p_number);
	unsigned int GetAlpha(int p_x, int p_y) const;
	short GetPixelZ(int p_x, int p_y) const;
	short GetBoxZ(int p_x, int p_y, int p_w, int p_h);
	unsigned int GetPixelT(int p_x, int p_y);
	int CalcCRC32();
	int* GetPixel(int* p_out, int p_x, int p_y) const;

#ifdef DECOMP_INLINE_MAKEVID_GETPIXEL_SRET
	__forceinline COLOR GetPixel(int p_x, int p_y) const
	{
		return COLOR(this, p_x, p_y);
	}
#else
	COLOR GetPixel(int p_x, int p_y) const
	{
		COLOR c;
		GetPixel((int*) &c, p_x, p_y);
		return c;
	}
#endif
	int WriteLight(RESOURCE* p_res);
	int MakeVid(unsigned int p_flags, STRING p_name);
	void CreateOnePalette();
	int WriteSurfaces(unsigned char* p_out, char* p_src, int p_width, int p_height);
	void WriteHardware(RESOURCE* p_res);
	void WriteSoftware(RESOURCE* p_res);
	void WritePseudo3d(RESOURCE* p_res);
	int GetShadow(char* p_out);
	int IsPixel(int p_x, int p_y);
	int IsPixelInBox(int p_x, int p_y, int p_w, int p_h);
};

DECOMP_SIZE_ASSERT(PICTURE_MAKEVID, 0x430)

// SYNTHETIC: ALIEN 0x4125f0
// PICTURE_MAKEVID::`scalar deleting destructor'

#endif
