#include "video/vid_software16.h"

#include "game/gametime.h"
#include "game/map.h"
#include "game/terrain_camera.h"
#include "gfx/asmdraw.h"
#include "gfx/gamma.h"
#include "gfx/gpu_backend.h"
#include "gfx/gpu_texture.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "gfx/texture.h"
#include "sprite/ex_sprite_data.h"
#include "sprite/sprite.h"
#include "util/myerror.h"
#include "util/packed.h"
#include "video/vid_hardware.h"

#include <string.h>

extern float FSin[256];

// FUNCTION: ALIEN 0x4126e0
VID* VID_SOFTWARE16::CreateMirror()
{
	return new VID_SOFTWARE16((STREAM*) this);
}

// FUNCTION: ALIEN 0x412710
int VID_SOFTWARE16::PaletteSize()
{
	return ((m_pixelFlag & 2) != 0 ? 4 : 2) << 8;
}

void VID_SOFTWARE16::SetReColorForArmy(unsigned int p_color)
{
	if (m_pixelFlag & 2) {
		VID_SOFTWARE::SetReColorForArmy(p_color);
		return;
	}
	unsigned char* base = PrepareRecolor();
	if (!base) {
		return;
	}
	int palSize = PaletteSize();

	const unsigned short* pristine = (const unsigned short*) m_recolorBase;
	unsigned short* entries = (unsigned short*) base;
	int destB = p_color & 0xff;
	int destG = (p_color >> 8) & 0xff;
	int destR = (p_color >> 16) & 0xff;
	for (int i = 0; i < palSize / 2; ++i) {
		unsigned short packed = pristine[i];
		int b = (packed & 0x1f) << 3;
		int g = ((packed << (8 - RGB16_gShift)) & 0xff00) >> 8;
		int r = ((packed << (16 - RGB16_rShift)) & 0xff0000) >> 16;
		float key = (float) r * (1.0f / 148.0f);
		if (r <= 0x14 || g > (int) (key * 80.0f) || abs(r - b) > (int) (key * 30.0f)) {
			continue;
		}
		float scale = (float) r * 0.0078125f;
		int nb = (int) ((float) destB * scale);
		int ng = (int) ((float) destG * scale);
		int nr = (int) ((float) destR * scale);
		nb = nb < 0 ? 0 : nb > 255 ? 255 : nb;
		ng = ng < 0 ? 0 : ng > 255 ? 255 : ng;
		nr = nr < 0 ? 0 : nr > 255 ? 255 : nr;
		nb += g;
		ng += g;
		nr += g;
		nb = nb > 255 ? 255 : nb;
		ng = ng > 255 ? 255 : ng;
		nr = nr > 255 ? 255 : nr;
		unsigned int value = ((unsigned int) nr << 16) | ((unsigned int) ng << 8) | (unsigned int) nb;
		entries[i] = (unsigned short) (((value >> 3) & 0x1f) | (RGB16_rMask & (value >> (16 - RGB16_rShift))) |
									   (RGB16_gMask & (value >> (8 - RGB16_gShift))));
	}

	if (m_pixelFlag16 & 0x400) {
		SetGamma(m_gamma[0], 0);
		SetGamma(m_gamma[1], 1);
		SetGamma(m_gamma[2], 2);
		SetGamma(m_gamma[3], 3);
	}
	else {
		SetGamma(*(const GAMMA*) &m_colorSub, 4);
	}
}

extern int RGB16_rMask;
extern int RGB16_gMask;

static void DrawTerrainAlphaSpan(unsigned char* p_src,
								 unsigned short* p_zbuf,
								 unsigned short* p_dest,
								 int p_count,
								 unsigned short p_z,
								 const int* p_palette,
								 int p_x,
								 int p_y)
{
	for (int i = 0; i < p_count; ++i) {
		if (p_z >= p_zbuf[i] && ((unsigned int) p_palette[p_src[i]] >> 24) == 0xff) {
			TerrainCoverageMarkPixel(p_x + i, p_y);
		}
	}
	AsmDrawWithAlpha16(p_src, p_zbuf, p_dest, p_count, p_z, p_palette);
}

static __forceinline GAMMA* CombineDrawGamma(GAMMA* p_result, const GAMMA& p_gamma, int p_screenB, int p_screenA)
{
	return p_result->Add(p_gamma, GAMMA(GAMMA::RAW_COPY, p_screenA, p_screenB));
}

