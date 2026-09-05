#include "video/vid_hardware.h"

#include "compress/qs1_coder.h"
#include "game/game_descriptor.h"
#include "game/gametime.h"
#include "game/map.h"
#include "game/terrain_camera.h"
#include "gfx/color.h"
#include "gfx/gamma.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "gfx/render_math.h"
#include "gfx/texture.h"
#include "sprite/ex_sprite_data.h"
#include "sprite/sprite.h"
#include "util/packed.h"
#include "util/resource.h"
#include "video/vid_exdata.h"

#include <bit>
#include <cmath>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern float FSin[256];

inline static VID_CHILD& VIDHardwareChild(VID_HARDWARE* p_vid, int p_idx)
{
	return ((VID_CHILD*) p_vid->m_unk0x484)[p_idx];
}

inline static TEXTURE*& VIDHardwareTexture(VID_HARDWARE* p_vid, int p_idx)
{
	return ((TEXTURE**) p_vid->m_unk0x48c)[p_idx];
}

inline static void VIDHardwareWordSet(char* p_dst, int p_value, int p_count)
{
	while (p_count > 0) {
		PackedWrite<unsigned short>(p_dst, (unsigned short) p_value);
		p_dst += 2;
		--p_count;
	}
}

// Round shared UI boundaries consistently.
static int ScaledUIBoundary(int p_source, float p_scale)
{
	double scaled = (double) p_source * (double) p_scale;
	return scaled >= 0.0 ? (int) std::floor(scaled + 0.5) : (int) std::ceil(scaled - 0.5);
}

// FUNCTION: ALIEN 0x412750
VID* VID_HARDWARE::CreateMirror()
{
	return new VID_HARDWARE(*this);
}

// FUNCTION: ALIEN 0x412780
void* VID_HARDWARE::ScalarDeletingDestructor(unsigned int p_flags)
{
	VID_HARDWARE* result = this;
	this->~VID_HARDWARE();
	if (p_flags & 1) {
		operator delete(result);
	}
	return result;
}

// FUNCTION: ALIEN 0x4196a0
VID_HARDWARE::VID_HARDWARE(VID_HARDWARE& p_other)
{
	m_weaponPtr = p_other.m_weaponPtr;
	p_other.m_weaponPtr = this;
	m_layer = p_other.m_layer;
	m_pixelFlag16 = p_other.m_pixelFlag16;
	m_defaultAniPeriod = p_other.m_defaultAniPeriod;
	m_dotFrameCount = p_other.m_dotFrameCount;
	m_unk0x2f6 = p_other.m_unk0x2f6;
	m_messageLineHeight = p_other.m_messageLineHeight;
	m_unk0x484 = p_other.m_unk0x484;
	m_unk0x48c = p_other.m_unk0x48c;
	m_unk0x488 = p_other.m_unk0x488;
	m_terrainCoverage = 0;
}

