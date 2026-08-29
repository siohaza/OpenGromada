#ifndef GRAPH_CORE_H
#define GRAPH_CORE_H

#include "util/decomp.h"
#include "gfx/color.h"
#include <dxsdk/d3d8.h>
#include "gfx/d3dfont.h"
#include "gfx/gamma.h"
#include "gfx/movie.h"

class TEXTURE;
class STRING;
class SETTINGS;

STRING GetPixelFormat(D3DFORMAT p_format);

struct RHW_VERTEX {
	float m_x; // 0x00
	float m_y; // 0x04
	float m_z; // 0x08
	float m_rhw; // 0x0c
	unsigned int m_color; // 0x10
};

struct ADAPTER {
	char m_name[0x3c]; // 0x00
	int m_vidMemory; // 0x3c
	int m_noModes; // 0x40
	int m_modeW[16]; // 0x44
	int m_modeH[16]; // 0x84
	int m_modeFmt[16]; // 0xc4
	int m_modeZFmt[16]; // 0x104
	int m_desktopFmt; // 0x144
	unsigned int m_caps; // 0x148

	ADAPTER()
	{
		m_caps &= 0xfffffff0;
		m_noModes = 0;
		m_name[0] = 0;
	}

	void EnumDisplayModes(unsigned int p_adapter, IDirect3D8* p_d3d, SETTINGS* p_settings);
	int FindDisplayMode(int p_w, int p_h, int p_bpp);
	STRING GetModeString(int p_idx);

};

DECOMP_SIZE_ASSERT(ADAPTER, 0x14c)

class DLGITEM;

class GRAPH_CORE {
public:
	GRAPH_CORE(SETTINGS* p_settings);
	~GRAPH_CORE();

	D3DPRESENT_PARAMETERS m_presentParams; // 0x00
	unsigned int m_flags; // 0x34
	int m_pixelShaderVersion; // 0x38

	unsigned short m_snowRamp[256]; // 0x3c
	int m_locked; // 0x23c
	float m_width; // 0x240
	float m_height; // 0x244
	undefined4 m_unk0x248; // 0x248
	void* m_zbuffer; // 0x24c
	int m_unk0x250; // 0x250
	float m_viewXMin; // 0x254
	float m_viewXMax; // 0x258
	float m_viewYMin; // 0x25c
	float m_viewYMax; // 0x260
	int m_noAdapter; // 0x264
	ADAPTER m_adapters[8]; // 0x268
	int m_curAdapter; // 0xcc8
	float m_unk0xccc; // 0xccc
	float m_unk0xcd0; // 0xcd0
	int m_effectA[16]; // 0xcd4
	int m_effectB[16]; // 0xd14
	unsigned int m_effectStart[16]; // 0xd54
	int m_effectDuration[16]; // 0xd94
	GAMMA m_gammaCur; // 0xdd4
	GAMMA m_gammaSet; // 0xddc
	int m_env; // 0xde4
	unsigned char m_windDirection; // 0xde8
	undefined m_pad0xde9[3]; // 0xde9
	float m_windForce; // 0xdec
	MOVIE m_movie; // 0xdf0
	void* m_hwnd; // 0xe00
	int m_unk0xe04; // 0xe04
	int m_unk0xe08; // 0xe08
	TEXTURE* m_texE0C; // 0xe0c
	TEXTURE* m_texE10; // 0xe10
	TEXTURE* m_texE14; // 0xe14
	IDirect3D8* m_d3d; // 0xe18
	IDirect3DDevice8* m_device; // 0xe1c
	IDirect3DSurface8* m_backBuffer; // 0xe20
	IDirect3DSurface8* m_screenSurf; // 0xe24
	CD3DFont* m_font; // 0xe28

	static COLOR BLACK;
	static COLOR CYAN;
	static COLOR MAGENTA;
	static COLOR BROWN;
	static COLOR WHITE;
	static COLOR GRAY;
	static COLOR BLUE;
	static COLOR LIGHTBLUE;
	static COLOR LIGHTGREEN;
	static COLOR RED;
	static COLOR LIGHTRED;
	static COLOR YELLOW;
	static COLOR GREEN;

	static void PrintfXY(GRAPH_CORE* p_graph, float p_x, float p_y, char* p_format, ...);

	static void GetTextureCaps(IDirect3DDevice8* p_device);
	static int TexArgToStr(char* p_buf, int p_arg);
	static char* TexStageStateToStr();
	int Init(void* p_hWnd);
	int SetViewPort(float p_x1, float p_y1, float p_x2, float p_y2);
	int PreTact();
	void Effect(int p_effect, int p_a, int p_b, int p_duration);
	void PostTact(int p_present);
	void ReloadPalettes();
	void ClearScreen(COLOR p_color);
	void FlipToGDI();
	int Pause();
	int Resume();
	int CreateFont(const STRING& p_name, int p_height, int p_flags);
	void PutsXY(float p_x, float p_y, char* p_text, COLOR p_color);
	void PutsXY(float p_x, float p_y, const STRING& p_text, COLOR p_color);
	int SetAlphaBlend(unsigned int p_src, unsigned int p_dst);
	int SetRenderState(int p_state, unsigned int p_value);
	int CopyToZBuffer(int* p_dst, int* p_src, void* p_texture);
	int DrawPrimitive(int p_type, unsigned int p_fvf, void* p_verts, unsigned int p_stride, int p_count);
	int Bar(float p_x1, float p_y1, float p_x2, float p_y2, COLOR p_color);
	int Lock();
	float GetWidth();
	float GetHeight();
	float GetViewYMin();
	float GetViewXMin();
	float GetViewXMax();
	float GetViewYMax();
	int SelectDisplayMode(DLGITEM* p_adapterList, DLGITEM* p_modeList, DLGITEM* p_windowChk);
	char* Error(int p_type, const char* p_msg, int p_size) const;
};

DECOMP_SIZE_ASSERT(GRAPH_CORE, 0xe2c)

#endif