// STUB: ALIEN 0x416030
void VID_SOFTWARE16::SetGammaToPalette(unsigned char* p_palette, const GAMMA& p_gamma)
{
	if (!p_palette) {
		return;
	}
	if (!p_gamma.m_a && !p_gamma.m_b) {
		return;
	}
	if (m_pixelFlag & 2) {
		COLOR* entry = (COLOR*) p_palette;
		int n = 256;
		do {
			*entry = COLOR(p_gamma, *entry);
			++entry;
		} while (--n);
	}
	else {
		unsigned short* entry = (unsigned short*) p_palette;
		int n = 256;
		do {
			COLOR c(p_gamma, COLOR(entry));
			*entry = (unsigned short) ((((unsigned int) c.m_value >> 3) & 0x1f) |
									   (RGB16_rMask & ((unsigned int) c.m_value >> (16 - RGB16_rShift))) |
									   (RGB16_gMask & ((unsigned int) c.m_value >> (8 - RGB16_gShift))));
			++entry;
		} while (--n);
	}
}

// STUB: ALIEN 0x417780
int VID_SOFTWARE16::Draw(SPRITE* p_sprite)
{

	if (PropHide()) {
		return 0;
	}
	return DrawFrame(p_sprite->m_noCadr,
					 p_sprite->m_x,
					 p_sprite->m_y,
					 p_sprite->m_z,
					 Map->m_shiftX,
					 Map->m_shiftY,
					 p_sprite->m_flag,
					 p_sprite->m_exData,
					 p_sprite);
}

