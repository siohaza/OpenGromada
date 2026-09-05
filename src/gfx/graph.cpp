
#include "gfx/graph.h"

#include "game/game_descriptor.h"
#include "game/gametime.h"
#include "game/settings.h"
#include "gfx/color.h"
#include "gfx/gamma.h"
#include "gfx/gpu_backend.h"
#include "gfx/gpu_graph.h"
#include "gfx/graph_core.h"
#include "gfx/render_math.h"
#include "gfx/texture.h"
#include "platform/portable_config.h"
#include "platform/render.h"
#include "sprite/plane_internal.h"
#include "util/myerror.h"
#include "util/registry.h"
#include "util/resource.h"
#include "util/string.h"

#include <SDL3/SDL.h>
#include <bit>
#include <cmath>
#include <cstdint>
#include <string.h>

inline static unsigned short RampPack(int p_r, int p_g, int p_b)
{
	if (p_r < 0) {
		p_r = 0;
	}
	else if (p_r > 255) {
		p_r = 255;
	}
	if (p_g < 0) {
		p_g = 0;
	}
	else if (p_g > 255) {
		p_g = 255;
	}
	if (p_b < 0) {
		p_b = 0;
	}
	else if (p_b > 255) {
		p_b = 255;
	}
	return (unsigned short) ((RGB16_gMask & (p_g << RGB16_gShift)) | ((p_b >> 3) & 0x1f) |
							 ((p_r & 0xf8) << RGB16_rShift));
}

inline static void SetRampFormat(int p_is565)
{
	if (p_is565) {
		RGB16_rMask = 0xf800;
		RGB16_gMask = 0x7e0;
		RGB16_rShift = 8;
		RGB16_gShift = 3;
	}
	else {
		RGB16_rMask = 0x7c00;
		RGB16_gMask = 0x3e0;
		RGB16_rShift = 7;
		RGB16_gShift = 2;
	}
}

// GLOBAL: ALIEN 0x4b2c64
GRAPH* Graph;

