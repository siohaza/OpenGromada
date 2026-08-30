#ifndef PICTURE_TGA_H
#define PICTURE_TGA_H

#include "gfx/picture_base.h"

#pragma pack(push, 1)
struct TGA_HEADER {
	unsigned char m_idLength;
	unsigned char m_colorMapType;
	unsigned char m_imageType;
	unsigned short m_colorMapFirst;
	unsigned short m_colorMapLength;
	unsigned char m_colorMapDepth;
	unsigned short m_xOrigin;
	unsigned short m_yOrigin;
	unsigned short m_width;
	unsigned short m_height;
	unsigned char m_depth;
	unsigned char m_descriptor;
};
#pragma pack(pop)

static_assert(sizeof(TGA_HEADER) == 0x12, "TGA_HEADER matches the ondisk TGA header");

// VTABLE: ALIEN 0x47a72c
class PICTURE_TGA : public PICTURE_BASE {
public:
	virtual int NextFrame();
	virtual int Rewind();
	virtual int Load(const STRING& p_name);

	int LoadHeader(TGA_HEADER* p_header);
	unsigned char m_imageType; // 0x42c
};

#endif