// STUB: ALIEN 0x419750
VID_HARDWARE::VID_HARDWARE(int p_idx, int p_w, int p_h)
{
	m_terrainCoverage = new (std::nothrow) TERRAIN_COVERAGE(p_w, p_h);
	if (m_terrainCoverage && !m_terrainCoverage->Valid()) {
		delete m_terrainCoverage;
		m_terrainCoverage = 0;
	}
	m_footprintWidth = 256.0f;
	m_footprintHeight = 256.0f;
	m_idx = p_idx;
	m_unk0x24 = 1.0f;
	m_unk0x2f6 = (short) p_w;
	m_messageLineHeight = (short) p_h;
	m_noDir = 1;
	m_layer = 0;
	SetName(
		// STRING: ALIEN 0x482c3c
		"Self Created Hardware Prerendered Ground "
	);
	int cells = (p_w / 256 + 1) * (p_h / 256 + 1) + 1;
	m_pixelFlag16 = 0x225;
	VID_CHILD* children = (VID_CHILD*) operator new(sizeof(VID_CHILD) * cells);
	m_unk0x484 = children;
	if (!m_unk0x484) {
		Error(2, "texcoor", cells);
		exit(1);
	}
	m_unk0x488 = 0;
	int index = 0;
	int remH = p_h;
	for (int yOff = 0; yOff < p_h; yOff += 256) {
		int remW = p_w;
		for (int xOff = 0; xOff < p_w; xOff += 256) {
			if (index) {
				VIDHardwareChild(this, index - 1).m_next = index;
			}
			VIDHardwareChild(this, index).m_texture = (short) m_unk0x488;
			VIDHardwareChild(this, index).m_x = 0;
			VIDHardwareChild(this, index).m_y = 0;
			VIDHardwareChild(this, index).m_offsetX = xOff;
			VIDHardwareChild(this, index).m_offsetY = yOff;
			VIDHardwareChild(this, index).m_w = remW > 256 ? 256 : remW;
			VIDHardwareChild(this, index).m_h = remH > 256 ? 256 : remH;
			VIDHardwareChild(this, index).m_next = 0;
			m_unk0x488 = m_unk0x488 + 2;
			++index;
			remW -= 256;
		}
		remH -= 256;
	}
	TEXTURE** textures = (TEXTURE**) operator new(sizeof(TEXTURE*) * (short) m_unk0x488);
	m_unk0x48c = textures;
	if (!m_unk0x48c) {
		Error(
			2,
			// STRING: ALIEN 0x482c28
			"textures",
			(short) m_unk0x488
		);
		return;
	}
	for (int t = 0; t < (short) m_unk0x488; t += 2) {
		VID_CHILD* cell = &VIDHardwareChild(this, t / 2);
		TEXTURE* color =
			new TEXTURE(VIDHardwareChild(this, t / 2).m_w, VIDHardwareChild(this, t / 2).m_h, D3DFMT_R5G6B5, 0);
		VIDHardwareTexture(this, t) = color;
		int pitch = 0;
		void* bits = VIDHardwareTexture(this, t)->Lock(&pitch, 0);
		memset(bits, 0, pitch * VIDHardwareTexture(this, t)->m_height);
		TEXTURE* zTex =
			new TEXTURE(VIDHardwareChild(this, t / 2).m_w, VIDHardwareChild(this, t / 2).m_h, D3DFMT_D16, 2);
		VIDHardwareTexture(this, t + 1) = zTex;
		unsigned short* zbits = (unsigned short*) VIDHardwareTexture(this, t + 1)->Lock(&pitch, 0);
		unsigned int words = pitch / 2 * VIDHardwareTexture(this, t + 1)->m_height;
		unsigned short* fill = zbits;
		VIDHardwareWordSet((char*) fill, 1024, words);
	}
}

// FUNCTION: ALIEN 0x419ad0
VID_HARDWARE::~VID_HARDWARE()
{
	delete m_terrainCoverage;
	m_terrainCoverage = 0;
	if (m_weaponPtr == this) {
		if (m_unk0x484) {
			operator delete((void*) m_unk0x484);
		}
		m_unk0x484 = 0;
		if (m_unk0x48c) {
			for (short i = --m_unk0x488; i >= 0; i = m_unk0x488) {
				TEXTURE* t = m_unk0x48c[i];
				if (t) {
					delete t;
				}
				--m_unk0x488;
			}
			operator delete((void*) m_unk0x48c);
			m_unk0x48c = 0;
		}
		m_unk0x488 = 0;
	}
}

// STUB: ALIEN 0x419b80
void VID_HARDWARE::DrawVidToVid(const SPRITE* p_sprite)
{
	if ((m_pixelFlag16 & 1) && (m_pixelFlag16 & 4) && m_noDir == 1) {
		if (!(p_sprite->m_vid->m_flag & 0x20000)) {
			GRAPH_CORE* graph = (GRAPH_CORE*) Graph;
			float savedXMin = graph->m_viewXMin;
			float savedXMax = graph->m_viewXMax;
			float savedYMin = graph->m_viewYMin;
			float savedYMax = graph->m_viewYMax;
			int sx = (int) p_sprite->m_x;
			int sy = (int) (p_sprite->m_y - p_sprite->m_z);
			for (VID_CHILD* child = m_unk0x484; child; child = &(m_unk0x484)[child->m_next]) {
				int halfW = child->m_w / 2;
				if (abs(child->m_offsetX - sx + child->m_x + halfW) < halfW + p_sprite->m_vid->m_unk0x2f6 / 2) {
					int halfH = child->m_h / 2;
					if (abs(child->m_offsetY - sy + child->m_y + halfH) <
						halfH + p_sprite->m_vid->m_messageLineHeight / 2) {
						graph->SetViewPort(
							(float) child->m_x,
							(float) child->m_y,
							(float) (child->m_x + child->m_w),
							(float) (child->m_y + child->m_h)
						);
						TerrainCoverageBegin(
							m_terrainCoverage,
							child->m_offsetX - child->m_x,
							child->m_offsetY - child->m_y
						);
						p_sprite->m_vid->DrawToVid(
							p_sprite,
							child,
							m_unk0x48c[child->m_texture],
							m_unk0x48c[child->m_texture + 1]
						);
						TerrainCoverageEnd();
					}
				}
				if (!child->m_next) {
					break;
				}
			}
			graph->SetViewPort(savedXMin, savedYMin, savedXMax, savedYMax);
		}
	}
}