// GLOBAL: ALIEN 0x47f7f0
float FSin[256] = {
	0.0f,           0.024541229f,   0.0490676723f,  0.0735645667f,  0.0980171412f,  0.122410677f,   0.146730468f,
	0.170961887f,   0.195090324f,   0.219101235f,   0.242980182f,   0.266712755f,   0.290284663f,   0.313681751f,
	0.336889863f,   0.359895051f,   0.382683426f,   0.405241311f,   0.427555084f,   0.449611336f,   0.471396744f,
	0.492898196f,   0.514102757f,   0.534997642f,   0.555570245f,   0.575808167f,   0.59569931f,    0.615231574f,
	0.634393275f,   0.653172851f,   0.671558976f,   0.689540565f,   0.707106769f,   0.724247098f,   0.740951121f,
	0.757208824f,   0.773010433f,   0.78834641f,    0.803207517f,   0.817584813f,   0.831469595f,   0.84485358f,
	0.857728601f,   0.870086968f,   0.881921291f,   0.893224299f,   0.903989315f,   0.914209783f,   0.923879504f,
	0.932992816f,   0.941544056f,   0.949528158f,   0.956940353f,   0.963776052f,   0.970031261f,   0.975702107f,
	0.980785251f,   0.985277653f,   0.989176512f,   0.992479563f,   0.99518472f,    0.997290432f,   0.99879545f,
	0.999698818f,   1.0f,           0.999698818f,   0.99879545f,    0.997290432f,   0.99518472f,    0.992479563f,
	0.989176512f,   0.985277653f,   0.980785251f,   0.975702107f,   0.970031261f,   0.963776052f,   0.956940353f,
	0.949528158f,   0.941544056f,   0.932992816f,   0.923879504f,   0.914209783f,   0.903989315f,   0.893224299f,
	0.881921291f,   0.870086968f,   0.857728601f,   0.84485358f,    0.831469595f,   0.817584813f,   0.803207517f,
	0.78834641f,    0.773010433f,   0.757208824f,   0.740951121f,   0.724247098f,   0.707106769f,   0.689540565f,
	0.671558976f,   0.653172851f,   0.634393275f,   0.615231574f,   0.59569931f,    0.575808167f,   0.555570245f,
	0.534997642f,   0.514102757f,   0.492898196f,   0.471396744f,   0.449611336f,   0.427555084f,   0.405241311f,
	0.382683426f,   0.359895051f,   0.336889863f,   0.313681751f,   0.290284663f,   0.266712755f,   0.242980182f,
	0.219101235f,   0.195090324f,   0.170961887f,   0.146730468f,   0.122410677f,   0.0980171412f,  0.0735645667f,
	0.0490676723f,  0.024541229f,   0.0f,           -0.0245412271f, -0.0490676723f, -0.0735645667f, -0.0980171412f,
	-0.122410677f,  -0.146730468f,  -0.170961887f,  -0.195090324f,  -0.219101235f,  -0.242980182f,  -0.266712755f,
	-0.290284663f,  -0.313681751f,  -0.336889863f,  -0.359895051f,  -0.382683426f,  -0.405241311f,  -0.427555084f,
	-0.449611336f,  -0.471396744f,  -0.492898196f,  -0.514102757f,  -0.534997642f,  -0.555570245f,  -0.575808167f,
	-0.59569931f,   -0.615231574f,  -0.634393275f,  -0.653172851f,  -0.671558976f,  -0.689540565f,  -0.707106769f,
	-0.724247098f,  -0.740951121f,  -0.757208824f,  -0.773010433f,  -0.78834641f,   -0.803207517f,  -0.817584813f,
	-0.831469595f,  -0.84485358f,   -0.857728601f,  -0.870086968f,  -0.881921291f,  -0.893224299f,  -0.903989315f,
	-0.914209783f,  -0.923879504f,  -0.932992816f,  -0.941544056f,  -0.949528158f,  -0.956940353f,  -0.963776052f,
	-0.970031261f,  -0.975702107f,  -0.980785251f,  -0.985277653f,  -0.989176512f,  -0.992479563f,  -0.99518472f,
	-0.997290432f,  -0.99879545f,   -0.999698818f,  -1.0f,          -0.999698818f,  -0.99879545f,   -0.997290432f,
	-0.99518472f,   -0.992479563f,  -0.989176512f,  -0.985277653f,  -0.980785251f,  -0.975702107f,  -0.970031261f,
	-0.963776052f,  -0.956940353f,  -0.949528158f,  -0.941544056f,  -0.932992816f,  -0.923879504f,  -0.914209783f,
	-0.903989315f,  -0.893224299f,  -0.881921291f,  -0.870086968f,  -0.857728601f,  -0.84485358f,   -0.831469595f,
	-0.817584813f,  -0.803207517f,  -0.78834641f,   -0.773010433f,  -0.757208824f,  -0.740951121f,  -0.724247098f,
	-0.707106769f,  -0.689540565f,  -0.671558976f,  -0.653172851f,  -0.634393275f,  -0.615231574f,  -0.59569931f,
	-0.575808167f,  -0.555570245f,  -0.534997642f,  -0.514102757f,  -0.492898196f,  -0.471396744f,  -0.449611336f,
	-0.427555084f,  -0.405241311f,  -0.382683426f,  -0.359895051f,  -0.336889863f,  -0.313681751f,  -0.290284663f,
	-0.266712755f,  -0.242980182f,  -0.219101235f,  -0.195090324f,  -0.170961887f,  -0.146730468f,  -0.122410677f,
	-0.0980171412f, -0.0735645667f, -0.0490676761f, -0.024541229f,
};

// FUNCTION: ALIEN 0x40c930
unsigned char* GRAPH::SetWind(int p_force, ANGLE p_direction)
{
	m_windForce = p_force * 0.001f;
	unsigned char* result = &m_windDirection;
	if (result != (unsigned char*) &p_direction) {
		*result = p_direction.m_dir;
	}
	return result;
}

inline static int DisplayFormatBits(int p_format)
{
	switch (p_format) {
	case 21:
	case 22:
		return 32;
	case 23:
	case 24:
	case 25:
	case 26:
		return 16;
	case 20:
		return 24;
	default:
		return 0;
	case 41:
	case 0x33545844 /* 'DXT3' */:
	case 0x35545844 /* 'DXT5' */:
		return 8;
	case 0x31545844 /* 'DXT1' */:
		return 4;
	}
}