int VID_SOFTWARE16::DrawFrame(int p_frame,
							  float p_x,
							  float p_y,
							  float p_z,
							  float p_shiftX,
							  float p_shiftY,
							  unsigned int p_spriteFlags,
							  EX_SPRITE_DATA* p_exData,
							  SPRITE* p_gammaSprite)
{
	if (PropHide()) {
		return 0;
	}

	int width = m_unk0x2f6;
	int height = m_messageLineHeight;
	int x0 = (int) p_x - (int) p_shiftX - width / 2;
	int y0 = (int) (p_y - p_z) - (int) p_shiftY - height / 2;
	if (x0 + width < ViewXMin() || x0 >= ViewXMax() || y0 + height < ViewYMin() || y0 >= ViewYMax()) {
		return 0;
	}

	int z = (int) (p_z * 8.0f);
	if ((m_flag & 0x8000) && z < 0x3fff) {
		z += 0x3fff;
	}
	else if (m_flag & 0x10000) {
		int bob = (int) (FSin[(CurrentTime >> 3) & 0xff] * m_unk0x60 * 8.0f);
		z += bob;
		y0 -= bob / 8;
	}

	unsigned char* frame = (unsigned char*) m_unk0x48c + m_unk0x484[p_frame];
	short contourCount = PackedRead<short>(frame);
	unsigned char* header = frame + 6 * contourCount + 2;
	int nRows = PackedRead<short>(header + 2);
	unsigned char* rle = header + 4;
	int yTop = y0 + PackedRead<short>(header);
	int yEnd = yTop + nRows;
	if (yTop >= ViewYMax() || yEnd < ViewYMin()) {
		return 0;
	}
	if (yEnd > ViewYMax()) {
		yEnd = ViewYMax();
	}

	// Expand RGB565 spans into the ARGB8888 framebuffer.
	GRAPH_CORE* graph = (GRAPH_CORE*) Graph;
	int pitch = GPU_RENDER::Active() ? graph->m_pitch : graph->Lock();
	int zpitch = graph->m_zpitch;
	char* surface = (char*) graph->m_color;
	char* zbase = (char*) graph->m_zbuffer;
	if ((!surface || !zbase) && !GPU_RENDER::Active()) {
		return 0;
	}

	alignas(int) unsigned char paletteBuf[1024];
	int* palette;
	EX_SPRITE_DATA* exData = p_exData;
	if (!exData || (!exData->m_unk0x24 && !exData->m_unk0x28)) {
		int armyOffset = (m_fontFlag & 4) ? ((p_spriteFlags >> 11) & 3) * PaletteSize() : 0;
		palette = (int*) ((char*) m_unk0x48c + armyOffset);
	}
	else {
		palette = (int*) paletteBuf;
		int gammaOffset = (m_fontFlag & 4) ? 4 * PaletteSize() : 0;
		memcpy(paletteBuf, (char*) m_unk0x48c + gammaOffset, PaletteSize());
		if (m_flag & 0x800) {
			SetGammaToPalette(paletteBuf, p_gammaSprite->GetGamma());
		}
		else {
			GAMMA combined;
			CombineDrawGamma(&combined,
							 p_gammaSprite->GetGamma(),
							 ((GRAPH_CORE*) Graph)->m_gammaSet.m_b,
							 ((GRAPH_CORE*) Graph)->m_gammaSet.m_a);
			SetGammaToPalette(paletteBuf, combined);
		}
	}
	unsigned short flag2 = m_pixelFlag16;
	if (GPU_RENDER::Active()) {
		GPU_VID::DrawFrame(this, rle, nRows, x0, yTop, z, 1.0f, 0, palette, true);
		return 0;
	}
	if ((flag2 & 2) && (flag2 & 1) && (flag2 & 8)) {

		z += 1024;
		if (z > 0x7fff) {
			z = 0x7fff;
		}
		int rows = nRows;
		if (yTop < ViewYMin()) {
			for (int skip = ViewYMin() - yTop; skip > 0; --skip) {
				while (PackedRleRun(rle)) {
					rle += rle[1] + 2;
				}
				rle += 2;
				--rows;
			}
			yTop = ViewYMin();
		}
		int zstep = 0;
		if (m_unk0x24 > m_footprintHeight) {
			z += 8 * rows;
			zstep = -8;
		}
		int y = yTop;
		COLOR* dst;
		unsigned short* zrow;
		int x;
		int count;
		unsigned char* src;
		int xEnd;
		int xc;
		if (x0 >= ViewXMin() && x0 + width <= ViewXMax()) {
			for (; y < yEnd; ++y, z += zstep) {
				dst = (COLOR*) (surface + 4 * y * pitch);
				zrow = (unsigned short*) (zbase + 2 * y * zpitch);
				x = x0;
				while (PackedRleRun(rle)) {
					x += rle[0];
					count = rle[1];
					src = rle + 2;
					rle += count + 2;
					AsmDrawWithAlpha32(src, zrow + x, dst + x, count, (unsigned short) z, palette);
					x += count;
				}
				rle += 2;
			}
		}
		else {
			for (; y < yEnd; ++y, z += zstep) {
				dst = (COLOR*) (surface + 4 * y * pitch);
				zrow = (unsigned short*) (zbase + 2 * y * zpitch);
				x = x0;
				while (PackedRleRun(rle)) {
					x += rle[0];
					count = rle[1];
					src = rle + 2;
					rle += count + 2;
					xEnd = x + count;
					xc = x;
					x = xEnd;
					if (xc < ViewXMin()) {
						src += ViewXMin() - xc;
						count -= ViewXMin() - xc;
						xc = ViewXMin();
					}
					if (xEnd > ViewXMax()) {
						count = ViewXMax() - xc;
					}
					if (count > 0) {
						AsmDrawWithAlpha32(src, zrow + xc, dst + xc, count, (unsigned short) z, palette);
					}
				}
				rle += 2;
			}
		}
		return 0;
	}
	if (!(flag2 & 1)) {
		return 0;
	}
	if ((flag2 & 8) && (flag2 & 4)) {

		if (yTop < ViewYMin()) {
			for (int skip = ViewYMin() - yTop; skip > 0; --skip) {
				while (PackedRleRun(rle)) {
					rle += 3 * rle[1] + 2;
				}
				rle += 2;
			}
			yTop = ViewYMin();
		}
		for (int y = yTop; y < yEnd; ++y) {
			unsigned int* dst = (unsigned int*) (surface + 4 * y * pitch);
			short* zrow = (short*) (zbase + 2 * y * zpitch);
			int x = x0;
			while (PackedRleRun(rle)) {
				x += rle[0];
				int count = rle[1];
				unsigned char* zdelta = rle + 2;
				unsigned char* src = rle + 2 + 2 * count;
				rle += 3 * count + 2;
				int xEnd = x + count;
				int xc = x;
				int sourceOffset = 0;
				x = xEnd;
				if (xc < ViewXMin()) {
					sourceOffset = ViewXMin() - xc;
					src += sourceOffset;
					count -= sourceOffset;
					xc = ViewXMin();
				}
				if (xEnd > ViewXMax()) {
					count = ViewXMax() - xc;
				}
				for (int i = 0; i < count; ++i) {
					short zv = (short) (PackedRead<short>(zdelta + 2 * (sourceOffset + i)) + z);
					if (zv > zrow[xc + i]) {
						zrow[xc + i] = zv;
						dst[xc + i] = ExpandRGB16(((unsigned short*) palette)[src[i]]);
					}
				}
			}
			rle += 2;
		}
		return 0;
	}
	if (!(flag2 & 8)) {

		z += 1024;
		if (z > 0x7fff) {
			z = 0x7fff;
		}
		int rows = nRows;
		if (yTop < ViewYMin()) {
			for (int skip = ViewYMin() - yTop; skip > 0; --skip) {
				while (PackedRleRun(rle)) {
					rle += 2 * rle[1] + 2;
				}
				rle += 2;
				--rows;
			}
			yTop = ViewYMin();
		}
		int zstep = 0;
		if (m_unk0x24 > m_footprintHeight) {
			z += 8 * rows;
			zstep = -8;
		}
		for (int y = yTop; y < yEnd; ++y, z += zstep) {
			unsigned int* dst = (unsigned int*) (surface + 4 * y * pitch);

			unsigned short* zrow = (unsigned short*) (zbase + 2 * y * zpitch);
			int x = x0;
			while (PackedRleRun(rle)) {
				x += rle[0];
				int count = rle[1];
				unsigned char* src = rle + 2;
				rle += 2 * count + 2;
				int xEnd = x + count;
				int xc = x;
				int sourceOffset = 0;
				x = xEnd;
				if (xc < ViewXMin()) {
					sourceOffset = ViewXMin() - xc;
					count -= sourceOffset;
					xc = ViewXMin();
				}
				if (xEnd > ViewXMax()) {
					count = ViewXMax() - xc;
				}
				for (int i = 0; i < count; ++i) {
					if (z >= zrow[xc + i]) {
						zrow[xc + i] = (unsigned short) z;
						dst[xc + i] = ExpandRGB16(PackedRead<unsigned short>(src + 2 * (sourceOffset + i)));
					}
				}
			}
			rle += 2;
		}
		return 0;
	}

	z += 1024;
	if (z > 0x7fff) {
		z = 0x7fff;
	}
	int rows = nRows;
	if (yTop < ViewYMin()) {
		for (int skip = ViewYMin() - yTop; skip > 0; --skip) {
			while (PackedRleRun(rle)) {
				rle += rle[1] + 2;
			}
			rle += 2;
			--rows;
		}
		yTop = ViewYMin();
	}
	int zstep = 0;
	if (m_unk0x24 > m_footprintHeight) {
		z += 8 * rows;
		zstep = -8;
	}
	for (int y = yTop; y < yEnd; ++y, z += zstep) {
		unsigned int* dst = (unsigned int*) (surface + 4 * y * pitch);

		unsigned short* zrow = (unsigned short*) (zbase + 2 * y * zpitch);
		int x = x0;
		while (PackedRleRun(rle)) {
			x += rle[0];
			int count = rle[1];
			unsigned char* src = rle + 2;
			rle += count + 2;
			int xEnd = x + count;
			int xc = x;
			x = xEnd;
			if (xc < ViewXMin()) {
				src += ViewXMin() - xc;
				count -= ViewXMin() - xc;
				xc = ViewXMin();
			}
			if (xEnd > ViewXMax()) {
				count = ViewXMax() - xc;
			}
			for (int i = 0; i < count; ++i) {
				if (z >= zrow[xc + i]) {
					zrow[xc + i] = (unsigned short) z;
					dst[xc + i] = ExpandRGB16(((unsigned short*) palette)[src[i]]);
				}
			}
		}
		rle += 2;
	}
	return 0;
}

