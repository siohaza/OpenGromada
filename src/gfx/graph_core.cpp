
#include "gfx/graph_core.h"

#include "game/game_descriptor.h"

#include "game/gametime.h"
#include "game/map.h"
#include "game/settings.h"
#include "gfx/display_math.h"
#include "gfx/graph.h"
#include "gfx/texture.h"
#include "platform/render.h"
#include "platform/paths.h"
#include "util/myerror.h"
#include "video/movie_player.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <limits.h>
#include <new>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct GRAPH_CORE_FLAGS {
	unsigned int m_unk0 : 1;
	unsigned int m_bpp32 : 1;
	unsigned int m_unk2 : 5;
	unsigned int m_fullscreen : 1;
	unsigned int m_unk8 : 24;
};

// GLOBAL: ALIEN 0x4b2828
COLOR GRAPH_CORE::GREEN(0, 255, 0);

RENDER_STATE::RENDER_STATE()
{
	m_alphaBlend = 0;
	m_srcBlend = D3DBLEND_SRCALPHA;
	m_dstBlend = D3DBLEND_INVSRCALPHA;
	m_specular = 0;
	m_cull = D3DCULL_NONE;
	m_zFunc = D3DCMP_ALWAYS;
	m_zWrite = 0;
	m_alphaTest = 0;
	m_alphaFunc = D3DCMP_ALWAYS;
	m_alphaRef = 0;
	m_textureFactor = 0xffffffff;
	m_magFilter = D3DTEXF_POINT;
	m_minFilter = D3DTEXF_POINT;
	m_colorOp = D3DTOP_MODULATE;
	m_colorArg1 = D3DTA_TEXTURE;
	m_colorArg2 = D3DTA_DIFFUSE;
	m_alphaOp = D3DTOP_MODULATE;
	m_alphaArg1 = D3DTA_TEXTURE;
	m_alphaArg2 = D3DTA_DIFFUSE;
}

ADAPTER::ADAPTER()
{
	m_name[0] = 0;
	m_noModes = 0;
	m_capacity = 0;
	m_modeW = 0;
	m_modeH = 0;
	m_modeBpp = 0;
}

ADAPTER::~ADAPTER()
{
	free(m_modeW);
	free(m_modeH);
	free(m_modeBpp);
}

int ADAPTER::Reserve(int p_wanted)
{
	if (p_wanted <= m_capacity) {
		return 0;
	}
	int capacity = m_capacity ? m_capacity : 16;
	while (capacity < p_wanted) {
		capacity *= 2;
	}

	int* w = (int*) realloc(m_modeW, (size_t) capacity * sizeof(int));
	if (!w) {
		return 1;
	}
	m_modeW = w;
	int* h = (int*) realloc(m_modeH, (size_t) capacity * sizeof(int));
	if (!h) {
		return 1;
	}
	m_modeH = h;
	int* bpp = (int*) realloc(m_modeBpp, (size_t) capacity * sizeof(int));
	if (!bpp) {
		return 1;
	}
	m_modeBpp = bpp;

	m_capacity = capacity;
	return 0;
}

void ADAPTER::AddMode(int p_w, int p_h, int p_bpp)
{
	for (int i = 0; i < m_noModes; ++i) {
		if (m_modeW[i] == p_w && m_modeH[i] == p_h && m_modeBpp[i] == p_bpp) {
			return;
		}
	}
	if (Reserve(m_noModes + 1)) {
		return;
	}
	m_modeW[m_noModes] = p_w;
	m_modeH[m_noModes] = p_h;
	m_modeBpp[m_noModes] = p_bpp;
	++m_noModes;
}

// Enumerate all SDL display modes.
void ADAPTER::EnumDisplayModes(unsigned int p_display, SETTINGS* p_settings)
{
	m_noModes = 0;

	int count = 0;
	SDL_DisplayID* displays = SDL_GetDisplays(&count);
	if (!displays || (int) p_display >= count) {
		SDL_free(displays);
		return;
	}
	SDL_DisplayID id = displays[p_display];
	SDL_free(displays);

	const char* name = SDL_GetDisplayName(id);
	if (name) {
		strncpy(m_name, name, sizeof(m_name) - 1);
		m_name[sizeof(m_name) - 1] = 0;
	}

	int nModes = 0;
	SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(id, &nModes);
	if (modes) {
		for (int i = 0; i < nModes; ++i) {
			AddMode(modes[i]->w, modes[i]->h, 32);
		}
		SDL_free(modes);
	}

	// A windowed game is not restricted to the display's own mode list, so the
	// desktop size is always available too.
	const SDL_DisplayMode* desktop = SDL_GetDesktopDisplayMode(id);
	if (desktop) {
		AddMode(desktop->w, desktop->h, 32);
	}

	if (p_settings && m_noModes == 0) {
		AddMode(p_settings->m_screenX, p_settings->m_screenY, 32);
	}
}

// FUNCTION: ALIEN 0x434720
int ADAPTER::FindDisplayMode(int p_w, int p_h, int p_bpp)
{
	for (int i = 0; i < m_noModes; ++i) {
		if (m_modeW[i] == p_w && m_modeH[i] == p_h && m_modeBpp[i] == p_bpp) {
			return i;
		}
	}
	return -1;
}

// FUNCTION: ALIEN 0x4347d0
STRING ADAPTER::GetModeString(int p_idx)
{
	if (p_idx < 0 || p_idx >= m_noModes) {
		return STRING::EMPTY;
	}
	return Printf("%i x %i", m_modeW[p_idx], m_modeH[p_idx]);
}

static DISPLAY_MATH::RESOLUTION DesktopResolution(SDL_DisplayID p_display)
{
	if (!p_display) {
		p_display = SDL_GetPrimaryDisplay();
	}
	const SDL_DisplayMode* desktop = p_display ? SDL_GetDesktopDisplayMode(p_display) : 0;
	return desktop ? DISPLAY_MATH::RESOLUTION{desktop->w, desktop->h} : DISPLAY_MATH::RESOLUTION{640, 480};
}

static DISPLAY_MATH::RESOLUTION UsableResolution(SDL_DisplayID p_display)
{
	if (!p_display) {
		p_display = SDL_GetPrimaryDisplay();
	}
	SDL_Rect usable = {};
	return p_display && SDL_GetDisplayUsableBounds(p_display, &usable) && usable.w > 0 && usable.h > 0
			   ? DISPLAY_MATH::RESOLUTION{usable.w, usable.h}
			   : DesktopResolution(p_display);
}

