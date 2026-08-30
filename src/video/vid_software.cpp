#include "video/vid_software.h"

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
#include "ui/ui_scaling.h"
#include "util/myerror.h"
#include "util/packed.h"
#include "util/resource.h"

#include <cmath>
#include <string.h>

extern float FSin[256];

inline static GAMMA ScreenGamma(const GRAPH_CORE* p_graph)
{
	return GAMMA(GAMMA::RAW_COPY, p_graph->m_gammaSet.m_a, p_graph->m_gammaSet.m_b);
}

static __forceinline GAMMA* CombineDrawGamma(GAMMA* p_result, const GAMMA& p_gamma, int p_screenB, int p_screenA)
{
	return p_result->Add(p_gamma, GAMMA(GAMMA::RAW_COPY, p_screenA, p_screenB));
}

inline static int LockDrawGraph(GRAPH_CORE* p_graph)
{
	return p_graph->Lock();
}

enum SCALED_UI_PIXEL_MODE {
	SCALED_UI_ALPHA_UNSIGNED_GE,
	SCALED_UI_OPAQUE_SIGNED_GT,
	SCALED_UI_OPAQUE_UNSIGNED_GE
};

static int ScaledUIBoundary(int p_source, float p_scale)
{
	double scaled = (double) p_source * (double) p_scale;
	return scaled >= 0.0 ? (int) std::floor(scaled + 0.5) : (int) std::ceil(scaled - 0.5);
}

static void DrawScaledUIPixel(
	const UI_SCALING::RASTER_TARGET32& p_target,
	unsigned short* p_zbuffer,
	int p_zpitch,
	int p_x,
	int p_y,
	int p_width,
	int p_height,
	unsigned int p_color,
	short p_z,
	SCALED_UI_PIXEL_MODE p_mode
)
{
	UI_SCALING::RECT_I block;
	if (!p_zbuffer || !UI_SCALING::PixelBlockBounds(p_target, p_x, p_y, p_width, p_height, &block)) {
		return;
	}

	for (int y = block.m_top; y < block.m_bottom; ++y) {
		unsigned int* color = p_target.m_pixels + (size_t) y * p_target.m_pitch;
		unsigned short* zUnsigned = p_zbuffer + (size_t) y * p_zpitch;
		short* zSigned = (short*) zUnsigned;
		for (int x = block.m_left; x < block.m_right; ++x) {
			if (p_mode == SCALED_UI_ALPHA_UNSIGNED_GE) {
				if ((unsigned short) p_z >= zUnsigned[x]) {
					COLOR source((int) p_color);
					((COLOR*) &color[x])->AlphaAdd(source, p_color >> 24);
				}
			}
			else if (p_mode == SCALED_UI_OPAQUE_UNSIGNED_GE) {
				if (UI_SCALING::OpaquePalettedDepthPass(p_z, zUnsigned[x], true)) {
					zUnsigned[x] = (unsigned short) p_z;
					color[x] = p_color;
				}
			}
			else if (UI_SCALING::OpaquePalettedDepthPass(p_z, zUnsigned[x], false)) {
				zSigned[x] = p_z;
				color[x] = p_color;
			}
		}
	}
}

