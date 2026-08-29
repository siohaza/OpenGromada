#include "video/vid_software.h"

#include <string.h>

#include "compress/qs1_coder.h"
#include "game/gametime.h"
#include "game/map.h"
#include "gfx/asmdraw.h"
#include "gfx/color.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "gfx/texture.h"
#include "sprite/ex_sprite_data.h"
#include "sprite/sprite.h"
#include "util/myerror.h"
#include "util/resource.h"

extern float FSin[256];

inline COLOR::COLOR(const GAMMA& p_gamma, const COLOR& p_color)
{
	if (p_gamma.m_a || p_gamma.m_b) {
		unsigned int c = p_color.m_value;
		unsigned int gb = p_gamma.m_b;
		int b = (gb & 0xff) + ((((~p_gamma.m_a) & 0xff) + 1) * (c & 0xff) >> 8);
		int g = ((gb >> 8) & 0xff) + (((((~p_gamma.m_a) >> 8) & 0xff) + 1) * ((c >> 8) & 0xff) >> 8);
		int r = ((gb >> 16) & 0xff) + (((((~p_gamma.m_a) >> 16) & 0xff) + 1) * ((c >> 16) & 0xff) >> 8);
		COLOR t(r, g, b);
		m_value = t.m_value;
	}
	else
		m_value = p_color.m_value;
}

static inline GAMMA ScreenGamma(const GRAPH_CORE* p_graph)
{
	return GAMMA(GAMMA::RAW_COPY, p_graph->m_gammaSet.m_a, p_graph->m_gammaSet.m_b);
}

static __forceinline GAMMA* CombineDrawGamma(
	GAMMA* p_result, const GAMMA& p_gamma, int p_screenB, int p_screenA)
{
	return p_result->Add(p_gamma, GAMMA(GAMMA::RAW_COPY, p_screenA, p_screenB));
}

static inline int LockDrawGraph(GRAPH_CORE* p_graph)
{
	int result = p_graph->m_locked;
	if (!result) {
		int rect[2];
		if (p_graph->m_backBuffer->LockRect((D3DLOCKED_RECT*) rect, 0, 0) < 0) {
			if (::Error)
				MYERROR::Error(::Error, "GRAPH", 0, "backBuffer", 0);
		}
		p_graph->m_locked = rect[1];
		result = rect[0] / ((p_graph->m_flags & 2) ? 4 : 2);
		p_graph->m_unk0x248 = result;
	}
	return result;
}

// FUNCTION: ALIEN 0x4126b0
int VID_SOFTWARE::PaletteSize()
{
	return 1024;
}

// FUNCTION: ALIEN 0x4126c0
int VID_SOFTWARE::HaveShadow()
{
	int v1 = m_unk0x48c;
	if (v1)
		return *(short*) (*m_unk0x484 + v1);
	return 0;
}

// FUNCTION: ALIEN 0x412730
void* VID_SOFTWARE::ScalarDeletingDestructor(unsigned int p_flags)
{
	VID_SOFTWARE* result = this;
	this->~VID_SOFTWARE();
	if (p_flags & 1)
		operator delete(result);
	return result;
}

// FUNCTION: ALIEN 0x4153e0
VID* VID_SOFTWARE::CreateMirror()
{
	return new VID_SOFTWARE((STREAM*) this);
}

// FUNCTION: ALIEN 0x415410
VID_SOFTWARE::VID_SOFTWARE()
{
	m_unk0x488 = 0;
	m_unk0x48c = 0;
	m_unk0x484 = 0;
}

// FUNCTION: ALIEN 0x415440
VID_SOFTWARE::~VID_SOFTWARE()
{
	if (m_weaponPtr == this) {
		if (m_unk0x48c)
			operator delete((void*) m_unk0x48c);
		m_unk0x48c = 0;
		if (m_unk0x484)
			operator delete(m_unk0x484);
		m_unk0x484 = 0;
		VID::MemoryInUse -= m_unk0x488;
		m_unk0x488 = 0;
	}
}