// FUNCTION: ALIEN 0x4018f0
GRAPH_CORE::GRAPH_CORE(SETTINGS* p_settings)
{
	m_flags = 0;
	m_flags = (m_flags & 0xfffffbff) | ((p_settings->m_flag & 1) << 10);
	m_flags = (m_flags & 0xfffffefe) | ((p_settings->m_flag & 2) << 7);
	m_font = 0;
	m_texE0C = 0;
	m_texE10 = 0;
	m_texE14 = 0;
	m_lightBufferToggle = 0;
	m_color = 0;
	m_pitch = 0;
	m_zbuffer = 0;
	m_zpitch = 0;
	m_screen = 0;
	m_window = 0;
	m_debugFontHeight = 0;
	m_shiftBaseX = 0.0f;
	m_shiftBaseY = 0.0f;
	m_flags &= 0xfffeffff;
	m_env = 0;
	AngleAssign((ANGLE*) &m_windDirection, ANGLE(0xdc));
	m_windForce = 20.0f;
	Effect(0, 0, 0, 0);

	SDL_InitSubSystem(SDL_INIT_VIDEO);

	m_noAdapter = 0;
	int count = 0;
	SDL_DisplayID* displays = SDL_GetDisplays(&count);
	if (count > 8) {
		count = 8;
	}
	for (int i = 0; i < count; ++i) {
		m_adapters[i].EnumDisplayModes(i, p_settings);
	}
	m_noAdapter = count;

	m_curAdapter = p_settings->m_device;
	if (m_curAdapter < 0 || m_curAdapter >= m_noAdapter) {
		m_curAdapter = 0;
	}
	m_displayID =
		displays && m_curAdapter >= 0 && m_curAdapter < count ? displays[m_curAdapter] : SDL_GetPrimaryDisplay();
	SDL_free(displays);
	DISPLAY_MATH::RESOLUTION desktop = DesktopResolution(m_displayID);
	DISPLAY_MATH::RESOLUTION usable = UsableResolution(m_displayID);
	m_automaticResolution = p_settings->m_desktopResolution != 0;
	DISPLAY_MATH::RESOLUTION output = DISPLAY_MATH::ResolveOutput(
		p_settings->m_screenX,
		p_settings->m_screenY,
		desktop.m_width,
		desktop.m_height,
		usable.m_width,
		usable.m_height,
		m_automaticResolution != 0,
		p_settings->m_fullscreen != 0
	);
	m_outputWidth = output.m_width;
	m_outputHeight = output.m_height;
	m_renderWidth = p_settings->m_renderWidth;
	m_nativeResolution = p_settings->m_nativeResolution != 0;
	m_uiScaleSetting = p_settings->m_uiScale;
	ResolveDisplaySize();

	((GRAPH_CORE_FLAGS*) &m_flags)->m_bpp32 = 1;
	((GRAPH_CORE_FLAGS*) &m_flags)->m_fullscreen = p_settings->m_fullscreen != 0;
}

void GRAPH_CORE::ResolveDisplaySize()
{
	DISPLAY_MATH::RESOLUTION logical = DISPLAY_MATH::ResolveInternal(
		m_outputWidth,
		m_outputHeight,
		m_renderWidth,
		m_nativeResolution != 0,
		GameDesc->m_uiBaseHeight
	);
	if (m_renderWidth <= 0 && GameDesc->m_fixedFrameWidth > 0) {
		logical = {GameDesc->m_fixedFrameWidth, GameDesc->m_fixedFrameHeight};
	}
	m_width = (float) logical.m_width;
	m_height = (float) logical.m_height;
	m_uiScale = DISPLAY_MATH::ResolveUIScale(
		logical.m_width,
		logical.m_height,
		m_uiScaleSetting,
		GameDesc->m_uiBaseWidth,
		GameDesc->m_uiBaseHeight
	);
	m_uiPresentationScale = 1.0f;
}

int GRAPH_CORE::ConfigureFrameForMap(float p_mapWidth, float p_mapHeight, int p_gameplay)
{
	DISPLAY_MATH::RESOLUTION target = DISPLAY_MATH::ResolveInternal(
		m_outputWidth,
		m_outputHeight,
		m_renderWidth,
		m_nativeResolution != 0,
		GameDesc->m_uiBaseHeight
	);
	bool mapSafeNative = p_gameplay && m_nativeResolution && std::isfinite(p_mapWidth) && std::isfinite(p_mapHeight) &&
						 p_mapWidth > 0.0f && p_mapHeight > 0.0f && p_mapWidth <= (float) INT_MAX &&
						 p_mapHeight <= (float) INT_MAX;
	if (m_renderWidth <= 0 && GameDesc->m_fixedFrameWidth > 0) {
		target = {GameDesc->m_fixedFrameWidth, GameDesc->m_fixedFrameHeight};
	}
	if (mapSafeNative) {
		target = DISPLAY_MATH::ResolveMapSafeInternal(
			target,
			{m_outputWidth, m_outputHeight},
			(int) p_mapWidth,
			(int) p_mapHeight,
			GameDesc->m_uiBaseHeight
		);
	}
	return ConfigureFrameSize(target.m_width, target.m_height, mapSafeNative ? 1 : 0);
}

int GRAPH_CORE::ConfigureFrameForTerrain(int p_width, int p_height)
{
	return ConfigureFrameSize(p_width, p_height, 1);
}

