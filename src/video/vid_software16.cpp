#include "video/vid_software16.h"

#include <string.h>

#include "game/gametime.h"
#include "game/map.h"
#include "gfx/asmdraw.h"
#include "gfx/gamma.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "sprite/ex_sprite_data.h"
#include "sprite/sprite.h"
#include "gfx/texture.h"
#include "util/myerror.h"
#include "video/vid_hardware.h"

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

extern int RGB16_rMask;
extern int RGB16_gMask;

static __forceinline GAMMA* CombineDrawGamma(
	GAMMA* p_result, const GAMMA& p_gamma, int p_screenB, int p_screenA)
{
	return p_result->Add(p_gamma, GAMMA(GAMMA::RAW_COPY, p_screenA, p_screenB));
}

// STUB: ALIEN 0x416030
void VID_SOFTWARE16::SetGammaToPalette(unsigned char* p_palette, const GAMMA& p_gamma)
{
	if (!p_palette)
		return;
	if (!p_gamma.m_a && !p_gamma.m_b)
		return;
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
			*entry = (unsigned short) ((((unsigned int) c.m_value >> 3) & 0x1f)
				| (RGB16_rMask & ((unsigned int) c.m_value >> (16 - RGB16_rShift)))
				| (RGB16_gMask & ((unsigned int) c.m_value >> (8 - RGB16_gShift))));
			++entry;
		} while (--n);
	}
}

