#include "platform/keycodes.h"

#include <SDL3/SDL.h>

int Platform_KeyToVirtualKey(unsigned int p_sdlKeycode)
{
	SDL_Keycode key = (SDL_Keycode) p_sdlKeycode;

	if (key >= SDLK_A && key <= SDLK_Z) {
		return (int) (key - SDLK_A) + 'A';
	}
	if (key >= SDLK_0 && key <= SDLK_9) {
		return (int) (key - SDLK_0) + '0';
	}

	switch (key) {
	case SDLK_LSHIFT:
	case SDLK_RSHIFT:
		return VK_SHIFT;
	case SDLK_LCTRL:
	case SDLK_RCTRL:
		return VK_CONTROL;
	case SDLK_PAGEUP:
		return VK_PRIOR;
	case SDLK_PAGEDOWN:
		return VK_NEXT;
	case SDLK_END:
		return VK_END;
	case SDLK_HOME:
		return VK_HOME;
	case SDLK_LEFT:
		return VK_LEFT;
	case SDLK_UP:
		return VK_UP;
	case SDLK_RIGHT:
		return VK_RIGHT;
	case SDLK_DOWN:
		return VK_DOWN;
	case SDLK_INSERT:
		return VK_INSERT;
	case SDLK_DELETE:
		return VK_DELETE;
	case SDLK_LGUI:
		return VK_LWIN;
	case SDLK_RGUI:
		return VK_RWIN;
	case SDLK_SPACE:
		return ' ';
	case SDLK_RETURN:
		return 0x0d;
	case SDLK_ESCAPE:
		return 0x1b;
	case SDLK_TAB:
		return 0x09;
	case SDLK_BACKSPACE:
		return 0x08;
	case SDLK_LEFTBRACKET:
		return '[';
	case SDLK_RIGHTBRACKET:
		return ']';
	default:
		break;
	}

	if (key >= SDLK_F1 && key <= SDLK_F12) {
		return VK_F1 + (int) (key - SDLK_F1);
	}

	return 0;
}