int GRAPH_CORE::ConfigureFrameSize(int p_width, int p_height, int p_mapSafeNative)
{
	DISPLAY_MATH::RESOLUTION target = {p_width, p_height};
	const int targetUIScale =
		p_mapSafeNative ? DISPLAY_MATH::ResolveGameplayUIScale(
							  m_outputWidth,
							  m_outputHeight,
							  m_uiScaleSetting,
							  GameDesc->m_uiBaseWidth,
							  GameDesc->m_uiBaseHeight
						  )
						: DISPLAY_MATH::ResolveUIScale(
							  target.m_width,
							  target.m_height,
							  m_uiScaleSetting,
							  GameDesc->m_uiBaseWidth,
							  GameDesc->m_uiBaseHeight
						  );
	float targetUIPresentationScale = 1.0f;
	if (p_mapSafeNative && (target.m_width < m_outputWidth || target.m_height < m_outputHeight) && m_outputWidth > 0 &&
		m_outputHeight > 0) {
		const float scaleX = (float) target.m_width / (float) m_outputWidth;
		const float scaleY = (float) target.m_height / (float) m_outputHeight;
		targetUIPresentationScale = scaleX < scaleY ? scaleX : scaleY;
		if (!(targetUIPresentationScale > 0.0f) || targetUIPresentationScale > 1.0f) {
			targetUIPresentationScale = 1.0f;
		}
	}

	const int oldWidth = (int) m_width;
	const int oldHeight = (int) m_height;
	if (target.m_width == oldWidth && target.m_height == oldHeight) {
		m_uiScale = targetUIScale;
		m_uiPresentationScale = targetUIPresentationScale;
		return 0;
	}
	if ((m_flags & 0x10000) || target.m_width <= 0 || target.m_height <= 0 ||
		(size_t) target.m_width > SIZE_MAX / (size_t) target.m_height ||
		(size_t) target.m_width * (size_t) target.m_height > SIZE_MAX / sizeof(unsigned short)) {
		MYERROR::Log(::Error, "Unable to resize render frame to %ix%i", target.m_width, target.m_height);
		return -1;
	}

	const size_t zCount = (size_t) target.m_width * (size_t) target.m_height;
	unsigned short* replacementZ = (unsigned short*) operator new(zCount * sizeof(unsigned short), std::nothrow);
	if (!replacementZ) {
		MYERROR::Log(::Error, "Unable to allocate render depth %ix%i", target.m_width, target.m_height);
		return -1;
	}
	for (size_t i = 0; i < zCount; ++i) {
		replacementZ[i] = 0x03ff;
	}

	if (Platform_RenderResizeLogical(target.m_width, target.m_height)) {
		operator delete(replacementZ);
		MYERROR::Log(::Error, "Unable to replace render frame %ix%i", target.m_width, target.m_height);
		return -1;
	}

	void* oldZ = m_zbuffer;
	m_width = (float) target.m_width;
	m_height = (float) target.m_height;
	m_color = Platform_RenderPixels();
	m_pitch = Platform_RenderPitch();
	m_zbuffer = replacementZ;
	m_zpitch = target.m_width;
	m_uiScale = targetUIScale;
	m_uiPresentationScale = targetUIPresentationScale;
	SetViewPort(0.0f, 0.0f, m_width, m_height);
	m_effectStart[5] = 0;
	free(m_screen);
	m_screen = 0;
	ClearScreen(BLACK);
	operator delete(oldZ);

	MYERROR::Log(::Error, "Resized render frame %ix%i -> %ix%i", oldWidth, oldHeight, target.m_width, target.m_height);
	return 1;
}