inline static unsigned int DisplayColor32Flag(int p_flags)
{
	return (p_flags >> 1) & 1;
}

inline static void LogViewPort(const char* p_format, float p_xMin, float p_yMin, float p_xMax, float p_yMax)
{
	MYERROR::Log(::Error, p_format, p_xMin, p_yMin, p_xMax, p_yMax);
}

inline static STRING FormatPixelShader(const char* p_format, int p_version)
{
	return Printf(p_format, p_version);
}

// STUB: ALIEN 0x42f4b0
int GRAPH::Init()
{
	GRAPH_CORE* core = (GRAPH_CORE*) this;
	core->m_flags = (core->m_flags & 0xfffffdff) | ((Registry->GetInt(
														 // STRING: ALIEN 0x483f10
														 STRING("LowDetail"),
														 0) &
													 1)
													<< 9);

	core->m_flags &= ~0x20u; // not the reference rasterizer
	core->m_flags &= ~0x10u; // no "non-GDI" fullscreen mode
	core->m_flags &= ~0x40u; // no triple buffering

	PortableConfig_SetInt("display", "ScreenX", core->m_outputWidth);
	PortableConfig_SetInt("display", "ScreenY", core->m_outputHeight);
	PortableConfig_SetInt("display", "RenderWidth", core->m_renderWidth);
	PortableConfig_SetInt("display", "NativeResolution", core->m_nativeResolution);
	PortableConfig_SetInt("display", "UIScale", core->m_uiScaleSetting);
	PortableConfig_SetInt("display", "AutomaticResolution", core->m_automaticResolution);
	PortableConfig_SetInt("display", "DisplayPolicyVersion", SETTINGS_DISPLAY_POLICY_VERSION);
	PortableConfig_SetInt("display", "BPP", 8 * core->BytesPerPixel());
	PortableConfig_SetInt("display", "Device", core->m_curAdapter);
	PortableConfig_SetInt("display", "FullScreen", (core->m_flags & 0x80) != 0);
	PortableConfig_SetInt("display", "VSync", (core->m_flags & 0x100) != 0);
	PortableConfig_SetInt("meta", "ConfigVersion", PORTABLE_CONFIG_VERSION);
	PortableConfig_Flush();

	core->m_debugFontHeight = 23;
	int result = core->Init();
	if (result) {
		return result;
	}
	core->m_window = Platform_RenderWindow();

	core->SetViewPort(0.0f, 0.0f, core->m_width, core->m_height);
	LogViewPort(
		// STRING: ALIEN 0x483ecc
		"SetViewPort (%.0f,%.0f) - (%.0f,%.0f)",
		core->m_viewXMin,
		core->m_viewYMin,
		core->m_viewXMax,
		core->m_viewYMax);

	core->SetTextureStageState(D3DTSS_COLORARG1, D3DTA_TEXTURE);
	core->SetTextureStageState(D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	core->SetTextureStageState(D3DTSS_COLOROP, D3DTOP_MODULATE);
	core->SetTextureStageState(D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	core->SetTextureStageState(D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
	core->SetTextureStageState(D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	core->SetTextureStageState(D3DTSS_MINFILTER, D3DTEXF_POINT);
	core->SetTextureStageState(D3DTSS_MAGFILTER, D3DTEXF_POINT);
	core->SetRenderState(D3DRS_SPECULARENABLE, 0);
	core->m_flags &= ~0x2000u;
	core->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
	core->m_flags &= ~0x4000u;
	core->SetRenderState(D3DRS_ALPHATESTENABLE, 0);

	int scratchWidth = RENDER_MATH::ScratchExtent(core->m_width);
	int scratchHeight = RENDER_MATH::ScratchExtent(core->m_height);
	core->m_texE0C = new TEXTURE(scratchWidth, scratchHeight, D3DFMT_P8, 0);
	if (!core->m_texE0C->m_data) {
		if (::Error) {
			MYERROR::Error(::Error,
						   "GRAPH",
						   3,
						   // STRING: ALIEN 0x483ebc
						   "light buffer",
						   0);
		}
		return 1;
	}
	if (core->m_texE0C->m_format == D3DFMT_P8) {
		unsigned int palette[256];
		for (int i = 0; i < 256; ++i) {
			palette[i] = COLOR(i, i, i).m_value;
		}
		core->m_texE0C->SetPalette(palette);
	}
	core->m_texE10 = new TEXTURE(scratchWidth, scratchHeight, D3DFMT_R5G6B5, 0);
	if (!core->m_texE10->m_data) {
		if (::Error) {
			MYERROR::Error(::Error,
						   "GRAPH",
						   3,
						   // STRING: ALIEN 0x483eb0
						   "hiBuffer",
						   0);
		}
		return 1;
	}
	core->m_texE14 = new TEXTURE(scratchWidth, scratchHeight, D3DFMT_A4R4G4B4, 0);
	if (!core->m_texE14->m_data) {
		if (::Error) {
			MYERROR::Error(::Error,
						   "GRAPH",
						   3,
						   // STRING: ALIEN 0x483ea4
						   "alphaBuffer",
						   0);
		}
		return 1;
	}

	PLANE_INTERNAL::RetailExactEmptyCheck(this);
	core->ClearScreen(COLOR((int) 0xff000000));
	PLANE_INTERNAL::RetailExactEmptyCheck(this);
	core->CreateDebugFont(
		// STRING: ALIEN 0x483e9c
		STRING("Courier"),
		7,
		8);

	SetRampFormat(core->m_texE10->m_format == D3DFMT_R5G6B5);
	unsigned short* entry = core->m_snowRamp;
	for (int i = 8; i - 8 < 256; i += 8) {
		entry[0] = RampPack(i - 8, i - 8, i - 8);
		entry[1] = RampPack(i - 7, i - 7, i);
		entry[2] = RampPack(i, i - 6, i - 6);
		entry[3] = RampPack(i, i - 5, i);
		entry[4] = RampPack(i - 4, i, i - 4);
		entry[5] = RampPack(i - 3, i, i);
		entry[6] = RampPack(i, i, i - 2);
		entry[7] = RampPack(i, i, i);
		entry += 8;
	}

	STRING capsStr;
	if (core->m_flags & 0x200) {
		capsStr +=
			// STRING: ALIEN 0x483e60
			"LOWDETAIL ";
	}
	if (!(core->m_flags & 0x80)) {
		capsStr +=
			// STRING: ALIEN 0x483e4c
			"WINDOWED ";
	}
	if (core->m_flags & 0x100) {
		capsStr +=
			// STRING: ALIEN 0x483de8
			"VSYNC ";
	}
	MYERROR::Log(::Error,
				 // STRING: ALIEN 0x483dd0
				 "caps=%s",
				 (const char*) capsStr);
	MYERROR::Log(::Error,
				 // STRING: ALIEN 0x483dbc
				 "Pitch=%i zPitch=%i",
				 core->m_pitch * core->BytesPerPixel(),
				 2 * core->m_zpitch);
	core->ReloadPalettes();
	return 0;
}

// FUNCTION: ALIEN 0x431c00
unsigned int GRAPH::GetEffectState(int p_effect) const
{
	if (p_effect > 0 && p_effect < 16 && m_effectStart[p_effect]) {
		return (100 * RealCurrentTime - 100 * m_effectStart[p_effect]) / m_effectDuration[p_effect];
	}
	return -1;
}

// FUNCTION: ALIEN 0x431cf0
int GRAPH::SetEnvironment(int p_env)
{
	if (!(p_env & 0x80000000)) {
		if (p_env == 1 || p_env == 2) {
			m_env &= 0xfffffffc;
		}
		if (p_env & 0x0c00) {
			m_env &= 0xfffff3ff;
		}
		if (p_env & 0xc000) {
			m_env &= 0xffff3fff;
		}
		m_env |= p_env;
		return p_env;
	}
	m_env &= ~p_env;
	return ~p_env;
}

inline static bool DrawLightInViewPort(const GRAPH_CORE* p_graph, float p_x, float p_y)
{
	return p_x >= p_graph->m_viewXMin && p_x < p_graph->m_viewXMax && p_y >= p_graph->m_viewYMin &&
		   p_y < p_graph->m_viewYMax;
}

// FUNCTION: ALIEN 0x433670
void GRAPH::DrawLight(float p_x, float p_y, float p_z, int p_a, int p_b, unsigned int p_color)
{
	GRAPH_CORE* core = (GRAPH_CORE*) this;
	float lw;
	float lh;
	memcpy(&lw, &p_a, sizeof(lw));
	memcpy(&lh, &p_b, sizeof(lh));
	core->m_lightBufferToggle ^= 1;
	if (!(p_color & 0xffffff)) {
		return;
	}
	int hw = (3 * ((int) lw / 2)) & ~3;
	int hh = (3 * ((int) lh / 2)) & ~3;
	if (hw > 512) {
		hw = 512;
	}
	if (hh > 512) {
		hh = 512;
	}
	int falloffDiv = (int) lw * (int) lh / 500;
	int zHeight = (int) p_z;
	int zCenter = 2 * zHeight / 3 + 8;
	p_y = p_y - ((float) zCenter - p_z);
	if ((float) hw + p_x < core->m_viewXMin) {
		return;
	}
	if (p_x - (float) hw >= core->m_viewXMax) {
		return;
	}
	if ((float) hh + p_y < core->m_viewYMin) {
		return;
	}
	if (p_y - (float) hh >= core->m_viewYMax) {
		return;
	}

	float z1 = p_z;
	z1 += lw;
	z1 += 50.0f;
	z1 *= 0.0001220703125f;
	z1 += 0.015625f;
	int dstRect[4];
	dstRect[0] = (int) (p_x - (float) hw);
	dstRect[1] = (int) (p_y - (float) hh);
	dstRect[2] = (int) ((float) hw + p_x);
	dstRect[3] = (int) ((float) hh + p_y);
	RECT srcRect;
	srcRect.left = 0;
	srcRect.top = 0;
	srcRect.right = hw / 2;
	srcRect.bottom = hh / 2;

	TEXTURE* tex = core->m_lightBufferToggle ? core->m_texE10 : core->m_texE0C;
	if (GPU_RENDER::Active()) {
		GPU_GRAPH::LightMap(core, tex, p_x, p_y, hw, hh, zHeight, zCenter, falloffDiv);
	}
	else {
		int pitch = 0;
		char* pix = (char*) tex->Lock(&pitch, &srcRect);
		if (!pix) {
			if (::Error) {
				MYERROR::Error(::Error,
							   // STRING: ALIEN 0x47f240
							   "GRAPH",
							   10,
							   "light buffer",
							   0);
			}
			return;
		}
		// The light map is written a pixel at a time, so the pitch is carried in
		// the map's own units.
		if (tex->m_format != D3DFMT_P8) {
			pitch /= 2;
		}

		for (int row = -hh; row < hh; row += 4) {
			for (int col = -hw; col < hw; col += 4) {
				int mapX = col + hw;
				float sy = (float) row + p_y;
				int za;
				if (!DrawLightInViewPort(core, (float) col + p_x, sy)) {
					za = 0x7fff;
				}
				else {
					za = (((unsigned short*) core->m_zbuffer)[((int) p_y + row) * core->m_zpitch + col + (int) p_x] >>
						  3) -
						 128;
				}
				int zb;
				if (!DrawLightInViewPort(core, (float) col + p_x + 3.0f, (float) row + p_y + 3.0f)) {
					zb = 0x7fff;
				}
				else {
					zb = (((unsigned short*)
							   core->m_zbuffer)[((int) p_y + row + 3) * core->m_zpitch + col + (int) p_x + 3] >>
						  3) -
						 128;
				}
				if (zb < za) {
					za = zb;
				}
				int intensity = 0;
				if (za != 0x7fff) {
					int d = row + za - zCenter;
					int zDiff = za - zHeight;
					d = 9 * d * d / 4;
					d += zDiff * zDiff / 4;
					d += col * col;
					intensity = falloffDiv ? 256 - d / falloffDiv : 256 - d;
					if (intensity < 0) {
						intensity = 0;
					}
					else if (intensity > 255) {
						intensity = 255;
					}
				}
				int mapY = row + hh;
				if (tex->m_format != D3DFMT_P8) {
					((unsigned short*) pix)[pitch * (mapY / 4) + mapX / 4] = core->m_snowRamp[intensity];
				}
				else {
					((unsigned char*) pix)[pitch * (mapY / 4) + mapX / 4] = (unsigned char) intensity;
				}
			}
		}
	}
	core->SetRenderState(D3DRS_SPECULARENABLE, 0);
	core->SetAlphaBlend(9, 2);
	srcRect.left++;
	srcRect.right--;
	srcRect.top++;
	srcRect.bottom--;
	GAMMA gamma(COLOR((int) p_color), COLOR((int) 0xff000000));
	int lightSrc[4] = {srcRect.left, srcRect.top, srcRect.right, srcRect.bottom};

	tex->Draw_z(z1,
				GameDesc->m_layerRules == GAME_LAYERS_LOCOLAND ? std::bit_cast<int>(z1) : 0,
				dstRect,
				lightSrc,
				&gamma);
}

static bool ReadGraphParameterBytes(STREAM* p_stream, unsigned char* p_bytes, int p_size)
{
	RESOURCE* resource = dynamic_cast<RESOURCE*>(p_stream);
	if (resource && resource->Remaining() < p_size) {
		return resource->Fail("truncated graphics parameter record");
	}
	if (!p_stream || p_stream->Read(p_bytes, p_size)) {
		if (resource) {
			resource->Fail("truncated graphics parameter record");
		}
		else {
			MYERROR::Log(::Error, "Truncated graphics parameter stream");
		}
		return false;
	}
	return true;
}

static uint32_t GraphParameterWord(const unsigned char* p_bytes)
{
	return uint32_t(p_bytes[0]) | (uint32_t(p_bytes[1]) << 8) | (uint32_t(p_bytes[2]) << 16) |
		   (uint32_t(p_bytes[3]) << 24);
}

// FUNCTION: ALIEN 0x433bb0
unsigned char* GRAPH::OldLoadParameters(STREAM* p_stream)
{

	unsigned char bytes[11];
	if (!ReadGraphParameterBytes(p_stream, bytes, sizeof(bytes))) {
		return nullptr;
	}
	const int force = std::bit_cast<int16_t>(uint16_t(uint16_t(bytes[9]) | (uint16_t(bytes[10]) << 8)));
	GAMMA gamma(GAMMA::DECODE, GraphParameterWord(bytes + 4));
	m_env = GraphParameterWord(bytes);
	SetGamma(gamma);
	return SetWind(force, bytes[8]);
}

// FUNCTION: ALIEN 0x433cd0
int GRAPH::LoadParameters(STREAM* p_stream)
{

	unsigned char bytes[20];
	if (!ReadGraphParameterBytes(p_stream, bytes, sizeof(bytes))) {
		return 1;
	}
	const float force = std::bit_cast<float>(GraphParameterWord(bytes + 16));
	if (!std::isfinite(force)) {
		if (RESOURCE* resource = dynamic_cast<RESOURCE*>(p_stream)) {
			resource->Fail("non-finite graphics wind force");
		}
		else {
			MYERROR::Log(::Error, "Non-finite graphics wind force");
		}
		return 1;
	}
	GAMMA gamma(GAMMA::RAW_COPY, GraphParameterWord(bytes + 4), GraphParameterWord(bytes + 8));
	m_env = GraphParameterWord(bytes);
	SetGamma(gamma);
	m_windDirection = (unsigned char) GraphParameterWord(bytes + 12);
	m_windForce = force;
	return 0;
}

// FUNCTION: ALIEN 0x433d40
int GRAPH::SaveParameters(STREAM* p_stream)
{
	p_stream->Write(&m_env, 4);
	p_stream->Write(&m_gammaSet, 8);
	int wd = m_windDirection;
	p_stream->Write(&wd, 4);
	return p_stream->Write(&m_windForce, 4);
}

// FUNCTION: ALIEN 0x4343e0
int GRAPH::PlayMovie(const char* p_filename)
{
	return GameDesc->m_nativeMoviePlayback ? OpenMovie(p_filename) : 0;
}

// FUNCTION: ALIEN 0x442600
unsigned char* GRAPH::WindDirection(unsigned char* p_out)
{
	*p_out = m_windDirection;
	return p_out;
}
