#include "platform/timing.h"

#include <SDL3/SDL.h>

unsigned int Platform_Ticks()
{
	return (unsigned int) SDL_GetTicks();
}

void Platform_Sleep(unsigned int p_ms)
{
	SDL_Delay(p_ms);
}