TERRAIN_COVERAGE* VID_HARDWARE::TakeTerrainCoverage()
{
	TERRAIN_COVERAGE* result = m_terrainCoverage;
	m_terrainCoverage = 0;
	return result;
}

// STUB: ALIEN 0x419d20
void VID_HARDWARE::Load(RESOURCE* p_res)
{

	QS1_CODER* colorCoder = 0;
	QS1_CODER* zCoder = 0;
	if (p_res->GoNext(0x46525553 /* 'SURF' */)) {
		Error(
			5,
			// STRING: ALIEN 0x482d10
			"SURF",
			0
		);
	}
	p_res->Read(&m_unk0x488, 2);
	if (!(short) m_unk0x488) {
		return;
	}
	m_unk0x48c = (TEXTURE**) operator new(sizeof(TEXTURE*) * (short) m_unk0x488);
	if (!m_unk0x48c) {
		Error(2, "textures", (short) m_unk0x488);
		return;
	}
	for (int t = 0; t < (short) m_unk0x488; ++t) {
		m_unk0x48c[t] = 0;
	}

	void* unpack = operator new(0x20008);
	if (!unpack) {
		Error(
			2,
			// STRING: ALIEN 0x482d04
			"(unpack)",
			0
		);
		return;
	}

	if (m_pixelFlag16 & 0x100) {
		if (m_pixelFlag16 & 0x800) {
			colorCoder = new QS1_CODER(1);
		}
		else {
			colorCoder = new QS1_CODER(2);
		}
		zCoder = new QS1_CODER(2);
	}

	unsigned char palette[768];
	int pitch;
	int i = 0;
	for (i = 0; i < (short) m_unk0x488;) {
		if (Graph) {
			VID* progress = EmptyVid;
			if (Map && Map->m_noVid > 0 && Map->m_vids[0]) {
				progress = Map->m_vids[0];
			}
			((GRAPH*) Graph)->DrawLoadBar(progress);
		}
		short w = 0;
		short h = 0;
		p_res->Read(&w, 2);
		p_res->Read(&h, 2);

		int format;
		if (m_pixelFlag16 & 0x800) {
			if ((m_pixelFlag16 & 2) && (m_pixelFlag16 & 1)) {
				m_unk0x48c[i] = new TEXTURE(w, h, 0x33545844, 0); // DXT3
			}
			else {
				m_unk0x48c[i] = new TEXTURE(w, h, 0x31545844, 0); // DXT1
			}
		}
		else if (m_pixelFlag16 & 8) {
			m_unk0x48c[i] = new TEXTURE(w, h, 41, 0); // P8
		}
		else if ((m_pixelFlag16 & 2) && (m_pixelFlag16 & 1)) {
			m_unk0x48c[i] = new TEXTURE(w, h, 26, 0); // A4R4G4B4
		}
		else {


			int format = GameDesc->m_layerRules == GAME_LAYERS_ZS1 ? D3DFMT_A1R5G5B5 : D3DFMT_R5G6B5;
			m_unk0x48c[i] = new TEXTURE(w, h, format, 0);
		}
		if (!m_unk0x48c[i]->m_data) {
			Error(
				3,
				// STRING: ALIEN 0x482c84
				"texture",
				0
			);

			if (colorCoder) {
				delete colorCoder;
			}
			if (zCoder) {
				delete zCoder;
			}
			operator delete(unpack);
			return;
		}

		if (m_pixelFlag16 & 8) {
			Error(
				10,
				// STRING: ALIEN 0x482cf8
				"palette %i",
				m_unk0x48c[i]->m_format == 41
			);
			p_res->Read(palette, 768);
			if (m_unk0x48c[i]->m_format == 41) {
				unsigned char quads[1024];
				const unsigned char* src = palette;
				unsigned char* dst = quads;
				for (int p = 0; p < 256; ++p) {
					PackedWrite<unsigned int>(dst, (unsigned int) COLOR(src[0], src[1], src[2]).m_value);
					src += 3;
					dst += 4;
				}
				m_unk0x48c[i]->SetPalette(quads);
			}
		}

		int offset = 0;
		p_res->Read(&offset, 4);
		int decoded = p_res->ReadPacked(unpack, offset, colorCoder);
		if (decoded) {
			Error(
				5,
				// STRING: ALIEN 0x482ce8
				"Can't decode",
				i
			);
		}

		if ((m_pixelFlag16 & 8) || offset >= 2 * w * h) {

			unsigned char* locked = (unsigned char*) m_unk0x48c[i]->Lock(&pitch, 0);
			if (!locked) {
				Error(
					0,
					// STRING: ALIEN 0x482c74
					"texture surface",
					0
				);

				if (colorCoder) {
					delete colorCoder;
				}
				if (zCoder) {
					delete zCoder;
				}
				operator delete(unpack);
				return;
			}
			else {
				for (int y = 0; y < h; ++y) {
					unsigned short* row = (unsigned short*) (locked + pitch * y);
					if (m_pixelFlag16 & 8) {
						if (m_unk0x48c[i]->m_format != 41) {
							for (int x = 0; x < w; ++x) {
								unsigned char idx = ((unsigned char*) unpack)[x + y * w];
								const unsigned char* pe = palette + 3 * idx;
								row[x] = (unsigned short) (((pe[2] >> 3) & 0x1f) | ((pe[0] & 0xf8) << RGB16_rShift) |
														   (RGB16_gMask & (pe[1] << RGB16_gShift)));
							}
						}
						else {
							memcpy(row, (unsigned char*) unpack + y * w, w);
						}
					}
					else if (m_unk0x48c[i]->m_format != 23 && m_unk0x48c[i]->m_format != 26) {
						unsigned short* s = (unsigned short*) unpack + y * w;
						for (int x = 0; x < w; ++x) {
							unsigned short v = s[x];
							row[x] = (unsigned short) ((v & 0x1f) | ((v >> 1) & 0x7fe0));
							if (m_unk0x48c[i]->m_format == D3DFMT_A1R5G5B5 && v) {
								row[x] |= 0x8000;
							}
						}
					}
					else {
						memcpy(row, (unsigned short*) unpack + y * w, 2 * w);
					}
				}
			}
		}
		else {

			Error(
				10,
				// STRING: ALIEN 0x482cdc
				"Load DXT",
				0
			);
			void* locked = m_unk0x48c[i]->Lock(&pitch, 0);
			if (!locked) {
				Error(
					0,
					// STRING: ALIEN 0x482cc8
					"DXT texture surface",
					0
				);
			}
			else {
				memcpy(locked, unpack, offset);
			}
		}

		int next = i;
		if (m_pixelFlag16 & 4) {

			++i;
			TEXTURE* ztex = new TEXTURE(w, h, 80, 2); // D16
			m_unk0x48c[i] = ztex;
			p_res->Read(&offset, 4);
			if (m_unk0x48c[i]->m_data) {
				void* locked = m_unk0x48c[i]->Lock(&pitch, 0);
				if (locked) {
					if (offset == pitch * h) {
						int zdec = p_res->ReadPacked(locked, offset, zCoder);
						if (zdec) {
							Error(
								5,
								// STRING: ALIEN 0x482c8c
								"Can't decode z",
								offset - zdec
							);
						}
					}
					else {
						Error(
							5,
							// STRING: ALIEN 0x482c9c
							"ZBuffer: invalid size",
							offset
						);
					}
				}
				else {
					Error(
						0,
						// STRING: ALIEN 0x482cb4
						"texture z surface",
						0
					);
					p_res->m_state = 2;
					fseek(p_res->m_file, offset, SEEK_CUR);
				}
			}
			else {
				Error(3, "texture z surface", 0);
				p_res->m_state = 2;
				fseek(p_res->m_file, offset, SEEK_CUR);
			}
		}
		++i;
	}

	if (p_res->GoNext(0x41544144 /* 'DATA' */)) {
		Error(
			5,
			// STRING: ALIEN 0x482bb8
			"DATA",
			0
		);
	}
	if (m_pixelFlag16 & 0x10) {
		p_res->SubLoad((void**) &m_unk0x484, 0);
		if (!m_unk0x484) {
			Error(
				5,
				// STRING: ALIEN 0x482c68
				"tex_coor",
				0
			);
		}
	}
	else {
		int n = p_res->m_subSize / 20;
		m_unk0x484 = (VID_CHILD*) operator new(sizeof(VID_CHILD) * n);
		VID_CHILD* children = m_unk0x484;
		for (int c = 0; c < n; ++c) {
			p_res->Read(&children[c].m_unk0x00, 4);
			short s;
			p_res->Read(&s, 2);
			children[c].m_texture = s;
			p_res->Read(&s, 2);
			children[c].m_x = s;
			p_res->Read(&s, 2);
			children[c].m_y = s;
			p_res->Read(&s, 2);
			children[c].m_w = s;
			p_res->Read(&s, 2);
			children[c].m_h = s;
			p_res->Read(&s, 2);
			children[c].m_offsetX = s;
			p_res->Read(&s, 2);
			children[c].m_offsetY = s;
			p_res->Read(&s, 2);
			children[c].m_next = s;
		}
	}

	p_res->GoNext(0x44414853 /* 'SHAD' */);

	if (colorCoder) {
		delete colorCoder;
	}
	if (zCoder) {
		delete zCoder;
	}
	operator delete(unpack);
}

