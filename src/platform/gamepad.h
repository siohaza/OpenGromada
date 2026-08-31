#ifndef PLATFORM_GAMEPAD_H
#define PLATFORM_GAMEPAD_H

#include <SDL3/SDL.h>
#include <stdint.h>

struct PLATFORM_GAMEPAD_STATE {
	bool m_connected;
	uint64_t m_generation;
	uint64_t m_buttons;
	float m_leftX;
	float m_leftY;
	float m_rightX;
	float m_rightY;
	float m_leftTrigger;
	float m_rightTrigger;
	float m_deadzone;
	float m_cursorSpeed;
	SDL_GamepadButton m_confirm;
	SDL_GamepadButton m_cancel;
};

bool Platform_GamepadInit();
void Platform_GamepadShutdown();
void Platform_GamepadProcessEvent(const SDL_Event& p_event);
void Platform_GamepadClearState();
PLATFORM_GAMEPAD_STATE Platform_GamepadState();
bool Platform_GamepadEventIsActive(const SDL_Event& p_event);

#endif
