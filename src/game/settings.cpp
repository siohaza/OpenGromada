#include "game/settings.h"

#include "game/game_descriptor.h"
#include "util/string.h"

#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace
{

enum OVERRIDE_BITS {
	OVERRIDE_WIDTH = 1 << 0,
	OVERRIDE_HEIGHT = 1 << 1,
	OVERRIDE_FULLSCREEN = 1 << 2,
	OVERRIDE_RENDER_WIDTH = 1 << 3,
	OVERRIDE_NATIVE = 1 << 4,
	OVERRIDE_UI_SCALE = 1 << 5,
	OVERRIDE_VSYNC = 1 << 6,
	OVERRIDE_DESKTOP_RESOLUTION = 1 << 7,
	OVERRIDE_RED_BLOOD = 1 << 8,
};

struct COMMAND_LINE_SETTINGS {
	unsigned int m_mask;
	int m_width;
	int m_height;
	int m_fullscreen;
	int m_renderWidth;
	int m_native;
	int m_uiScale;
	int m_vsync;
};

COMMAND_LINE_SETTINGS g_commandLine;
SETTINGS_RENDERER g_renderer = SETTINGS_RENDERER_AUTO;
const char* g_gpuDriver;

const char* OptionValue(const char* p_arg, const char* p_name, int* p_index, int p_argc, char** p_argv)
{
	size_t length = strlen(p_name);
	if (!strncmp(p_arg, p_name, length) && p_arg[length] == '=') {
		return p_arg + length + 1;
	}
	if (!strcmp(p_arg, p_name) && *p_index + 1 < p_argc) {
		return p_argv[++*p_index];
	}
	return 0;
}

int PositiveInt(const char* p_value)
{
	if (!p_value || !*p_value) {
		return 0;
	}
	char* end = 0;
	long value = strtol(p_value, &end, 10);
	return end && !*end && value > 0 && value <= 0x7fffffffL ? (int) value : 0;
}

void SetGameArgument(STRING* p_result, const char* p_arg)
{
	if (!p_result || !p_arg) {
		return;
	}
	*p_result = p_arg;
}

bool IsLegacyDisplay(int p_width, int p_height)
{
	return (p_width == 640 && p_height == 480) || (p_width == 800 && p_height == 600) ||
		   (p_width == 1024 && p_height == 768);
}

} // namespace

// GLOBAL: ALIEN 0x47f4b0
SETTINGS Settings = {{0}, {640, 800, 1024}, {480, 600, 768}, {16, 32}, 0, 0, 640, 480, 32, 1, 0, 0, 0, 0, 0};

// FUNCTION: ALIEN 0x4348c0
int SETTINGS::CheckMode(int p_width, int p_height, int p_bpp)
{
	// The portable framebuffer is always ARGB8888.
	return p_width > 0 && p_height > 0 && p_bpp == 32;
}

