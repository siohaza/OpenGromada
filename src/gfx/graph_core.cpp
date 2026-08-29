#define DECOMP_INLINE_STRING_COPY_LIFETIME
#define DECOMP_GAMMA_DEFAULT_CTOR_ZERO

#include "gfx/graph.h"
#include "gfx/graph_core.h"

#include "ui/dlgitem.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "game/gametime.h"
#include "game/map.h"
#include "game/settings.h"
#include "gfx/texture.h"
#include "util/myerror.h"

extern int g_textureSquare;
extern int g_textureDefaultPool;
extern int g_texturePalette;
extern int g_textureDxt;
extern int g_textureAlphaPalette;
extern int g_textureCondNonPow2;
extern int g_texturePowerOfTwo;
extern int g_textureMaxWidth;
extern int g_textureMaxHeight;

struct GRAPH_CORE_FLAGS {
	unsigned int m_unk0 : 1;
	unsigned int m_bpp32 : 1;
	unsigned int m_unk2 : 5;
	unsigned int m_fullscreen : 1;
	unsigned int m_unk8 : 24;
};

// GLOBAL: ALIEN 0x4b2828
COLOR GRAPH_CORE::GREEN(0, 255, 0);

// GLOBAL: ALIEN 0x4b2854
char g_texStageStr[0x400];

// FUNCTION: ALIEN 0x4018f0
GRAPH_CORE::GRAPH_CORE(SETTINGS* p_settings)
{
	m_flags = (m_flags & 0xfffffbff) | ((p_settings->m_flag & 1) << 10);
	m_flags = (m_flags & 0xfffffefe) | ((p_settings->m_flag & 2) << 7);
	m_d3d = 0;
	m_device = 0;
	m_font = 0;
	m_texE0C = 0;
	m_texE10 = 0;
	m_texE14 = 0;
	m_screenSurf = 0;
	m_backBuffer = 0;
	m_zbuffer = 0;
	m_locked = 0;
	m_flags &= 0xfffeffff;
	m_env = 0;
	AngleAssign((ANGLE*) &m_windDirection, ANGLE(0xdc));
	m_windForce = 20.0f;
	m_unk0xe04 = 0;
	Effect(0, 0, 0, 0);

	m_d3d = Direct3DCreate8(D3D_SDK_VERSION);
	if (!m_d3d) {
		MYERROR::Window(::Error,
			// STRING: ALIEN 0x47f180
			"Can't create Direct3D8");
		return;
	}
	m_noAdapter = 0;
	while (m_noAdapter < (int) m_d3d->GetAdapterCount()) {
		m_adapters[m_noAdapter].EnumDisplayModes(m_noAdapter, m_d3d, p_settings);
		++m_noAdapter;
	}
	m_curAdapter = 0;
	m_width = (float) (unsigned int) p_settings->m_screenX;
	m_height = (float) (unsigned int) p_settings->m_screenY;
	((GRAPH_CORE_FLAGS*) &m_flags)->m_bpp32 = p_settings->m_screenBpp == 32;
	((GRAPH_CORE_FLAGS*) &m_flags)->m_fullscreen = p_settings->m_fullscreen != 0;
	if (m_noAdapter <= 0)
		m_curAdapter = 0;
}

// FUNCTION: ALIEN 0x401b10
GRAPH_CORE::~GRAPH_CORE()
{
	m_movie.Stop();
	if (m_locked) {
		m_backBuffer->UnlockRect();
		m_locked = 0;
	}
	if (m_device)
		m_device->SetTexture(0, 0);
	if (m_font) {
		delete m_font;
		m_font = 0;
	}
	if (m_zbuffer) {
		operator delete(m_zbuffer);
		m_zbuffer = 0;
	}
	if (m_screenSurf) {
		m_screenSurf->Release();
		m_screenSurf = 0;
	}
	if (m_texE14) {
		delete m_texE14;
		m_texE14 = 0;
	}
	if (m_texE0C) {
		delete m_texE0C;
		m_texE0C = 0;
	}
	if (m_texE10) {
		delete m_texE10;
		m_texE10 = 0;
	}
	if (m_device) {
		int released = m_device->Release();
		m_device = 0;
		MYERROR::Log(::Error,
					 // STRING: ALIEN 0x47f1c0
					 "d3dDevice release %i", released);
	}
	if (m_backBuffer) {
		int released = m_backBuffer->Release();
		m_backBuffer = 0;
		MYERROR::LogStatus(::Error, "backBuffer release %i", released);
	}
	if (m_d3d) {
		int released = m_d3d->Release();
		m_d3d = 0;
		MYERROR::Log(::Error,
					 // STRING: ALIEN 0x47f198
					 "d3d release %i", released);
	}
	m_movie.Stop();
}