// FUNCTION: ALIEN 0x41a730
void VID_HARDWARE::SetLayer()
{
	if (GameDesc->m_layerRules == GAME_LAYERS_LOCOLAND) {


		if (m_flag & 0x40000000) {
			m_layer = 1;
		}
		else if (m_unk0x0c == 64) {
			m_layer = 7;
		}
		else if (!m_unk0x488) {
			m_layer = 12;
		}
		else if (m_pixelFlag16 & 4) {
			m_layer = (m_pixelFlag16 & 2) ? 6 : 0;
		}
		else if (m_pixelFlag16 & 2) {
			m_layer = (m_flag & 0x10000) ? 6 : 9;
		}
		else {
			m_layer = 5;
		}
		return;
	}
	unsigned int flag = m_flag;
	if (flag & 0x40000000) {
		m_layer = 4;
		return;
	}
	if (m_unk0x0c == 64) {
		m_layer = 10;
		return;
	}
	if (!m_unk0x488) {
		m_layer = GameDesc->m_layerRules == GAME_LAYERS_ZS1 ? 19 : 15;
		return;
	}
	unsigned short pixelFlag = m_pixelFlag16;
	if ((m_pixelFlag16 & 4) && (m_pixelFlag16 & 2)) {
		m_layer = 9;
		return;
	}
	if (m_pixelFlag16 & 4) {
		m_layer = 0;
		return;
	}
	if (GameDesc->m_layerRules == GAME_LAYERS_ZS1) {


		if ((flag & 0x8000) && m_noDir == 255) {
			m_layer = 18;
			return;
		}
		if (flag & 0x8000) {
			m_layer = m_unk0x0c == 16 ? 17 : 14;
			return;
		}
		if (m_unk0x0c == 16) {
			m_layer = 15;
			return;
		}
	}
	if (GameDesc->m_layerRules == GAME_LAYERS_AS1 && m_idx == 1) {
		m_layer = 14;
		return;
	}
	if (m_pixelFlag16 & 2) {
		if (flag & 0x10000) {
			m_layer = 9;
		}
		else {
			m_layer = 12;
		}
		return;
	}
	m_layer = 8;
}