void Settings_ParseCommandLine(int p_argc, char** p_argv, STRING* p_gameArgument)
{
	g_commandLine = {};
	g_renderer = SETTINGS_RENDERER_AUTO;
	g_gpuDriver = nullptr;
	if (p_gameArgument) {
		*p_gameArgument = STRING::EMPTY;
	}

	for (int i = 1; i < p_argc; ++i) {
		const char* arg = p_argv[i];
		const char* value;
		if ((value = OptionValue(arg, "--renderer", &i, p_argc, p_argv))) {
			if (!strcmp(value, "auto")) {
				g_renderer = SETTINGS_RENDERER_AUTO;
			}
			else if (!strcmp(value, "gpu")) {
				g_renderer = SETTINGS_RENDERER_GPU;
			}
			else if (!strcmp(value, "software")) {
				g_renderer = SETTINGS_RENDERER_SOFTWARE;
			}
			else {
				fprintf(stderr, "Invalid --renderer value '%s'; expected auto, gpu, or software.\n", value);
				exit(1);
			}
		}
		else if ((value = OptionValue(arg, "--gpu-driver", &i, p_argc, p_argv))) {
			if (!strcmp(value, "vulkan")) {
				g_gpuDriver = "vulkan";
			}
			else if (!strcmp(value, "direct3d12")) {
				g_gpuDriver = "direct3d12";
			}
			else if (!strcmp(value, "metal")) {
				g_gpuDriver = "metal";
			}
			else {
				fprintf(stderr, "Invalid --gpu-driver value '%s'; expected vulkan, direct3d12, or metal.\n", value);
				exit(1);
			}
		}
		else if ((value = OptionValue(arg, "--width", &i, p_argc, p_argv))) {
			int n = PositiveInt(value);
			if (n) {
				g_commandLine.m_width = n;
				g_commandLine.m_mask |= OVERRIDE_WIDTH;
			}
		}
		else if ((value = OptionValue(arg, "--height", &i, p_argc, p_argv))) {
			int n = PositiveInt(value);
			if (n) {
				g_commandLine.m_height = n;
				g_commandLine.m_mask |= OVERRIDE_HEIGHT;
			}
		}
		else if ((value = OptionValue(arg, "--resolution", &i, p_argc, p_argv))) {
			if (!SDL_strcasecmp(value, "desktop") || !SDL_strcasecmp(value, "auto")) {
				g_commandLine.m_mask |= OVERRIDE_DESKTOP_RESOLUTION;
			}
			else {
				int width = 0;
				int height = 0;
				char tail = 0;
				if (sscanf(value, "%dx%d%c", &width, &height, &tail) == 2 && width > 0 && height > 0) {
					g_commandLine.m_width = width;
					g_commandLine.m_height = height;
					g_commandLine.m_mask |= OVERRIDE_WIDTH | OVERRIDE_HEIGHT;
				}
			}
		}
		else if ((value = OptionValue(arg, "--render-width", &i, p_argc, p_argv))) {
			int n = PositiveInt(value);
			if (n) {
				g_commandLine.m_renderWidth = n;
				g_commandLine.m_mask |= OVERRIDE_RENDER_WIDTH;
			}
		}
		else if ((value = OptionValue(arg, "--ui-scale", &i, p_argc, p_argv))) {
			int scale = !strcmp(value, "auto") ? 0 : PositiveInt(value);
			if (!strcmp(value, "auto") || (scale >= 1 && scale <= 3)) {
				g_commandLine.m_uiScale = scale;
				g_commandLine.m_mask |= OVERRIDE_UI_SCALE;
			}
		}
		else if ((value = OptionValue(arg, "--data-path", &i, p_argc, p_argv)) ||
				 (value = OptionValue(arg, "--data-dir", &i, p_argc, p_argv))) {
			SDL_setenv_unsafe("ALIEN_SHOOTER_DATA_PATH", value, 1);
		}
		else if ((value = OptionValue(arg, "--pref-path", &i, p_argc, p_argv))) {
			SDL_setenv_unsafe("ALIEN_SHOOTER_PREF_PATH", value, 1);
		}
		else if ((value = OptionValue(arg, "--game", &i, p_argc, p_argv)) ||
				 (value = OptionValue(arg, "--profile", &i, p_argc, p_argv))) {
			if (!Game_SetCliOverride(value)) {
				fprintf(stderr,
						"Unknown --game value '%s'. Valid games: as1, zs1, theseus, crazy-lunch, last-hope, "
						"chacks-temple, locoland.\n",
						value);
				exit(1);
			}
		}
		else if ((value = OptionValue(arg, "--probe-game", &i, p_argc, p_argv))) {
			if (strcmp(value, "json")) {
				fprintf(stderr, "--probe-game accepts only 'json'.\n");
				exit(1);
			}
			Game_SetProbeJson(true);
		}
		else if ((value = OptionValue(arg, "--config", &i, p_argc, p_argv))) {
			Game_SetConfigOverride(value);
		}
		else if ((value = OptionValue(arg, "--script", &i, p_argc, p_argv))) {
			SetGameArgument(p_gameArgument, value);
		}
		else if (!strcmp(arg, "--fullscreen")) {
			g_commandLine.m_fullscreen = 1;
			g_commandLine.m_mask |= OVERRIDE_FULLSCREEN;
		}
		else if (!strcmp(arg, "--windowed")) {
			g_commandLine.m_fullscreen = 0;
			g_commandLine.m_mask |= OVERRIDE_FULLSCREEN;
		}
		else if (!strcmp(arg, "--native-resolution")) {
			g_commandLine.m_native = 1;
			g_commandLine.m_mask |= OVERRIDE_NATIVE;
		}
		else if (!strcmp(arg, "--no-native-resolution")) {
			g_commandLine.m_native = 0;
			g_commandLine.m_mask |= OVERRIDE_NATIVE;
		}
		else if (!strcmp(arg, "--vsync")) {
			g_commandLine.m_vsync = 1;
			g_commandLine.m_mask |= OVERRIDE_VSYNC;
		}
		else if (!strcmp(arg, "--no-vsync")) {
			g_commandLine.m_vsync = 0;
			g_commandLine.m_mask |= OVERRIDE_VSYNC;
		}
		else if (!strcmp(arg, "--red-blood")) {
			g_commandLine.m_mask |= OVERRIDE_RED_BLOOD;
		}
		else if (!strncmp(arg, "--", 2)) {
			fprintf(stderr,
					"Unknown or incomplete option '%s'. Use --data-dir, --game, --config or --script for startup "
					"options.\n",
					arg);
			exit(1);
		}
		else {
			SetGameArgument(p_gameArgument, arg);
			size_t length = strlen(arg);
			if (length >= 4 && !SDL_strcasecmp(arg + length - 4, ".cfg")) {
				Game_SetConfigOverride(arg);
			}
		}
	}
	if (g_gpuDriver && g_renderer != SETTINGS_RENDERER_GPU) {
		fprintf(stderr, "--gpu-driver requires --renderer=gpu.\n");
		exit(1);
	}
}