// FUNCTION: ALIEN 0x401c80
int GRAPH_CORE::Init(void* p_hWnd)
{
	memset(&m_presentParams, 0, sizeof(m_presentParams));
	m_presentParams.Windowed = (~m_flags >> 7) & 1;
	m_presentParams.BackBufferCount = 1;
	m_presentParams.SwapEffect = (m_flags & 0x100) ? D3DSWAPEFFECT_COPY_VSYNC : D3DSWAPEFFECT_COPY;
	m_presentParams.EnableAutoDepthStencil = 0;
	m_presentParams.hDeviceWindow = (HWND) m_hwnd;
	m_presentParams.MultiSampleType = D3DMULTISAMPLE_NONE;
	m_presentParams.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

	int format;
	if (m_flags & 0x80) {
		int mode = m_adapters[m_curAdapter].FindDisplayMode((int) m_width, (int) m_height,
			(m_flags & 2) ? 32 : 16);
		if (mode < 0) {
			MYERROR::Log(::Error,
				// STRING: ALIEN 0x47f260
				"Selected display mode %.0fx%.0f", m_width, m_height);
			if (::Error)
				MYERROR::Error(::Error, "GRAPH", 10,
					// STRING: ALIEN 0x47f248
					"Unsupported resolution", 0);
			return 1;
		}
		format = m_adapters[m_curAdapter].m_modeFmt[mode];
	}
	else {
		format = m_adapters[m_curAdapter].m_desktopFmt;
	}
	m_unk0xe08 = format;
	m_presentParams.BackBufferFormat = (D3DFORMAT) format;
	m_presentParams.BackBufferWidth = (int) m_width;
	m_presentParams.BackBufferHeight = (int) m_height;

	MYERROR::Log(::Error,
		// STRING: ALIEN 0x47f204
		"Selected display mode %.0fx%.0f %s desktop %s zbuffer %s", m_width, m_height,
		GetPixelFormat((D3DFORMAT) m_unk0xe08).m_str,
		GetPixelFormat((D3DFORMAT) m_adapters[m_curAdapter].m_desktopFmt).m_str,
		GetPixelFormat(m_presentParams.AutoDepthStencilFormat).m_str);

	int result = m_d3d->CreateDevice(m_curAdapter, (m_flags & 0x20) ? D3DDEVTYPE_REF : D3DDEVTYPE_HAL,
		(HWND) m_hwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &m_presentParams, &m_device);
	if (result) {
		result = m_d3d->CreateDevice(m_curAdapter, (m_flags & 0x20) ? D3DDEVTYPE_REF : D3DDEVTYPE_HAL,
			(HWND) m_hwnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &m_presentParams, &m_device);
		if (result) {
			if (::Error)
				MYERROR::Error(::Error, "GRAPH", 3,
					// STRING: ALIEN 0x47f1f8
					"3dDevice", result);
			return 1;
		}
	}

	result = m_device->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &m_backBuffer);
	if (result) {
		if (::Error)
			MYERROR::Error(::Error, "GRAPH", 9,
				// STRING: ALIEN 0x47f1ec
				"BackBuffer", result);
		return 1;
	}
	m_device->SetRenderState(D3DRS_ZWRITEENABLE, 0);
	m_device->SetRenderState(D3DRS_ZENABLE, 0);

	D3DCAPS8 caps;
	result = m_device->GetDeviceCaps(&caps);
	if (result) {
		if (::Error)
			MYERROR::Error(::Error, "GRAPH", 9,
				// STRING: ALIEN 0x47f1e4
				"Caps", result);
		return 1;
	}
	m_flags = (m_flags & 0xffffe7f3) | ((caps.TextureOpCaps & 0x10) << 8)
			| (((caps.TextureCaps & 0x80)
				   | (((caps.DevCaps & 0x1000) | ((caps.TextureOpCaps >> 3) & 0x100000)) >> 4))
				>> 5);
	m_pixelShaderVersion = caps.PixelShaderVersion;

	result = m_device->CreateImageSurface((int) m_width, (int) m_height, (D3DFORMAT) m_unk0xe08,
		&m_screenSurf);
	if (result) {
		if (::Error)
			MYERROR::Error(::Error, "GRAPH", 3,
				// STRING: ALIEN 0x47f1d8
				"tempBuffer", result);
		return 1;
	}
	m_zbuffer = operator new(2 * (int) m_width * (int) m_height);
	m_unk0x250 = (int) m_width;
	return 0;
}

// FUNCTION: ALIEN 0x402090
int GRAPH_CORE::PreTact()
{
	if (m_flags & 0x10000)
		return 0;
	int result = m_device->TestCooperativeLevel();
	if (result != 0x88760868) {
		if (result != 0x88760869) {
			if (result)
				return 3;
		}
		else {
			if (m_locked) {
				m_backBuffer->UnlockRect();
				m_locked = 0;
			}
			if (m_font)
				m_font->InvalidateDeviceObjects();
			if (Map)
				Map->ReleaseVidSurfaces();
			if (m_backBuffer) {
				int released = m_backBuffer->Release();
				m_backBuffer = 0;
				MYERROR::LogStatus(::Error,
								   // STRING: ALIEN 0x47f1a8
								   "backBuffer release %i", released);
			}
			result = m_device->Reset(&m_presentParams);
			if (::Error)
				MYERROR::Error(::Error, "GRAPH", 4,
							   // STRING: ALIEN 0x47f2a8
							   "device notreset", result);
			if (result < 0)
				return 2;
			if (m_font)
				m_font->RestoreDeviceObjects();
			if (Map)
				Map->RestoreVidSurfaces();
			result = m_device->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &m_backBuffer);
			if (result) {
				if (::Error)
					MYERROR::Error(::Error, "GRAPH", 9,
								   "BackBuffer", result);
				return 2;
			}
		}
		result = m_device->BeginScene();
		if (result) {
			if (::Error)
				MYERROR::Error(::Error, "GRAPH", 10,
							   // STRING: ALIEN 0x47f28c
							   "3dBeginScene for PreTact", result);
			return 4;
		}
		m_flags |= 0x10000;
		return 0;
	}
	ReloadPalettes();
	if (::Error)
		MYERROR::Error(::Error, "GRAPH", 10,
					   // STRING: ALIEN 0x47f280
					   "device lost", 0);
	return 1;
}

// FUNCTION: ALIEN 0x402250
void GRAPH_CORE::PostTact(int p_present)
{
	if (m_flags & 0x10000) {
		if (m_locked) {
			m_backBuffer->UnlockRect();
			m_locked = 0;
		}
		int result = m_device->EndScene();
		if (result) {
			if (::Error)
				MYERROR::Error(::Error, "GRAPH", 10,
							   // STRING: ALIEN 0x47f2c0
							   "3dEndScene for PostTact", result);
		}
		if (p_present && !m_effectStart[6] && !m_effectStart[7]) {
			RECT dst;
			RECT src;
			src.left = (int) m_viewXMin;
			src.top = (int) m_viewYMin;
			src.right = (int) m_viewXMax;
			src.bottom = (int) m_viewYMax;
			dst.bottom = src.bottom;
			dst.left = src.left;
			dst.top = src.top;
			dst.right = src.right;
			if (!(m_flags & 0x80)) {
				RECT client;
				RECT win;
				GetClientRect((HWND) m_hwnd, &client);
				ClientToScreen((HWND) m_hwnd, (POINT*) &client);
				GetWindowRect((HWND) m_hwnd, &win);
				dst.left += win.left - client.left;
				dst.right += win.left - client.left;
				dst.top += win.top - client.top;
				dst.bottom += win.top - client.top;
			}
			result = m_device->Present(&src, &dst, 0, 0);
			if (result) {
				if (::Error)
					MYERROR::Error(::Error, "GRAPH", 4,
								   // STRING: ALIEN 0x47f2b8
								   "Present", result);
			}
		}
		m_flags &= ~0x10000;
	}
}

