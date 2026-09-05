#ifndef TEXTURE_H
#define TEXTURE_H

#include "platform/platform_types.h"
#include "util/decomp.h"

#include <cstdint>

class GAMMA;
class STREAM;

class TEXTURE {
public:
	TEXTURE(int p_width, int p_height, int p_format, unsigned int p_flags);
	TEXTURE(int p_width, int p_height, int p_format, unsigned int p_flags, const void* p_palette, STREAM* p_stream);
	virtual ~TEXTURE();

	TEXTURE(const TEXTURE&) = delete;
	TEXTURE& operator=(const TEXTURE&) = delete;

	void Create(int p_width, int p_height, int p_format, unsigned int p_flags);

	void* Lock(int* p_pitch, const RECT* p_rect);

	int CopyFromScreen(const RECT* p_rect, const POINT* p_point);

	int Draw(const RECT* p_dst, const RECT* p_src, const GAMMA* p_gamma);
	char* Draw_z(float p_z1, int p_z2, const int* p_dst, const int* p_src, const GAMMA* p_gamma);

	int SetPalette(const void* p_palette);

	int BitsPerPixel() const;
	uint32_t GpuData();
	uint32_t GpuPalette();
	void GpuWritten();
	bool GpuReadback();

	void* m_data;
	int m_format;
	int m_width;
	int m_height;
	int m_pitch;             // row stride, in bytes
	unsigned int* m_palette; // 256 ARGB entries
	unsigned int m_flags;
	uint32_t m_gpuData = 0;
	uint64_t m_gpuGeneration = 0;
	bool m_gpuWritten = false;
	bool m_gpuCpuDirty = true;
};

extern int TextureMemoryInUse;

extern int g_textureMaxWidth;
extern int g_textureMaxHeight;

#endif