// FUNCTION: ALIEN 0x401b10
GRAPH_CORE::~GRAPH_CORE()
{
	StopMovie();
	delete m_movie;
	m_movie = nullptr;
	if (m_font) {
		delete m_font;
		m_font = 0;
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
	ReleaseFrameBuffer();
	Platform_RenderClose();
	m_window = 0;
}

void GRAPH_CORE::ReleaseFrameBuffer()
{
	if (m_zbuffer) {
		operator delete(m_zbuffer);
		m_zbuffer = 0;
	}
	free(m_screen);
	m_screen = 0;
	m_color = 0;
	m_pitch = 0;
	m_zpitch = 0;
}

int GRAPH_CORE::CreateFrameBuffer()
{
	int w = (int) m_width;
	int h = (int) m_height;
	if (w <= 0 || h <= 0) {
		return 1;
	}

	// The renderer owns the colour pixels; nothing is copied on the way to the
	// screen.
	m_color = Platform_RenderPixels();
	m_pitch = Platform_RenderPitch();
	if (!m_color) {
		return 1;
	}

	m_zbuffer = operator new(2 * (size_t) w * (size_t) h);
	m_zpitch = w;
	return 0;
}

// FUNCTION: ALIEN 0x401c80
int GRAPH_CORE::Init()
{
	int w = (int) m_width;
	int h = (int) m_height;

	if (m_flags & 0x80) {
		if (m_noAdapter > 0 && m_adapters[m_curAdapter].FindDisplayMode(m_outputWidth, m_outputHeight, 32) < 0) {
			MYERROR::Log(::Error, "Selected output mode %ix%i", m_outputWidth, m_outputHeight);
			MYERROR::Log(::Error, "Display does not advertise this mode; scaling instead");
		}
	}

	if (Platform_RenderOpen(
			GameDesc->m_title,
			m_outputWidth,
			m_outputHeight,
			w,
			h,
			(m_flags & 0x80) != 0,
			m_automaticResolution,
			m_displayID
		)) {
		if (::Error) {
			MYERROR::Error(
				::Error,
				"GRAPH",
				3,
				// STRING: ALIEN 0x47f1f8
				"3dDevice",
				0
			);
		}
		return 1;
	}
	Platform_RenderSetVSync((m_flags & 0x100) != 0);
	m_window = Platform_RenderWindow();

	if (CreateFrameBuffer()) {
		if (::Error) {
			MYERROR::Error(
				::Error,
				"GRAPH",
				3,
				// STRING: ALIEN 0x47f1d8
				"tempBuffer",
				0
			);
		}
		return 1;
	}

	MYERROR::Log(
		::Error,
		// STRING: ALIEN 0x47f204
		"Selected output %ix%i, render %.0fx%.0f %s",
		m_outputWidth,
		m_outputHeight,
		m_width,
		m_height,
		GetPixelFormat(D3DFMT_A8R8G8B8).m_str
	);
	return 0;
}

// FUNCTION: ALIEN 0x402090
int GRAPH_CORE::PreTact()
{
	if (m_flags & 0x10000) {
		return 0;
	}
	m_flags |= 0x10000;
	return 0;
}

// FUNCTION: ALIEN 0x402250
void GRAPH_CORE::PostTact(int p_present)
{
	if (!(m_flags & 0x10000)) {
		return;
	}
	PollMovie();
	if (p_present && m_movieActive) {



		PresentMovieFrame();
	}
	else if (p_present && !m_effectStart[6] && !m_effectStart[7]) {
		Platform_RenderPresent();
	}
	m_flags &= ~0x10000;
}

int GRAPH_CORE::OpenMovie(const char* p_filename)
{



	StopMovie();
	m_movieName = p_filename ? p_filename : "";
	m_movieErrorReported = false;
	if (!MoviePlayer::Available()) {
		MYERROR::Log(::Error, "Movie playback is unavailable in this build: '%s'", m_movieName.c_str());
		return 1;
	}
	FILE* file = Platform_FOpen(m_movieName.c_str(), "rb");
	if (!file) {
		MYERROR::Log(::Error, "Movie open failed for '%s'; playback stopped", m_movieName.c_str());
		return 1;
	}
	if (!m_movie) m_movie = new MoviePlayer;
	if (!m_movie->Open(file, SDL_GetTicks())) {
		MYERROR::Log(::Error, "Movie open failed for '%s': %s", m_movieName.c_str(), m_movie->Error().c_str());
		m_movieErrorReported = true;
		return 1;
	}


	int outputWidth = 0, outputHeight = 0;
	SDL_GetRenderOutputSize(Platform_RenderRenderer(), &outputWidth, &outputHeight);
	const auto center = [](float edge, float logicalSize, int outputSize, int size) {
		const double pixels = double(edge) * outputSize / logicalSize;
		return std::isfinite(pixels) && pixels >= 0 && pixels <= 65536 ? (((int) pixels - size) >> 1) : 0;
	};
	m_movieX = center(m_viewXMax, m_width, outputWidth, 640);
	m_movieY = center(m_viewYMax, m_height, outputHeight, 480);
	m_movieActive = true;
	return 0;
}

void GRAPH_CORE::PollMovie(bool p_consumeCompletion)
{
	if (!m_movieActive) return;
	m_movie->Update(SDL_GetTicks());
	if (!m_movie->Error().empty() && !m_movieErrorReported) {
		MYERROR::Log(::Error, "Movie playback stopped for '%s': %s", m_movieName.c_str(), m_movie->Error().c_str());
		m_movieErrorReported = true;
	}
	if (!m_movie->Error().empty() || (p_consumeCompletion && !m_movie->IsPlaying())) m_movieActive = false;
}

void GRAPH_CORE::PresentMovieFrame()
{
	if (!m_movieActive) return;
	if (!Platform_RenderPresentMovie(m_movie->Pixels(), m_movie->Width(), m_movie->Height(), m_movieX, m_movieY)) {
		MYERROR::Log(::Error, "Movie presentation failed for '%s': %s", m_movieName.c_str(), SDL_GetError());
		StopMovie();
	}
}

void GRAPH_CORE::PresentIdleMovie()
{



	PollMovie(false);
	PresentMovieFrame();
}

int GRAPH_CORE::IsMoviePlaying()
{


	return m_movieActive;
}

void GRAPH_CORE::StopMovie()
{
	m_movieActive = false;
	if (m_movie) m_movie->Stop();
	Platform_RenderCloseMovie();
}

// FUNCTION: ALIEN 0x402400
int GRAPH_CORE::SetViewPort(float p_x1, float p_y1, float p_x2, float p_y2)
{
	m_viewXMin = p_x1;
	m_viewXMax = p_x2;
	m_viewYMin = p_y1;
	m_viewYMax = p_y2;
	return (int) m_viewYMax;
}

// FUNCTION: ALIEN 0x402600
void GRAPH_CORE::ClearScreen(COLOR p_color)
{
	int h = (int) m_height;
	if (m_color) {
		unsigned int* dst = (unsigned int*) m_color;
		for (int y = 0; y < h; ++y) {
			for (int x = 0; x < m_pitch; ++x) {
				dst[x] = p_color.m_value;
			}
			dst += m_pitch;
		}
	}

	int count = h * m_zpitch;
	unsigned short* zdst = (unsigned short*) m_zbuffer;
	for (int i = 0; i < count; ++i) {
		zdst[i] = 0x03ff;
	}
}

// FUNCTION: ALIEN 0x402690
void GRAPH_CORE::Effect(int p_effect, int p_a, int p_b, int p_duration)
{
	if (p_effect >= 16 || p_effect < 0) {
		return;
	}
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
		for (int i = 0; i < 16; i++) {
			m_effectStart[i] = 0;
		}
		return;
	}
	if (p_effect == 5) {
		if (!m_color) {
			return;
		}
		size_t bytes = (size_t) m_pitch * (size_t) (int) m_height * sizeof(unsigned int);
		void* screen = realloc(m_screen, bytes);
		if (!screen) {
			m_effectStart[5] = 0;
			return;
		}
		m_screen = screen;
		memcpy(m_screen, m_color, bytes);
		m_effectStart[10] = 0;
		m_effectStart[9] = 0;
		m_effectStart[3] = 0;
		return;
	}
	if (p_effect == 2) {
		// Start scripted camera pans from the current shift.
		m_shiftBaseX = Map->m_shiftX;
		m_shiftBaseY = Map->m_shiftY;
		return;
	}
	if (p_effect == 11) {
		m_gammaCur.m_a = m_gammaSet.m_a;
		m_gammaCur.m_b = m_gammaSet.m_b;
	}
}

