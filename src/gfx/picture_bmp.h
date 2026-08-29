#ifndef PICTURE_BMP_H
#define PICTURE_BMP_H

#include "gfx/picture_base.h"

#pragma pack(push, 1)
struct BMP_HEADER {
	unsigned short m_signature;
	unsigned int m_fileSize;
	unsigned short m_reserved1;
	unsigned short m_reserved2;
	unsigned int m_dataOffset;
	unsigned int m_infoSize;
	int m_width;
	int m_height;
	unsigned short m_planes;
	unsigned short m_depth;
	unsigned int m_compression;
	unsigned int m_imageSize;
	int m_xPixelsPerMeter;
	int m_yPixelsPerMeter;
	unsigned int m_colorsUsed;
	unsigned int m_colorsImportant;
};
#pragma pack(pop)

DECOMP_SIZE_ASSERT(BMP_HEADER, 0x36)

// VTABLE: ALIEN 0x47a768
class PICTURE_BMP : public PICTURE_BASE {
public:
	virtual int NextFrame();
	virtual int Rewind();
	virtual int Load(const STRING& p_name);

	int LoadHeader(BMP_HEADER* p_header);
};

DECOMP_SIZE_ASSERT(PICTURE_BMP, 0x42c)

#endif