// FUNCTION: ALIEN 0x402400
int GRAPH_CORE::SetViewPort(float p_x1, float p_y1, float p_x2, float p_y2)
{
	m_viewXMin = p_x1;
	m_viewXMax = p_x2;
	m_viewYMin = p_y1;
	m_viewYMax = p_y2;
	VID::viewXMin = (int) p_x1;
	VID::viewXMax = (int) p_x2;
	VID::viewYMin = (int) p_y1;
	VID::viewYMax = (int) p_y2;
	int result = VID::viewYMax;
	if (m_device) {
		D3DVIEWPORT8 vp;
		vp.X = (int) m_viewXMin;
		vp.Y = (int) m_viewYMin;
		vp.Width = (int) (m_viewXMax - m_viewXMin);
		vp.Height = (int) (m_viewYMax - m_viewYMin);
		vp.MinZ = 0.0f;
		vp.MaxZ = 1.0f;
		result = m_device->SetViewport(&vp);
		if (result && ::Error)
			MYERROR::Error(::Error, "GRAPH", 8,
				// STRING: ALIEN 0x47f2f0
				"viewport", result);
		D3DMATRIX proj = {
			2.0f / vp.Width, 0.0f, 0.0f, 0.0f,
			0.0f, -2.0f / vp.Height, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f / (vp.MaxZ - vp.MinZ) * 0.001f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		};
		result = m_device->SetTransform(D3DTS_PROJECTION, &proj);
		if (result < 0 && ::Error)
			return (int) MYERROR::Error(::Error, "GRAPH", 8,
				// STRING: ALIEN 0x47f2d8
				"Transform projection", result);
	}
	return result;
}

// FUNCTION: ALIEN 0x402600
void GRAPH_CORE::ClearScreen(COLOR p_color)
{
	if (m_locked) {
		m_backBuffer->UnlockRect();
		m_locked = 0;
	}
	m_device->Clear(0, 0, 1, p_color.m_value, 0, 0);
	p_color.m_value = (int) m_height * m_unk0x250;
	void* dst = m_zbuffer;
	__asm {
		mov eax, 3FFh
		mov edi, dst
		mov ecx, p_color
		cld
		mov bx, ax
		shl eax, 10h
		mov ax, bx
		shr ecx, 1
		jnb skip
		mov word ptr [edi], ax
		add edi, 2
	skip:
		jecxz done
		rep stosd
	done:
	}
}

// FUNCTION: ALIEN 0x402690
void GRAPH_CORE::Effect(int p_effect, int p_a, int p_b, int p_duration)
{
	if (p_effect >= 16 || p_effect < 0)
		return;
	if (p_effect == 3 || p_effect == 9 || p_effect == 10) {
		m_effectStart[10] = 0;
		m_effectStart[9] = 0;
		m_effectStart[3] = 0;
	}
	int duration = p_duration;
	if (duration == 0) {
		switch (p_effect) {
		case 2:
			duration = 0x200;
			break;
		case 1:
		case 3:
			duration = 0x900;
			break;
		case 9:
			duration = 0x500;
			break;
		case 10:
			duration = 0x400;
			break;
		default:
			duration = 0x400;
		}
	}
	m_effectStart[p_effect] = RealCurrentTime;
	m_effectDuration[p_effect] = duration;
	m_effectA[p_effect] = p_a;
	m_effectB[p_effect] = p_b;
	if (p_effect == 0) {
		for (int i = 0; i < 16; i++)
			m_effectStart[i] = 0;
		return;
	}
	if (p_effect == 5) {
		if (m_screenSurf == 0)
			return;
		m_effectStart[10] = 0;
		m_effectStart[9] = 0;
		m_effectStart[3] = 0;
		int result = m_device->CopyRects(m_backBuffer, 0, 0, m_screenSurf, 0);
		if (result) {
			if (::Error)
				MYERROR::Error(::Error, "GRAPH", 1,
							   // STRING: ALIEN 0x47f2fc
							   "for EFF_ALPHAAPPEAR", result);
		}
		return;
	}
	if (p_effect == 2) {
		m_unk0xccc = Map->m_shiftY;
		m_unk0xcd0 = Map->m_shiftY;
		return;
	}
	if (p_effect == 11) {
		*(volatile int*) &m_gammaCur.m_a = m_gammaSet.m_a;
		m_gammaCur.m_b = m_gammaSet.m_b;
	}
}

// STUB: ALIEN 0x402820
int GRAPH_CORE::CopyToZBuffer(int* p_dst, int* p_src, void* p_texture)
{
	RECT dstRect;
	RECT srcRect;
	memcpy(&dstRect, p_dst, sizeof(dstRect));
	memcpy(&srcRect, p_src, sizeof(srcRect));

	if ((float) dstRect.right < m_viewXMin || (float) dstRect.left >= m_viewXMax
		|| (float) dstRect.bottom < m_viewYMin || (float) dstRect.top >= m_viewYMax)
		return 0;
	if ((float) dstRect.left < m_viewXMin) {
		srcRect.left += (int) m_viewXMin - dstRect.left;
		dstRect.left = (int) m_viewXMin;
	}
	if ((float) dstRect.top < m_viewYMin) {
		srcRect.top += (int) m_viewYMin - dstRect.top;
		dstRect.top = (int) m_viewYMin;
	}
	if ((float) dstRect.right > m_viewXMax) {
		srcRect.right += (int) m_viewXMax - dstRect.right;
		dstRect.right = (int) m_viewXMax;
	}
	if ((float) dstRect.bottom > m_viewYMax) {
		srcRect.bottom += (int) m_viewYMax - dstRect.bottom;
		dstRect.bottom = (int) m_viewYMax;
	}

	unsigned short* dst = (unsigned short*) m_zbuffer + dstRect.left + dstRect.top * m_unk0x250;
	unsigned short* end = (unsigned short*) ((char*) m_zbuffer
		+ m_unk0x250 * (2 * dstRect.bottom - 2) + 2 * dstRect.right);
	int pitch;
	unsigned short* src = (unsigned short*) ((TEXTURE*) p_texture)->Lock(&pitch, &srcRect);
	pitch /= 2;
	while (dst < end) {
		for (int x = dstRect.left; x < dstRect.right; ++x)
			*dst++ = *src++;
		dst += m_unk0x250 + dstRect.left - dstRect.right;
		src += dstRect.left + pitch - dstRect.right;
	}
	return 0;
}