inline static float DrawInterpolate(float* p_table, float p_frame)
{
	int fi = (int) p_frame;
	return (fi >= 7) ? p_table[7] : (p_table[fi + 1] - p_table[fi]) * (p_frame - fi) + p_table[fi];
}

inline static bool DrawInViewPort(const GRAPH_CORE* p_graph, float p_x, float p_y)
{
	return p_x >= p_graph->m_viewXMin && p_x < p_graph->m_viewXMax && p_y >= p_graph->m_viewYMin &&
		   p_y < p_graph->m_viewYMax;
}

static int CopyScaledUIZBuffer(GRAPH_CORE* p_graph, const int* p_dst, const int* p_src, TEXTURE* p_texture)
{
	if (!p_graph->m_zbuffer || !p_texture || !p_texture->m_data) {
		return 0;
	}
	int dstWidth = p_dst[2] - p_dst[0];
	int dstHeight = p_dst[3] - p_dst[1];
	int srcWidth = p_src[2] - p_src[0];
	int srcHeight = p_src[3] - p_src[1];
	if (dstWidth <= 0 || dstHeight <= 0 || srcWidth <= 0 || srcHeight <= 0) {
		return 0;
	}

	int left = p_dst[0] > (int) p_graph->m_viewXMin ? p_dst[0] : (int) p_graph->m_viewXMin;
	int top = p_dst[1] > (int) p_graph->m_viewYMin ? p_dst[1] : (int) p_graph->m_viewYMin;
	int right = p_dst[2] < (int) p_graph->m_viewXMax ? p_dst[2] : (int) p_graph->m_viewXMax;
	int bottom = p_dst[3] < (int) p_graph->m_viewYMax ? p_dst[3] : (int) p_graph->m_viewYMax;
	if (left < 0) {
		left = 0;
	}
	if (top < 0) {
		top = 0;
	}
	if (right > (int) p_graph->m_width) {
		right = (int) p_graph->m_width;
	}
	if (bottom > (int) p_graph->m_height) {
		bottom = (int) p_graph->m_height;
	}
	if (left >= right || top >= bottom) {
		return 0;
	}

	int sourcePitch;
	unsigned short* source = (unsigned short*) p_texture->Lock(&sourcePitch, 0);
	if (!source) {
		return 0;
	}
	sourcePitch /= (int) sizeof(unsigned short);
	unsigned short* destination = (unsigned short*) p_graph->m_zbuffer;
	for (int y = top; y < bottom; ++y) {
		int sy = p_src[1] + (int) ((((long long) (y - p_dst[1]) * 2 + 1) * srcHeight) / (2 * dstHeight));
		if (sy < 0) {
			sy = 0;
		}
		else if (sy >= p_texture->m_height) {
			sy = p_texture->m_height - 1;
		}
		for (int x = left; x < right; ++x) {
			int sx = p_src[0] + (int) ((((long long) (x - p_dst[0]) * 2 + 1) * srcWidth) / (2 * dstWidth));
			if (sx < 0) {
				sx = 0;
			}
			else if (sx >= p_texture->m_width) {
				sx = p_texture->m_width - 1;
			}
			destination[(size_t) y * p_graph->m_zpitch + x] = source[(size_t) sy * sourcePitch + sx];
		}
	}
	return 0;
}

