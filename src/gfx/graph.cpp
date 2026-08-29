
#define DECOMP_COLOR_COPY_OUT_OF_LINE

#define DECOMP_INLINE_STRING_CHARP_CTOR
#define DECOMP_INLINE_STRING_DTOR
#define DECOMP_INLINE_STRING_CHARP_CONVERSION

#include "gfx/graph.h"

#include <windows.h>

#include "game/gametime.h"
#include "gfx/color.h"
#include "gfx/graph_core.h"
#include "gfx/gamma.h"
#include "gfx/texture.h"
#include "sprite/plane.h"
#include "util/myerror.h"
#include "util/registry.h"
#include "util/string.h"

static inline unsigned short RampPack(int p_r, int p_g, int p_b)
{
	if (p_r < 0)
		p_r = 0;
	else if (p_r > 255)
		p_r = 255;
	if (p_g < 0)
		p_g = 0;
	else if (p_g > 255)
		p_g = 255;
	if (p_b < 0)
		p_b = 0;
	else if (p_b > 255)
		p_b = 255;
	return (unsigned short) ((RGB16_gMask & (p_g << RGB16_gShift)) | ((p_b >> 3) & 0x1f)
		| ((p_r & 0xf8) << RGB16_rShift));
}

static inline void SetRampFormat(int p_is565)
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
	0.0f, 0.024541229f, 0.0490676723f, 0.0735645667f, 0.0980171412f, 0.122410677f, 0.146730468f, 0.170961887f,
	0.195090324f, 0.219101235f, 0.242980182f, 0.266712755f, 0.290284663f, 0.313681751f, 0.336889863f, 0.359895051f,
	0.382683426f, 0.405241311f, 0.427555084f, 0.449611336f, 0.471396744f, 0.492898196f, 0.514102757f, 0.534997642f,
	0.555570245f, 0.575808167f, 0.59569931f, 0.615231574f, 0.634393275f, 0.653172851f, 0.671558976f, 0.689540565f,
	0.707106769f, 0.724247098f, 0.740951121f, 0.757208824f, 0.773010433f, 0.78834641f, 0.803207517f, 0.817584813f,
	0.831469595f, 0.84485358f, 0.857728601f, 0.870086968f, 0.881921291f, 0.893224299f, 0.903989315f, 0.914209783f,
	0.923879504f, 0.932992816f, 0.941544056f, 0.949528158f, 0.956940353f, 0.963776052f, 0.970031261f, 0.975702107f,
	0.980785251f, 0.985277653f, 0.989176512f, 0.992479563f, 0.99518472f, 0.997290432f, 0.99879545f, 0.999698818f,
	1.0f, 0.999698818f, 0.99879545f, 0.997290432f, 0.99518472f, 0.992479563f, 0.989176512f, 0.985277653f,
	0.980785251f, 0.975702107f, 0.970031261f, 0.963776052f, 0.956940353f, 0.949528158f, 0.941544056f, 0.932992816f,
	0.923879504f, 0.914209783f, 0.903989315f, 0.893224299f, 0.881921291f, 0.870086968f, 0.857728601f, 0.84485358f,
	0.831469595f, 0.817584813f, 0.803207517f, 0.78834641f, 0.773010433f, 0.757208824f, 0.740951121f, 0.724247098f,
	0.707106769f, 0.689540565f, 0.671558976f, 0.653172851f, 0.634393275f, 0.615231574f, 0.59569931f, 0.575808167f,
	0.555570245f, 0.534997642f, 0.514102757f, 0.492898196f, 0.471396744f, 0.449611336f, 0.427555084f, 0.405241311f,
	0.382683426f, 0.359895051f, 0.336889863f, 0.313681751f, 0.290284663f, 0.266712755f, 0.242980182f, 0.219101235f,
	0.195090324f, 0.170961887f, 0.146730468f, 0.122410677f, 0.0980171412f, 0.0735645667f, 0.0490676723f, 0.024541229f,
	0.0f, -0.0245412271f, -0.0490676723f, -0.0735645667f, -0.0980171412f, -0.122410677f, -0.146730468f, -0.170961887f,
	-0.195090324f, -0.219101235f, -0.242980182f, -0.266712755f, -0.290284663f, -0.313681751f, -0.336889863f, -0.359895051f,
	-0.382683426f, -0.405241311f, -0.427555084f, -0.449611336f, -0.471396744f, -0.492898196f, -0.514102757f, -0.534997642f,
	-0.555570245f, -0.575808167f, -0.59569931f, -0.615231574f, -0.634393275f, -0.653172851f, -0.671558976f, -0.689540565f,
	-0.707106769f, -0.724247098f, -0.740951121f, -0.757208824f, -0.773010433f, -0.78834641f, -0.803207517f, -0.817584813f,
	-0.831469595f, -0.84485358f, -0.857728601f, -0.870086968f, -0.881921291f, -0.893224299f, -0.903989315f, -0.914209783f,
	-0.923879504f, -0.932992816f, -0.941544056f, -0.949528158f, -0.956940353f, -0.963776052f, -0.970031261f, -0.975702107f,
	-0.980785251f, -0.985277653f, -0.989176512f, -0.992479563f, -0.99518472f, -0.997290432f, -0.99879545f, -0.999698818f,
	-1.0f, -0.999698818f, -0.99879545f, -0.997290432f, -0.99518472f, -0.992479563f, -0.989176512f, -0.985277653f,
	-0.980785251f, -0.975702107f, -0.970031261f, -0.963776052f, -0.956940353f, -0.949528158f, -0.941544056f, -0.932992816f,
	-0.923879504f, -0.914209783f, -0.903989315f, -0.893224299f, -0.881921291f, -0.870086968f, -0.857728601f, -0.84485358f,
	-0.831469595f, -0.817584813f, -0.803207517f, -0.78834641f, -0.773010433f, -0.757208824f, -0.740951121f, -0.724247098f,
	-0.707106769f, -0.689540565f, -0.671558976f, -0.653172851f, -0.634393275f, -0.615231574f, -0.59569931f, -0.575808167f,
	-0.555570245f, -0.534997642f, -0.514102757f, -0.492898196f, -0.471396744f, -0.449611336f, -0.427555084f, -0.405241311f,
	-0.382683426f, -0.359895051f, -0.336889863f, -0.313681751f, -0.290284663f, -0.266712755f, -0.242980182f, -0.219101235f,
	-0.195090324f, -0.170961887f, -0.146730468f, -0.122410677f, -0.0980171412f, -0.0735645667f, -0.0490676761f, -0.024541229f,
};