// STUB: ALIEN 0x418940
void VID_SOFTWARE16::DrawToVid(const SPRITE* p_sprite,
							   const VID_TEXCOOR* p_texCoor,
							   TEXTURE* p_texture,
							   TEXTURE* p_zTexture)
{
	if (m_unk0x47c & 0x40) {
		return;
	}
	DrawFrameToVid(p_sprite->m_noCadr,
				   p_sprite->m_x,
				   p_sprite->m_y,
				   p_sprite->m_z,
				   p_sprite->m_flag,
				   p_texCoor,
				   p_texture,
				   p_zTexture);
}

void VID_SOFTWARE16::DrawFrameToVid(int p_frame,
									float p_x,
									float p_y,
									float p_z,
									unsigned int p_spriteFlags,
									const VID_TEXCOOR* p_texCoor,
									TEXTURE* p_texture,
									TEXTURE* p_zTexture)
{
	const VID_TEXCOOR* coor = p_texCoor;
	TEXTURE* colorTex = p_texture;
	TEXTURE* zTex = p_zTexture;

	if (m_unk0x47c & 0x40) {
		return;
	}

	int width = m_unk0x2f6;
	int height = m_messageLineHeight;

	int x0 = (int) (p_x - width / 2 - coor->m_offsetX - coor->m_x);
	int y0 = (int) (p_y - p_z - height / 2 - coor->m_offsetY - coor->m_y);
	if (!(x0 + width >= ViewXMin() && x0 < ViewXMax() && y0 + height >= ViewYMin() && y0 < ViewYMax())) {
		return;
	}

	int z = (int) (p_z * 8.0f);
	if ((m_flag & 0x8000) && z < 0x3fff) {
		z += 0x3fff;
	}
	else if (m_flag & 0x10000) {
		int bob = (int) (FSin[(CurrentTime >> 3) & 0xff] * m_unk0x60 * 8.0f);
		z += bob;
		y0 -= bob / 8;
	}

	unsigned char* frame = (unsigned char*) m_unk0x48c + m_unk0x484[p_frame];
	short contourCount = PackedRead<short>(frame);
	unsigned char* header = frame + 6 * contourCount + 2;
	int nRows = y0 + PackedRead<short>(header);
	unsigned char* rle = header + 4;
	int yTop = PackedRead<short>(header + 2);
	int yEnd = nRows + yTop;
	if (nRows >= ViewYMax() || yEnd < ViewYMin()) {
		return;
	}
	if (yEnd > ViewYMax()) {
		yEnd = ViewYMax();
	}

	if (GPU_RENDER::Active()) {
		int army = (m_fontFlag & 4) ? ((p_spriteFlags >> 11) & 3) * PaletteSize() : 0;
		const void* palette = static_cast<const char*>(m_unk0x48c) + army;
		GPU_VID::DrawFrame(this, rle, yTop, x0, nRows, z, 1.0f, 0, palette, true, colorTex, zTex);
		return;
	}
	int zpitch;
	int pitch;
	char* zbase = (char*) zTex->Lock(&zpitch, 0);
	zpitch /= 2;
	char* surface = (char*) colorTex->Lock(&pitch, 0);
	pitch /= 2;

	unsigned short flag2 = m_pixelFlag16;
	if ((flag2 & 2) && (flag2 & 1) && (flag2 & 8)) {

		z += 1024;
		if (z > 0x7fff) {
			z = 0x7fff;
		}
		int rows = yTop;
		if (nRows < ViewYMin()) {
			for (int skip = ViewYMin() - nRows; skip > 0; --skip) {
				while (PackedRleRun(rle)) {
					rle += rle[1] + 2;
				}
				rle += 2;
				--rows;
			}
			nRows = ViewYMin();
		}
		int zstep = 0;
		if (m_unk0x24 > m_footprintHeight) {
			z += 8 * rows;
			zstep = -8;
		}
		int army = (m_fontFlag & 4) ? ((p_spriteFlags >> 11) & 3) * PaletteSize() : 0;
		int* palette = (int*) ((char*) m_unk0x48c + army);
		unsigned short* dst = (unsigned short*) surface + nRows * pitch;
		unsigned short* dstEnd = (unsigned short*) surface + yEnd * pitch;
		unsigned short* zrow = (unsigned short*) zbase + nRows * zpitch;
		unsigned short spanZ = (unsigned short) z;
		if (x0 >= ViewXMin() && x0 + width <= ViewXMax()) {
			while (dst < dstEnd) {
				int x = x0;
				while (PackedRleRun(rle)) {
					int count = rle[1];
					x += rle[0];
					rle += 2;
					DrawTerrainAlphaSpan(rle,
										 zrow + x,
										 dst + x,
										 count,
										 spanZ,
										 palette,
										 x,
										 (int) ((dst - (unsigned short*) surface) / pitch));
					rle += count;
					x += count;
				}
				rle += 2;
				dst += pitch;
				zrow += zpitch;
				spanZ = (unsigned short) (spanZ + zstep);
			}
		}
		else {
			while (dst < dstEnd) {
				int x = x0;
				while (PackedRleRun(rle)) {
					int skip = rle[0];
					int count = rle[1];
					x += skip;
					unsigned char* src = rle + 2;
					int runEnd = x + count;
					if (x < ViewXMin()) {
						if (runEnd > ViewXMax()) {
							DrawTerrainAlphaSpan(src + ViewXMin() - x,
												 zrow + ViewXMin(),
												 dst + ViewXMin(),
												 ViewXMax() - ViewXMin(),
												 spanZ,
												 palette,
												 ViewXMin(),
												 (int) ((dst - (unsigned short*) surface) / pitch));
						}
						else if (runEnd > ViewXMin()) {
							DrawTerrainAlphaSpan(src + ViewXMin() - x,
												 zrow + ViewXMin(),
												 dst + ViewXMin(),
												 count - (ViewXMin() - x),
												 spanZ,
												 palette,
												 ViewXMin(),
												 (int) ((dst - (unsigned short*) surface) / pitch));
						}
					}
					else if (runEnd > ViewXMax()) {
						if (x < ViewXMax()) {
							DrawTerrainAlphaSpan(src,
												 zrow + x,
												 dst + x,
												 ViewXMax() - x,
												 spanZ,
												 palette,
												 x,
												 (int) ((dst - (unsigned short*) surface) / pitch));
						}
					}
					else {
						DrawTerrainAlphaSpan(src,
											 zrow + x,
											 dst + x,
											 count,
											 spanZ,
											 palette,
											 x,
											 (int) ((dst - (unsigned short*) surface) / pitch));
					}
					rle = src + count;
					x = runEnd;
				}
				rle += 2;
				dst += pitch;
				zrow += zpitch;
				spanZ = (unsigned short) (spanZ + zstep);
			}
		}
	}
	else if (flag2 & 1) {
		if ((flag2 & 8) && (flag2 & 4)) {

			int army = (m_fontFlag & 4) ? ((p_spriteFlags >> 11) & 3) * PaletteSize() : 0;
			unsigned short* palette = (unsigned short*) ((char*) m_unk0x48c + army);
			short baseZ = (short) z;
			if (nRows < ViewYMin()) {
				int skip = ViewYMin() - nRows;
				nRows += skip;
				do {
					while (PackedRleRun(rle)) {
						rle += 3 * rle[1] + 2;
					}
					rle += 2;
				} while (--skip);
			}
			if (x0 >= ViewXMin() && x0 + width <= ViewXMax()) {
				for (int y = nRows; y < yEnd; ++y) {
					unsigned short* dst = (unsigned short*) (surface + 2 * y * pitch);
					short* zrow = (short*) (zbase + 2 * y * zpitch);
					int x = x0;
					while (PackedRleRun(rle)) {
						x += rle[0];
						int count = rle[1];
						rle += 2;
						unsigned char* zdelta = rle;
						unsigned char* src = rle + 2 * count;
						for (int i = 0; i < count; ++i) {
							short zv = (short) (baseZ + PackedRead<short>(zdelta + 2 * i));
							if (zv > zrow[x + i]) {
								zrow[x + i] = zv;
								dst[x + i] = palette[src[i]];
								TerrainCoverageMarkPixel(x + i, y);
							}
						}
						rle = src + count;
						x += count;
					}
					rle += 2;
				}
			}
			else {
				for (int y = nRows; y < yEnd; ++y) {
					unsigned short* dst = (unsigned short*) (surface + 2 * y * pitch);
					short* zrow = (short*) (zbase + 2 * y * zpitch);
					int x = x0;
					while (PackedRleRun(rle)) {
						x += rle[0];
						int count = rle[1];
						rle += 2;
						unsigned char* zdelta = rle;
						unsigned char* src = rle + 2 * count;
						rle = src + count;
						int xEnd = x + count;
						int xc = x;
						int sourceOffset = 0;
						if (xc < ViewXMin()) {
							sourceOffset = ViewXMin() - xc;
							src += sourceOffset;
							count -= sourceOffset;
							xc = ViewXMin();
						}
						if (xEnd > ViewXMax()) {
							count = ViewXMax() - xc;
						}
						for (int i = 0; i < count; ++i) {
							short zv = (short) (baseZ + PackedRead<short>(zdelta + 2 * (sourceOffset + i)));
							if (zv > zrow[xc + i]) {
								zrow[xc + i] = zv;
								dst[xc + i] = palette[src[i]];
								TerrainCoverageMarkPixel(xc + i, y);
							}
						}
						x = xEnd;
					}
					rle += 2;
				}
			}
		}
		else if (!(flag2 & 8)) {

			z += 1024;
			if (z > 0x7fff) {
				z = 0x7fff;
			}
			int rows = yTop;
			if (nRows < ViewYMin()) {
				for (int skip = ViewYMin() - nRows; skip > 0; --skip) {
					while (PackedRleRun(rle)) {
						rle += 2 * rle[1] + 2;
					}
					rle += 2;
					--rows;
				}
				nRows = ViewYMin();
			}
			int zstep = 0;
			if (m_unk0x24 > m_footprintHeight) {
				z += 8 * rows;
				zstep = -8;
			}
			for (int y = nRows; y < yEnd; ++y, z += zstep) {
				unsigned short* dst = (unsigned short*) (surface + 2 * y * pitch);

				unsigned short* zrow = (unsigned short*) (zbase + 2 * y * zpitch);
				int x = x0;
				while (PackedRleRun(rle)) {
					x += rle[0];
					int count = rle[1];
					rle += 2;
					unsigned char* src = rle;
					rle += 2 * count;
					int xEnd = x + count;
					int xc = x;
					int sourceOffset = 0;
					x = xEnd;
					if (xc < ViewXMin()) {
						sourceOffset = ViewXMin() - xc;
						count -= sourceOffset;
						xc = ViewXMin();
					}
					if (xEnd > ViewXMax()) {
						count = ViewXMax() - xc;
					}
					for (int i = 0; i < count; ++i) {
						if (z >= zrow[xc + i]) {
							zrow[xc + i] = (unsigned short) z;
							dst[xc + i] = PackedRead<unsigned short>(src + 2 * (sourceOffset + i));
							TerrainCoverageMarkPixel(xc + i, y);
						}
					}
				}
				rle += 2;
			}
		}
		else {

			int army = (m_fontFlag & 4) ? ((p_spriteFlags >> 11) & 3) * PaletteSize() : 0;
			unsigned short* palette = (unsigned short*) ((char*) m_unk0x48c + army);
			z += 1024;
			if (z > 0x7fff) {
				z = 0x7fff;
			}
			int rows = yTop;
			if (nRows < ViewYMin()) {
				for (int skip = ViewYMin() - nRows; skip > 0; --skip) {
					while (PackedRleRun(rle)) {
						rle += rle[1] + 2;
					}
					rle += 2;
					--rows;
				}
				nRows = ViewYMin();
			}
			int zstep = 0;
			if (m_unk0x24 > m_footprintHeight) {
				z += 8 * rows;
				zstep = -8;
			}
			for (int y = nRows; y < yEnd; ++y, z += zstep) {
				unsigned short* dst = (unsigned short*) (surface + 2 * y * pitch);

				unsigned short* zrow = (unsigned short*) (zbase + 2 * y * zpitch);
				int x = x0;
				while (PackedRleRun(rle)) {
					x += rle[0];
					int count = rle[1];
					unsigned char* src = rle + 2;
					rle += count + 2;
					int xEnd = x + count;
					int xc = x;
					x = xEnd;
					if (xc < ViewXMin()) {
						src += ViewXMin() - xc;
						count -= ViewXMin() - xc;
						xc = ViewXMin();
					}
					if (xEnd > ViewXMax()) {
						count = ViewXMax() - xc;
					}
					for (int i = 0; i < count; ++i) {
						if (z >= zrow[xc + i]) {
							zrow[xc + i] = (unsigned short) z;
							dst[xc + i] = palette[src[i]];
							TerrainCoverageMarkPixel(xc + i, y);
						}
					}
				}
				rle += 2;
			}
		}
	}
}
