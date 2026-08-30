
#include "game/map.h"
#include "game/map_steam.h"
#include "game/settings.h"
#include "util/string.h"

#include <SDL3/SDL.h>

// STUB: ALIEN 0x404f00
int main(int argc, char** argv)
{
	SDL_SetAppMetadata("Alien Shooter", "1.2", "com.sigmateam.alienshooter");

	STRING commandLine;
	Settings_ParseCommandLine(argc, argv, &commandLine);

	Map = new MAP_STEAM(commandLine, &Settings);
	if (Map) {
		if (Map->m_flag & 4) {
			while (!Map->Tact())
				;
		}
		delete Map;
		Map = 0;
	}
	SDL_Quit();
	return 0;
}