SETTINGS_RENDERER Settings_Renderer()
{
	return g_renderer;
}

const char* Settings_RendererName()
{
	switch (g_renderer) {
	case SETTINGS_RENDERER_GPU:
		return "gpu";
	case SETTINGS_RENDERER_SOFTWARE:
		return "software";
	default:
		return "auto";
	}
}

const char* Settings_GPUDriver()
{
	return g_gpuDriver;
}

bool Settings_ForceRedBlood()
{
	return (g_commandLine.m_mask & OVERRIDE_RED_BLOOD) != 0;
}

bool Settings_MigrateLegacyDisplayForDesktop(SETTINGS* p_settings, int p_desktopWidth, int p_desktopHeight)
{
	if (!p_settings) {
		return false;
	}

	int oldVersion = p_settings->m_displayPolicyVersion;
	// Version 1 did not persist automatic resolution.
	if (oldVersion < 2) {
		p_settings->m_desktopResolution = 0;
	}
	bool oldPolicy = oldVersion < SETTINGS_DISPLAY_POLICY_VERSION;
	bool commandLineResolution =
		(g_commandLine.m_mask & (OVERRIDE_WIDTH | OVERRIDE_HEIGHT | OVERRIDE_DESKTOP_RESOLUTION)) != 0;
	bool legacyResolution = oldVersion < 1 && IsLegacyDisplay(p_settings->m_screenX, p_settings->m_screenY);
	// Recover version-1 desktop-sized windows.
	bool desktopSizedVersion1Window = oldVersion == 1 && p_settings->m_fullscreen == 0 && p_desktopWidth > 0 &&
									  p_desktopHeight > 0 && p_settings->m_screenX == p_desktopWidth &&
									  p_settings->m_screenY == p_desktopHeight;
	bool migrateResolution = oldPolicy && !commandLineResolution && (legacyResolution || desktopSizedVersion1Window);
	if (migrateResolution) {
		p_settings->m_desktopResolution = 1;
	}

	// Remove the generated version-1 render-width cap.
	bool migrateRenderWidth =
		oldVersion <= 1 && !(g_commandLine.m_mask & OVERRIDE_RENDER_WIDTH) && p_settings->m_renderWidth == 1280;
	if (migrateRenderWidth) {
		p_settings->m_renderWidth = 0;
	}

	// Upgrade generated version-1/2 windows to automatic fullscreen.
	bool commandLineDisplay = (g_commandLine.m_mask & (OVERRIDE_WIDTH | OVERRIDE_HEIGHT | OVERRIDE_FULLSCREEN)) != 0;
	bool generatedAutomaticWindow =
		(oldVersion == 2 && p_settings->m_desktopResolution != 0) || desktopSizedVersion1Window;
	bool migrateFullscreen = generatedAutomaticWindow && p_settings->m_fullscreen == 0 && !commandLineDisplay;
	if (migrateFullscreen) {
		p_settings->m_fullscreen = 1;
	}

	// Upgrade generated automatic fullscreen to native rendering.
	bool effectiveAutomatic = p_settings->m_desktopResolution != 0;
	if (g_commandLine.m_mask & (OVERRIDE_WIDTH | OVERRIDE_HEIGHT)) {
		effectiveAutomatic = false;
	}
	else if (g_commandLine.m_mask & OVERRIDE_DESKTOP_RESOLUTION) {
		effectiveAutomatic = true;
	}
	bool effectiveFullscreen = p_settings->m_fullscreen != 0;
	if (g_commandLine.m_mask & OVERRIDE_FULLSCREEN) {
		effectiveFullscreen = g_commandLine.m_fullscreen != 0;
	}
	bool migrateNative = oldPolicy && effectiveAutomatic && effectiveFullscreen && p_settings->m_renderWidth == 0 &&
						 p_settings->m_nativeResolution == 0 &&
						 !(g_commandLine.m_mask & (OVERRIDE_RENDER_WIDTH | OVERRIDE_NATIVE));
	if (migrateNative) {
		p_settings->m_nativeResolution = 1;
	}
	p_settings->m_displayPolicyVersion = SETTINGS_DISPLAY_POLICY_VERSION;
	return migrateResolution || migrateRenderWidth || migrateFullscreen || migrateNative;
}

