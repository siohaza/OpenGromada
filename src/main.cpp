
#include "game/map.h"
#include "game/map_steam.h"
#include "game/settings.h"
#include "platform/paths.h"
#include "util/string.h"

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
		"Alien Shooter game data was not found.\n\n"
		"Expected objects.res and the Maps directory under:\n%s\n"
		"Copy the contents of an Alien Shooter 1.2 retail or GOG installation there or run:\n\n"
		"AlienShooter --data-path=\"/path/to/Alien Shooter\"\n\n",
		Platform_BasePath()
	);
	fprintf(stderr, "%s\n", message);
	fflush(stderr);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Alien Shooter - Game Data Not Found", message, 0);
	return false;
}

} // namespace

// STUB: ALIEN 0x404f00
int main(int argc, char** argv)
{
	SDL_SetAppMetadata("Alien Shooter", "1.2", "com.sigmateam.alienshooter");

	STRING commandLine;
	Settings_ParseCommandLine(argc, argv, &commandLine);
	if (!CheckGameData()) {
		SDL_Quit();
		return 1;
	}

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
	SDL_Quit();
	return result;
}