// FUNCTION: ALIEN 0x402a20
int GRAPH_CORE::DrawPrimitive(int p_type, unsigned int p_fvf, void* p_verts, unsigned int p_stride, int p_count)
{
	if (p_type == 5 || p_type == 6)
		p_count -= 2;
	else if (p_type == 2)
		p_count /= 2;
	else if (p_type == 4)
		p_count /= 3;
	m_device->SetVertexShader(p_fvf);
	int result =
		m_device->DrawPrimitiveUP((D3DPRIMITIVETYPE) p_type, p_count, p_verts, p_stride);
	if (result) {
		if (::Error)
			return (int) MYERROR::Error(::Error, "GRAPH", 10,
										// STRING: ALIEN 0x47f310
										"DrawPrimitiveUP", result);
	}
	return result;
}

// FUNCTION: ALIEN 0x402ad0
void GRAPH_CORE::FlipToGDI()
{
	if (m_flags & 0x80) {
		PostTact(1);
		HDC dc = GetDC(*(HWND*) ((char*) Map + 0x234));
		if (!dc && ::Error)
			MYERROR::Error(::Error, "GRAPH", 9,
						   // STRING: ALIEN 0x47f334
						   "DC in FlipToGDI", 0);
		if (SetPixel(dc, 0, 0, 0xffffff) == -1 && ::Error)
			MYERROR::Error(::Error, "GRAPH", 8,
						   // STRING: ALIEN 0x47f320
						   "pixel in FlipToGDI", 0);
		Sleep(0x96);
		for (int i = 0; GetPixel(dc, 0, 0) == 0xffffff; ++i) {
			if (i >= 8)
				break;
			PreTact();
			PostTact(1);
			Sleep(0x96);
		}
		ReleaseDC(*(HWND*) ((char*) Map + 0x234), dc);
	}
}

static inline const char* CharsOf(const STRING& p_str)
{
	return p_str.m_str;
}

// FUNCTION: ALIEN 0x402bc0
int GRAPH_CORE::CreateFont(const STRING& p_name, int p_height, int p_flags)
{
	if (m_font)
		delete m_font;
	m_font = new CD3DFont(CharsOf(p_name), p_height, p_flags, 8);
	m_font->InitDeviceObjects(m_device);
	return m_font->RestoreDeviceObjects();
}

// FUNCTION: ALIEN 0x402c40
void GRAPH_CORE::PutsXY(float p_x, float p_y, char* p_text, COLOR p_color)
{
	if (m_font) {
		if (m_locked) {
			m_backBuffer->UnlockRect();
			m_locked = 0;
		}
		m_font->DrawText(p_x, p_y, p_color.m_value, p_text, 0);
	}
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

// FUNCTION: ALIEN 0x402ca0
void ADAPTER::EnumDisplayModes(unsigned int p_adapter, IDirect3D8* p_d3d, SETTINGS* p_settings)
{
	D3DADAPTER_IDENTIFIER8 ident;
	p_d3d->GetAdapterIdentifier(p_adapter, 0, &ident);
	D3DDISPLAYMODE mode;
	p_d3d->GetAdapterDisplayMode(p_adapter, &mode);
	strncpy(m_name, ident.Description, 0x28);
	m_desktopFmt = mode.Format;
	m_noModes = 0;
	for (int i = p_d3d->GetAdapterModeCount(p_adapter) - 1; i >= 0; --i) {
		p_d3d->EnumAdapterModes(p_adapter, i, &mode);
		if (mode.Format == D3DFMT_X8R8G8B8 || mode.Format == D3DFMT_A8R8G8B8) {
			if (!p_settings->CheckMode(mode.Width, mode.Height, 32))
				continue;
		} else if (mode.Format == D3DFMT_X1R5G5B5 || mode.Format == D3DFMT_R5G6B5) {
			if (!p_settings->CheckMode(mode.Width, mode.Height, 16))
				continue;
		} else {
			continue;
		}
		if (FindDisplayMode(mode.Width, mode.Height, DisplayFormatBits(mode.Format)) >= 0)
			continue;
		m_modeW[m_noModes] = mode.Width;
		m_modeH[m_noModes] = mode.Height;
		if (!p_d3d->CheckDepthStencilMatch(p_adapter, D3DDEVTYPE_HAL, mode.Format, mode.Format, D3DFMT_D16))
			m_modeZFmt[m_noModes] = D3DFMT_D16;
		else if (!p_d3d->CheckDepthStencilMatch(p_adapter, D3DDEVTYPE_HAL, mode.Format, mode.Format, D3DFMT_D32))
			m_modeZFmt[m_noModes] = D3DFMT_D32;
		else if (!p_d3d->CheckDepthStencilMatch(p_adapter, D3DDEVTYPE_HAL, mode.Format, mode.Format,
					 D3DFMT_D24X8))
			m_modeZFmt[m_noModes] = D3DFMT_D24X8;
		else if (!p_d3d->CheckDepthStencilMatch(p_adapter, D3DDEVTYPE_HAL, mode.Format, mode.Format,
					 D3DFMT_D24S8))
			m_modeZFmt[m_noModes] = D3DFMT_D24S8;
		else
			m_modeZFmt[m_noModes] = 0;
		m_modeFmt[m_noModes] = mode.Format;
		++m_noModes;
		MYERROR::Log(::Error,
			// STRING: ALIEN 0x47f344
			"   Enum display modes %ix%i %s %s", mode.Width, mode.Height,
			CharsOf(GetPixelFormat(mode.Format)),
			CharsOf(GetPixelFormat((D3DFORMAT) m_modeZFmt[m_noModes - 1])));
	}
	D3DCAPS8 caps;
	p_d3d->GetDeviceCaps(p_adapter, D3DDEVTYPE_HAL, &caps);
	m_caps = (m_caps & 0xfffffff0) | (4 * (caps.Caps2 & 2)) | ((caps.Caps2 >> 19) & 1);
	m_vidMemory = 8000000;
}

// FUNCTION: ALIEN 0x402fa0
void GRAPH_CORE::GetTextureCaps(IDirect3DDevice8* p_device)
{
	D3DCAPS8 caps;
	if (p_device->GetDeviceCaps(&caps) < 0 && ::Error) {
		MYERROR::Error(::Error, "TEXTURE", 9,
					   "Caps", 0);
	}
	g_textureMaxWidth = caps.MaxTextureWidth;
	g_textureMaxHeight = caps.MaxTextureHeight;
	g_textureSquare = caps.TextureCaps & 0x20;
	g_textureDefaultPool = 0;
	g_texturePowerOfTwo = caps.TextureCaps & 2;
	g_textureAlphaPalette = caps.TextureCaps & 0x80;
	g_textureCondNonPow2 = caps.TextureCaps & 0x100;
	IDirect3DSurface8* surf;
	if (!p_device->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &surf)) {
		D3DSURFACE_DESC desc;
		surf->GetDesc(&desc);
		IDirect3D8* d3d;
		p_device->GetDirect3D(&d3d);
		if (!d3d->CheckDeviceFormat(caps.AdapterOrdinal, caps.DeviceType, desc.Format, 0, D3DRTYPE_TEXTURE, D3DFMT_P8))
			g_texturePalette = 1;
		else
			g_textureAlphaPalette = 0;
		if (!d3d->CheckDeviceFormat(caps.AdapterOrdinal, caps.DeviceType, desc.Format, 0, D3DRTYPE_TEXTURE,
									(D3DFORMAT) 0x31545844))
			g_textureDxt |= 1;
		if (!d3d->CheckDeviceFormat(caps.AdapterOrdinal, caps.DeviceType, desc.Format, 0, D3DRTYPE_TEXTURE,
									(D3DFORMAT) 0x33545844))
			g_textureDxt |= 4;
		if (!d3d->CheckDeviceFormat(caps.AdapterOrdinal, caps.DeviceType, desc.Format, 0, D3DRTYPE_TEXTURE,
									(D3DFORMAT) 0x35545844))
			g_textureDxt |= 0x10;
		surf->Release();
		d3d->Release();
	}
	g_texturePalette = 0;
	g_textureAlphaPalette = 0;
	STRING text;
	if (g_textureSquare) {
		text +=
			// STRING: ALIEN 0x47f3ec
			"SQUARE ";
	}
	if (g_texturePowerOfTwo) {
		text +=
			// STRING: ALIEN 0x47f3e4
			"POWER2 ";
	}
	else {
		text +=
			// STRING: ALIEN 0x47f3d8
			"NOTPOWER2 ";
	}
	if (g_textureCondNonPow2) {
		text +=
			// STRING: ALIEN 0x47f3c8
			"COND_NON_POW2 ";
	}
	if (g_textureDefaultPool) {
		text +=
			// STRING: ALIEN 0x47f3bc
			"DYNAMIC ";
	}
	if (g_texturePalette) {
		text +=
			// STRING: ALIEN 0x47f3b0
			"PALETTE ";
	}
	if (g_textureAlphaPalette) {
		text +=
			// STRING: ALIEN 0x47f3a0
			"ALPHA_PALETTE ";
	}
	if (g_textureDxt & 1) {
		text +=
			// STRING: ALIEN 0x47f398
			"DXT1 ";
	}
	if (g_textureDxt & 4) {
		text +=
			// STRING: ALIEN 0x47f390
			"DXT3 ";
	}
	if (g_textureDxt & 0x10) {
		text +=
			// STRING: ALIEN 0x47f388
			"DXT5 ";
	}
	text += Printf(
		// STRING: ALIEN 0x47f378
		"MAXSIZE=%i,%i", g_textureMaxWidth, g_textureMaxHeight);
	MYERROR::Log(::Error,
				 // STRING: ALIEN 0x47f368
				 "TextureCaps=%s", text.m_str);
	g_texturePowerOfTwo = 1;
}

