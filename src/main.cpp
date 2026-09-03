
#include "game/game_descriptor.h"
#include "game/map.h"
#include "game/map_steam.h"
#include "game/settings.h"
#include "platform/gamepad.h"
#include "platform/paths.h"
#include "platform/store.h"
#include "util/string.h"
#include "version.h"

#include <SDL3/SDL.h>
#include <stdio.h>

namespace
{

bool CheckGameData()
{
	FILE* file = Platform_FOpen("objects.res", "rb");
	if (file) {
		fclose(file);
		return true;
	}

	char message[2048];
	snprintf(
		message,
		sizeof(message),
		"Game data was not found.\n\n"
		"Expected objects.res and the Maps directory under:\n%s\n"
		"Copy the contents of an Alien Shooter (GOG/Steam or retail 1.2) or\n"
		"Zombie Shooter (Steam) installation there or run:\n\n"
		"OpenGromada --data-path=\"/path/to/game\"\n\n",
		Platform_BasePath()
	);
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
	if (!Game_Detect()) {
		SDL_Quit();
		return 1;
	}
	SDL_SetAppMetadata(GameDesc->m_title, OPENGROMADA_VERSION, GameDesc->m_appMetaId);
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
			while (!Map->Tact())
				;
		}
		else {
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
