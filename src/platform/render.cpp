#include "platform/render.h"

#include "game/settings.h"
#include "gfx/gpu_backend.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <stdint.h>
#include <stdlib.h>
#include <string>

static SDL_Window* s_window;
static SDL_Renderer* s_renderer;
static SDL_GPUDevice* s_device;
static SDL_Texture* s_texture;
static SDL_Texture* s_movieTexture;
static int s_movieWidth, s_movieHeight;
static bool s_hasPresented;
static unsigned int* s_pixels;
static int s_width;
static int s_height;
static bool s_fitAutomaticWindow;
static bool s_failed;
static std::string s_error;

static bool RenderFailure(const char* p_operation, const char* p_detail = nullptr)
{
	if (!s_failed) {
		const char* detail = p_detail && *p_detail ? p_detail : SDL_GetError();
		s_error = p_operation;
		if (detail && *detail) {
			s_error += std::string(": ") + detail;
		}
		SDL_LogError(SDL_LOG_CATEGORY_RENDER, "%s", s_error.c_str());
	}
	s_failed = true;
	SDL_SetError("%s", s_error.c_str());
	return false;
}

static std::string CurrentRenderError()
{
	const char* gpuError = s_device ? GPU_RENDER::Error() : nullptr;
	if (gpuError && *gpuError) {
		return gpuError;
	}
	const char* error = SDL_GetError();
	return error && *error ? error : "Renderer initialization failed";
}

static void SetPortableWindowIcon()
{
#if !defined(_WIN32) && !defined(__APPLE__)
	const char* basePath = SDL_GetBasePath();
	if (!s_window || !basePath) {
		SDL_ClearError();
		return;
	}

	std::string iconPath(basePath);
	iconPath += "OpenGromada.png";
	SDL_Surface* icon = SDL_LoadPNG(iconPath.c_str());
	if (!icon) {
		SDL_ClearError();
		return;
	}
	if (!SDL_SetWindowIcon(s_window, icon)) {
		SDL_ClearError();
	}
	SDL_DestroySurface(icon);
#endif
}

static bool IsFullscreen()
{
	return s_window && (SDL_GetWindowFlags(s_window) & SDL_WINDOW_FULLSCREEN);
}

static SDL_DisplayID WindowDisplay()
{
	SDL_DisplayID display = s_window ? SDL_GetDisplayForWindow(s_window) : 0;
	return display ? display : SDL_GetPrimaryDisplay();
}

static bool WindowUsableBounds(SDL_DisplayID p_display, SDL_Rect* p_bounds)
{
	return p_display && SDL_GetDisplayUsableBounds(p_display, p_bounds) && p_bounds->w > 0 && p_bounds->h > 0;
}

static bool IsDummyVideo()
{
	const char* driver = SDL_GetCurrentVideoDriver();
	return driver && SDL_strcasecmp(driver, "dummy") == 0;
}

static void WindowBorders(int* p_top, int* p_left, int* p_bottom, int* p_right)
{
	*p_top = 0;
	*p_left = 0;
	*p_bottom = 0;
	*p_right = 0;
	if (s_window) {
		SDL_GetWindowBordersSize(s_window, p_top, p_left, p_bottom, p_right);
	}
}

static void FitWindowToUsableBounds(SDL_DisplayID p_display)
{
	if (!s_window || IsFullscreen() || IsDummyVideo()) {
		return;
	}

	SDL_SyncWindow(s_window);
	SDL_Rect usable;
	if (!WindowUsableBounds(p_display ? p_display : WindowDisplay(), &usable)) {
		return;
	}

	int top;
	int left;
	int bottom;
	int right;
	WindowBorders(&top, &left, &bottom, &right);
	const int maxWidth = usable.w - left - right;
	const int maxHeight = usable.h - top - bottom;
	if (maxWidth <= 0 || maxHeight <= 0) {
		return;
	}

	int width;
	int height;
	if (!SDL_GetWindowSize(s_window, &width, &height) || width <= 0 || height <= 0 ||
		(width <= maxWidth && height <= maxHeight)) {
		return;
	}

	const double scale = std::min((double) maxWidth / width, (double) maxHeight / height);
	const int fittedWidth = std::max(1, (int) std::floor(width * scale));
	const int fittedHeight = std::max(1, (int) std::floor(height * scale));
	SDL_SetWindowSize(s_window, fittedWidth, fittedHeight);
	SDL_SyncWindow(s_window);
}