// FUNCTION: ALIEN 0x405a80
float GRAPH_CORE::GetWidth()
{
	return m_width;
}

// FUNCTION: ALIEN 0x40b090
float GRAPH_CORE::GetHeight()
{
	return m_height;
}

// GLOBAL: ALIEN 0x483d28
int RGB16_rMask = 0xf800;

// GLOBAL: ALIEN 0x483d2c
int RGB16_gMask = 0x7e0;

extern int RGB16_rShift;
extern int RGB16_gShift;

// FUNCTION: ALIEN 0x41eba0
void GRAPH::PutPixel(float p_x, float p_y, COLOR p_color)
{
	if (p_x >= m_viewXMin && p_x < m_viewXMax && p_y >= m_viewYMin && p_y < m_viewYMax) {
		GRAPH_CORE* g = (GRAPH_CORE*) this;
		if (!g->m_locked) {
			int rect[2];
			if (g->m_backBuffer->LockRect((D3DLOCKED_RECT*) rect, 0, 0) < 0 && ::Error)
				MYERROR::Error(::Error, "GRAPH", 0, "backBuffer", 0);
			g->m_locked = rect[1];
			g->m_unk0x248 = rect[0] / ((g->m_flags & 2) ? 4 : 2);
		}
		if (g->m_flags & 2)
			*(int*) (g->m_locked + 4 * ((int) p_x + (int) p_y * g->m_unk0x248)) =
				p_color.m_value;
		else
			*(short*) (g->m_locked + 2 * ((int) p_x + (int) p_y * g->m_unk0x248)) =
				((p_color.m_value >> 3) & 0x1f)
				| RGB16_rMask & (p_color.m_value >> (16 - RGB16_rShift))
				| RGB16_gMask & (p_color.m_value >> (8 - RGB16_gShift));
	}
}

// FUNCTION: ALIEN 0x41ed10
void GRAPH::PutBigPixel(float p_x, float p_y, COLOR p_color)
{
	GRAPH_CORE* g = (GRAPH_CORE*) this;
	if (!g->m_locked) {
		int rect[2];
		if (g->m_backBuffer->LockRect((D3DLOCKED_RECT*) rect, 0, 0) < 0 && ::Error)
			MYERROR::Error(::Error, "GRAPH", 0, "backBuffer", 0);
		g->m_locked = rect[1];
		g->m_unk0x248 = rect[0] / ((g->m_flags & 2) ? 4 : 2);
	}
	if (p_x >= m_viewXMin && p_y >= m_viewYMin && m_viewXMax - 1.0f > p_x
		&& m_viewYMax - 1.0f > p_y) {
		if (g->m_flags & 2) {
			int py = (int) p_y;
			int py1 = py + 1;
			int* p = (int*) (g->m_locked + 4 * ((int) p_x + py1 * g->m_unk0x248) + 4);
			*p = p_color.m_value;
			int* q = (int*) (g->m_locked + 4 * ((int) p_x + py * g->m_unk0x248) + 4);
			*q = *p;
			p = (int*) (g->m_locked + 4 * ((int) p_x + py1 * g->m_unk0x248));
			*p = *q;
			q = (int*) (g->m_locked + 4 * ((int) p_x + py * g->m_unk0x248));
			*q = *p;
		}
		else {
			int py = (int) p_y;
			int px = (int) p_x;
			*(short*) (g->m_locked + 2 * (px + g->m_unk0x248 * (py + 1)) + 2) =
				((p_color.m_value >> 3) & 0x1f)
				| RGB16_rMask & (p_color.m_value >> (16 - RGB16_rShift))
				| RGB16_gMask & (p_color.m_value >> (8 - RGB16_gShift));
			*(short*) (g->m_locked + 2 * (px + py * g->m_unk0x248) + 2) =
				*(short*) (g->m_locked + 2 * (px + (py + 1) * g->m_unk0x248) + 2);
			*(short*) (g->m_locked + 2 * (px + (py + 1) * g->m_unk0x248)) =
				*(short*) (g->m_locked + 2 * (px + py * g->m_unk0x248) + 2);
			*(short*) (g->m_locked + 2 * (px + py * g->m_unk0x248)) =
				*(short*) (g->m_locked + 2 * (px + (py + 1) * g->m_unk0x248));
		}
	}
}