// STUB: ALIEN 0x4154b0
void VID_SOFTWARE::Load(RESOURCE* p_res)
{
	QS1_CODER* coder;
	if (m_pixelFlag16 & 0x100) {
		coder = new QS1_CODER(1);
	}
	else {
		coder = 0;
	}

	COLOR palette[256];
	if (m_pixelFlag16 & 8) {
		if (!p_res->GoNext(0x204c4150 /* 'PAL ' */ )) {
			if (m_pixelFlag16 & 0x10) {
				p_res->Read(palette, 1024);
			}
			else {
				unsigned char rgb[768];
				p_res->Read(rgb, 768);
				unsigned int* dst = (unsigned int*) palette;
				unsigned char* src = rgb;
				for (int c = 256; c; --c) {
					*dst = COLOR(src[0], src[1], src[2]).m_value;
					src += 3;
					++dst;
				}
			}
			unsigned int* col = (unsigned int*) palette;
			for (int c = 0; c < 256; ++c) {
				palette[c].m_value = COLOR(*(GAMMA*) &m_colorSub, palette[c]).m_value;
			}
		}
		else {
			Error(5,
				"PAL ", 0);
		}
	}

	if (p_res->GoNext(0x41544144 /* 'DATA' */ ))
		Error(5,
			"DATA", 0);
	m_unk0x488 = p_res->m_resSize;
	if (m_pixelFlag16 & 8)
		m_unk0x488 += 2 * PaletteSize();
	m_unk0x48c = (int) operator new(m_unk0x488);
	if (!m_unk0x48c) {
		Error(2,
			"cadr", m_unk0x488);
		return;
	}
	int* frames = (int*) operator new(4 * m_dotFrameCount);
	m_unk0x484 = (char**) frames;
	if (!frames) {
		Error(2,
			// STRING: ALIEN 0x482bd8
			"cadrShift", m_dotFrameCount);
		return;
	}

	int offset;
	if (m_pixelFlag16 & 8) {

		int w = 0;
		for (int c = 0; c < 256; ++c) {
			if (PaletteSize() == 1024)
				*(unsigned int*) ((char*) m_unk0x48c + 4 * c) = palette[c].m_value;
			else
				*(unsigned short*) ((char*) m_unk0x48c + w) = (unsigned short) (((palette[c].m_value >> 3) & 0x1f)
					| (RGB16_rMask & (palette[c].m_value >> (16 - RGB16_rShift)))
					| (RGB16_gMask & (palette[c].m_value >> (8 - RGB16_gShift))));
			w += 2;
		}
		offset = 2 * PaletteSize();
		memcpy((char*) m_unk0x48c + PaletteSize(), (char*) m_unk0x48c, PaletteSize());
	}
	else {
		offset = 0;
	}

	for (int frame = 0; frame < m_dotFrameCount; ++frame) {
		int packedSize;
		p_res->Read(&packedSize, 4);
		int left = p_res->ReadPacked((char*) m_unk0x48c + offset, packedSize, coder);
		if (left)
			Error(5,
				// STRING: ALIEN 0x482bc0
				"Can't decode software", packedSize - left);
		if (packedSize != 2) {
			short flag2 = m_pixelFlag16;
			if (!(flag2 & 8)
				&& ((TEXTURE*) ((GRAPH_CORE*) Graph)->m_texE10)->m_format == 24
				&& !(flag2 & 2)) {

				short* fr = (short*) ((char*) m_unk0x48c + offset);
				short* header = fr + 3 * fr[0] + 1;
				int yTop = header[0];
				int yEnd = yTop + header[1];
				unsigned short* k = (unsigned short*) (header + 2);
				while (yTop < yEnd) {
					while (*k) {
						unsigned char* run = (unsigned char*) k + 1;
						int count = *run;
						k = (unsigned short*) (run + 1);
						while (count > 0) {
							unsigned short px = *k++;
							--count;
							*(k - 1) = (unsigned short) ((px & 0x1f) | ((px >> 1) & 0x7fe0));
						}
					}
					++k;
					++yTop;
				}
			}
			frames[frame] = offset;
			offset += packedSize;
		}
		else {
			frames[frame] = frames[*(short*) ((char*) m_unk0x48c + offset)];
		}
		p_res->GoNextSub(0x41544144 /* 'DATA' */ );
	}
	if (coder)
		delete coder;
	VID::MemoryInUse += m_unk0x488;
	SetLayer();

	if (m_flag & 0x20) {
		short flag2 = m_pixelFlag16;
		if ((flag2 & 8) && (flag2 & 1)) {

			float* scratch = (float*) operator new(0x3000000);
			m_dotFrameStarts = (int*) operator new(4 * m_dotFrameCount);
			for (int fr = 0; fr < m_dotFrameCount; ++fr) {
				short grid[65536];
				int* fill = (int*) grid;
				for (int c = 0; c < 0x8000; ++c)
					fill[c] = 0x83008300;
				m_dotFrameStarts[fr] = m_nLinkDots;
				int off = frames[fr];
				short flags = m_pixelFlag16;
				short* header = (short*) ((char*) m_unk0x48c + off + 6 * *(short*) ((char*) m_unk0x48c + off) + 2);
				if (flags & 8) {
					if ((flags & 1) && (flags & 4)) {

						int y = header[0];
						int yEnd = y + header[1];
						unsigned char* rle = (unsigned char*) (header + 2);
						while (y < yEnd) {
							int x = 0;
							while (*(unsigned short*) rle) {
								int xr = *rle + x;
								int count = rle[1];
								unsigned char* px = rle + 2;
								if (count > 0) {
									unsigned char* zp = px;
									int xc = xr;
									for (int c = count; c; --c) {
										int z = (*(unsigned short*) zp >> 3) - 128;
										int zz = z + y;
										if (zz >= 0 && zz < 2048 && xc >= 0 && xc < 0x800) {
											int cell = xc / 8 + ((zz / 8) << 8);
											if (z > grid[cell])
												grid[cell] = (short) z;
										}
										++xc;
										zp += 2;
									}
								}
								rle = px + 3 * count;
								x = count + xr;
							}
							++y;
							rle += 2;
						}
					}
					else if ((flags & 8) && (flags & 1)) {

						int y = header[0];
						int yEnd = y + header[1];
						unsigned char* rle = (unsigned char*) (header + 2);
						for (; y < yEnd; ++y, rle += 2) {
							int x = 0;
							while (*(unsigned short*) rle) {
								int xr = *rle + x;
								int count = rle[1];
								unsigned char* px = rle + 2;
								if (count > 0) {
									int xc = xr;
									for (int c = count; c; --c) {
										if (y >= 0 && y < 0x800 && xc >= 0 && xc < 0x800)
											grid[256 * (y / 8) + xc / 8] = 0;
										++xc;
									}
								}
								rle = px + count;
								x = count + xr;
							}
						}
					}
				}
				int row = m_messageLineHeight / 8 - 1;
				if (row >= 0) {
					short* line = &grid[256 * row];
					do {
						int width = m_unk0x2f6;
						int colIdx = 0;
						if (width / 8 > 0) {
							short* cell = line;
							do {
								short z = *cell;
								if (*cell != -32000) {
									scratch[3 * m_nLinkDots] = colIdx * 8.0f - width / 2;
									scratch[3 * m_nLinkDots + 1] =
										row * 8.0f - m_messageLineHeight / 2;
									scratch[3 * m_nLinkDots + 2] = z;
									++m_nLinkDots;
								}
								width = m_unk0x2f6;
								++colIdx;
								++cell;
							} while (colIdx < width / 8);
						}
						--row;
						line -= 256;
					} while (row >= 0);
				}
			}
			m_dotCoords = (float*) operator new(12 * m_nLinkDots);
			int n = 0;
			if (m_nLinkDots > 0) {
				int idx = 0;
				do {
					int* d = (int*) m_dotCoords + idx;
					d[0] = *(int*) &scratch[idx];
					d[1] = *(int*) &scratch[idx + 1];
					d[2] = *(int*) &scratch[idx + 2];
					++n;
					idx += 3;
				} while (n < m_nLinkDots);
			}
			operator delete(scratch);
		}
	}
}