// FUNCTION: ALIEN 0x40c930
unsigned char* GRAPH::SetWind(int p_force, ANGLE p_direction)
{
	m_windForce = p_force * 0.001f;
	unsigned char* result = &m_windDirection;
	if (result != (unsigned char*) &p_direction)
		*result = p_direction.m_dir;
	return result;
}

static inline int DisplayFormatBits(int p_format)
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
	case 0x33545844 /* 'DXT3' */ :
	case 0x35545844 /* 'DXT5' */ :
		return 8;
	case 0x31545844 /* 'DXT1' */ :
		return 4;
	}
}

static inline unsigned int DisplayColor32Flag(int p_flags)
{
	return (p_flags >> 1) & 1;
}

static inline void LogViewPort(const char* p_format, float p_xMin, float p_yMin, float p_xMax, float p_yMax)
{
	MYERROR::Log(::Error, p_format, p_xMin, p_yMin, p_xMax, p_yMax);
}

static inline STRING FormatPixelShader(const char* p_format, int p_version)
{
	return Printf(p_format, p_version);
}

// STUB: ALIEN 0x42f4b0
int GRAPH::Init(void* p_hWnd)
{
	GRAPH_CORE* core = (GRAPH_CORE*) this;
	Registry->SetInt(STRING("ScreenX"), (int) core->m_width);
	Registry->SetInt(STRING("ScreenY"), (int) core->m_height);
	Registry->SetInt(STRING("BPP"), 8 * ((core->m_flags & 2) ? 4 : 2));
	Registry->SetInt(STRING("Device"), core->m_curAdapter);
	Registry->SetInt(STRING("FullScreen"), (core->m_flags & 0x80) != 0);
	core->m_flags = (core->m_flags & 0xfffffdff)
		| ((Registry->GetInt(
				// STRING: ALIEN 0x483f10
				STRING("LowDetail"), 0)
			   & 1)
			<< 9);

	core->m_flags &= ~0x20;
	core->m_flags = (core->m_flags & 0xffffffef) | (8 * (core->m_adapters[core->m_curAdapter].m_caps & 2));
	if (core->m_adapters[core->m_curAdapter].m_caps & 2)
		MYERROR::Log(::Error,
			// STRING: ALIEN 0x483f04
			"zm_nongdi");
	core->m_flags &= ~0x10;

	core->m_flags = (core->m_flags & 0xffffffbf)
		| (((Registry->GetInt(
				 // STRING: ALIEN 0x483ef4
				 STRING("TripleBuffer"), 1)
				&& (core->m_flags & 0x80) && !((core->m_flags >> 10) & 1))
			   & 1)
			<< 6);
	if (!(core->m_adapters[core->m_curAdapter].m_caps & 1))
		core->m_flags |= 0x80;

	if (!(core->m_flags & 0x80) && (float) GetSystemMetrics(SM_CXSCREEN) < core->m_width)
		core->m_width = (float) GetSystemMetrics(SM_CXSCREEN);
	if (!(core->m_flags & 0x80) && (float) GetSystemMetrics(SM_CYSCREEN) < core->m_height)
		core->m_height = (float) GetSystemMetrics(SM_CYSCREEN);

	if (!(core->m_flags & 0x80)) {

		if (DisplayColor32Flag(core->m_flags)
			!= (unsigned int) (DisplayFormatBits(
								   core->m_adapters[core->m_curAdapter].m_desktopFmt)
				   == 32))
			core->m_flags = (core->m_flags & 0xfffffffd)
				| (((DisplayFormatBits(core->m_adapters[core->m_curAdapter].m_desktopFmt)
						== 32)
					   & 1)
					<< 1);
	}

	if ((float) (core->m_adapters[core->m_curAdapter].m_vidMemory / 2)
		<= (core->m_width * core->m_height + core->m_width * core->m_height) * 3.0f)
		core->m_flags &= ~0x40;
	core->m_flags &= ~0x40;

	core->m_hwnd = p_hWnd;
	RECT winRect;
	GetWindowRect((HWND) p_hWnd, &winRect);
	RECT clientRect;
	GetClientRect((HWND) core->m_hwnd, &clientRect);
	ClientToScreen((HWND) core->m_hwnd, (POINT*) &clientRect);
	ClientToScreen((HWND) core->m_hwnd, (POINT*) &clientRect.right);
	core->m_unk0xe04 = 23;
	int result = core->Init(p_hWnd);
	if (result)
		return result;

	if (!(core->m_flags & 0x80) || (core->m_flags & 0x400)) {
		core->SetViewPort((float) clientRect.left - (float) winRect.left,
			(float) clientRect.top - (float) winRect.top,
			(float) clientRect.right - (float) winRect.left,
			(float) clientRect.bottom - (float) winRect.top);
	}
	else {
		core->SetViewPort(0.0f, 0.0f, core->m_width, core->m_height);
	}
	LogViewPort(
		// STRING: ALIEN 0x483ecc
		"SetViewPort (%.0f,%.0f) - (%.0f,%.0f)", core->m_viewXMin, core->m_viewYMin, core->m_viewXMax,
		core->m_viewYMax);
	MYERROR::Log(::Error, GRAPH_CORE::TexStageStateToStr());

	core->m_device->SetTextureStageState(0, D3DTSS_COLORARG1, 2);
	core->m_device->SetTextureStageState(0, D3DTSS_COLORARG2, 0);
	core->m_device->SetTextureStageState(0, D3DTSS_COLOROP, 4);
	core->m_device->SetTextureStageState(0, D3DTSS_ALPHAARG1, 2);
	core->m_device->SetTextureStageState(0, D3DTSS_ALPHAARG2, 0);
	core->m_device->SetTextureStageState(0, D3DTSS_ALPHAOP, 4);
	core->m_device->SetTextureStageState(0, D3DTSS_MINFILTER, 1);
	core->m_device->SetTextureStageState(0, D3DTSS_MAGFILTER, 1);
	core->m_device->SetRenderState(D3DRS_SPECULARENABLE, 0);
	core->m_flags &= ~0x2000;
	core->m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
	core->m_flags &= ~0x4000;
	core->SetRenderState(D3DRS_DITHERENABLE, 1);
	core->SetRenderState(D3DRS_LOCALVIEWER, 0);
	core->SetRenderState(D3DRS_LIGHTING, 0);
	core->SetRenderState(D3DRS_ALPHATESTENABLE, 0);
	core->m_device->SetRenderState(D3DRS_ZFUNC, 8);
	core->m_device->SetRenderState(D3DRS_ZWRITEENABLE, 0);
	core->SetRenderState(D3DRS_ZENABLE, 1);
	core->SetRenderState(D3DRS_ZFUNC, 7);
	MYERROR::Log(::Error, GRAPH_CORE::TexStageStateToStr());
	GRAPH_CORE::GetTextureCaps(core->m_device);

	core->m_texE0C = new TEXTURE(256, 256, D3DFMT_P8, 0);
	if (!core->m_texE0C->m_texture && !core->m_texE0C->m_data) {
		if (::Error)
			MYERROR::Error(::Error, "GRAPH", 3,
				// STRING: ALIEN 0x483ebc
				"light buffer", 0);
		return 1;
	}
	if (core->m_texE0C->m_format == D3DFMT_P8) {
		unsigned int palette[256];
		for (int i = 0; i < 256; ++i)
			palette[i] = COLOR(i, i, i).m_value;
		core->m_texE0C->SetPalette(palette);
	}
	core->m_texE10 = new TEXTURE(256, 256, D3DFMT_R5G6B5, 0);
	if (!core->m_texE10->m_texture && !core->m_texE10->m_data) {
		if (::Error)
			MYERROR::Error(::Error, "GRAPH", 3,
				// STRING: ALIEN 0x483eb0
				"hiBuffer", 0);
		return 1;
	}
	core->m_texE14 = new TEXTURE(256, 256, D3DFMT_A4R4G4B4, 0);
	if (!core->m_texE14->m_texture && !core->m_texE14->m_data) {
		if (::Error)
			MYERROR::Error(::Error, "GRAPH", 3,
				// STRING: ALIEN 0x483ea4
				"alphaBuffer", 0);
		return 1;
	}

	((PLANE*) this)->PLANE::CheckFlightProperties();
	core->ClearScreen(COLOR((int) 0xff000000));
	((PLANE*) this)->PLANE::CheckFlightProperties();
	core->CreateFont(
		// STRING: ALIEN 0x483e9c
		STRING("Courier"), 7, 8);

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
	if (core->m_flags & 4)
		capsStr +=
			// STRING: ALIEN 0x483e8c
			"ALPHAPALETTE ";
	if (core->m_flags & 0x10)
		capsStr +=
			// STRING: ALIEN 0x483e84
			"NONGDI ";
	if (core->m_flags & 0x20)
		capsStr +=
			// STRING: ALIEN 0x483e78
			"SOFTWARE ";
	else
		capsStr +=
			// STRING: ALIEN 0x483e6c
			"HARDWARE ";
	if (core->m_flags & 0x200)
		capsStr +=
			// STRING: ALIEN 0x483e60
			"LOWDETAIL ";
	if (core->m_flags & 8)
		capsStr +=
			// STRING: ALIEN 0x483e58
			"AGP ";
	if (!(core->m_flags & 0x80))
		capsStr +=
			// STRING: ALIEN 0x483e4c
			"WINDOWED ";
	if (core->m_flags & 0x40)
		capsStr +=
			// STRING: ALIEN 0x483e3c
			"TRIPLEBUFFER ";
	else
		capsStr +=
			// STRING: ALIEN 0x483e2c
			"DOUBLEBUFFER ";
	if (core->m_flags & 0x800)
		capsStr +=
			// STRING: ALIEN 0x483e1c
			"DOTPRODUCT3 ";
	if (!(core->m_flags & 0x1000))
		capsStr +=
			// STRING: ALIEN 0x483e0c
			"NOTMODULATE2X ";
	if (!(core->m_flags & 0x8000))
		capsStr +=
			// STRING: ALIEN 0x483dfc
			"CAN'T_Z_BLT ";
	if (core->m_flags & 2)
		capsStr +=
			// STRING: ALIEN 0x483df0
			"COLOR32 ";
	if (core->m_flags & 0x100)
		capsStr +=
			// STRING: ALIEN 0x483de8
			"VSYNC ";
	capsStr += FormatPixelShader(
		// STRING: ALIEN 0x483dd8
		"PIXELSHADER=%i", core->m_pixelShaderVersion);
	MYERROR::Log(::Error,
		// STRING: ALIEN 0x483dd0
		"caps=%s", (const char*) capsStr);

	if (!core->m_locked) {
		D3DLOCKED_RECT lockedRect;
		if (core->m_backBuffer->LockRect(&lockedRect, 0, 0) < 0 && ::Error)
			MYERROR::Error(::Error, "GRAPH", 0,
				// STRING: ALIEN 0x482c1c
				"backBuffer", 0);
		core->m_locked = (int) lockedRect.pBits;
		core->m_unk0x248 = lockedRect.Pitch / ((core->m_flags & 2) ? 4 : 2);
	}
	if (core->m_locked) {
		core->m_backBuffer->UnlockRect();
		core->m_locked = 0;
	}
	MYERROR::Log(::Error,
		// STRING: ALIEN 0x483dbc
		"Pitch=%i zPitch=%i", core->m_unk0x248 * ((core->m_flags & 2) ? 4 : 2), 2 * core->m_unk0x250);
	core->ReloadPalettes();
	return 0;
}