// FUNCTION: ALIEN 0x42e690
float GRAPH_CORE::GetViewYMin()
{
	return m_viewYMin;
}

// STUB: ALIEN 0x42ea30
int GRAPH_CORE::TexArgToStr(char* p_buf, int p_arg)
{
	if (p_arg & 0x20)
		p_buf = strcat(p_buf,
			   // STRING: ALIEN 0x483d6c
			   "alp-");
	if (p_arg & 0x10)
		strcat(p_buf,
			   // STRING: ALIEN 0x483d64
			   "inv-");
	int result = p_arg & 0xf;
	const char* v3;
	if (result == 2) {
		// STRING: ALIEN 0x483d60
		v3 = "tex";
	} else if ((p_arg & 0xf) == 0) {
		// STRING: ALIEN 0x483d5c
		v3 = "dif";
	} else {
		if (result == 4) {
			// STRING: ALIEN 0x483d54
			v3 = "spec";
		} else if (result == 1) {
			// STRING: ALIEN 0x483d50
			v3 = "cur";
		} else if (result == 3) {
			// STRING: ALIEN 0x483d48
			v3 = "tfac";
		} else {
			return result;
		}
	}
	strcat(p_buf, v3);
	return 0;
}

// FUNCTION: ALIEN 0x42eb10
char* GRAPH_CORE::TexStageStateToStr()
{
	strcpy(g_texStageStr,
		   // STRING: ALIEN 0x483db8
		   "Op=");
	int v8;
	((GRAPH_CORE*) Graph)->m_device->GetTextureStageState(0, D3DTSS_COLOROP, (DWORD*) &v8);
	const char* v0;
	if (v8 == 1)
		// STRING: ALIEN 0x483db4
		v0 = "dis";
	else if (v8 == 2)
		// STRING: ALIEN 0x483dac
		v0 = "sel1";
	else if (v8 == 3)
		// STRING: ALIEN 0x483da4
		v0 = "sel2";
	else if (v8 == 4)
		// STRING: ALIEN 0x483da0
		v0 = "mod";
	else {
		// STRING: ALIEN 0x483d94
		v0 = "tex_alpha";
		if (v8 != 13)
			// STRING: ALIEN 0x483d8c
			v0 = "unknown";
	}
	strcat(g_texStageStr, v0);
	strcat(g_texStageStr,
		   // STRING: ALIEN 0x483d84
		   " Arg1=");
	((GRAPH_CORE*) Graph)->m_device->GetTextureStageState(0, D3DTSS_COLORARG1, (DWORD*) &v8);
	TexArgToStr(g_texStageStr, v8);
	strcat(g_texStageStr,
		   // STRING: ALIEN 0x483d7c
		   " Arg2=");
	((GRAPH_CORE*) Graph)->m_device->GetTextureStageState(0, D3DTSS_COLORARG2, (DWORD*) &v8);
	TexArgToStr(g_texStageStr, v8);
	strcat(g_texStageStr,
		   // STRING: ALIEN 0x483d74
		   " AOp=");
	((GRAPH_CORE*) Graph)->m_device->GetTextureStageState(0, D3DTSS_ALPHAOP, (DWORD*) &v8);
	const char* v4;
	if (v8 == 1)
		v4 = "dis";
	else if (v8 == 2)
		v4 = "sel1";
	else if (v8 == 3)
		v4 = "sel2";
	else if (v8 == 4)
		v4 = "mod";
	else {
		v4 = "tex_alpha";
		if (v8 != 13)
			v4 = "unknown";
	}
	strcat(g_texStageStr, v4);
	strcat(g_texStageStr, " Arg1=");
	((GRAPH_CORE*) Graph)->m_device->GetTextureStageState(0, D3DTSS_ALPHAARG1, (DWORD*) &v8);
	TexArgToStr(g_texStageStr, v8);
	strcat(g_texStageStr, " Arg2=");
	((GRAPH_CORE*) Graph)->m_device->GetTextureStageState(0, D3DTSS_ALPHAARG2, (DWORD*) &v8);
	TexArgToStr(g_texStageStr, v8);
	return g_texStageStr;
}