// FUNCTION: ALIEN 0x415e90
void VID_SOFTWARE::SetLayer()
{
	if (m_unk0x47c & 0x20) {
		m_layer = (m_pixelFlag & 2) ? 2 : 1;
		return;
	}
	unsigned int flag = m_flag;
	if (flag & 0x40000000) {
		m_layer = 3;
		return;
	}
	if (flag & 0x8000) {
		m_layer = 0xd;
		return;
	}
	if (m_pixelFlag & 2) {
		m_layer = 7;
		return;
	}
	if (!(flag & 0x28))
		m_layer = 6;
	else
		m_layer = 5;
}

// FUNCTION: ALIEN 0x415f10
void VID_SOFTWARE::SetGammaToPalette(unsigned char* p_palette, const GAMMA& p_gamma)
{
	unsigned int* palette = (unsigned int*) p_palette;
	if (!palette || (!p_gamma.m_a && !p_gamma.m_b))
		return;
	for (int i = 0; i < 256; ++i) {
		unsigned int color;
		int b;
		int g;
		int r;
		if (!p_gamma.m_a && !p_gamma.m_b) {
			color = palette[i];
		}
		else {
			color = palette[i];
			b = (((color & 0xff) * (((~p_gamma.m_a) & 0xff) + 1)) >> 8) + (p_gamma.m_b & 0xff);
			g = ((((color >> 8) & 0xff) * ((((~p_gamma.m_a) >> 8) & 0xff) + 1)) >> 8)
				+ ((p_gamma.m_b >> 8) & 0xff);
			r = ((((color >> 16) & 0xff) * ((((~p_gamma.m_a) >> 16) & 0xff) + 1)) >> 8)
				+ ((p_gamma.m_b >> 16) & 0xff);
			if (r < 0)
				r = 0;
			else if (r > 255)
				r = 255;
			if (g < 0)
				g = 0;
			else if (g > 255)
				g = 255;
			if (b < 0)
				b = 0;
			else if (b > 255)
				b = 255;
			color = 0xff000000 | (r << 16) | (g << 8) | b;
		}
		palette[i] = color;
	}
}