#pragma optimize("y", off)
// STUB: ALIEN 0x417780
int VID_SOFTWARE16::Draw(SPRITE* p_sprite)
{
	if (m_unk0x47c & 0x40)
		return 0;

	int width = m_unk0x2f6;
	int height = m_messageLineHeight;
	int x0 = (int) p_sprite->m_x - (int) Map->m_shiftX - width / 2;
	int y0 = (int) (p_sprite->m_y - p_sprite->m_z) - (int) Map->m_shiftY - height / 2;
	if (x0 + width < viewXMin || x0 >= viewXMax || y0 + height < viewYMin || y0 >= viewYMax)
		return 0;

	int z = (int) (p_sprite->m_z * 8.0f);
	if ((m_flag & 0x8000) && z < 0x3fff) {
		z += 0x3fff;
	}
	else if (m_flag & 0x10000) {
		int bob = (int) (FSin[(CurrentTime >> 3) & 0xff] * m_unk0x60 * 8.0f);
		z += bob;
		y0 -= bob / 8;
	}

	short* frame = (short*) ((char*) m_unk0x48c + ((int*) m_unk0x484)[p_sprite->m_noCadr]);
	short* header = frame + 3 * frame[0] + 1;
	int nRows = header[1];
	unsigned char* rle = (unsigned char*) (header + 2);
	int yTop = y0 + header[0];
	int yEnd = yTop + nRows;
	if (yTop >= viewYMax || yEnd < viewYMin)
		return 0;
	if (yEnd > viewYMax)
		yEnd = viewYMax;

	GRAPH_CORE* graph = (GRAPH_CORE*) Graph;
	if (!graph->m_locked) {
		D3DLOCKED_RECT rect;
		rect.pBits = 0;
		rect.Pitch = 0;
		if (graph->m_backBuffer->LockRect(&rect, 0, 0) < 0 && ::Error)
			MYERROR::Error(::Error,
				"GRAPH", 0,
				"backBuffer", 0);
		graph->m_locked = (int) rect.pBits;
		graph->m_unk0x248 = rect.Pitch / ((graph->m_flags & 2) ? 4 : 2);
	}
	int pitch = graph->m_unk0x248;
	int zpitch = graph->m_unk0x250;
	char* surface = (char*) graph->m_locked;
	char* zbase = (char*) graph->m_zbuffer;

	unsigned char paletteBuf[1024];
	EX_SPRITE_DATA* exData = p_sprite->m_exData;
	if (!exData || (!exData->m_unk0x24 && !exData->m_unk0x28)) {
		int armyOffset =
			(m_fontFlag & 4) ? ((p_sprite->m_flag >> 11) & 3) * PaletteSize() : 0;
		AsmDrawPalette = (int*) ((char*) m_unk0x48c + armyOffset);
	}
	else {
		AsmDrawPalette = (int*) paletteBuf;
		int gammaOffset = (m_fontFlag & 4) ? 4 * PaletteSize() : 0;
		memcpy(paletteBuf, (char*) m_unk0x48c + gammaOffset, PaletteSize());
		if (m_flag & 0x800) {
			SetGammaToPalette(paletteBuf, p_sprite->GetGamma());
		}
		else {
			GAMMA combined;
			CombineDrawGamma(
				&combined, p_sprite->GetGamma(),
				((GRAPH_CORE*) Graph)->m_gammaSet.m_b,
				((GRAPH_CORE*) Graph)->m_gammaSet.m_a);
			SetGammaToPalette(paletteBuf, combined);
		}
	}
	frame = (short*) AsmDrawPalette;

	unsigned short flag2 = m_pixelFlag16;
	if ((flag2 & 2) && (flag2 & 1) && (flag2 & 8)) {

		z += 1024;
		if (z > 0x7fff)
			z = 0x7fff;
		int rows = nRows;
		if (yTop < viewYMin) {
			for (int skip = viewYMin - yTop; skip > 0; --skip) {
				while (*(unsigned short*) rle)
					rle += rle[1] + 2;
				rle += 2;
				--rows;
			}
			yTop = viewYMin;
		}
		int zstep = 0;
		if (m_unk0x24 > m_footprintHeight) {
			z += 8 * rows;
			zstep = -8;
		}
		int y = yTop;
		unsigned short* dst;
		unsigned short* zrow;
		int x;
		int count;
		unsigned char* src;
		int xEnd;
		int xc;
		if (x0 >= viewXMin && x0 + width <= viewXMax) {
			for (; y < yEnd; ++y, z += zstep) {
				AsmDrawData[0] = (short) z;
				dst = (unsigned short*) (surface + 2 * y * pitch);
				zrow = (unsigned short*) (zbase + 2 * y * zpitch);
				x = x0;
				while (*(unsigned short*) rle) {
					x += rle[0];
					count = rle[1];
					src = rle + 2;
					rle += count + 2;
					AsmDrawWithAlpha16(src, zrow + x, dst + x, count);
					x += count;
				}
				rle += 2;
			}
		}
		else {
			for (; y < yEnd; ++y, z += zstep) {
				AsmDrawData[0] = (short) z;
				dst = (unsigned short*) (surface + 2 * y * pitch);
				zrow = (unsigned short*) (zbase + 2 * y * zpitch);
				x = x0;
				while (*(unsigned short*) rle) {
					x += rle[0];
					count = rle[1];
					src = rle + 2;
					rle += count + 2;
					xEnd = x + count;
					xc = x;
					x = xEnd;
					if (xc < viewXMin) {
						src += viewXMin - xc;
						count -= viewXMin - xc;
						xc = viewXMin;
					}
					if (xEnd > viewXMax)
						count = viewXMax - xc;
					if (count > 0)
						AsmDrawWithAlpha16(src, zrow + xc, dst + xc, count);
				}
				rle += 2;
			}
		}
		return 0;
	}
	if (!(flag2 & 1))
		return 0;
	if ((flag2 & 8) && (flag2 & 4)) {

		if (yTop < viewYMin) {
			for (int skip = viewYMin - yTop; skip > 0; --skip) {
				while (*(unsigned short*) rle)
					rle += 3 * rle[1] + 2;
				rle += 2;
			}
			yTop = viewYMin;
		}
		for (int y = yTop; y < yEnd; ++y) {
			unsigned short* dst = (unsigned short*) (surface + 2 * y * pitch);
			short* zrow = (short*) (zbase + 2 * y * zpitch);
			int x = x0;
			while (*(unsigned short*) rle) {
				x += rle[0];
				int count = rle[1];
				short* zdelta = (short*) (rle + 2);
				unsigned char* src = rle + 2 + 2 * count;
				rle += 3 * count + 2;
				int xEnd = x + count;
				int xc = x;
				x = xEnd;
				if (xc < viewXMin) {
					zdelta += viewXMin - xc;
					src += viewXMin - xc;
					count -= viewXMin - xc;
					xc = viewXMin;
				}
				if (xEnd > viewXMax)
					count = viewXMax - xc;
				for (int i = 0; i < count; ++i) {
					short zv = (short) (zdelta[i] + z);
					if (zv > zrow[xc + i]) {
						zrow[xc + i] = zv;
						dst[xc + i] = ((unsigned short*) AsmDrawPalette)[src[i]];
					}
				}
			}
			rle += 2;
		}
		return 0;
	}
	if (!(flag2 & 8)) {

		z += 1024;
		if (z > 0x7fff)
			z = 0x7fff;
		int rows = nRows;
		if (yTop < viewYMin) {
			for (int skip = viewYMin - yTop; skip > 0; --skip) {
				while (*(unsigned short*) rle)
					rle += 2 * rle[1] + 2;
				rle += 2;
				--rows;
			}
			yTop = viewYMin;
		}
		int zstep = 0;
		if (m_unk0x24 > m_footprintHeight) {
			z += 8 * rows;
			zstep = -8;
		}
		for (int y = yTop; y < yEnd; ++y, z += zstep) {
			unsigned short* dst = (unsigned short*) (surface + 2 * y * pitch);

			unsigned short* zrow = (unsigned short*) (zbase + 2 * y * zpitch);
			int x = x0;
			while (*(unsigned short*) rle) {
				x += rle[0];
				int count = rle[1];
				unsigned short* src = (unsigned short*) (rle + 2);
				rle += 2 * count + 2;
				int xEnd = x + count;
				int xc = x;
				x = xEnd;
				if (xc < viewXMin) {
					src += viewXMin - xc;
					count -= viewXMin - xc;
					xc = viewXMin;
				}
				if (xEnd > viewXMax)
					count = viewXMax - xc;
				for (int i = 0; i < count; ++i) {
					if (z >= zrow[xc + i]) {
						zrow[xc + i] = (unsigned short) z;
						dst[xc + i] = src[i];
					}
				}
			}
			rle += 2;
		}
		return 0;
	}

	z += 1024;
	if (z > 0x7fff)
		z = 0x7fff;
	int rows = nRows;
	if (yTop < viewYMin) {
		for (int skip = viewYMin - yTop; skip > 0; --skip) {
			while (*(unsigned short*) rle)
				rle += rle[1] + 2;
			rle += 2;
			--rows;
		}
		yTop = viewYMin;
	}
	int zstep = 0;
	if (m_unk0x24 > m_footprintHeight) {
		z += 8 * rows;
		zstep = -8;
	}
	for (int y = yTop; y < yEnd; ++y, z += zstep) {
		unsigned short* dst = (unsigned short*) (surface + 2 * y * pitch);

		unsigned short* zrow = (unsigned short*) (zbase + 2 * y * zpitch);
		int x = x0;
		while (*(unsigned short*) rle) {
			x += rle[0];
			int count = rle[1];
			unsigned char* src = rle + 2;
			rle += count + 2;
			int xEnd = x + count;
			int xc = x;
			x = xEnd;
			if (xc < viewXMin) {
				src += viewXMin - xc;
				count -= viewXMin - xc;
				xc = viewXMin;
			}
			if (xEnd > viewXMax)
				count = viewXMax - xc;
			for (int i = 0; i < count; ++i) {
				if (z >= zrow[xc + i]) {
					zrow[xc + i] = (unsigned short) z;
					dst[xc + i] = frame[src[i]];
				}
			}
		}
		rle += 2;
	}
	return 0;
}

