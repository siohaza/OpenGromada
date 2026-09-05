#ifndef GRAPH_CORE_H
#define GRAPH_CORE_H

#include "gfx/color.h"
#include "gfx/debugfont.h"
#include "gfx/gamma.h"
#include "gfx/gfxdefs.h"
#include "util/decomp.h"

#include <string>

class TEXTURE;
class STRING;
class SETTINGS;
class MoviePlayer;

STRING GetPixelFormat(D3DFORMAT p_format);

struct RHW_VERTEX {
	float m_x;            // 0x00
	float m_y;            // 0x04
	float m_z;            // 0x08
	float m_rhw;          // 0x0c
	unsigned int m_color; // 0x10
};

struct ADAPTER {
	ADAPTER();
	~ADAPTER();

	ADAPTER(const ADAPTER&) = delete;
	ADAPTER& operator=(const ADAPTER&) = delete;

	char m_name[0x3c];
	int m_noModes;
	int m_capacity;
	int* m_modeW;
	int* m_modeH;
	int* m_modeBpp;

	void EnumDisplayModes(unsigned int p_display, SETTINGS* p_settings);
	int FindDisplayMode(int p_w, int p_h, int p_bpp);
	STRING GetModeString(int p_idx);

private:
	friend struct ADAPTER_TEST_ACCESS;
	int Reserve(int p_wanted);
	void AddMode(int p_w, int p_h, int p_bpp);
};

struct RENDER_STATE {
	int m_alphaBlend;
	int m_srcBlend;
	int m_dstBlend;
	int m_specular;
	int m_cull;
	int m_zFunc;
	int m_zWrite;
	int m_alphaTest;
	int m_alphaFunc;
	int m_alphaRef;
	unsigned int m_textureFactor;
	int m_magFilter;
	int m_minFilter;
	int m_colorOp;
	int m_colorArg1;
	int m_colorArg2;
	int m_alphaOp;
	int m_alphaArg1;
	int m_alphaArg2;

	RENDER_STATE();
};

inline unsigned int GraphBlendFactor(int p_factor,
									 unsigned int p_src,
									 unsigned int p_dst,
									 unsigned int p_srcAlpha,
									 unsigned int p_dstAlpha)
{
	switch (p_factor) {
	case D3DBLEND_ZERO:
		return 0;
	case D3DBLEND_SRCCOLOR:
		return p_src;
	case D3DBLEND_INVSRCCOLOR:
		return 255 - p_src;
	case D3DBLEND_SRCALPHA:
		return p_srcAlpha;
	case D3DBLEND_INVSRCALPHA:
		return 255 - p_srcAlpha;
	case D3DBLEND_DESTALPHA:
		return p_dstAlpha;
	case D3DBLEND_INVDESTALPHA:
		return 255 - p_dstAlpha;
	case D3DBLEND_DESTCOLOR:
		return p_dst;
	case D3DBLEND_INVDESTCOLOR:
		return 255 - p_dst;
	default:
		return 255;
	}
}

class GRAPH_CORE {
public:
	GRAPH_CORE(SETTINGS* p_settings);
	~GRAPH_CORE();

	unsigned int m_flags;
	unsigned short m_snowRamp[256];

	void* m_color;
	int m_pitch; // frame buffer stride, in pixels

	float m_width;
	float m_height;
	int m_outputWidth;
	int m_outputHeight;
	int m_renderWidth;
	int m_nativeResolution;
	int m_automaticResolution;
	unsigned int m_displayID;
	int m_uiScaleSetting;
	int m_uiScale;
	float m_uiPresentationScale;

	void* m_zbuffer;
	int m_zpitch; // depth buffer stride, in pixels

	float m_viewXMin;
	float m_viewXMax;
	float m_viewYMin;
	float m_viewYMax;

	int m_noAdapter;
	ADAPTER m_adapters[8];
	int m_curAdapter;

	float m_shiftBaseX;
	float m_shiftBaseY;

	int m_effectA[16];
	int m_effectB[16];
	unsigned int m_effectStart[16];
	int m_effectDuration[16];
	GAMMA m_gammaCur;
	GAMMA m_gammaSet;
	int m_env;
	unsigned char m_windDirection;
	float m_windForce;

	RENDER_STATE m_state;

	// SDL_Window* owned by the platform layer.
	void* m_window;

	MoviePlayer* m_movie = nullptr;
	std::string m_movieName;
	int m_movieX = 0, m_movieY = 0;
	bool m_movieErrorReported = false;
	bool m_movieActive = false;

	void* m_screen;
	unsigned int m_gpuScreen = 0;

	int m_debugFontHeight;

	TEXTURE* m_texE0C;
	TEXTURE* m_texE10;
	TEXTURE* m_texE14;
	int m_lightBufferToggle;
	DEBUG_FONT* m_font;

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

	static void PrintfXY(GRAPH_CORE* p_graph, float p_x, float p_y, const char* p_format, ...) DECOMP_PRINTF(4, 5);

	int Init();
	int SetViewPort(float p_x1, float p_y1, float p_x2, float p_y2);
	int PreTact();
	void Effect(int p_effect, int p_a, int p_b, int p_duration);
	void PostTact(int p_present);
	int OpenMovie(const char* p_filename);
	int IsMoviePlaying();
	void StopMovie();
	void PollMovie(bool p_consumeCompletion = true);
	void PresentIdleMovie();
	void PresentMovieFrame();
	void ReloadPalettes();
	void ClearScreen(COLOR p_color);
	int Pause();
	int Resume();
	int CreateDebugFont(const STRING& p_name, int p_height, int p_flags);
	void PutsXY(float p_x, float p_y, const char* p_text, COLOR p_color);
	void PutsXY(float p_x, float p_y, const STRING& p_text, COLOR p_color);
	int SetAlphaBlend(unsigned int p_src, unsigned int p_dst);
	int SetRenderState(int p_state, unsigned int p_value);
	int SetTextureStageState(int p_state, unsigned int p_value);
	int CopyToZBuffer(int* p_dst, int* p_src, void* p_texture);
	int DrawPrimitive(int p_type, unsigned int p_fvf, void* p_verts, unsigned int p_stride, int p_count);
	int Bar(float p_x1, float p_y1, float p_x2, float p_y2, COLOR p_color);

	int FillRect(float p_x1, float p_y1, float p_x2, float p_y2, unsigned int p_color, unsigned int p_specular);

	int Lock();

	int BytesPerPixel() const { return (m_flags & 2) ? 4 : 2; }

	float GetWidth();
	float GetHeight();
	float GetViewYMin();
	float GetViewXMin();
	float GetViewXMax();
	float GetViewYMax();
	int Error(int p_type, const char* p_msg, int p_size) const;
	void ResolveDisplaySize();

private:
	friend class MAP;
	int ConfigureFrameForMap(float p_mapWidth, float p_mapHeight, int p_gameplay);
	int ConfigureFrameForTerrain(int p_width, int p_height);
	int ConfigureFrameSize(int p_width, int p_height, int p_mapSafeNative);
	void ReleaseFrameBuffer();
	int CreateFrameBuffer();
};

#endif