// STUB: ALIEN 0x42edf0
int GRAPH_CORE::SelectDisplayMode(DLGITEM* p_adapterList, DLGITEM* p_modeList,
	DLGITEM* p_windowChk)
{

	if (SendDlgItemMessageA((HWND) p_adapterList->m_hDlg, p_adapterList->m_id, CB_GETCOUNT, 0, 0)) {
		int sel = p_adapterList->SendMsg(CB_GETCURSEL, 0, 0);
		m_curAdapter = SendDlgItemMessageA((HWND) p_adapterList->m_hDlg, p_adapterList->m_id,
			CB_GETITEMDATA, sel, 0);
	}
	else {
		if (p_windowChk)
			SendDlgItemMessageA((HWND) p_windowChk->m_hDlg, p_windowChk->m_id, BM_SETCHECK,
				(m_flags >> 7) & 1, 0);
		SendDlgItemMessageA((HWND) p_adapterList->m_hDlg, p_adapterList->m_id, CB_RESETCONTENT, 0, 0);
		for (int a = 0; a < m_noAdapter; ++a) {
			int idx;
			{
				STRING name(m_adapters[a].m_name, STRING::INLINE_CHARP);
				idx = p_adapterList->SendMsg(CB_ADDSTRING, 0, (long) name.m_str);
				if (idx != -1)
					p_adapterList->SendMsg(CB_SETITEMDATA, idx, a);
			}
			if (a == m_curAdapter)
				SendDlgItemMessageA((HWND) p_adapterList->m_hDlg, p_adapterList->m_id,
					CB_SETCURSEL, idx, 0);
		}
	}

	if (SendDlgItemMessageA((HWND) p_modeList->m_hDlg, p_modeList->m_id, CB_GETCOUNT, 0, 0)) {
		int sel = p_modeList->SendMsg(CB_GETCURSEL, 0, 0);
		int data = SendDlgItemMessageA((HWND) p_modeList->m_hDlg, p_modeList->m_id, CB_GETITEMDATA,
			sel, 0);

		m_width = (float) (data & 0x7fff);
		m_height = (float) (data >> 16);
		m_flags = (m_flags & ~2u) | (((unsigned int) data >> 14) & 2);
	}

	if (m_adapters[m_curAdapter].FindDisplayMode((int) m_width, (int) m_height,
			(m_flags & 2) ? 32 : 16) < 0) {
		m_width = (float) m_adapters[m_curAdapter].m_modeW[0];
		m_height = (float) m_adapters[m_curAdapter].m_modeH[0];
		int bpp = DisplayFormatBits(m_adapters[m_curAdapter].m_modeFmt[0]);
		m_flags = (m_flags & ~2u) | (((bpp == 32) & 1) << 1);
	}

	if (p_windowChk) {
		if (m_adapters[m_curAdapter].m_caps & 1) {
			int desktopBpp = DisplayFormatBits(m_adapters[m_curAdapter].m_desktopFmt);
			BOOL fits = ((m_flags >> 1) & 1) == (desktopBpp == 32)
				&& (float) GetSystemMetrics(SM_CXSCREEN) >= m_width
				&& (float) GetSystemMetrics(SM_CYSCREEN) >= m_height;
			if (!fits) {
				SendDlgItemMessageA((HWND) p_windowChk->m_hDlg, p_windowChk->m_id, BM_SETCHECK, 1, 0);
			}
			EnableWindow(GetDlgItem((HWND) p_windowChk->m_hDlg, p_windowChk->m_id), fits);
		}
		else {
			SendDlgItemMessageA((HWND) p_windowChk->m_hDlg, p_windowChk->m_id, BM_SETCHECK, 1, 0);
			EnableWindow(GetDlgItem((HWND) p_windowChk->m_hDlg, p_windowChk->m_id), FALSE);
		}
		m_flags = (m_flags & ~0x80u)
			| (((SendDlgItemMessageA((HWND) p_windowChk->m_hDlg, p_windowChk->m_id, BM_GETCHECK, 0, 0)
				== 1) & 1) << 7);
	}

	SendDlgItemMessageA((HWND) p_modeList->m_hDlg, p_modeList->m_id, CB_RESETCONTENT, 0, 0);
	for (int j = 0; j < m_adapters[m_curAdapter].m_noModes; ++j) {
		int bpp = DisplayFormatBits(m_adapters[m_curAdapter].m_modeFmt[j]);
		long data = (bpp != 32 ? 0 : 0x8000) | m_adapters[m_curAdapter].m_modeW[j]
			| (m_adapters[m_curAdapter].m_modeH[j] << 16);
		long idx;
		{
			STRING label(m_adapters[m_curAdapter].GetModeString(j));
			idx = p_modeList->SendMsg(CB_ADDSTRING, 0, (long) label.m_str);
			if (idx != -1)
				SendDlgItemMessageA((HWND) p_modeList->m_hDlg, p_modeList->m_id, CB_SETITEMDATA, idx,
					data);
		}
		if ((float) m_adapters[m_curAdapter].m_modeW[j] == m_width
			&& (float) m_adapters[m_curAdapter].m_modeH[j] == m_height) {

			if (((m_flags >> 1) & 1) == (DisplayFormatBits(m_adapters[m_curAdapter].m_modeFmt[j]) == 32))
				SendDlgItemMessageA((HWND) p_modeList->m_hDlg, p_modeList->m_id, CB_SETCURSEL, idx, 0);
		}
	}
	return m_curAdapter;
}

// FUNCTION: ALIEN 0x430650
void GRAPH_CORE::ReloadPalettes()
{
	TEXTURE* tex = m_texE0C;
	if (!tex || tex->m_format != 41) // D3DFMT_P8
		return;
	delete m_texE0C;
	m_texE0C = new TEXTURE(256, 256, D3DFMT_P8, 0);
	if (!m_texE0C->m_texture && !m_texE0C->m_data && ::Error)
		MYERROR::Error(::Error,
			"GRAPH", 3,
			// STRING: ALIEN 0x483f2c
			"Light at RelodPalette()", 0);
	if (m_texE0C->m_format == 41) {
		unsigned int palette[256];
		for (int i = 0; i < 256; ++i)
			palette[i] = COLOR(i, i, i).m_value;
		m_texE0C->SetPalette(palette);
	}
	MYERROR::Log(::Error,
		// STRING: ALIEN 0x483f1c
		"ReloadPalettes");
}

// FUNCTION: ALIEN 0x430780
int GRAPH_CORE::SetAlphaBlend(unsigned int p_src, unsigned int p_dst)
{
	if ((m_flags & 0x4000) == 0) {
		m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
		m_flags |= 0x4000;
	}
	m_device->SetRenderState(D3DRS_SRCBLEND, p_src);
	return m_device->SetRenderState(D3DRS_DESTBLEND, p_dst);
}

// FUNCTION: ALIEN 0x4307e0
int GRAPH_CORE::SetRenderState(int p_state, unsigned int p_value)
{
	if (p_state != 0x17 && p_state != 0xe && p_state != 7) {
		if (p_state == 0x1d) {
			unsigned int flags = m_flags;
			if (!(((flags >> 13) & 1) ^ p_value))
				return 0;
			m_flags = (((p_value != 0) & 1) << 13) | (flags & 0xffffdfff);
		}
		else if (p_state == 0x1b) {
			unsigned int flags = m_flags;
			if (!(((flags >> 14) & 1) ^ p_value))
				return 0;
			m_flags = (((p_value != 0) & 1) << 14) | (flags & 0xffffbfff);
		}
		return m_device->SetRenderState((D3DRENDERSTATETYPE) p_state, p_value);
	}
	return 0;
}

// FUNCTION: ALIEN 0x430d20
int GRAPH_CORE::Lock()
{
	int result = m_locked;
	if (!result) {
		int rect[2];
		if (m_backBuffer->LockRect((D3DLOCKED_RECT*) rect, 0, 0) < 0) {
			if (::Error)
				MYERROR::Error(::Error, "GRAPH", 0,
							   "backBuffer", 0);
		}
		m_locked = rect[1];
		result = rect[0] / ((m_flags & 2) ? 4 : 2);
		m_unk0x248 = result;
	}
	return result;
}