struct DEBUG_TEXT {
	float m_x;
	float m_y;
	unsigned int m_color;
	int m_height;
	char m_text[128];
};

enum {
	DEBUG_TEXT_MAX = 64
};
static DEBUG_TEXT s_debugText[DEBUG_TEXT_MAX];
static int s_debugTextCount;

static SDL_Texture* CreateTexture(int p_width, int p_height)
{
	SDL_Texture* texture =
		SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, p_width, p_height);
	if (!texture) {
		return 0;
	}

	if (!SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST) ||
		!SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE)) {
		SDL_DestroyTexture(texture);
		return nullptr;
	}
	return texture;
}

static int ReplaceLogicalTargets(int p_width, int p_height)
{
	if (!s_renderer || p_width <= 0 || p_height <= 0 || (size_t) p_width > SIZE_MAX / (size_t) p_height ||
		(size_t) p_width * (size_t) p_height > SIZE_MAX / sizeof(unsigned int)) {
		return 1;
	}
	SDL_Texture* texture = s_device ? nullptr : CreateTexture(p_width, p_height);
	if (!s_device && !texture) {
		return 1;
	}

	unsigned int* pixels =
		s_device ? nullptr : (unsigned int*) calloc((size_t) p_width * p_height, sizeof(unsigned int));
	if (!s_device && !pixels) {
		SDL_DestroyTexture(texture);
		SDL_OutOfMemory();
		return 1;
	}
	if (!SDL_SetRenderLogicalPresentation(s_renderer, p_width, p_height, SDL_LOGICAL_PRESENTATION_STRETCH)) {
		free(pixels);
		SDL_DestroyTexture(texture);
		return 1;
	}
	if (s_device && !(GPU_RENDER::Active() ? GPU_RENDER::Resize(p_width, p_height)
										   : GPU_RENDER::Open(s_renderer, p_width, p_height))) {
		const std::string error = CurrentRenderError();
		if (s_width > 0 && s_height > 0) {
			SDL_SetRenderLogicalPresentation(s_renderer, s_width, s_height, SDL_LOGICAL_PRESENTATION_STRETCH);
		}
		SDL_DestroyTexture(texture);
		SDL_SetError("%s", error.c_str());
		return 1;
	}
	if (s_device) {
		texture = GPU_RENDER::OutputTexture(false);
	}

	if (s_texture && !s_device) {
		SDL_DestroyTexture(s_texture);
	}
	free(s_pixels);

	s_texture = texture;
	s_hasPresented = false;
	s_pixels = pixels;
	s_width = p_width;
	s_height = p_height;

	return 0;
}

