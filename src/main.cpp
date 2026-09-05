
#include "game/game_descriptor.h"
#include "game/map.h"
#include "game/map_steam.h"
#include "game/settings.h"
#include "platform/gamepad.h"
#include "platform/paths.h"
#include "platform/render.h"
#include "platform/store.h"
#include "util/string.h"
#include "version.h"

#include <SDL3/SDL.h>
#include <stdio.h>

namespace
{

bool CheckGameData()
{
	FILE* file = Platform_FOpen(Game_ResourceName(), "rb");
	if (file) {
		fclose(file);
		return true;
	}

	char message[2048];
	snprintf(message,
			 sizeof(message),
			 "Game data was not found.\n\n"
			 "Expected %s and the Maps directory under:\n%s\n"
			 "Select a complete matching installation with:\n\n"
			 "OpenGromada --data-path=\"/path/to/game\"\n\n",
			 Game_ResourceName(),
			 Platform_BasePath());
	fprintf(stderr, "%s\n", message);
	fflush(stderr);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Sigma Game Data Not Found", message, 0);
	return false;
}

} // namespace

// STUB: ALIEN 0x404f00
int main(int argc, char** argv)
{
	STRING commandLine;
	Settings_ParseCommandLine(argc, argv, &commandLine);
	const bool detected = Game_Detect();
	if (Game_WantsProbeJson()) {
		Game_PrintProbeJson();
		SDL_Quit();
		return detected ? 0 : 1;
	}
	if (!detected) {
		SDL_Quit();
		return 1;
	}
	if (!Game_RuntimeAvailable()) {
		if (GameDesc->m_runtimeEnabled && GameDesc->m_nativeMoviePlayback) {
			fprintf(stderr,
					"Profile '%s' requires movie playback, unavailable in this build. Install FFmpeg 6+ development "
					"libraries and configure with -DALIEN_MOVIES=ON. --probe-game=json reports build capabilities.\n",
					GameDesc->m_profileId);
		}
		else {
			fprintf(stderr,
					"Profile '%s' was identified, but its runtime is not yet enabled. --probe-game=json is available "
					"for data inspection.\n",
					GameDesc->m_profileId);
		}
		SDL_Quit();
		return 1;
	}

	SDL_SetAppMetadata(GameDesc->m_title, OPENGROMADA_VERSION, "io.github.siohaza.OpenGromada");
	if (!CheckGameData()) {
		SDL_Quit();
		return 1;
	}
	Platform_GamepadInit();
	Platform_StoreInit();

	int result = 0;
	Map = new MAP_STEAM(commandLine, &Settings);
	if (Map) {
		if (Map->m_flag & 4) {
			while (!Map->Tact() && !Platform_RenderFailed())
				;
		}
		else {
			result = 1;
		}
		if (Map->m_logic.m_runtimeFault) {
			result = 1;
		}
		if (Platform_RenderFailed()) {
			result = 1;
		}
		delete Map;
		Map = 0;
	}
	else {
		result = 1;
	}
	Platform_StoreShutdown();
	Platform_GamepadShutdown();
	SDL_Quit();
	return result;
}