bool Settings_MigrateLegacyDisplay(SETTINGS* p_settings)
{
	if (!(SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) && !SDL_InitSubSystem(SDL_INIT_VIDEO)) {
		return Settings_MigrateLegacyDisplayForDesktop(p_settings, 0, 0);
	}

	// Migrate against the saved display.
	SDL_DisplayID id = 0;
	int count = 0;
	SDL_DisplayID* displays = SDL_GetDisplays(&count);
	if (displays && p_settings && p_settings->m_device >= 0 && p_settings->m_device < 8 &&
		p_settings->m_device < count) {
		id = displays[p_settings->m_device];
	}
	SDL_free(displays);
	if (!id) {
		id = SDL_GetPrimaryDisplay();
	}
	const SDL_DisplayMode* desktop = id ? SDL_GetDesktopDisplayMode(id) : 0;
	return Settings_MigrateLegacyDisplayForDesktop(p_settings, desktop ? desktop->w : 0, desktop ? desktop->h : 0);
}

void Settings_ApplyCommandLine(SETTINGS* p_settings)
{
	if (!p_settings) {
		return;
	}
	bool explicitResolution = (g_commandLine.m_mask & (OVERRIDE_WIDTH | OVERRIDE_HEIGHT)) != 0;
	if (g_commandLine.m_mask & OVERRIDE_WIDTH) {
		p_settings->m_screenX = g_commandLine.m_width;
	}
	if (g_commandLine.m_mask & OVERRIDE_HEIGHT) {
		p_settings->m_screenY = g_commandLine.m_height;
	}
	if (explicitResolution) {
		p_settings->m_desktopResolution = 0;
	}
	else if (g_commandLine.m_mask & OVERRIDE_DESKTOP_RESOLUTION) {
		p_settings->m_desktopResolution = 1;
	}
	if (g_commandLine.m_mask & OVERRIDE_FULLSCREEN) {
		p_settings->m_fullscreen = g_commandLine.m_fullscreen;
	}
	if (g_commandLine.m_mask & OVERRIDE_RENDER_WIDTH) {
		p_settings->m_renderWidth = g_commandLine.m_renderWidth;
		// A render-width request selects the capped non-native policy unless the
		// caller also chose native behavior explicitly (in either argument order).
		if (!(g_commandLine.m_mask & OVERRIDE_NATIVE)) {
			p_settings->m_nativeResolution = 0;
		}
	}
	if (g_commandLine.m_mask & OVERRIDE_NATIVE) {
		p_settings->m_nativeResolution = g_commandLine.m_native;
	}
	if (g_commandLine.m_mask & OVERRIDE_UI_SCALE) {
		p_settings->m_uiScale = g_commandLine.m_uiScale;
	}
	if (g_commandLine.m_mask & OVERRIDE_VSYNC) {
		if (g_commandLine.m_vsync) {
			p_settings->m_flag |= 2;
		}
		else {
			p_settings->m_flag &= ~2u;
		}
	}
}