static int OpenRenderer(const char* p_title,
						int p_outputWidth,
						int p_outputHeight,
						int p_frameWidth,
						int p_frameHeight,
						int p_fullscreen,
						int p_fitAutomaticWindow,
						unsigned int p_display,
						bool p_gpu)
{
	SDL_WindowFlags flags = p_fullscreen ? SDL_WINDOW_FULLSCREEN : 0;
	s_window = SDL_CreateWindow(p_title, p_outputWidth, p_outputHeight, flags);
	if (!s_window) {
		return 1;
	}
	if (p_gpu) {
		s_device =
			SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL,
								false,
								Settings_GPUDriver());
		if (!s_device) {
			return 1;
		}
		s_renderer = SDL_CreateGPURenderer(s_device, s_window);
	}
	else {
		s_renderer = SDL_CreateRenderer(s_window, "software");
	}
	if (!s_renderer) {
		return 1;
	}
	SetPortableWindowIcon();
	s_fitAutomaticWindow = p_fitAutomaticWindow != 0;
	if (p_display) {
		SDL_SetWindowPosition(s_window,
							  SDL_WINDOWPOS_CENTERED_DISPLAY(p_display),
							  SDL_WINDOWPOS_CENTERED_DISPLAY(p_display));
		SDL_SyncWindow(s_window);
	}
	if (s_fitAutomaticWindow) {
		FitWindowToUsableBounds(p_display);
	}
	if (ReplaceLogicalTargets(p_frameWidth, p_frameHeight)) {
		return 1;
	}
	if (!SDL_StartTextInput(s_window)) {
		return 1;
	}
	return 0;
}

int Platform_RenderOpen(const char* p_title,
						int p_outputWidth,
						int p_outputHeight,
						int p_frameWidth,
						int p_frameHeight,
						int p_fullscreen,
						int p_fitAutomaticWindow,
						unsigned int p_display)
{
	if (s_window) {
		return s_failed ? 1 : 0;
	}
	s_failed = false;
	s_error.clear();
	if (p_outputWidth <= 0 || p_outputHeight <= 0 || p_frameWidth <= 0 || p_frameHeight <= 0) {
		RenderFailure("Invalid render dimensions");
		return 1;
	}

	if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
		RenderFailure("SDL video initialization failed");
		return 1;
	}
	const bool gpu = Settings_Renderer() != SETTINGS_RENDERER_SOFTWARE;
	std::string fallback;
	if (OpenRenderer(p_title,
					 p_outputWidth,
					 p_outputHeight,
					 p_frameWidth,
					 p_frameHeight,
					 p_fullscreen,
					 p_fitAutomaticWindow,
					 p_display,
					 gpu)) {
		const std::string error = CurrentRenderError();
		Platform_RenderClose();
		if (!gpu || Settings_Renderer() == SETTINGS_RENDERER_GPU) {
			RenderFailure(gpu ? "GPU renderer startup failed" : "Software renderer startup failed", error.c_str());
			return 1;
		}
		fallback = error;
		SDL_LogWarn(SDL_LOG_CATEGORY_RENDER,
					"GPU renderer unavailable; falling back to software: %s",
					fallback.c_str());
		SDL_ClearError();
		if (OpenRenderer(p_title,
						 p_outputWidth,
						 p_outputHeight,
						 p_frameWidth,
						 p_frameHeight,
						 p_fullscreen,
						 p_fitAutomaticWindow,
						 p_display,
						 false)) {
			const std::string softwareError = CurrentRenderError();
			Platform_RenderClose();
			RenderFailure("Software renderer fallback failed", softwareError.c_str());
			return 1;
		}
	}
	int outputWidth = 0, outputHeight = 0;
	Platform_RenderOutputSize(&outputWidth, &outputHeight);
	SDL_Log("Renderer selected: %s; driver=%s; requested=%s; logical=%dx%d; output=%dx%d%s%s",
			Platform_RenderBackendName(),
			Platform_RenderDriverName(),
			Settings_RendererName(),
			s_width,
			s_height,
			outputWidth,
			outputHeight,
			fallback.empty() ? "" : "; fallback=",
			fallback.c_str());
	return 0;
}

void Platform_RenderClose()
{
	Platform_RenderCloseMovie();
	s_hasPresented = false;
	if (s_window && SDL_TextInputActive(s_window)) {
		SDL_StopTextInput(s_window);
	}
	if (s_renderer) {
		SDL_FlushRenderer(s_renderer);
	}
	GPU_RENDER::Close();
	if (s_texture && !s_device) {
		SDL_DestroyTexture(s_texture);
	}
	s_texture = 0;
	free(s_pixels);
	s_pixels = 0;
	if (s_renderer) {
		SDL_DestroyRenderer(s_renderer);
		s_renderer = 0;
	}
	if (s_device) {
		SDL_DestroyGPUDevice(s_device);
		s_device = nullptr;
	}
	if (s_window) {
		SDL_DestroyWindow(s_window);
		s_window = 0;
	}
	s_width = 0;
	s_height = 0;
	s_fitAutomaticWindow = false;
	s_debugTextCount = 0;
}