static int DrawScaledUIFrame(
	VID_SOFTWARE* p_vid,
	GRAPH_CORE* p_graph,
	unsigned char* p_rle,
	int p_rows,
	int p_x0,
	int p_yTop,
	int p_z,
	float p_scale,
	int p_horizontalGap,
	unsigned short p_flags,
	const int* p_palette
)
{
	if (!p_graph->m_color || !p_graph->m_zbuffer || !p_palette || !(p_flags & 1)) {
		return 0;
	}

	UI_SCALING::RECT_I clip =
		{(int) p_graph->m_viewXMin, (int) p_graph->m_viewYMin, (int) p_graph->m_viewXMax, (int) p_graph->m_viewYMax};
	UI_SCALING::RASTER_TARGET32 target = UI_SCALING::MakeRasterTarget32(
		(unsigned int*) p_graph->m_color,
		(int) p_graph->m_width,
		(int) p_graph->m_height,
		p_graph->m_pitch,
		clip
	);
	unsigned short* zbuffer = (unsigned short*) p_graph->m_zbuffer;

	bool alpha = (p_flags & 2) && (p_flags & 8);
	bool withDepthDelta = !alpha && (p_flags & 8) && (p_flags & 4);
	bool directRgb16 = !(p_flags & 8);
	int sourceSplit = p_vid->m_unk0x2f6 / 2;
	const int sourceTileWidth = 2;
	int targetWidth = ScaledUIBoundary(p_vid->m_unk0x2f6, p_scale) + p_horizontalGap;
	bool horizontallyUnclipped = p_x0 >= (int) p_graph->m_viewXMin && p_x0 + targetWidth <= (int) p_graph->m_viewXMax;
	if (!withDepthDelta) {
		p_z += 1024;
		if (p_z > 0x7fff) {
			p_z = 0x7fff;
		}
	}
	bool sloped = !withDepthDelta && p_vid->m_unk0x24 > p_vid->m_footprintHeight;

	for (int row = 0; row < p_rows; ++row) {
		int rowZ = p_z + (sloped ? 8 * (p_rows - row) : 0);
		int rowTop = p_yTop + ScaledUIBoundary(row, p_scale);
		int rowBottom = p_yTop + ScaledUIBoundary(row + 1, p_scale);
		int rowHeight = rowBottom - rowTop;
		int x = 0;
		bool haveBridgePixel[2] = {false, false};
		unsigned int bridgeColor[2] = {0, 0};
		short bridgeZ[2] = {0, 0};
		SCALED_UI_PIXEL_MODE bridgeMode[2] = {SCALED_UI_OPAQUE_SIGNED_GT, SCALED_UI_OPAQUE_SIGNED_GT};
		while (p_rle[0] || p_rle[1]) {
			x += p_rle[0];
			int count = p_rle[1];
			p_rle += 2;
			unsigned char* zdelta = 0;
			unsigned char* indices = 0;
			unsigned char* rgb16 = 0;
			if (withDepthDelta) {
				zdelta = p_rle;
				indices = p_rle + 2 * count;
				p_rle = indices + count;
			}
			else if (directRgb16) {
				rgb16 = p_rle;
				p_rle += 2 * count;
			}
			else {
				indices = p_rle;
				p_rle += count;
			}

			for (int i = 0; i < count; ++i) {
				unsigned int color;
				short pixelZ;
				SCALED_UI_PIXEL_MODE mode;
				if (withDepthDelta) {
					color = (unsigned int) p_palette[indices[i]];
					short delta;
					memcpy(&delta, zdelta + 2 * i, sizeof(delta));
					pixelZ = (short) (p_z + delta);
					mode = SCALED_UI_OPAQUE_SIGNED_GT;
				}
				else if (directRgb16) {
					unsigned short sourceColor;
					memcpy(&sourceColor, rgb16 + 2 * i, sizeof(sourceColor));
					color = ExpandRGB16(sourceColor);
					pixelZ = (short) rowZ;
					mode = SCALED_UI_OPAQUE_UNSIGNED_GE;
				}
				else {
					color = (unsigned int) p_palette[indices[i]];
					pixelZ = (short) rowZ;
					mode = alpha ? SCALED_UI_ALPHA_UNSIGNED_GE
								 : (horizontallyUnclipped ? SCALED_UI_OPAQUE_UNSIGNED_GE : SCALED_UI_OPAQUE_SIGNED_GT);
				}
				int sourceX = x + i;
				if (row < 8 && sourceX >= sourceSplit && sourceX < sourceSplit + sourceTileWidth) {
					int bridgePixel = sourceX - sourceSplit;
					haveBridgePixel[bridgePixel] = true;
					bridgeColor[bridgePixel] = color;
					bridgeZ[bridgePixel] = pixelZ;
					bridgeMode[bridgePixel] = mode;
				}
				bool insertedTilePixel =
					p_horizontalGap > 0 && sourceX >= sourceSplit && sourceX < sourceSplit + sourceTileWidth;
				if (!insertedTilePixel) {
					int splitOffset = sourceX >= sourceSplit + sourceTileWidth ? p_horizontalGap : 0;
					int pixelLeft = ScaledUIBoundary(sourceX, p_scale);
					int pixelRight = ScaledUIBoundary(sourceX + 1, p_scale);
					DrawScaledUIPixel(
						target,
						zbuffer,
						p_graph->m_zpitch,
						p_x0 + pixelLeft + splitOffset,
						rowTop,
						pixelRight - pixelLeft,
						rowHeight,
						color,
						pixelZ,
						mode
					);
				}
			}
			x += count;
		}
		p_rle += 2;
		if (p_horizontalGap > 0 && haveBridgePixel[0] && haveBridgePixel[1]) {
			int firstPixelWidth = ScaledUIBoundary(1, p_scale);
			int tileWidth = ScaledUIBoundary(sourceTileWidth, p_scale);
			int tiledExtent = p_horizontalGap + tileWidth;
			for (int offset = 0; tileWidth > 0 && offset < tiledExtent;) {
				int phase = offset % tileWidth;
				int bridgePixel = phase < firstPixelWidth ? 0 : 1;
				int phaseEnd = bridgePixel == 0 ? firstPixelWidth : tileWidth;
				int blockWidth = phaseEnd - phase;
				if (blockWidth <= 0) {
					blockWidth = 1;
				}
				if (blockWidth > tiledExtent - offset) {
					blockWidth = tiledExtent - offset;
				}
				DrawScaledUIPixel(
					target,
					zbuffer,
					p_graph->m_zpitch,
					p_x0 + ScaledUIBoundary(sourceSplit, p_scale) + offset,
					rowTop,
					blockWidth,
					rowHeight,
					bridgeColor[bridgePixel],
					bridgeZ[bridgePixel],
					bridgeMode[bridgePixel]
				);
				offset += blockWidth;
			}
		}
	}
	return 0;
}