// STUB: ALIEN 0x402820
int GRAPH_CORE::CopyToZBuffer(int* p_dst, int* p_src, void* p_texture)
{
	if (!p_dst || !p_src || !p_texture || !m_zbuffer) {
		return 0;
	}

	TEXTURE* texture = (TEXTURE*) p_texture;
	int frameWidth = (int) m_width;
	int frameHeight = (int) m_height;
	if (frameWidth <= 0 || frameHeight <= 0 || m_zpitch < frameWidth || texture->m_width <= 0 ||
		texture->m_height <= 0 || texture->m_format != D3DFMT_D16 || !texture->m_data || texture->m_pitch <= 0 ||
		(texture->m_pitch & 1) || texture->m_pitch / 2 < texture->m_width) {
		return 0;
	}

	RECT dstRect;
	RECT srcRect;
	memcpy(&dstRect, p_dst, sizeof(dstRect));
	memcpy(&srcRect, p_src, sizeof(srcRect));

	int clipLeft = (int) m_viewXMin;
	int clipTop = (int) m_viewYMin;
	int clipRight = (int) m_viewXMax;
	int clipBottom = (int) m_viewYMax;
	if (clipLeft < 0) {
		clipLeft = 0;
	}
	if (clipTop < 0) {
		clipTop = 0;
	}
	if (clipRight > frameWidth) {
		clipRight = frameWidth;
	}
	if (clipBottom > frameHeight) {
		clipBottom = frameHeight;
	}
	if (clipRight <= clipLeft || clipBottom <= clipTop) {
		return 0;
	}

	int64_t dstWidth = (int64_t) dstRect.right - dstRect.left;
	int64_t dstHeight = (int64_t) dstRect.bottom - dstRect.top;
	int64_t srcWidth = (int64_t) srcRect.right - srcRect.left;
	int64_t srcHeight = (int64_t) srcRect.bottom - srcRect.top;
	if (dstWidth <= 0 || dstHeight <= 0 || srcWidth <= 0 || srcHeight <= 0) {
		return 0;
	}

	int64_t xBegin = 0;
	int64_t candidate = (int64_t) clipLeft - dstRect.left;
	if (candidate > xBegin) {
		xBegin = candidate;
	}
	candidate = -(int64_t) srcRect.left;
	if (candidate > xBegin) {
		xBegin = candidate;
	}
	int64_t xEnd = dstWidth < srcWidth ? dstWidth : srcWidth;
	candidate = (int64_t) clipRight - dstRect.left;
	if (candidate < xEnd) {
		xEnd = candidate;
	}
	candidate = (int64_t) texture->m_width - srcRect.left;
	if (candidate < xEnd) {
		xEnd = candidate;
	}

	int64_t yBegin = 0;
	candidate = (int64_t) clipTop - dstRect.top;
	if (candidate > yBegin) {
		yBegin = candidate;
	}
	candidate = -(int64_t) srcRect.top;
	if (candidate > yBegin) {
		yBegin = candidate;
	}
	int64_t yEnd = dstHeight < srcHeight ? dstHeight : srcHeight;
	candidate = (int64_t) clipBottom - dstRect.top;
	if (candidate < yEnd) {
		yEnd = candidate;
	}
	candidate = (int64_t) texture->m_height - srcRect.top;
	if (candidate < yEnd) {
		yEnd = candidate;
	}

	if (xEnd <= xBegin || yEnd <= yBegin) {
		return 0;
	}

	int dstX = (int) ((int64_t) dstRect.left + xBegin);
	int dstY = (int) ((int64_t) dstRect.top + yBegin);
	int srcX = (int) ((int64_t) srcRect.left + xBegin);
	int srcY = (int) ((int64_t) srcRect.top + yBegin);
	int width = (int) (xEnd - xBegin);
	int height = (int) (yEnd - yBegin);
	RECT locked = {srcX, srcY, srcX + width, srcY + height};
	int srcPitch = 0;
	const unsigned char* src = (const unsigned char*) texture->Lock(&srcPitch, &locked);
	if (!src || srcPitch <= 0 || (srcPitch & 1) || srcPitch / 2 < texture->m_width) {
		return 0;
	}

	unsigned char* dst = (unsigned char*) m_zbuffer;
	for (int y = 0; y < height; ++y) {
		unsigned char* dstRow =
			dst + ((size_t) (dstY + y) * (size_t) m_zpitch + (size_t) dstX) * sizeof(unsigned short);
		const unsigned char* srcRow = src + (size_t) y * (size_t) srcPitch;
		memcpy(dstRow, srcRow, (size_t) width * sizeof(unsigned short));
	}
	return 0;
}

struct SHADOW_POINT {
	double m_x;
	double m_y;
	double m_z;
};

static SHADOW_POINT ReadShadowPoint(const unsigned char* p_bytes, unsigned int p_stride, int p_idx)
{
	float x;
	float y;
	float z;
	const unsigned char* vertex = p_bytes + (size_t) p_idx * p_stride;
	memcpy(&x, vertex, sizeof(x));
	memcpy(&y, vertex + sizeof(x), sizeof(y));
	memcpy(&z, vertex + 2 * sizeof(float), sizeof(z));
	SHADOW_POINT result = {x, y, z};
	return result;
}

static unsigned int ReadShadowDiffuse(const unsigned char* p_bytes)
{
	unsigned int diffuse;
	memcpy(&diffuse, p_bytes + 0x10, sizeof(diffuse));
	return diffuse;
}

inline static double ShadowEdge(const SHADOW_POINT& p_a, const SHADOW_POINT& p_b, double p_x, double p_y)
{
	return (p_b.m_x - p_a.m_x) * (p_y - p_a.m_y) - (p_b.m_y - p_a.m_y) * (p_x - p_a.m_x);
}

inline static int ShadowTopLeft(const SHADOW_POINT& p_a, const SHADOW_POINT& p_b)
{
	double dx = p_b.m_x - p_a.m_x;
	double dy = p_b.m_y - p_a.m_y;
	return dy < 0.0 || (dy == 0.0 && dx > 0.0);
}

inline static int ShadowInside(double p_edge, int p_topLeft)
{
	return p_edge > 0.0 || (p_edge == 0.0 && p_topLeft);
}

inline static unsigned int BlendShadowPixel(const RENDER_STATE& p_state, unsigned int p_src, unsigned int p_dst)
{
	unsigned int sa = p_src >> 24;
	unsigned int sr = (p_src >> 16) & 0xff;
	unsigned int sg = (p_src >> 8) & 0xff;
	unsigned int sb = p_src & 0xff;
	if (!p_state.m_alphaBlend) {
		return 0xff000000u | (sr << 16) | (sg << 8) | sb;
	}

	unsigned int da = p_dst >> 24;
	unsigned int dr = (p_dst >> 16) & 0xff;
	unsigned int dg = (p_dst >> 8) & 0xff;
	unsigned int db = p_dst & 0xff;
	unsigned int r = (sr * GraphBlendFactor(p_state.m_srcBlend, sr, dr, sa, da) +
					  dr * GraphBlendFactor(p_state.m_dstBlend, sr, dr, sa, da)) /
					 255;
	unsigned int g = (sg * GraphBlendFactor(p_state.m_srcBlend, sg, dg, sa, da) +
					  dg * GraphBlendFactor(p_state.m_dstBlend, sg, dg, sa, da)) /
					 255;
	unsigned int b = (sb * GraphBlendFactor(p_state.m_srcBlend, sb, db, sa, da) +
					  db * GraphBlendFactor(p_state.m_dstBlend, sb, db, sa, da)) /
					 255;
	if (r > 255) {
		r = 255;
	}
	if (g > 255) {
		g = 255;
	}
	if (b > 255) {
		b = 255;
	}
	return 0xff000000u | (r << 16) | (g << 8) | b;
}