// STUB: ALIEN 0x418940
void VID_SOFTWARE16::DrawToVid(const SPRITE* p_sprite, const VID_TEXCOOR* p_texCoor,
	TEXTURE* p_texture, TEXTURE* p_zTexture)
{
	const VID_TEXCOOR* coor = p_texCoor;
	TEXTURE* colorTex = p_texture;
	TEXTURE* zTex = p_zTexture;
	SPRITE* sprite = (SPRITE*) p_sprite;

	if (m_unk0x47c & 0x40)
		return;

	int width = m_unk0x2f6;
	int height = m_messageLineHeight;

	int x0 = (int) (sprite->m_x - width / 2 - coor->m_offsetX - coor->m_x);
	int y0 = (int) (sprite->m_y - sprite->m_z - height / 2 - coor->m_offsetY - coor->m_y);
	if (!(x0 + width >= viewXMin && x0 < viewXMax && y0 + height >= viewYMin && y0 < viewYMax))
		return;

	int z = (int) (sprite->m_z * 8.0f);
	if ((m_flag & 0x8000) && z < 0x3fff) {
		z += 0x3fff;
	}
	else if (m_flag & 0x10000) {
		int bob = (int) (FSin[(CurrentTime >> 3) & 0xff] * m_unk0x60 * 8.0f);
		z += bob;
		y0 -= bob / 8;
	}

	short* frame = (short*) ((char*) m_unk0x48c + ((int*) m_unk0x484)[sprite->m_noCadr]);
	short* header = frame + 3 * frame[0] + 1;
	int nRows = y0 + header[0];
	unsigned char* rle = (unsigned char*) (header + 2);
	int yTop = header[1];
	int yEnd = nRows + yTop;
	if (nRows >= viewYMax || yEnd < viewYMin)
		return;
	if (yEnd > viewYMax)
		yEnd = viewYMax;

	int zpitch;
	int pitch;
	char* zbase = (char*) zTex->Lock(&zpitch, 0);
	zpitch /= 2;
	char* surface = (char*) colorTex->Lock(&pitch, 0);
	pitch /= 2;

	unsigned short flag2 = m_pixelFlag16;
	if ((flag2 & 2) && (flag2 & 1) && (flag2 & 8)) {

		z += 1024;
		if (z > 0x7fff)
			z = 0x7fff;
		int rows = yTop;
		if (nRows < viewYMin) {
			for (int skip = viewYMin - nRows; skip > 0; --skip) {
				while (*(unsigned short*) rle)
					rle += rle[1] + 2;
				rle += 2;
				--rows;
			}
			nRows = viewYMin;
		}
		int zstep = 0;
		if (m_unk0x24 > m_footprintHeight) {
			z += 8 * rows;
			zstep = -8;
		}
		int army = (m_fontFlag & 4) ? ((sprite->m_flag >> 11) & 3) * PaletteSize() : 0;
		AsmDrawPalette = (int*) ((char*) m_unk0x48c + army);
		unsigned short* dst = (unsigned short*) surface + nRows * pitch;
		unsigned short* dstEnd = (unsigned short*) surface + yEnd * pitch;
		unsigned short* zrow = (unsigned short*) zbase + nRows * zpitch;
		AsmDrawData[0] = (short) z;
		if (x0 >= viewXMin && x0 + width <= viewXMax) {
			while (dst < dstEnd) {
				int x = x0;
				while (*(unsigned short*) rle) {
					int count = rle[1];
					x += rle[0];
					rle += 2;
					AsmDrawWithAlpha16(rle, zrow + x, dst + x, count);
					rle += count;
					x += count;
				}
				rle += 2;
				dst += pitch;
				zrow += zpitch;
				AsmDrawData[0] += (short) zstep;
			}
		}
		else {
			while (dst < dstEnd) {
				int x = x0;
				while (*(unsigned short*) rle) {
					int skip = rle[0];
					int count = rle[1];
					x += skip;
					unsigned char* src = rle + 2;
					int runEnd = x + count;
					if (x < viewXMin) {
						if (runEnd > viewXMax) {
							AsmDrawWithAlpha16(src + viewXMin - x, zrow + viewXMin,
								dst + viewXMin, viewXMax - viewXMin);
						}
						else if (runEnd > viewXMin) {
							AsmDrawWithAlpha16(src + viewXMin - x, zrow + viewXMin,
								dst + viewXMin, count - (viewXMin - x));
						}
					}
					else if (runEnd > viewXMax) {
						if (x < viewXMax)
							AsmDrawWithAlpha16(src, zrow + x, dst + x, viewXMax - x);
					}
					else {
						AsmDrawWithAlpha16(src, zrow + x, dst + x, count);
					}
					rle = src + count;
					x = runEnd;
				}
				rle += 2;
				dst += pitch;
				zrow += zpitch;
				AsmDrawData[0] += (short) zstep;
			}
		}
	}
	else if (flag2 & 1) {
		if ((flag2 & 8) && (flag2 & 4)) {

			int army =
				(m_fontFlag & 4) ? ((sprite->m_flag >> 11) & 3) * PaletteSize() : 0;
			AsmDrawPalette = (int*) ((char*) m_unk0x48c + army);
			AsmDrawData[0] = (short) z;
			AsmDrawData[1] = (short) z;
			AsmDrawData[2] = (short) z;
			AsmDrawData[3] = (short) z;
			if (nRows < viewYMin) {
				int skip = viewYMin - nRows;
				nRows += skip;
				do {
					while (*(unsigned short*) rle)
						rle += 3 * rle[1] + 2;
					rle += 2;
				} while (--skip);
			}
			if (x0 >= viewXMin && x0 + width <= viewXMax) {
				for (int y = nRows; y < yEnd; ++y) {
					unsigned short* dst = (unsigned short*) (surface + 2 * y * pitch);
					short* zrow = (short*) (zbase + 2 * y * zpitch);
					int x = x0;
					while (*(unsigned short*) rle) {
						x += rle[0];
						int count = rle[1];
						rle += 2;
						short* zdelta = (short*) rle;
						unsigned char* src = rle + 2 * count;
						for (int i = 0; i < count; ++i) {
							short zv = (short) (AsmDrawData[0] + zdelta[i]);
							if (zv > zrow[x + i]) {
								zrow[x + i] = zv;
								dst[x + i] = ((unsigned short*) AsmDrawPalette)[src[i]];
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
					while (*(unsigned short*) rle) {
						x += rle[0];
						int count = rle[1];
						rle += 2;
						short* zdelta = (short*) rle;
						unsigned char* src = rle + 2 * count;
						rle = src + count;
						int xEnd = x + count;
						int xc = x;
						if (xc < viewXMin) {
							zdelta += viewXMin - xc;
							src += viewXMin - xc;
							count -= viewXMin - xc;
							xc = viewXMin;
						}
						if (xEnd > viewXMax)
							count = viewXMax - xc;
						for (int i = 0; i < count; ++i) {
							short zv = (short) (AsmDrawData[0] + zdelta[i]);
							if (zv > zrow[xc + i]) {
								zrow[xc + i] = zv;
								dst[xc + i] = ((unsigned short*) AsmDrawPalette)[src[i]];
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
			if (z > 0x7fff)
				z = 0x7fff;
			int rows = yTop;
			if (nRows < viewYMin) {
				for (int skip = viewYMin - nRows; skip > 0; --skip) {
					while (*(unsigned short*) rle)
						rle += 2 * rle[1] + 2;
					rle += 2;
					--rows;
				}
				nRows = viewYMin;
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
				while (*(unsigned short*) rle) {
					x += rle[0];
					int count = rle[1];
					rle += 2;
					unsigned short* src = (unsigned short*) rle;
					rle = (unsigned char*) (src + count);
					int xEnd = x + count;
					int xc = x;
					x = xEnd;
					if (xc < viewXMin) {
						src += viewXMin - xc;
						count -= viewXMin - xc;
						xc = viewXMin;
					}
					if (xEnd > viewXMax)
						count = viewXMax - xc;
					for (int i = 0; i < count; ++i) {
						if (z >= zrow[xc + i]) {
							zrow[xc + i] = (unsigned short) z;
							dst[xc + i] = src[i];
						}
					}
				}
				rle += 2;
			}
		}
		else {

			int army =
				(m_fontFlag & 4) ? ((sprite->m_flag >> 11) & 3) * PaletteSize() : 0;
			AsmDrawPalette = (int*) ((char*) m_unk0x48c + army);
			z += 1024;
			if (z > 0x7fff)
				z = 0x7fff;
			int rows = yTop;
			if (nRows < viewYMin) {
				for (int skip = viewYMin - nRows; skip > 0; --skip) {
					while (*(unsigned short*) rle)
						rle += rle[1] + 2;
					rle += 2;
					--rows;
				}
				nRows = viewYMin;
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
				while (*(unsigned short*) rle) {
					x += rle[0];
					int count = rle[1];
					unsigned char* src = rle + 2;
					rle += count + 2;
					int xEnd = x + count;
					int xc = x;
					x = xEnd;
					if (xc < viewXMin) {
						src += viewXMin - xc;
						count -= viewXMin - xc;
						xc = viewXMin;
					}
					if (xEnd > viewXMax)
						count = viewXMax - xc;
					for (int i = 0; i < count; ++i) {
						if (z >= zrow[xc + i]) {
							zrow[xc + i] = (unsigned short) z;
							dst[xc + i] = ((unsigned short*) AsmDrawPalette)[src[i]];
						}
					}
				}
				rle += 2;
			}
		}
	}

	if (colorTex->m_texture)
		colorTex->m_texture->UnlockRect(0);
	if (zTex->m_texture)
		zTex->m_texture->UnlockRect(0);
}
#pragma optimize("", on)