unsigned int* Platform_RenderPixels()
{
	return s_pixels;
}

int Platform_RenderPitch()
{
	return s_width;
}

int Platform_RenderWidth()
{
	return s_width;
}

int Platform_RenderHeight()
{
	return s_height;
}

int Platform_RenderResizeLogical(int p_width, int p_height)
{
	if (s_failed || !s_window || !s_renderer || (!s_device && !s_pixels)) {
		return 1;
	}
	if (p_width == s_width && p_height == s_height) {
		return 0;
	}
	return ReplaceLogicalTargets(p_width, p_height);
}

void Platform_RenderPresent()
{
	if (s_failed || !s_renderer || !s_texture) {
		return;
	}

	static int s_profile = -1;
	if (s_profile < 0) {
		const char* env = SDL_getenv("ALIEN_FRAME_PROFILE");
		s_profile = env && *env && *env != '0';
	}
	static Uint64 s_lastPresentNs, s_gameNs, s_updateNs, s_drawNs, s_presentNs;
	static int s_frames;
	Uint64 t0 = SDL_GetTicksNS();
	if (s_profile && s_lastPresentNs) {
		s_gameNs += t0 - s_lastPresentNs;
	}

	if (s_device) {
		s_texture = GPU_RENDER::OutputTexture();
		if (!s_texture) {
			RenderFailure("GPU output export failed", GPU_RENDER::Error());
			return;
		}
	}
	else if (!SDL_UpdateTexture(s_texture, 0, s_pixels, s_width * (int) sizeof(unsigned int))) {
		RenderFailure("Software frame upload failed");
		return;
	}
	Uint64 t1 = s_profile ? SDL_GetTicksNS() : 0;

	if (!SDL_SetRenderDrawColor(s_renderer, 0, 0, 0, 255) || !SDL_RenderClear(s_renderer) ||
		!SDL_RenderTexture(s_renderer, s_texture, 0, 0)) {
		RenderFailure("Frame composition failed");
		return;
	}

	for (int i = 0; i < s_debugTextCount; ++i) {
		const DEBUG_TEXT& t = s_debugText[i];
		float scale = (float) t.m_height / (float) SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE;
		if (scale < 1.0f) {
			scale = 1.0f;
		}
		if (!SDL_SetRenderScale(s_renderer, scale, scale) ||
			!SDL_SetRenderDrawColor(s_renderer,
									(t.m_color >> 16) & 0xff,
									(t.m_color >> 8) & 0xff,
									t.m_color & 0xff,
									(t.m_color >> 24) & 0xff) ||
			!SDL_RenderDebugText(s_renderer, t.m_x / scale, t.m_y / scale, t.m_text)) {
			SDL_SetRenderScale(s_renderer, 1.0f, 1.0f);
			RenderFailure("Debug text presentation failed");
			return;
		}
	}
	SDL_SetRenderScale(s_renderer, 1.0f, 1.0f);
	s_debugTextCount = 0;

	static int s_dump = -1;
	static int s_dumpFrame;
	if (s_dump < 0) {
		const char* env = SDL_getenv("ALIEN_FRAME_DUMP");
		s_dump = env && *env ? 1 : 0;
	}
	if (s_dump) {
		++s_dumpFrame;
		if (s_dumpFrame % 300 == 0) {
			SDL_Surface* shot =
				s_device ? SDL_CreateSurface(s_width, s_height, SDL_PIXELFORMAT_XRGB8888)
						 : SDL_CreateSurfaceFrom(s_width, s_height, SDL_PIXELFORMAT_XRGB8888, s_pixels, s_width * 4);
			if (shot) {
				if (s_device && !GPU_RENDER::ReadColor(0,
													   0,
													   s_width,
													   s_height,
													   static_cast<uint32_t*>(shot->pixels),
													   shot->pitch / 4)) {
					SDL_DestroySurface(shot);
					RenderFailure("GPU frame capture failed", GPU_RENDER::Error());
					return;
				}
				char path[512];
				SDL_snprintf(path, sizeof(path), "%s/frame%04d.bmp", SDL_getenv("ALIEN_FRAME_DUMP"), s_dumpFrame);
				SDL_SaveBMP(shot, path);
				SDL_DestroySurface(shot);
			}
		}
	}

	Uint64 t2 = s_profile ? SDL_GetTicksNS() : 0;
	if (!SDL_RenderPresent(s_renderer)) {
		RenderFailure("Frame presentation failed");
		return;
	}
	s_hasPresented = true;
	if (s_profile) {
		Uint64 t3 = SDL_GetTicksNS();
		s_updateNs += t1 - t0;
		s_drawNs += t2 - t1;
		s_presentNs += t3 - t2;
		s_lastPresentNs = t3;
		if (++s_frames >= 120) {
			SDL_Log("frame profile [%s]: game+draw %.1fms, upload/submit %.1fms, composition/capture %.1fms, present "
					"%.1fms (%dx%d)",
					Platform_RenderBackendName(),
					(double) s_gameNs / s_frames * 1e-6,
					(double) s_updateNs / s_frames * 1e-6,
					(double) s_drawNs / s_frames * 1e-6,
					(double) s_presentNs / s_frames * 1e-6,
					s_width,
					s_height);
			s_frames = 0;
			s_gameNs = s_updateNs = s_drawNs = s_presentNs = 0;
		}
	}
}