static void DrawShadowTriangle(
	GRAPH_CORE* p_graph,
	SHADOW_POINT p_a,
	SHADOW_POINT p_b,
	SHADOW_POINT p_c,
	unsigned int p_color
)
{
	if (!std::isfinite(p_a.m_x) || !std::isfinite(p_a.m_y) || !std::isfinite(p_b.m_x) || !std::isfinite(p_b.m_y) ||
		!std::isfinite(p_c.m_x) || !std::isfinite(p_c.m_y)) {
		return;
	}




	const bool depthTest =
		GameDesc->m_layerRules == GAME_LAYERS_LOCOLAND && p_graph->m_state.m_zFunc == D3DCMP_GREATEREQUAL;
	if (depthTest && (!std::isfinite(p_a.m_z) || !std::isfinite(p_b.m_z) || !std::isfinite(p_c.m_z) ||
					  !p_graph->m_zbuffer || p_graph->m_zpitch < p_graph->m_width)) {
		return;
	}
	double area = ShadowEdge(p_a, p_b, p_c.m_x, p_c.m_y);
	if (area == 0.0) {
		return;
	}

	if ((p_graph->m_state.m_cull == D3DCULL_CW && area > 0.0) ||
		(p_graph->m_state.m_cull == D3DCULL_CCW && area < 0.0)) {
		return;
	}
	if (area < 0.0) {
		SHADOW_POINT t = p_b;
		p_b = p_c;
		p_c = t;
		area = -area;
	}

	double minX = p_a.m_x;
	double maxX = p_a.m_x;
	double minY = p_a.m_y;
	double maxY = p_a.m_y;
	if (p_b.m_x < minX) {
		minX = p_b.m_x;
	}
	if (p_c.m_x < minX) {
		minX = p_c.m_x;
	}
	if (p_b.m_x > maxX) {
		maxX = p_b.m_x;
	}
	if (p_c.m_x > maxX) {
		maxX = p_c.m_x;
	}
	if (p_b.m_y < minY) {
		minY = p_b.m_y;
	}
	if (p_c.m_y < minY) {
		minY = p_c.m_y;
	}
	if (p_b.m_y > maxY) {
		maxY = p_b.m_y;
	}
	if (p_c.m_y > maxY) {
		maxY = p_c.m_y;
	}

	// D3D's transformed screen coordinates put pixel centres at integers. The
	// candidate bounds are half-open; edge ownership decides shared samples.
	if (minX < p_graph->m_viewXMin) {
		minX = p_graph->m_viewXMin;
	}
	if (minY < p_graph->m_viewYMin) {
		minY = p_graph->m_viewYMin;
	}
	if (maxX > p_graph->m_viewXMax) {
		maxX = p_graph->m_viewXMax;
	}
	if (maxY > p_graph->m_viewYMax) {
		maxY = p_graph->m_viewYMax;
	}
	if (minX < 0.0) {
		minX = 0.0;
	}
	if (minY < 0.0) {
		minY = 0.0;
	}
	if (maxX > p_graph->m_width) {
		maxX = p_graph->m_width;
	}
	if (maxY > p_graph->m_height) {
		maxY = p_graph->m_height;
	}
	if (minX >= maxX || minY >= maxY) {
		return;
	}

	int x0 = (int) std::ceil(minX);
	int y0 = (int) std::ceil(minY);
	int x1 = (int) std::ceil(maxX);
	int y1 = (int) std::ceil(maxY);
	if (x0 >= x1 || y0 >= y1) {
		return;
	}

	int topLeft0 = ShadowTopLeft(p_a, p_b);
	int topLeft1 = ShadowTopLeft(p_b, p_c);
	int topLeft2 = ShadowTopLeft(p_c, p_a);
	double stepX0 = -(p_b.m_y - p_a.m_y);
	double stepX1 = -(p_c.m_y - p_b.m_y);
	double stepX2 = -(p_a.m_y - p_c.m_y);
	double stepY0 = p_b.m_x - p_a.m_x;
	double stepY1 = p_c.m_x - p_b.m_x;
	double stepY2 = p_a.m_x - p_c.m_x;
	double rowEdge0 = ShadowEdge(p_a, p_b, (double) x0, (double) y0);
	double rowEdge1 = ShadowEdge(p_b, p_c, (double) x0, (double) y0);
	double rowEdge2 = ShadowEdge(p_c, p_a, (double) x0, (double) y0);

	for (int y = y0; y < y1; ++y) {
		double edge0 = rowEdge0;
		double edge1 = rowEdge1;
		double edge2 = rowEdge2;
		unsigned int* dst = (unsigned int*) p_graph->m_color + (size_t) y * p_graph->m_pitch;
		for (int x = x0; x < x1; ++x) {
			if (ShadowInside(edge0, topLeft0) && ShadowInside(edge1, topLeft1) && ShadowInside(edge2, topLeft2)) {
				bool visible = true;
				if (depthTest) {
					const double z = (edge1 * p_a.m_z + edge2 * p_b.m_z + edge0 * p_c.m_z) / area;
					const auto fixedZ = (unsigned short) std::lround(std::clamp(z * 65536.0, 0.0, 65535.0));
					auto* depth = static_cast<unsigned short*>(p_graph->m_zbuffer) + (size_t) y * p_graph->m_zpitch + x;
					visible = fixedZ >= *depth;
					if (visible && p_graph->m_state.m_zWrite) {
						*depth = fixedZ;
					}
				}
				if (visible) {
					dst[x] = BlendShadowPixel(p_graph->m_state, p_color, dst[x]);
				}
			}
			edge0 += stepX0;
			edge1 += stepX1;
			edge2 += stepX2;
		}
		rowEdge0 += stepY0;
		rowEdge1 += stepY1;
		rowEdge2 += stepY2;
	}
}

// FUNCTION: ALIEN 0x402a20
int GRAPH_CORE::DrawPrimitive(int p_type, unsigned int p_fvf, void* p_verts, unsigned int p_stride, int p_count)
{
	const unsigned int shadowFvf = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR;
	if (p_type != D3DPT_TRIANGLESTRIP || p_fvf != shadowFvf || p_stride != 0x18 || !p_verts || p_count < 3 ||
		!m_color || m_pitch <= 0 || !std::isfinite(m_width) || !std::isfinite(m_height) || !std::isfinite(m_viewXMin) ||
		!std::isfinite(m_viewXMax) || !std::isfinite(m_viewYMin) || !std::isfinite(m_viewYMax) || m_width <= 0.0f ||
		m_height <= 0.0f || (double) m_width >= (double) INT_MAX || (double) m_height >= (double) INT_MAX ||
		(double) m_pitch < std::ceil((double) m_width)) {
		return 0;
	}

	const unsigned char* bytes = (const unsigned char*) p_verts;
	unsigned int color = ReadShadowDiffuse(bytes);
	for (int i = 0; i < p_count - 2; ++i) {
		int ia = i;
		int ib = i + 1;
		int ic = i + 2;
		if (i & 1) {
			ib = i + 2;
			ic = i + 1;
		}
		SHADOW_POINT pa = ReadShadowPoint(bytes, p_stride, ia);
		SHADOW_POINT pb = ReadShadowPoint(bytes, p_stride, ib);
		SHADOW_POINT pc = ReadShadowPoint(bytes, p_stride, ic);
		DrawShadowTriangle(this, pa, pb, pc, color);
	}
	return 0;
}