// STUB: ALIEN 0x4162b0
int VID_SOFTWARE::SetGamma(const GAMMA& p_gamma, unsigned int p_idx)
{
	int palSize = PaletteSize();
	char* pixels = (char*) m_unk0x48c;
	if (pixels && (m_pixelFlag16 & 8)) {
		if (p_idx == 4) {
			if (m_pixelFlag16 & 0x400) {
				SetGamma(m_gamma[0], 0);
				SetGamma(m_gamma[1], 1);
				SetGamma(m_gamma[2], 2);
				SetGamma(m_gamma[3], 3);
			}
			else {
				memcpy(pixels, pixels + palSize, palSize);
				if ((int) m_sprClass != 8 && !(m_flag & 0x800))
					SetGammaToPalette((unsigned char*) m_unk0x48c, p_gamma);
			}
		}
		else if (p_idx < 4) {
			m_gamma[p_idx].m_a = p_gamma.m_a;
			m_gamma[p_idx].m_b = p_gamma.m_b;

			if (!(m_pixelFlag16 & 0x400)) {
				char* old = (char*) m_unk0x48c;
				m_unk0x488 += 3 * palSize;
				VID::MemoryInUse += 3 * palSize;
				m_unk0x48c = (int) operator new(m_unk0x488);
				if (!m_unk0x48c)
					return Error(2,
						// STRING: ALIEN 0x482c10
						"SetGamma", m_unk0x488);
				memcpy((char*) m_unk0x48c + 4 * palSize, old + palSize,
					m_unk0x488 - 4 * palSize);
				memcpy((char*) m_unk0x48c, (char*) m_unk0x48c + 4 * palSize, palSize);
				memcpy((char*) m_unk0x48c + palSize, (char*) m_unk0x48c + 4 * palSize,
					palSize);
				memcpy((char*) m_unk0x48c + 2 * palSize, (char*) m_unk0x48c, 2 * palSize);
				operator delete(old);
				if (m_unk0x484) {
					for (int i = 0; i < m_dotFrameCount; ++i)
						((int*) m_unk0x484)[i] += 3 * palSize;
				}
				m_pixelFlag16 |= 0x400;
				for (VID* mirror = m_weaponPtr; mirror != this; mirror = mirror->m_weaponPtr) {
					mirror->m_pixelFlag16 |= 0x400;
					((VID_SOFTWARE*) mirror)->m_unk0x48c = m_unk0x48c;
				}
			}

			memcpy((char*) m_unk0x48c + p_idx * palSize,
				(char*) m_unk0x48c + 4 * palSize, palSize);
			if ((int) m_sprClass == 8 || (m_flag & 0x800)) {
				SetGammaToPalette((unsigned char*) ((char*) m_unk0x48c + p_idx * palSize),
					p_gamma);
			}
			else {
				GAMMA combined;
				combined.Add(p_gamma, ScreenGamma((GRAPH_CORE*) Graph));
				SetGammaToPalette((unsigned char*) ((char*) m_unk0x48c + p_idx * palSize),
					combined);
			}
		}
		else {
			Error(4,
				// STRING: ALIEN 0x482bec
				"n_gamma in VID_SOFTWARE::SetGamma", p_idx);
		}
	}
	if (0)
		return 0;
}