// FUNCTION: ALIEN 0x430da0
char* GRAPH_CORE::Error(int p_type, const char* p_msg, int p_size) const
{
	int result = ::Error;
	if (result)
		result = (int) MYERROR::Error(result, "GRAPH", p_type, p_msg, p_size);
	return (char*) result;
}

// FUNCTION: ALIEN 0x430ed0
void GRAPH_CORE::PutsXY(float p_x, float p_y, const STRING& p_text, COLOR p_color)
{
	PutsXY(p_x, p_y, (char*) (const char*) p_text, p_color);
}

// FUNCTION: ALIEN 0x4335f0
int GRAPH_CORE::Pause()
{
	int locked = m_locked;
	m_flags |= 1u;
	if (locked) {
		m_backBuffer->UnlockRect();
		m_locked = 0;
	}
	FlipToGDI();
	DrawMenuBar((HWND) m_hwnd);
	RedrawWindow((HWND) m_hwnd, 0, 0, 0x400);
	return m_movie.Pause();
}

// FUNCTION: ALIEN 0x433660
int GRAPH_CORE::Resume()
{
	m_flags &= ~1u;
	return m_movie.Resume();
}

struct BAR_VERTEX {
	float m_x; // 0x00
	float m_y; // 0x04
	float m_z; // 0x08
	float m_rhw; // 0x0c
	unsigned int m_color; // 0x10
	unsigned int m_specular; // 0x14
};

// FUNCTION: ALIEN 0x4342b0
int GRAPH_CORE::Bar(float p_x1, float p_y1, float p_x2, float p_y2, COLOR p_color)
{
	BAR_VERTEX v[4];
	v[0].m_x = p_x1;
	v[0].m_y = p_y1;
	v[0].m_z = 0.99999988f;
	v[0].m_rhw = 1.0f;
	v[0].m_color = p_color.m_value;
	v[0].m_specular = 0xffffffff;

	v[1].m_x = p_x2;
	v[1].m_y = p_y1;
	v[1].m_z = 0.99999988f;
	v[1].m_rhw = 1.0f;
	v[1].m_color = p_color.m_value;
	v[1].m_specular = 0xffffffff;

	v[2].m_x = p_x2;
	v[2].m_y = p_y2;
	v[2].m_z = 0.99999988f;
	v[2].m_rhw = 1.0f;
	v[2].m_color = p_color.m_value;
	v[2].m_specular = 0xffffffff;

	v[3].m_x = p_x1;
	v[3].m_y = p_y2;
	v[3].m_z = 0.99999988f;
	v[3].m_rhw = 1.0f;
	v[3].m_color = p_color.m_value;
	v[3].m_specular = 0xffffffff;

	if (m_locked) {
		m_backBuffer->UnlockRect();
		m_locked = 0;
	}
	m_device->SetTexture(0, 0);
	if ((p_color.m_value & 0xff000000) == 0xff000000)
		SetRenderState(0x1b, 0);
	else
		SetAlphaBlend(5, 6);
	SetRenderState(0xe, 0);
	DrawPrimitive(6, 0xc4, v, 0x18, 4);
	return SetRenderState(0xe, 1);
}

// FUNCTION: ALIEN 0x434420
void GRAPH_CORE::PrintfXY(GRAPH_CORE* p_graph, float p_x, float p_y, char* p_format, ...)
{
	char buf[0x400];
	va_list args;
	va_start(args, p_format);
	vsprintf(buf, p_format, args);
	p_graph->PutsXY(p_x, p_y, buf, GREEN);
}

// FUNCTION: ALIEN 0x434720
int ADAPTER::FindDisplayMode(int p_w, int p_h, int p_bpp)
{
	int n = m_noModes;
	int i = 0;
	if (n <= 0)
		return -1;
	do {
		if (m_modeW[i] == p_w && m_modeH[i] == p_h) {
			int fmt = m_modeFmt[i];
			int bpp;
			switch (fmt) {
			case D3DFMT_A8R8G8B8:
			case D3DFMT_X8R8G8B8:
				bpp = 32;
				break;
			case D3DFMT_R5G6B5:
			case D3DFMT_X1R5G5B5:
			case D3DFMT_A1R5G5B5:
			case D3DFMT_A4R4G4B4:
				bpp = 16;
				break;
			case D3DFMT_R8G8B8:
				bpp = 24;
				break;
			default:
				bpp = 0;
				break;
			case D3DFMT_P8:
			case D3DFMT_DXT3:
			case D3DFMT_DXT5:
				bpp = 8;
				break;
			case D3DFMT_DXT1:
				bpp = 4;
			}
			if (bpp == p_bpp)
				return i;
		}
		++i;
	} while (i < n);
	return -1;
}

// FUNCTION: ALIEN 0x4347d0
STRING ADAPTER::GetModeString(int p_idx)
{
	for (int i = m_noModes - 1; i >= 1; --i) {
		if (m_modeFmt[i] == m_modeFmt[0])
			continue;
		int bpp;
		switch (m_modeFmt[p_idx]) {
		case D3DFMT_A8R8G8B8:
		case D3DFMT_X8R8G8B8:
			bpp = 32;
			break;
		case D3DFMT_R5G6B5:
		case D3DFMT_X1R5G5B5:
		case D3DFMT_A1R5G5B5:
		case D3DFMT_A4R4G4B4:
			bpp = 16;
			break;
		case D3DFMT_R8G8B8:
			bpp = 24;
			break;
		default:
			bpp = 0;
			break;
		case D3DFMT_P8:
		case D3DFMT_DXT3:
		case D3DFMT_DXT5:
			bpp = 8;
			break;
		case D3DFMT_DXT1:
			bpp = 4;
		}
		// STRING: ALIEN 0x483fa8
		return Printf("%i x %i x %ibpp", m_modeW[p_idx], m_modeH[p_idx], bpp);
	}
	// STRING: ALIEN 0x483fb8
	return Printf("%i x %i", m_modeW[p_idx], m_modeH[p_idx]);
}

// FUNCTION: ALIEN 0x4428f0
float GRAPH_CORE::GetViewXMin()
{
	return m_viewXMin;
}

// FUNCTION: ALIEN 0x449bc0
float GRAPH_CORE::GetViewXMax()
{
	return m_viewXMax;
}

// FUNCTION: ALIEN 0x449bd0
float GRAPH_CORE::GetViewYMax()
{
	return m_viewYMax;
}