// FUNCTION: ALIEN 0x431c00
unsigned int GRAPH::GetEffectState(int p_effect) const
{
	if (p_effect > 0 && p_effect < 16 && m_effectStart[p_effect])
		return (100 * RealCurrentTime - 100 * m_effectStart[p_effect]) / m_effectDuration[p_effect];
	return -1;
}

// FUNCTION: ALIEN 0x431cf0
int GRAPH::SetEnvironment(int p_env)
{
	if (!(p_env & 0x80000000)) {
		if (p_env == 1 || p_env == 2)
			m_env &= 0xfffffffc;
		if (p_env & 0x0c00)
			m_env &= 0xfffff3ff;
		if (p_env & 0xc000)
			m_env &= 0xffff3fff;
		m_env |= p_env;
		return p_env;
	}
	m_env &= ~p_env;
	return ~p_env;
}

// GLOBAL: ALIEN 0x4b2c8c
static int g_lightBufferToggle;

static inline bool DrawLightInViewPort(const GRAPH_CORE* p_graph, float p_x, float p_y)
{
	return p_x >= p_graph->m_viewXMin && p_x < p_graph->m_viewXMax
		&& p_y >= p_graph->m_viewYMin && p_y < p_graph->m_viewYMax;
}

// FUNCTION: ALIEN 0x433670
void GRAPH::DrawLight(float p_x, float p_y, float p_z, int p_a, int p_b, unsigned int p_color)
{
	GRAPH_CORE* core = (GRAPH_CORE*) this;
	float lw = *(float*) &p_a;
	float lh = *(float*) &p_b;
	g_lightBufferToggle ^= 1;
	if (!(p_color & 0xffffff))
		return;
	int hw = (3 * ((int) lw / 2)) & ~3;
	int hh = (3 * ((int) lh / 2)) & ~3;
	if (hw > 512)
		hw = 512;
	if (hh > 512)
		hh = 512;
	int falloffDiv = (int) lw * (int) lh / 500;
	int zHeight = (int) p_z;
	int zCenter = 2 * zHeight / 3 + 8;
	p_y = p_y - ((float) zCenter - p_z);
	if ((float) hw + p_x < core->m_viewXMin)
		return;
	if (p_x - (float) hw >= core->m_viewXMax)
		return;
	if ((float) hh + p_y < core->m_viewYMin)
		return;
	if (p_y - (float) hh >= core->m_viewYMax)
		return;

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

	TEXTURE* tex = g_lightBufferToggle ? core->m_texE10 : core->m_texE0C;
	char* pix = (char*) tex->Lock((int*) &p_z, &srcRect);
	if (!pix) {
		if (::Error)
			MYERROR::Error(::Error,
				// STRING: ALIEN 0x47f240
				"GRAPH", 10,
				"light buffer", 0);
		return;
	}
	if (tex->m_format != 41)
		*(int*) &p_z /= 2;

	for (int row = -hh; row < hh; row += 4) {
		for (int col = -hw; col < hw; col += 4) {
			*(int*) &p_a = col + hw;
			float sy = (float) row + p_y;
			float sy3 = sy + 3.0f;
			float sx = (float) col + p_x;
			int za;
			if (!DrawLightInViewPort(core, (float) col + p_x, sy))
				za = 0x7fff;
			else
				za = (((unsigned short*) core->m_zbuffer)[
					((int) p_y + row) * core->m_unk0x250 + col + (int) p_x] >> 3) - 128;
			int zb;
			if (!DrawLightInViewPort(core, (float) col + p_x + 3.0f,
				(float) row + p_y + 3.0f))
				zb = 0x7fff;
			else
				zb = (((unsigned short*) core->m_zbuffer)[
					((int) p_y + row + 3) * core->m_unk0x250 + col + (int) p_x + 3] >> 3) - 128;
			if (zb < za)
				za = zb;
			if (za == 0x7fff)
				continue;
			int d = row + za - zCenter;
			int zDiff = za - zHeight;
			d = 9 * d * d / 4;
			d += zDiff * zDiff / 4;
			d += col * col;
			int intensity = falloffDiv ? 256 - d / falloffDiv : 256 - d;
			if (intensity < 0)
				intensity = 0;
			else if (intensity > 255)
				intensity = 255;
			*(int*) &p_b = row + hh;
			if (tex->m_format != 41)
				((unsigned short*) pix)[*(int*) &p_z * (*(int*) &p_b / 4)
					+ *(int*) &p_a / 4] =
					core->m_snowRamp[intensity];
			else
				((unsigned char*) pix)[*(int*) &p_z * (*(int*) &p_b / 4)
					+ *(int*) &p_a / 4] =
					(unsigned char) intensity;
		}
	}

	if (tex->m_texture)
		tex->m_texture->UnlockRect(0);
	core->SetRenderState(D3DRS_SPECULARENABLE, 0);
	core->SetAlphaBlend(9, 2);
	srcRect.left++;
	srcRect.right--;
	srcRect.top++;
	srcRect.bottom--;
	tex->Draw_z(z1, *(int*) &z1, dstRect, (int*) &srcRect,
		&GAMMA(*(COLOR*) &p_color, COLOR((int) 0xff000000)));
}

// FUNCTION: ALIEN 0x433bb0
unsigned char* GRAPH::OldLoadParameters(STREAM* p_stream)
{
	STREAM* stream = p_stream;
	stream->Read(&m_env, 4);
	unsigned int packed;
	stream->Read(&packed, 4);
	GAMMA gamma(GAMMA::DECODE, packed);
	SetGamma(gamma);
	unsigned char dir;
	stream->Read(&dir, 1);
	short force;
	stream->Read(&force, 2);
	return SetWind(force, dir);
}

// FUNCTION: ALIEN 0x433cd0
int GRAPH::LoadParameters(STREAM* p_stream)
{
	int gamma[2] = { 0, 0 };
	p_stream->Read(&m_env, 4);
	p_stream->Read(gamma, 8);
	SetGamma(*(GAMMA*) gamma);
	int wd;
	p_stream->Read(&wd, 4);
	m_windDirection = (unsigned char) wd;
	return p_stream->Read(&m_windForce, 4);
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
	return m_movie.Play(p_filename, (int) m_width / 2, (int) m_height / 2);
}

// FUNCTION: ALIEN 0x442600
unsigned char* GRAPH::WindDirection(unsigned char* p_out)
{
	*p_out = m_windDirection;
	return p_out;
}