#pragma optimize("y", off)
// STUB: ALIEN 0x416860
int VID_SOFTWARE::Draw(SPRITE* p_sprite)
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
	int pitch;
	int zpitch = graph->m_unk0x250;
	char* surface;
	char* zbase = (char*) graph->m_zbuffer;
	LockDrawGraph(graph);
	pitch = graph->m_unk0x248;
	surface = (char*) graph->m_locked;

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
		for (int y = yTop; y < yEnd; ++y, z += zstep) {
			AsmDrawData[0] = (short) z;
			int* dst = (int*) (surface + 4 * y * pitch);
			unsigned short* zrow = (unsigned short*) (zbase + 2 * y * zpitch);
			int x = x0;
			while (*(unsigned short*) rle) {
				x += *rle++;
				int count = *rle++;
				unsigned char* src = rle;
				rle += count;
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
				if (count > 0)
					AsmDrawWithAlpha32(src, zrow + xc, (COLOR*) (dst + xc), count);
			}
			rle += 2;
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
			int* dst = (int*) (surface + 4 * y * pitch);
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
						dst[xc + i] = AsmDrawPalette[src[i]];
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
			int* dst = (int*) (surface + 4 * y * pitch);

			unsigned short* zrow = (unsigned short*) (zbase + 2 * y * zpitch);
			int x = x0;
			while (*(unsigned short*) rle) {
				x += *rle++;
				int count = *rle++;
				unsigned short* src = (unsigned short*) rle;
				rle += 2 * count;
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
						unsigned int c = src[i];
						dst[xc + i] = 0xff000000 | ((c & 0x1f) << 3)
							| ((c << (8 - RGB16_gShift)) & 0xff00)
							| ((c << (16 - RGB16_rShift)) & 0xff0000);
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

	int unclipped = x0 >= viewXMin && x0 + width <= viewXMax;
	for (int y = yTop; y < yEnd; ++y, z += zstep) {
		AsmDrawData[0] = (short) z;
		int* dst = (int*) (surface + 4 * y * pitch);
		short* zrow = (short*) (zbase + 2 * y * zpitch);
		int x = x0;
		if (unclipped) {
			unsigned short* zurow = (unsigned short*) zrow;
			while (*(unsigned short*) rle) {
				x += *rle++;
				int count = *rle++;
				unsigned char* src = rle;
				rle += count;
				for (int i = 0; i < count; ++i) {
					if (z >= zurow[x + i]) {
						zurow[x + i] = (unsigned short) z;
						dst[x + i] = AsmDrawPalette[src[i]];
					}
				}
				x += count;
			}
			rle += 2;
			continue;
		}
		while (*(unsigned short*) rle) {
			x += *rle++;
			int count = *rle++;
			unsigned char* src = rle;
			rle += count;
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
			if (count > 0)
				AsmDraw32(src, zrow + xc, dst + xc, count);
		}
		rle += 2;
	}
	return 0;
}
#pragma optimize("", on)

// FUNCTION: ALIEN 0x418640
void VID_SOFTWARE::DrawShadow(SPRITE* p_sprite)
{
	struct SHADOWVERTEX {
		float x;
		float y;
		float z;
		float rhw;
		unsigned int diffuse;
		unsigned int specular;
	};

	float shadowH = p_sprite->m_z;
	char* base = (char*) m_unk0x48c;
	if (!base || !m_unk0x484)
		return;
	if (m_unk0x47c & 0x40)
		return;

	int height = m_messageLineHeight;
	float baseX = p_sprite->m_x - Map->m_shiftX - m_unk0x2f6 / 2;
	float groundY = p_sprite->m_y - p_sprite->m_z - Map->m_shiftY - height / 2;
	if ((int) baseX + m_unk0x2f6 + 200 < viewXMin || (int) baseX >= viewXMax)
		return;
	if ((int) groundY + height + 100 < viewYMin || (int) groundY >= viewYMax)
		return;

	float lineY = groundY + shadowH;
	if (m_flag & 0x10000)
		shadowH = FSin[(CurrentTime >> 3) & 0xff] * m_unk0x60 + shadowH;

	short* pts = (short*) (base + ((int*) m_unk0x484)[p_sprite->m_noCadr]);
	int count = *pts++;
	if (!count)
		return;

	SHADOWVERTEX verts[514];
	int i;
	for (i = 0; i < 2 * count; i += 2) {
		verts[i].x = *pts++ + baseX;
		float py = *pts++ + lineY;
		float elev = *pts++ + shadowH;
		verts[i].y = py - elev;
		verts[i].z = elev * 0.00012207031f + 0.015625f;
		verts[i].rhw = 1.0f;
		verts[i].diffuse = 0xa4a4a4;
		verts[i + 1].x = elev * 0.34999999f + verts[i].x;
		verts[i + 1].y = py - elev * 0.69999999f;
		verts[i + 1].z = 0.015625f;
		verts[i + 1].rhw = 1.0f;
		verts[i + 1].diffuse = 0xa4a4a4;
	}
	verts[i] = verts[0];
	verts[i + 1] = verts[1];

	((GRAPH_CORE*) Graph)->SetRenderState(D3DRS_SPECULARENABLE, 0);
	((GRAPH_CORE*) Graph)->m_device->SetTexture(0, 0);
	((GRAPH_CORE*) Graph)->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	((GRAPH_CORE*) Graph)->SetAlphaBlend(1, 3);
	int total = 2 * count + 2;
	for (i = 0; i < total; ++i)
		verts[i].diffuse = 0xa4a4a4;
	((GRAPH_CORE*) Graph)->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0xc4, verts, 0x18, total);

	((GRAPH_CORE*) Graph)->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
	((GRAPH_CORE*) Graph)->SetAlphaBlend(9, 2);
	for (i = 0; i < total; ++i)
		verts[i].diffuse = 0x8f8f8f;
	((GRAPH_CORE*) Graph)->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0xc4, verts, 0x18, total);

	((GRAPH_CORE*) Graph)->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
}