// FUNCTION: ALIEN 0x4126b0
int VID_SOFTWARE::PaletteSize()
{
	return 1024;
}

// FUNCTION: ALIEN 0x4126c0
int VID_SOFTWARE::HaveShadow()
{
	if (m_unk0x48c && m_unk0x484) {
		return PackedRead<short>((char*) m_unk0x48c + m_unk0x484[0]);
	}
	return 0;
}

// FUNCTION: ALIEN 0x412730
void* VID_SOFTWARE::ScalarDeletingDestructor(unsigned int p_flags)
{
	VID_SOFTWARE* result = this;
	this->~VID_SOFTWARE();
	if (p_flags & 1) {
		operator delete(result);
	}
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
		if (m_unk0x48c) {
			operator delete(m_unk0x48c);
		}
		m_unk0x48c = 0;
		if (m_unk0x484) {
			operator delete((void*) m_unk0x484);
		}
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
		if (!p_res->GoNext(0x204c4150 /* 'PAL ' */)) {
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
				palette[c].m_value = COLOR(GAMMA(GAMMA::RAW_COPY, m_colorSub, m_colorAdd), palette[c]).m_value;
			}
		}
		else {
			Error(5, "PAL ", 0);
		}
	}

	if (p_res->GoNext(0x41544144 /* 'DATA' */)) {
		Error(5, "DATA", 0);
	}
	m_unk0x488 = p_res->m_resSize + (m_dotFrameCount > 0 ? m_dotFrameCount : 0);
	if (m_pixelFlag16 & 8) {
		m_unk0x488 += 2 * PaletteSize();
	}
	m_unk0x48c = operator new(m_unk0x488);
	if (!m_unk0x48c) {
		Error(2, "cadr", m_unk0x488);
		return;
	}
	int* frames = (int*) operator new(4 * m_dotFrameCount);
	m_unk0x484 = frames;
	if (!frames) {
		Error(
			2,
			// STRING: ALIEN 0x482bd8
			"cadrShift",
			m_dotFrameCount
		);
		return;
	}

	int offset;
	if (m_pixelFlag16 & 8) {

		int w = 0;
		for (int c = 0; c < 256; ++c) {
			if (PaletteSize() == 1024) {
				PackedWrite<unsigned int>((char*) m_unk0x48c + 4 * c, palette[c].m_value);
			}
			else {
				PackedWrite<unsigned short>(
					(char*) m_unk0x48c + w,
					(unsigned short) (((palette[c].m_value >> 3) & 0x1f) |
									  (RGB16_rMask & (palette[c].m_value >> (16 - RGB16_rShift))) |
									  (RGB16_gMask & (palette[c].m_value >> (8 - RGB16_gShift))))
				);
			}
			w += 2;
		}
		offset = 2 * PaletteSize();
		memcpy((char*) m_unk0x48c + PaletteSize(), (char*) m_unk0x48c, PaletteSize());
	}
	else {
		offset = 0;
	}

	for (int frame = 0; frame < m_dotFrameCount; ++frame) {
		// Paletted frames can have an odd encoded size. Keep each internal
		// frame start aligned because its fixed header and shadow contour are
		// arrays of 16-bit values; RLE payloads themselves remain byte streams.
		offset = (offset + 1) & ~1;
		int packedSize;
		p_res->Read(&packedSize, 4);
		int left = p_res->ReadPacked((char*) m_unk0x48c + offset, packedSize, coder);
		if (left) {
			Error(
				5,
				// STRING: ALIEN 0x482bc0
				"Can't decode software",
				packedSize - left
			);
		}
		if (packedSize != 2) {
			short flag2 = m_pixelFlag16;
			if (!(flag2 & 8) && ((TEXTURE*) ((GRAPH_CORE*) Graph)->m_texE10)->m_format == 24 && !(flag2 & 2)) {

				unsigned char* frameData = (unsigned char*) m_unk0x48c + offset;
				short contourCount = PackedRead<short>(frameData);
				unsigned char* header = frameData + 6 * contourCount + 2;
				int yTop = PackedRead<short>(header);
				int yEnd = yTop + PackedRead<short>(header + 2);
				unsigned char* k = header + 4;
				while (yTop < yEnd) {
					while (PackedRleRun(k)) {
						int count = k[1];
						unsigned char* pixels = k + 2;
						while (count > 0) {
							unsigned short px = PackedRead<unsigned short>(pixels);
							--count;
							PackedWrite<unsigned short>(pixels, (unsigned short) ((px & 0x1f) | ((px >> 1) & 0x7fe0)));
							pixels += 2;
						}
						k = pixels;
					}
					k += 2;
					++yTop;
				}
			}
			frames[frame] = offset;
			offset += packedSize;
		}
		else {
			frames[frame] = frames[PackedRead<short>((char*) m_unk0x48c + offset)];
		}
		p_res->GoNextSub(0x41544144 /* 'DATA' */);
	}
	if (coder) {
		delete coder;
	}
	VID::MemoryInUse += m_unk0x488;
	SetLayer();

	if (m_flag & 0x20) {
		short flag2 = m_pixelFlag16;
		if ((flag2 & 8) && (flag2 & 1)) {

			float* scratch = (float*) operator new(0x3000000);
			m_dotFrameStarts = (int*) operator new(4 * m_dotFrameCount);
			for (int fr = 0; fr < m_dotFrameCount; ++fr) {
				short grid[65536];
				for (short& cell : grid) {
					cell = -32000;
				}
				m_dotFrameStarts[fr] = m_nLinkDots;
				int off = frames[fr];
				short flags = m_pixelFlag16;
				unsigned char* frameData = (unsigned char*) m_unk0x48c + off;
				short contourCount = PackedRead<short>(frameData);
				unsigned char* header = frameData + 6 * contourCount + 2;
				if (flags & 8) {
					if ((flags & 1) && (flags & 4)) {

						int y = PackedRead<short>(header);
						int yEnd = y + PackedRead<short>(header + 2);
						unsigned char* rle = header + 4;
						while (y < yEnd) {
							int x = 0;
							while (PackedRleRun(rle)) {
								int xr = *rle + x;
								int count = rle[1];
								unsigned char* px = rle + 2;
								if (count > 0) {
									unsigned char* zp = px;
									int xc = xr;
									for (int c = count; c; --c) {
										int z = (PackedRead<unsigned short>(zp) >> 3) - 128;
										int zz = z + y;
										if (zz >= 0 && zz < 2048 && xc >= 0 && xc < 0x800) {
											int cell = xc / 8 + ((zz / 8) << 8);
											if (z > grid[cell]) {
												grid[cell] = (short) z;
											}
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

						int y = PackedRead<short>(header);
						int yEnd = y + PackedRead<short>(header + 2);
						unsigned char* rle = header + 4;
						for (; y < yEnd; ++y, rle += 2) {
							int x = 0;
							while (PackedRleRun(rle)) {
								int xr = *rle + x;
								int count = rle[1];
								unsigned char* px = rle + 2;
								if (count > 0) {
									int xc = xr;
									for (int c = count; c; --c) {
										if (y >= 0 && y < 0x800 && xc >= 0 && xc < 0x800) {
											grid[256 * (y / 8) + xc / 8] = 0;
										}
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
									scratch[3 * m_nLinkDots + 1] = row * 8.0f - m_messageLineHeight / 2;
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
					m_dotCoords[idx] = scratch[idx];
					m_dotCoords[idx + 1] = scratch[idx + 1];
					m_dotCoords[idx + 2] = scratch[idx + 2];
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
	if (!(flag & 0x28)) {
		m_layer = 6;
	}
	else {
		m_layer = 5;
	}
}

// FUNCTION: ALIEN 0x415f10
void VID_SOFTWARE::SetGammaToPalette(unsigned char* p_palette, const GAMMA& p_gamma)
{
	unsigned int* palette = (unsigned int*) p_palette;
	if (!palette || (!p_gamma.m_a && !p_gamma.m_b)) {
		return;
	}
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
			g = ((((color >> 8) & 0xff) * ((((~p_gamma.m_a) >> 8) & 0xff) + 1)) >> 8) + ((p_gamma.m_b >> 8) & 0xff);
			r = ((((color >> 16) & 0xff) * ((((~p_gamma.m_a) >> 16) & 0xff) + 1)) >> 8) + ((p_gamma.m_b >> 16) & 0xff);
			if (r < 0) {
				r = 0;
			}
			else if (r > 255) {
				r = 255;
			}
			if (g < 0) {
				g = 0;
			}
			else if (g > 255) {
				g = 255;
			}
			if (b < 0) {
				b = 0;
			}
			else if (b > 255) {
				b = 255;
			}
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
				if ((int) m_sprClass != 8 && !(m_flag & 0x800)) {
					SetGammaToPalette((unsigned char*) m_unk0x48c, p_gamma);
				}
			}
		}
		else if (p_idx < 4) {
			m_gamma[p_idx].m_a = p_gamma.m_a;
			m_gamma[p_idx].m_b = p_gamma.m_b;

			if (!(m_pixelFlag16 & 0x400)) {
				char* old = (char*) m_unk0x48c;
				m_unk0x488 += 3 * palSize;
				VID::MemoryInUse += 3 * palSize;
				m_unk0x48c = operator new(m_unk0x488);
				if (!m_unk0x48c) {
					return Error(
						2,
						// STRING: ALIEN 0x482c10
						"SetGamma",
						m_unk0x488
					);
				}
				memcpy((char*) m_unk0x48c + 4 * palSize, old + palSize, m_unk0x488 - 4 * palSize);
				memcpy((char*) m_unk0x48c, (char*) m_unk0x48c + 4 * palSize, palSize);
				memcpy((char*) m_unk0x48c + palSize, (char*) m_unk0x48c + 4 * palSize, palSize);
				memcpy((char*) m_unk0x48c + 2 * palSize, (char*) m_unk0x48c, 2 * palSize);
				operator delete(old);
				if (m_unk0x484) {
					for (int i = 0; i < m_dotFrameCount; ++i) {
						m_unk0x484[i] += 3 * palSize;
					}
				}
				m_pixelFlag16 |= 0x400;
				for (VID* mirror = m_weaponPtr; mirror != this; mirror = mirror->m_weaponPtr) {
					mirror->m_pixelFlag16 |= 0x400;
					((VID_SOFTWARE*) mirror)->m_unk0x48c = m_unk0x48c;
				}
			}

			memcpy((char*) m_unk0x48c + p_idx * palSize, (char*) m_unk0x48c + 4 * palSize, palSize);
			if ((int) m_sprClass == 8 || (m_flag & 0x800)) {
				SetGammaToPalette((unsigned char*) ((char*) m_unk0x48c + p_idx * palSize), p_gamma);
			}
			else {
				GAMMA combined;
				combined.Add(p_gamma, ScreenGamma((GRAPH_CORE*) Graph));
				SetGammaToPalette((unsigned char*) ((char*) m_unk0x48c + p_idx * palSize), combined);
			}
		}
		else {
			Error(
				4,
				// STRING: ALIEN 0x482bec
				"n_gamma in VID_SOFTWARE::SetGamma",
				p_idx
			);
		}
	}
	return 0;
}

// STUB: ALIEN 0x416860
int VID_SOFTWARE::Draw(SPRITE* p_sprite)
{
	if (m_unk0x47c & 0x40) {
		return 0;
	}
	if (!m_unk0x48c || !m_unk0x484) {
		return 0;
	}

	int width = m_unk0x2f6;
	int height = m_messageLineHeight;
	float uiScale = p_sprite->UIDrawScale();
	int horizontalGap = p_sprite->UIHorizontalGap();
	int naturalScaledWidth = ScaledUIBoundary(width, uiScale);
	int scaledWidth = naturalScaledWidth + horizontalGap;
	int scaledHeight = ScaledUIBoundary(height, uiScale);
	int x0 = (int) p_sprite->m_x - (int) Map->m_shiftX - naturalScaledWidth / 2;
	int y0 = (int) (p_sprite->m_y - p_sprite->m_z) - (int) Map->m_shiftY - scaledHeight / 2;
	if (x0 + scaledWidth < ViewXMin() || x0 >= ViewXMax() || y0 + scaledHeight < ViewYMin() || y0 >= ViewYMax()) {
		return 0;
	}

	int z = (int) (p_sprite->m_z * 8.0f);
	if ((m_flag & 0x8000) && z < 0x3fff) {
		z += 0x3fff;
	}
	else if (m_flag & 0x10000) {
		int bob = (int) (FSin[(CurrentTime >> 3) & 0xff] * m_unk0x60 * 8.0f);
		z += bob;
		y0 -= ScaledUIBoundary(bob / 8, uiScale);
	}

	unsigned char* frame = (unsigned char*) m_unk0x48c + m_unk0x484[p_sprite->m_noCadr];
	short contourCount = PackedRead<short>(frame);
	unsigned char* header = frame + 6 * contourCount + 2;
	int nRows = PackedRead<short>(header + 2);
	unsigned char* rle = header + 4;
	int yTop = y0 + ScaledUIBoundary(PackedRead<short>(header), uiScale);
	int yEnd = yTop + ScaledUIBoundary(nRows, uiScale);
	if (yTop >= ViewYMax() || yEnd < ViewYMin()) {
		return 0;
	}
	if (yEnd > ViewYMax()) {
		yEnd = ViewYMax();
	}

	GRAPH_CORE* graph = (GRAPH_CORE*) Graph;
	int pitch;
	int zpitch = graph->m_zpitch;
	char* surface;
	char* zbase = (char*) graph->m_zbuffer;
	LockDrawGraph(graph);
	pitch = graph->m_pitch;
	surface = (char*) graph->m_color;

	alignas(int) unsigned char paletteBuf[1024];
	int* palette;
	EX_SPRITE_DATA* exData = p_sprite->m_exData;
	if (!exData || (!exData->m_unk0x24 && !exData->m_unk0x28)) {
		int armyOffset = (m_fontFlag & 4) ? ((p_sprite->m_flag >> 11) & 3) * PaletteSize() : 0;
		palette = (int*) ((char*) m_unk0x48c + armyOffset);
	}
	else {
		palette = (int*) paletteBuf;
		int gammaOffset = (m_fontFlag & 4) ? 4 * PaletteSize() : 0;
		memcpy(paletteBuf, (char*) m_unk0x48c + gammaOffset, PaletteSize());
		if (m_flag & 0x800) {
			SetGammaToPalette(paletteBuf, p_sprite->GetGamma());
		}
		else {
			GAMMA combined;
			CombineDrawGamma(
				&combined,
				p_sprite->GetGamma(),
				((GRAPH_CORE*) Graph)->m_gammaSet.m_b,
				((GRAPH_CORE*) Graph)->m_gammaSet.m_a
			);
			SetGammaToPalette(paletteBuf, combined);
		}
	}

	unsigned short flag2 = m_pixelFlag16;
	if (p_sprite->m_uiScale && (uiScale != 1.0f || horizontalGap > 0)) {
		return DrawScaledUIFrame(this, graph, rle, nRows, x0, yTop, z, uiScale, horizontalGap, flag2, palette);
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
		for (int y = yTop; y < yEnd; ++y, z += zstep) {
			int* dst = (int*) (surface + 4 * y * pitch);
			unsigned short* zrow = (unsigned short*) (zbase + 2 * y * zpitch);
			int x = x0;
			while (PackedRleRun(rle)) {
				x += *rle++;
				int count = *rle++;
				unsigned char* src = rle;
				rle += count;
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
				if (count > 0) {
					AsmDrawWithAlpha32(src, zrow + xc, (COLOR*) (dst + xc), count, (unsigned short) z, palette);
				}
			}
			rle += 2;
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
			int* dst = (int*) (surface + 4 * y * pitch);
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
						dst[xc + i] = palette[src[i]];
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
			int* dst = (int*) (surface + 4 * y * pitch);

			unsigned short* zrow = (unsigned short*) (zbase + 2 * y * zpitch);
			int x = x0;
			while (PackedRleRun(rle)) {
				x += *rle++;
				int count = *rle++;
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
						unsigned int c = PackedRead<unsigned short>(src + 2 * (sourceOffset + i));
						dst[xc + i] = 0xff000000 | ((c & 0x1f) << 3) | ((c << (8 - RGB16_gShift)) & 0xff00) |
									  ((c << (16 - RGB16_rShift)) & 0xff0000);
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

	int unclipped = x0 >= ViewXMin() && x0 + width <= ViewXMax();
	for (int y = yTop; y < yEnd; ++y, z += zstep) {
		int* dst = (int*) (surface + 4 * y * pitch);
		short* zrow = (short*) (zbase + 2 * y * zpitch);
		int x = x0;
		if (unclipped) {
			unsigned short* zurow = (unsigned short*) zrow;
			while (PackedRleRun(rle)) {
				x += *rle++;
				int count = *rle++;
				unsigned char* src = rle;
				rle += count;
				for (int i = 0; i < count; ++i) {
					if (z >= zurow[x + i]) {
						zurow[x + i] = (unsigned short) z;
						dst[x + i] = palette[src[i]];
					}
				}
				x += count;
			}
			rle += 2;
			continue;
		}
		while (PackedRleRun(rle)) {
			x += *rle++;
			int count = *rle++;
			unsigned char* src = rle;
			rle += count;
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
			if (count > 0) {
				AsmDraw32(src, zrow + xc, dst + xc, count, (short) z, palette);
			}
		}
		rle += 2;
	}
	return 0;
}

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
	if (!base || !m_unk0x484) {
		return;
	}
	if (m_unk0x47c & 0x40) {
		return;
	}

	int height = m_messageLineHeight;
	float baseX = p_sprite->m_x - Map->m_shiftX - m_unk0x2f6 / 2;
	float groundY = p_sprite->m_y - p_sprite->m_z - Map->m_shiftY - height / 2;
	if ((int) baseX + m_unk0x2f6 + 200 < ViewXMin() || (int) baseX >= ViewXMax()) {
		return;
	}
	if ((int) groundY + height + 100 < ViewYMin() || (int) groundY >= ViewYMax()) {
		return;
	}

	float lineY = groundY + shadowH;
	if (m_flag & 0x10000) {
		shadowH = FSin[(CurrentTime >> 3) & 0xff] * m_unk0x60 + shadowH;
	}

	unsigned char* pts = (unsigned char*) base + m_unk0x484[p_sprite->m_noCadr];
	int count = PackedRead<short>(pts);
	pts += 2;
	if (!count) {
		return;
	}

	SHADOWVERTEX verts[514];
	int i;
	for (i = 0; i < 2 * count; i += 2) {
		verts[i].x = PackedRead<short>(pts) + baseX;
		pts += 2;
		float py = PackedRead<short>(pts) + lineY;
		pts += 2;
		float elev = PackedRead<short>(pts) + shadowH;
		pts += 2;
		verts[i].y = py - elev;
		verts[i].z = elev * 0.00012207031f + 0.015625f;
		verts[i].rhw = 1.0f;
		verts[i].diffuse = 0xa4a4a4;
		verts[i].specular = 0;
		verts[i + 1].x = elev * 0.34999999f + verts[i].x;
		verts[i + 1].y = py - elev * 0.69999999f;
		verts[i + 1].z = 0.015625f;
		verts[i + 1].rhw = 1.0f;
		verts[i + 1].diffuse = 0xa4a4a4;
		verts[i + 1].specular = 0;
	}
	verts[i] = verts[0];
	verts[i + 1] = verts[1];

	((GRAPH_CORE*) Graph)->SetRenderState(D3DRS_SPECULARENABLE, 0);
	((GRAPH_CORE*) Graph)->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	((GRAPH_CORE*) Graph)->SetAlphaBlend(1, 3);
	int total = 2 * count + 2;
	for (i = 0; i < total; ++i) {
		verts[i].diffuse = 0xa4a4a4;
	}
	((GRAPH_CORE*) Graph)->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0xc4, verts, 0x18, total);

	((GRAPH_CORE*) Graph)->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
	((GRAPH_CORE*) Graph)->SetAlphaBlend(9, 2);
	for (i = 0; i < total; ++i) {
		verts[i].diffuse = 0x8f8f8f;
	}
	((GRAPH_CORE*) Graph)->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0xc4, verts, 0x18, total);

	((GRAPH_CORE*) Graph)->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
}