// FUNCTION: ALIEN 0x41a7f0
int VID_HARDWARE::Draw(SPRITE* p_sprite)
{
	if (m_unk0x488) {
		VID_CHILD* frameChild = &m_unk0x484[p_sprite->m_noCadr];
		if (frameChild->m_h) {
			int needEx = m_unk0x47c;


			if (!PropHide()) {
				GRAPH_CORE* graph = (GRAPH_CORE*) Graph;
				if (GameDesc->m_layerRules == GAME_LAYERS_ZS1) {

					graph->SetRenderState(D3DRS_ALPHATESTENABLE, 1);
					graph->SetRenderState(D3DRS_ALPHAREF, 0);
					graph->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
				}
				bool uiSprite = p_sprite->m_uiScale != 0;
				int savedMinFilter = graph->m_state.m_minFilter;
				int savedMagFilter = graph->m_state.m_magFilter;
				if (uiSprite) {
					graph->SetTextureStageState(D3DTSS_MINFILTER, D3DTEXF_POINT);
					graph->SetTextureStageState(D3DTSS_MAGFILTER, D3DTEXF_POINT);
				}
				int baseX = (int) (p_sprite->m_x - Map->m_shiftX);
				int baseY = (int) (p_sprite->m_y - p_sprite->m_z - Map->m_shiftY);
				int zval = (int) p_sprite->m_z;
				if (GameDesc->m_layerRules == GAME_LAYERS_ZS1 && (m_flag & 0x200)) {
					zval = 3;
				}
				int segments = 1;
				float scaleX = m_gammaR * p_sprite->UIDrawScale();
				float scaleY = m_gammaG * p_sprite->UIDrawScale();

				if (needEx & 2) {
					EX_SPRITE_DATA* ex = p_sprite->m_exData;
					scaleX *= DrawInterpolate(m_exData->m_unk0xe4, ex->m_unk0x1c);
					scaleY *= DrawInterpolate(m_exData->m_unk0x104, ex->m_unk0x1c);
				}
				if (needEx & 4) {
					EX_SPRITE_DATA* ex = p_sprite->m_exData;
					float ox = DrawInterpolate(m_exData->m_unk0x144, ex->m_unk0x1c);
					baseX += (int) ox;
					float oy = DrawInterpolate(m_exData->m_unk0x164, ex->m_unk0x1c);
					baseY += (int) oy;
					float oz = DrawInterpolate(m_exData->m_unk0x184, ex->m_unk0x1c);
					zval += (int) oz;
				}
				if (m_flag & 0x200000) {
					int span = frameChild->m_w;
					if (span) {
						EX_SPRITE_DATA* ex = p_sprite->m_exData;
						int dx = abs((int) (ex->m_x - p_sprite->X())) / span;
						int dy = abs((int) (ex->m_y - ex->m_z - p_sprite->m_y + p_sprite->m_z)) / frameChild->m_h;
						segments = (dx > dy ? 2 * dx : 2 * dy) + 1;
					}
				}

				for (int s = 0; s < segments; ++s) {
					unsigned int flag = m_flag;
					if (flag & 0x200000) {
						EX_SPRITE_DATA* ex = p_sprite->m_exData;
						baseX = (int) (p_sprite->m_x - Map->m_shiftX + (ex->m_x - p_sprite->X()) * s / segments);
						baseY = (int) (p_sprite->m_y - p_sprite->m_z - Map->m_shiftY +
									   (ex->m_y - ex->m_z - p_sprite->m_y + p_sprite->m_z) * s / segments);
						zval = (int) ((ex->m_z - p_sprite->Z()) * s / segments + p_sprite->m_z);
					}
					bool alphaDepthGate = false;
					bool alphaAnchorVisible = false;







					const bool independentQuad = GameDesc->m_layerRules == GAME_LAYERS_LOCOLAND ||
												 (GameDesc->m_layerRules == GAME_LAYERS_ZS1 &&
												  (m_unk0x0c == 16 || (m_unk0x0c == 1 && !(m_pixelFlag16 & 2))));
					if (!(flag & 0x8000) && !(m_pixelFlag16 & 4) && !independentQuad) {
						GRAPH_CORE* g = (GRAPH_CORE*) Graph;
						int threshold = 8 * zval + 1024;
						if ((m_pixelFlag16 & 2) && !uiSprite) {
							alphaDepthGate = true;
							alphaAnchorVisible = RENDER_MATH::AlphaSpriteAnchorVisible(
								(const unsigned short*) g->m_zbuffer,
								g->m_zpitch,
								(int) g->m_width,
								(int) g->m_height,
								(int) g->m_viewXMin,
								(int) g->m_viewYMin,
								(int) g->m_viewXMax,
								(int) g->m_viewYMax,
								baseX,
								baseY,
								threshold,
								m_footprintWidth,
								m_footprintHeight
							);
						}
						else {
							if (!DrawInViewPort(g, (float) baseX, (float) baseY)) {
								continue;
							}
							if (((unsigned short*) g->m_zbuffer)[baseX + baseY * g->m_zpitch] > threshold) {
								continue;
							}
						}
					}

					VID_CHILD* child = &m_unk0x484[p_sprite->m_noCadr];
					while (child) {
						int clipW = child->m_w;
						int clipH = child->m_h;
						int sx;
						int sy;
						int drawW;
						int drawH;
						if (uiSprite) {
							int spriteLeft = baseX - ScaledUIBoundary(m_unk0x2f6, scaleX) / 2;
							int spriteTop = baseY - ScaledUIBoundary(m_messageLineHeight, scaleY) / 2;
							int childLeft = ScaledUIBoundary(child->m_offsetX, scaleX);
							int childTop = ScaledUIBoundary(child->m_offsetY, scaleY);
							int childRight = ScaledUIBoundary(child->m_offsetX + clipW, scaleX);
							int childBottom = ScaledUIBoundary(child->m_offsetY + clipH, scaleY);
							sx = spriteLeft + childLeft;
							sy = spriteTop + childTop;
							drawW = childRight - childLeft;
							drawH = childBottom - childTop;
						}
						else {
							int cxOff = child->m_offsetX - m_unk0x2f6 / 2;
							int cyOff = child->m_offsetY - m_messageLineHeight / 2;
							sx = baseX + (int) (cxOff * scaleX);
							sy = baseY + (int) (cyOff * scaleY);
							drawW = (int) (clipW * scaleX);
							drawH = (int) (clipH * scaleY);
						}
						bool childVisible = uiSprite ? drawW > 0 && drawH > 0 && sx + drawW > ViewXMin() &&
														   sx < ViewXMax() && sy + drawH > ViewYMin() && sy < ViewYMax()
													 : sx + drawW >= ViewXMin() && sx < ViewXMax() &&
														   sy + drawH >= ViewYMin() && sy < ViewYMax();
						if (childVisible) {
							if ((m_pixelFlag16 & 4) && !uiSprite) {
								if (sx + clipW > ViewXMax()) {
									clipW = ViewXMax() - sx;
								}
								if (sy + clipH > ViewYMax()) {
									clipH = ViewYMax() - sy;
								}
							}
							int dst[4];
							dst[0] = sx;
							dst[1] = sy;
							dst[2] = sx + (uiSprite ? drawW : (int) (clipW * scaleX));
							dst[3] = sy + (uiSprite ? drawH : (int) (clipH * scaleY));
							if (alphaDepthGate && !alphaAnchorVisible) {
								GRAPH_CORE* g = (GRAPH_CORE*) Graph;
								if (!RENDER_MATH::AlphaSpriteChildVisible(
										(const unsigned short*) g->m_zbuffer,
										g->m_zpitch,
										(int) g->m_width,
										(int) g->m_height,
										(int) g->m_viewXMin,
										(int) g->m_viewYMin,
										(int) g->m_viewXMax,
										(int) g->m_viewYMax,
										dst,
										8 * zval + 1024
									)) {
									if (!child->m_next) {
										break;
									}
									child = &m_unk0x484[child->m_next];
									continue;
								}
							}
							int src[4];
							src[0] = child->m_x;
							src[1] = child->m_y;
							src[2] = src[0] + clipW;
							src[3] = src[1] + clipH;

							int zfunc;
							if (m_pixelFlag16 & 4) {
								zfunc = uiSprite ? CopyScaledUIZBuffer(
													   (GRAPH_CORE*) Graph,
													   dst,
													   src,
													   m_unk0x48c[child->m_texture + 1]
												   )
												 : ((GRAPH_CORE*) Graph)
													   ->CopyToZBuffer(dst, src, m_unk0x48c[child->m_texture + 1]);
								if (zfunc) {
									Error(
										1,
										// STRING: ALIEN 0x482d18
										"zbuffer",
										zfunc
									);
								}
							}

							float z1 = zval * 0.00012207031f + 0.015625f;
							if ((m_flag & 0x10000) && m_unk0x60 != 0.0f) {
								z1 = 0.5f * (FSin[(CurrentTime >> 3) & 0xff] * m_unk0x60 * 0.00012207031f) + z1;
							}
							float z2;
							if ((m_flag & 0x8000) || (m_pixelFlag16 & 4)) {
								z1 = z2 = 0.99999988f;
								((GRAPH_CORE*) Graph)->SetRenderState(D3DRS_ZFUNC, 8);
							}
							else if (m_unk0x24 > m_footprintHeight) {
								float d = clipH * 0.00012207031f;
								z2 = z1 - d;
								z1 = d + z1;
								if (GameDesc->m_layerRules == GAME_LAYERS_ZS1) {

									z2 = z1;
								}
								((GRAPH_CORE*) Graph)->SetRenderState(D3DRS_ZFUNC, 7);
							}
							else {
								z2 = z1;
								((GRAPH_CORE*) Graph)->SetRenderState(D3DRS_ZFUNC, 7);
							}
							if (m_pixelFlag16 & 2) {
								if (m_pixelFlag16 & 1) {
									((GRAPH_CORE*) Graph)->SetAlphaBlend(5, 6);
								}
								else {
									((GRAPH_CORE*) Graph)->SetAlphaBlend(9, 2);
								}
							}
							else {
								((GRAPH_CORE*) Graph)->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
							}

							if (m_flag & 0x800) {
								GAMMA composited;
								composited.Add(
									GAMMA(GAMMA::RAW_COPY, m_colorSub, m_colorAdd),
									GAMMA(GAMMA::RAW_COPY, p_sprite->GetGamma())
								);
								m_unk0x48c[child->m_texture]->Draw_z(z1, std::bit_cast<int>(z2), dst, src, &composited);
							}
							else {
								int graphNeg = ((GRAPH_CORE*) Graph)->m_gammaSet.m_a;
								int graphPos = ((GRAPH_CORE*) Graph)->m_gammaSet.m_b;
								GAMMA composited;
								composited.Add(
									GAMMA(GAMMA::RAW_COPY, m_colorSub, m_colorAdd),
									GAMMA(GAMMA::RAW_COPY, p_sprite->GetGamma())
								);
								GAMMA withGraph;
								withGraph.Add(composited, GAMMA(GAMMA::RAW_COPY, graphNeg, graphPos));
								m_unk0x48c[child->m_texture]->Draw_z(z1, std::bit_cast<int>(z2), dst, src, &withGraph);
							}
						}
						if (!child->m_next) {
							break;
						}
						child = &m_unk0x484[child->m_next];
					}
				}
				if (uiSprite) {
					graph->SetTextureStageState(D3DTSS_MINFILTER, savedMinFilter);
					graph->SetTextureStageState(D3DTSS_MAGFILTER, savedMagFilter);
				}
			}
		}
	}
	return 0;
}