void GRAPH_CORE::ReloadPalettes()
{
	TEXTURE* tex = m_texE0C;
	if (!tex || tex->m_format != D3DFMT_P8) {
		return;
	}
	int width = tex->m_width;
	int height = tex->m_height;
	delete m_texE0C;
	m_texE0C = new TEXTURE(width, height, D3DFMT_P8, 0);
	if (!m_texE0C->m_data && ::Error) {
		MYERROR::Error(
			::Error,
			"GRAPH",
			3,
			// STRING: ALIEN 0x483f2c
			"Light at RelodPalette()",
			0
		);
	}
	if (m_texE0C->m_format == D3DFMT_P8) {
		unsigned int palette[256];
		for (int i = 0; i < 256; ++i) {
			palette[i] = COLOR(i, i, i).m_value;
		}
		m_texE0C->SetPalette(palette);
	}
	MYERROR::Log(
		::Error,
		// STRING: ALIEN 0x483f1c
		"ReloadPalettes"
	);
}

// FUNCTION: ALIEN 0x402bc0
int GRAPH_CORE::CreateDebugFont(const STRING& p_name, int p_height, int p_flags)
{
	if (m_font) {
		delete m_font;
	}
	m_font = new DEBUG_FONT(p_name.m_str, p_height, p_flags);
	m_debugFontHeight = p_height;
	return 0;
}

// FUNCTION: ALIEN 0x402c40
void GRAPH_CORE::PutsXY(float p_x, float p_y, const char* p_text, COLOR p_color)
{
	if (m_font) {
		m_font->DrawDebugText(p_x, p_y, p_color.m_value, p_text, 0);
	}
}

// FUNCTION: ALIEN 0x430780
int GRAPH_CORE::SetAlphaBlend(unsigned int p_src, unsigned int p_dst)
{
	m_state.m_alphaBlend = 1;
	m_flags |= 0x4000;
	m_state.m_srcBlend = (int) p_src;
	m_state.m_dstBlend = (int) p_dst;
	return 0;
}

// FUNCTION: ALIEN 0x4307e0
int GRAPH_CORE::SetRenderState(int p_state, unsigned int p_value)
{
	switch (p_state) {
	case D3DRS_ZENABLE:

		return 0;
	case D3DRS_ZWRITEENABLE:
		m_state.m_zWrite = p_value != 0;
		return 0;
	case D3DRS_ZFUNC:
		m_state.m_zFunc = (int) p_value;
		return 0;
	case D3DRS_ALPHABLENDENABLE:
		m_state.m_alphaBlend = p_value != 0;
		m_flags = (((p_value != 0) & 1) << 14) | (m_flags & 0xffffbfff);
		return 0;
	case D3DRS_SPECULARENABLE:
		m_state.m_specular = p_value != 0;
		m_flags = (((p_value != 0) & 1) << 13) | (m_flags & 0xffffdfff);
		return 0;
	case D3DRS_SRCBLEND:
		m_state.m_srcBlend = (int) p_value;
		return 0;
	case D3DRS_DESTBLEND:
		m_state.m_dstBlend = (int) p_value;
		return 0;
	case D3DRS_CULLMODE:
		m_state.m_cull = (int) p_value;
		return 0;
	case D3DRS_ALPHATESTENABLE:
		m_state.m_alphaTest = p_value != 0;
		return 0;
	case D3DRS_ALPHAFUNC:
		m_state.m_alphaFunc = (int) p_value;
		return 0;
	case D3DRS_ALPHAREF:
		m_state.m_alphaRef = (int) p_value;
		return 0;
	case D3DRS_TEXTUREFACTOR:
		m_state.m_textureFactor = p_value;
		return 0;
	default:
		return 0;
	}
}

int GRAPH_CORE::SetTextureStageState(int p_state, unsigned int p_value)
{
	switch (p_state) {
	case D3DTSS_MAGFILTER:
		m_state.m_magFilter = (int) p_value;
		return 0;
	case D3DTSS_MINFILTER:
		m_state.m_minFilter = (int) p_value;
		return 0;
	case D3DTSS_COLOROP:
		m_state.m_colorOp = (int) p_value;
		return 0;
	case D3DTSS_COLORARG1:
		m_state.m_colorArg1 = (int) p_value;
		return 0;
	case D3DTSS_COLORARG2:
		m_state.m_colorArg2 = (int) p_value;
		return 0;
	case D3DTSS_ALPHAOP:
		m_state.m_alphaOp = (int) p_value;
		return 0;
	case D3DTSS_ALPHAARG1:
		m_state.m_alphaArg1 = (int) p_value;
		return 0;
	case D3DTSS_ALPHAARG2:
		m_state.m_alphaArg2 = (int) p_value;
		return 0;
	default:
		return 0;
	}
}

// FUNCTION: ALIEN 0x430d20
int GRAPH_CORE::Lock()
{
	return m_pitch;
}

// FUNCTION: ALIEN 0x430da0
int GRAPH_CORE::Error(int p_type, const char* p_msg, int p_size) const
{
	MYERROR* handler = ::Error;
	int result = 0;
	if (handler) {
		result = MYERROR::Error(handler, "GRAPH", p_type, p_msg, p_size);
	}
	return result;
}

// FUNCTION: ALIEN 0x433660
void GRAPH_CORE::PutsXY(float p_x, float p_y, const STRING& p_text, COLOR p_color)
{
	PutsXY(p_x, p_y, (char*) (const char*) p_text, p_color);
}

// FUNCTION: ALIEN 0x430ed0
int GRAPH_CORE::Pause()
{
	m_flags |= 1u;
	PostTact(1);


	if (m_movie) m_movie->Pause(SDL_GetTicks());
	return 0;
}