bool Platform_RenderFailed()
{
	if (!s_failed && s_device) {
		const char* error = GPU_RENDER::Error();
		if (error && *error) {
			RenderFailure("GPU rendering failed", error);
		}
	}
	return s_failed;
}

const char* Platform_RenderError()
{
	return s_error.c_str();
}

const char* Platform_RenderBackendName()
{
	return !s_renderer ? "none" : s_device ? "gpu" : "software";
}

const char* Platform_RenderDriverName()
{
	const char* name = s_device     ? SDL_GetGPUDeviceDriver(s_device)
					   : s_renderer ? SDL_GetRendererName(s_renderer)
									: nullptr;
	return name ? name : "none";
}

void Platform_RenderCloseMovie()
{
	SDL_DestroyTexture(s_movieTexture);
	s_movieTexture = nullptr;
	s_movieWidth = s_movieHeight = 0;
}

bool Platform_RenderPresentMovie(const unsigned int* p_pixels, int p_width, int p_height, int p_x, int p_y)
{
	if (s_failed || !s_renderer || !s_texture) {
		return false;
	}

	int logicalWidth = 0, logicalHeight = 0;
	SDL_RendererLogicalPresentation mode;
	if (!SDL_GetRenderLogicalPresentation(s_renderer, &logicalWidth, &logicalHeight, &mode) ||
		!SDL_SetRenderLogicalPresentation(s_renderer, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED)) {
		return RenderFailure("Movie coordinate setup failed");
	}
	struct RestorePresentation {
		int w, h;
		SDL_RendererLogicalPresentation mode;
		~RestorePresentation() { SDL_SetRenderLogicalPresentation(s_renderer, w, h, mode); }
	} restore{logicalWidth, logicalHeight, mode};

	if (!SDL_SetRenderDrawColor(s_renderer, 0, 0, 0, 255) || !SDL_RenderClear(s_renderer)) {
		return RenderFailure("Movie background clear failed");
	}
	if (s_hasPresented && !SDL_RenderTexture(s_renderer, s_texture, nullptr, nullptr)) {
		return RenderFailure("Movie background restore failed");
	}
	const SDL_FRect rect{(float) p_x, (float) p_y, 640.0f, 480.0f};
	if (!SDL_RenderFillRect(s_renderer, &rect)) {
		return RenderFailure("Movie rectangle draw failed");
	}
	if (p_pixels) {
		if (p_width <= 0 || p_height <= 0 || p_width > 1920 || p_height > 1080) {
			return false;
		}
		if (!s_movieTexture || s_movieWidth != p_width || s_movieHeight != p_height) {
			Platform_RenderCloseMovie();
			s_movieTexture =
				SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, p_width, p_height);
			if (!s_movieTexture) {
				return RenderFailure("Movie texture allocation failed");
			}
			if (!SDL_SetTextureScaleMode(s_movieTexture, SDL_SCALEMODE_LINEAR) ||
				!SDL_SetTextureBlendMode(s_movieTexture, SDL_BLENDMODE_NONE)) {
				return RenderFailure("Movie texture state failed");
			}
			s_movieWidth = p_width;
			s_movieHeight = p_height;
		}
		if (!SDL_UpdateTexture(s_movieTexture, nullptr, p_pixels, p_width * 4) ||
			!SDL_RenderTexture(s_renderer, s_movieTexture, nullptr, &rect)) {
			return RenderFailure("Movie frame draw failed");
		}
	}
	s_debugTextCount = 0;
	return SDL_RenderPresent(s_renderer) || RenderFailure("Movie frame presentation failed");
}