// FUNCTION: ALIEN 0x433660
int GRAPH_CORE::Resume()
{
	m_flags &= ~1u;
	if (m_movie) m_movie->Resume(SDL_GetTicks());
	return 0;
}

int GRAPH_CORE::FillRect(float p_x1, float p_y1, float p_x2, float p_y2, unsigned int p_color, unsigned int p_specular)
{
	if (!m_color) {
		return 0;
	}

	int x0 = (int) p_x1;
	int y0 = (int) p_y1;
	int x1 = (int) p_x2;
	int y1 = (int) p_y2;
	if (x0 > x1) {
		int t = x0;
		x0 = x1;
		x1 = t;
	}
	if (y0 > y1) {
		int t = y0;
		y0 = y1;
		y1 = t;
	}
	if (x0 < (int) m_viewXMin) {
		x0 = (int) m_viewXMin;
	}
	if (y0 < (int) m_viewYMin) {
		y0 = (int) m_viewYMin;
	}
	if (x1 > (int) m_viewXMax) {
		x1 = (int) m_viewXMax;
	}
	if (y1 > (int) m_viewYMax) {
		y1 = (int) m_viewYMax;
	}
	if (x0 >= x1 || y0 >= y1) {
		return 0;
	}

	// Specular adds RGB without changing alpha.
	unsigned int sa = p_color >> 24;
	unsigned int sr = (p_color >> 16) & 0xff;
	unsigned int sg = (p_color >> 8) & 0xff;
	unsigned int sb = p_color & 0xff;
	if (m_state.m_specular) {
		sr += (p_specular >> 16) & 0xff;
		sg += (p_specular >> 8) & 0xff;
		sb += p_specular & 0xff;
		if (sr > 255) {
			sr = 255;
		}
		if (sg > 255) {
			sg = 255;
		}
		if (sb > 255) {
			sb = 255;
		}
	}

	unsigned int* row = (unsigned int*) m_color + (size_t) y0 * m_pitch;

	if (!m_state.m_alphaBlend) {
		unsigned int out = 0xff000000u | (sr << 16) | (sg << 8) | sb;
		for (int y = y0; y < y1; ++y) {
			for (int x = x0; x < x1; ++x) {
				row[x] = out;
			}
			row += m_pitch;
		}
		return 0;
	}

	int srcBlend = m_state.m_srcBlend;
	int dstBlend = m_state.m_dstBlend;
	for (int y = y0; y < y1; ++y) {
		for (int x = x0; x < x1; ++x) {
			unsigned int d = row[x];
			unsigned int da = d >> 24;
			unsigned int dr = (d >> 16) & 0xff;
			unsigned int dg = (d >> 8) & 0xff;
			unsigned int db = d & 0xff;

			unsigned int r =
				(sr * GraphBlendFactor(srcBlend, sr, dr, sa, da) + dr * GraphBlendFactor(dstBlend, sr, dr, sa, da)) /
				255;
			unsigned int g =
				(sg * GraphBlendFactor(srcBlend, sg, dg, sa, da) + dg * GraphBlendFactor(dstBlend, sg, dg, sa, da)) /
				255;
			unsigned int b =
				(sb * GraphBlendFactor(srcBlend, sb, db, sa, da) + db * GraphBlendFactor(dstBlend, sb, db, sa, da)) /
				255;
			if (r > 255) {
				r = 255;
			}
			if (g > 255) {
				g = 255;
			}
			if (b > 255) {
				b = 255;
			}
			row[x] = 0xff000000u | (r << 16) | (g << 8) | b;
		}
		row += m_pitch;
	}
	return 0;
}

// FUNCTION: ALIEN 0x4342b0
int GRAPH_CORE::Bar(float p_x1, float p_y1, float p_x2, float p_y2, COLOR p_color)
{
	int specular = m_state.m_specular;
	m_state.m_specular = 0;
	if ((p_color.m_value & 0xff000000) == 0xff000000) {
		SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
	}
	else {
		SetAlphaBlend(D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
	}
	FillRect(p_x1, p_y1, p_x2, p_y2, p_color.m_value, 0xffffffff);
	m_state.m_specular = specular;
	return 0;
}

// FUNCTION: ALIEN 0x434420
void GRAPH_CORE::PrintfXY(GRAPH_CORE* p_graph, float p_x, float p_y, const char* p_format, ...)
{
	char buf[0x400];
	va_list args;
	va_start(args, p_format);
	vsnprintf(buf, sizeof(buf), p_format, args);
	va_end(args);
	p_graph->PutsXY(p_x, p_y, buf, GREEN);
}

// FUNCTION: ALIEN 0x40b080
float GRAPH_CORE::GetWidth()
{
	return m_width;
}

// FUNCTION: ALIEN 0x40b090
float GRAPH_CORE::GetHeight()
{
	return m_height;
}

// FUNCTION: ALIEN 0x42e690
float GRAPH_CORE::GetViewYMin()
{
	return m_viewYMin;
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

// GLOBAL: ALIEN 0x483d28
int RGB16_rMask = 0xf800;

// GLOBAL: ALIEN 0x483d2c
int RGB16_gMask = 0x7e0;

// FUNCTION: ALIEN 0x41eba0
void GRAPH::PutPixel(float p_x, float p_y, COLOR p_color)
{
	if (p_x >= m_viewXMin && p_x < m_viewXMax && p_y >= m_viewYMin && p_y < m_viewYMax) {
		GRAPH_CORE* g = (GRAPH_CORE*) this;
		if (!g->m_color) {
			return;
		}
		((unsigned int*) g->m_color)[(int) p_x + (int) p_y * g->m_pitch] = p_color.m_value;
	}
}

// FUNCTION: ALIEN 0x41ed10
void GRAPH::PutBigPixel(float p_x, float p_y, COLOR p_color)
{
	GRAPH_CORE* g = (GRAPH_CORE*) this;
	if (!g->m_color) {
		return;
	}
	if (p_x >= m_viewXMin && p_y >= m_viewYMin && m_viewXMax - 1.0f > p_x && m_viewYMax - 1.0f > p_y) {
		int px = (int) p_x;
		int py = (int) p_y;
		unsigned int* pixels = (unsigned int*) g->m_color;
		pixels[px + py * g->m_pitch] = p_color.m_value;
		pixels[px + 1 + py * g->m_pitch] = p_color.m_value;
		pixels[px + (py + 1) * g->m_pitch] = p_color.m_value;
		pixels[px + 1 + (py + 1) * g->m_pitch] = p_color.m_value;
	}
}