int Platform_RenderHandleDeviceReset()
{
	Platform_RenderCloseMovie();
	const bool hadPresented = s_hasPresented;
	s_hasPresented = false;
	if (s_failed || !s_renderer || (!s_device && !s_pixels) || s_width <= 0 || s_height <= 0) {
		RenderFailure("Renderer reset cannot recover invalid targets");
		return 1;
	}
	SDL_Texture* texture = s_device ? nullptr : CreateTexture(s_width, s_height);
	if (!s_device && !texture) {
		RenderFailure("Renderer reset target allocation failed");
		return 1;
	}
	if (!SDL_SetRenderLogicalPresentation(s_renderer, s_width, s_height, SDL_LOGICAL_PRESENTATION_STRETCH)) {
		SDL_DestroyTexture(texture);
		RenderFailure("Renderer reset logical presentation failed");
		return 1;
	}
	if (s_device && !GPU_RENDER::Recreate()) {
		SDL_DestroyTexture(texture);
		RenderFailure("GPU renderer reset failed", GPU_RENDER::Error());
		return 1;
	}
	if (s_device) {
		texture = GPU_RENDER::OutputTexture(false);
		s_hasPresented = hadPresented;
	}
	if (s_texture && !s_device) {
		SDL_DestroyTexture(s_texture);
	}
	s_texture = texture;
	return 0;
}

void Platform_RenderHandleDeviceLoss()
{
	RenderFailure("Graphics device was lost and cannot be recovered",
				  "Restart the engine after restoring the graphics device");
}

void Platform_RenderSetFullscreen(int p_fullscreen)
{
	if (s_window) {
		SDL_SetWindowFullscreen(s_window, p_fullscreen != 0);
	}
}

void Platform_RenderSetVSync(int p_vsync)
{
	if (s_renderer) {
		if (!SDL_SetRenderVSync(s_renderer, p_vsync ? 1 : SDL_RENDERER_VSYNC_DISABLED)) {
			SDL_LogWarn(SDL_LOG_CATEGORY_RENDER,
						"Renderer vsync request %i was not applied: %s",
						p_vsync,
						SDL_GetError());
		}
		int applied = 0;
		if (SDL_GetRenderVSync(s_renderer, &applied)) {
			SDL_Log("Renderer vsync: requested=%i, applied=%i", p_vsync != 0, applied);
		}
	}
}

SDL_Window* Platform_RenderWindow()
{
	return s_window;
}

SDL_Renderer* Platform_RenderRenderer()
{
	return s_renderer;
}

bool Platform_RenderConvertEvent(SDL_Event* p_event)
{
	return s_renderer && p_event && SDL_ConvertEventToRenderCoordinates(s_renderer, p_event);
}

bool Platform_RenderOutputSize(int* p_width, int* p_height)
{
	if (p_width) {
		*p_width = 0;
	}
	if (p_height) {
		*p_height = 0;
	}
	return s_renderer && p_width && p_height && SDL_GetRenderOutputSize(s_renderer, p_width, p_height);
}

void Platform_RenderRestoreWindowPosition(int p_x, int p_y)
{
	if (!s_window || IsFullscreen()) {
		return;
	}

	const bool saved = !SDL_WINDOWPOS_ISUNDEFINED(p_x) && !SDL_WINDOWPOS_ISUNDEFINED(p_y) &&
					   !SDL_WINDOWPOS_ISCENTERED(p_x) && !SDL_WINDOWPOS_ISCENTERED(p_y);
	SDL_DisplayID display = 0;
	if (saved) {
		SDL_Point point = {p_x, p_y};
		display = SDL_GetDisplayForPoint(&point);
	}
	if (!display) {
		display = WindowDisplay();
	}
	if (s_fitAutomaticWindow) {
		FitWindowToUsableBounds(display);
	}

	SDL_Rect usable;
	if (!WindowUsableBounds(display, &usable)) {
		SDL_SetWindowPosition(s_window, saved ? p_x : SDL_WINDOWPOS_CENTERED, saved ? p_y : SDL_WINDOWPOS_CENTERED);
		return;
	}

	int width;
	int height;
	if (!SDL_GetWindowSize(s_window, &width, &height)) {
		return;
	}
	int top;
	int left;
	int bottom;
	int right;
	WindowBorders(&top, &left, &bottom, &right);

	const int minX = usable.x + left;
	const int minY = usable.y + top;
	const int maxX = usable.x + usable.w - right - width;
	const int maxY = usable.y + usable.h - bottom - height;
	const int centerX = minX + (std::max(minX, maxX) - minX) / 2;
	const int centerY = minY + (std::max(minY, maxY) - minY) / 2;
	const int x = saved ? std::clamp(p_x, minX, std::max(minX, maxX)) : centerX;
	const int y = saved ? std::clamp(p_y, minY, std::max(minY, maxY)) : centerY;
	SDL_SetWindowPosition(s_window, x, y);
	SDL_SyncWindow(s_window);
}

void Platform_RenderWindowToFrame(float* p_x, float* p_y)
{
	if (!s_renderer) {
		return;
	}
	SDL_RenderCoordinatesFromWindow(s_renderer, *p_x, *p_y, p_x, p_y);
}

void Platform_WarpMouse(float p_x, float p_y)
{
	if (!s_renderer || !s_window) {
		return;
	}
	float wx = p_x;
	float wy = p_y;
	SDL_RenderCoordinatesToWindow(s_renderer, p_x, p_y, &wx, &wy);
	SDL_WarpMouseInWindow(s_window, wx, wy);
}

void Platform_RenderDebugText(float p_x, float p_y, unsigned int p_argb, const char* p_text, int p_height)
{
	if (!p_text || s_debugTextCount >= DEBUG_TEXT_MAX) {
		return;
	}
	DEBUG_TEXT& t = s_debugText[s_debugTextCount++];
	t.m_x = p_x;
	t.m_y = p_y;
	t.m_color = (p_argb & 0xff000000u) ? p_argb : (p_argb | 0xff000000u);
	t.m_height = p_height > 0 ? p_height : SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE;
	SDL_strlcpy(t.m_text, p_text, sizeof(t.m_text));
}
