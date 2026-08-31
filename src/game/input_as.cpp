#include "game/input_as.h"

#include "game/map.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "platform/gamepad.h"
#include "platform/keycodes.h"
#include "ui/mouse.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>

// GLOBAL: ALIEN 0x4836a8
int g_keyScrollLeft2 = 37;

// GLOBAL: ALIEN 0x4836ac
int g_keyScrollLeft = '%';

// GLOBAL: ALIEN 0x4836b0
int g_keyScrollRight2 = 39;

// GLOBAL: ALIEN 0x4836b4
int g_keyScrollRight = '\'';

// GLOBAL: ALIEN 0x4836b8
int g_keyScrollUp2 = 38;

// GLOBAL: ALIEN 0x4836bc
int g_keyScrollUp = '&';

// GLOBAL: ALIEN 0x4836c0
int g_keyScrollDown2 = 40;

// GLOBAL: ALIEN 0x4836c4
int g_keyScrollDown = '(';

// GLOBAL: ALIEN 0x492760
int g_relativeControl;

// GLOBAL: ALIEN 0x492758
int g_windowPosX;

// GLOBAL: ALIEN 0x49275c
int g_windowPosY;

// GLOBAL: ALIEN 0x4836c8
int INPUT_AS::firstKey1 = 1;

// GLOBAL: ALIEN 0x4836cc
int INPUT_AS::firstKey2 = 1;

// GLOBAL: ALIEN 0x4836d0
int INPUT_AS::secondKey1 = 2;

// GLOBAL: ALIEN 0x4836d4
int INPUT_AS::secondKey2 = 2;

// GLOBAL: ALIEN 0x4836d8
int INPUT_AS::prevKey1 = '[';

// GLOBAL: ALIEN 0x4836dc
int INPUT_AS::nextKey1 = ']';

// GLOBAL: ALIEN 0x4836e0
int INPUT_AS::firstClearForButtonUp = 1;

// GLOBAL: ALIEN 0x4836e4
int INPUT_AS::secondClearForButtonUp = 1;

namespace
{

enum POINTER_SOURCE {
	POINTER_MOUSE,
	POINTER_GAMEPAD
};

unsigned int s_keyboardHeld;
unsigned int s_mouseHeld;
unsigned int s_gamepadHeld;
uint64_t s_previousGamepadButtons;
uint64_t s_gamepadGeneration;
bool s_leftTriggerHeld;
bool s_rightTriggerHeld;
bool s_pointerInitialized;
POINTER_SOURCE s_pointerSource = POINTER_MOUSE;

struct STICK_STATE {
	float m_x;
	float m_y;
	float m_magnitude;
};

uint64_t GamepadButton(SDL_GamepadButton p_button)
{
	const int button = (int) p_button;
	return button >= 0 && button < 64 ? uint64_t{1} << button : 0;
}

bool IsGamepadButtonDown(uint64_t p_buttons, SDL_GamepadButton p_button)
{
	return (p_buttons & GamepadButton(p_button)) != 0;
}

STICK_STATE ApplyRadialDeadzone(float p_x, float p_y, float p_deadzone)
{
	const float magnitude = std::sqrt(p_x * p_x + p_y * p_y);
	if (magnitude <= p_deadzone || magnitude <= 0.0f) {
		return {};
	}
	const float scaled = std::min((magnitude - p_deadzone) / std::max(1.0f - p_deadzone, 0.001f), 1.0f);
	const float factor = scaled / magnitude;
	return {p_x * factor, p_y * factor, scaled};
}

unsigned int KeyHeldMask(unsigned int p_key)
{
	if (p_key == (unsigned int) g_keyScrollLeft2 || p_key == (unsigned int) g_keyScrollLeft) {
		return 0x80;
	}
	if (p_key == (unsigned int) g_keyScrollRight2 || p_key == (unsigned int) g_keyScrollRight) {
		return 0x100;
	}
	if (p_key == (unsigned int) g_keyScrollUp2 || p_key == (unsigned int) g_keyScrollUp) {
		return 0x400;
	}
	if (p_key == (unsigned int) g_keyScrollDown2 || p_key == (unsigned int) g_keyScrollDown) {
		return 0x200;
	}
	if (p_key == (unsigned int) INPUT_AS::firstKey1 || p_key == (unsigned int) INPUT_AS::firstKey2) {
		return 0x4000;
	}
	if (p_key == (unsigned int) INPUT_AS::secondKey1 || p_key == (unsigned int) INPUT_AS::secondKey2) {
		return 0x8000;
	}
	if (p_key == VK_SHIFT) {
		return 0x800;
	}
	if (p_key == VK_CONTROL) {
		return 0x1000;
	}
	return 0;
}

void ComposeHeldInput(INPUT_AS* p_input)
{
	p_input->m_button = (p_input->m_button & 0x1f) | s_keyboardHeld | s_mouseHeld | s_gamepadHeld;
}

void ResetLiveInput(INPUT_AS* p_input)
{
	s_keyboardHeld = 0;
	s_mouseHeld = 0;
	s_gamepadHeld = 0;
	s_previousGamepadButtons = 0;
	s_leftTriggerHeld = false;
	s_rightTriggerHeld = false;
	s_pointerSource = POINTER_MOUSE;
	p_input->m_button = 0;
	p_input->m_wheel = 0;
	p_input->m_key = 0;
	p_input->m_unk0x1c = 0;
}

bool MenuCursorMode()
{
	return !Map || !(Map->m_shiftFlag & 0x1c);
}

int UpdatePointer(INPUT_AS* p_input, float p_x, float p_y)
{
	p_input->m_x = p_x;
	p_input->m_y = p_y;

	GRAPH_CORE* graph = (GRAPH_CORE*) Graph;
	if (!graph) {
		p_input->m_worldX = p_x;
		p_input->m_worldY = p_y;
		return 0;
	}
	float worldX = p_x;
	float worldY = p_y;
	if (graph->m_viewXMin > worldX) {
		worldX = graph->m_viewXMin;
	}
	if (graph->m_viewXMax <= worldX) {
		worldX = graph->m_viewXMax - 1.0f;
	}
	if ((double) graph->m_viewYMin > worldY) {
		worldY = graph->m_viewYMin;
	}
	if ((double) graph->m_viewYMax <= worldY) {
		worldY = graph->m_viewYMax - 1.0f;
	}

	p_input->m_worldX = worldX + (Map ? Map->m_shiftX : 0.0f);
	p_input->m_worldY = worldY + (Map ? Map->m_shiftY : 0.0f);
	if (Mouse) {
		Mouse->ChangeCoor(p_input->m_worldX, p_input->m_worldY, Mouse->Z());
	}

	return p_input->m_x >= graph->m_viewXMin && p_input->m_x < graph->m_viewXMax && p_input->m_y >= graph->m_viewYMin &&
		   p_input->m_y < graph->m_viewYMax;
}

void ClaimMousePointer()
{
	s_pointerInitialized = true;
	s_pointerSource = POINTER_MOUSE;
}

bool GamepadPointerEvent(const SDL_Event& p_event)
{
	if (!Platform_GamepadEventIsActive(p_event)) {
		return false;
	}
	if (p_event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
		return MenuCursorMode() && (p_event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_UP ||
									p_event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_DOWN ||
									p_event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_LEFT ||
									p_event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
	}
	if (p_event.type != SDL_EVENT_GAMEPAD_AXIS_MOTION) {
		return false;
	}
	const bool menu = MenuCursorMode();
	const PLATFORM_GAMEPAD_STATE state = Platform_GamepadState();
	if (p_event.gaxis.axis == SDL_GAMEPAD_AXIS_RIGHTX || p_event.gaxis.axis == SDL_GAMEPAD_AXIS_RIGHTY) {
		return std::hypot(state.m_rightX, state.m_rightY) > state.m_deadzone;
	}
	return menu && (p_event.gaxis.axis == SDL_GAMEPAD_AXIS_LEFTX || p_event.gaxis.axis == SDL_GAMEPAD_AXIS_LEFTY) &&
		   std::hypot(state.m_leftX, state.m_leftY) > state.m_deadzone;
}

} // namespace

// FUNCTION: ALIEN 0x425d60
INPUT_AS::INPUT_AS()
{
	s_gamepadGeneration = 0;
	s_pointerInitialized = false;
	m_x = 0;
	m_worldX = 0;
	m_y = 0;
	m_worldY = 0;
	ResetLiveInput(this);
}

// FUNCTION: ALIEN 0x425d80
int INPUT_AS::ProcessEvent(const SDL_Event& p_event)
{
	switch (p_event.type) {
	case SDL_EVENT_MOUSE_MOTION:
		ClaimMousePointer();
		return UpdatePointer(this, p_event.motion.x, p_event.motion.y);

	case SDL_EVENT_WINDOW_FOCUS_LOST:
	case SDL_EVENT_WILL_ENTER_BACKGROUND:
	case SDL_EVENT_DID_ENTER_BACKGROUND:
		ResetLiveInput(this);
		break;

	case SDL_EVENT_WINDOW_MOVED:
		g_windowPosX = p_event.window.data1;
		g_windowPosY = p_event.window.data2;
		break;

	case SDL_EVENT_KEY_DOWN: {
		if (p_event.key.repeat) {
			break;
		}
		unsigned int key = (unsigned int) Platform_KeyToVirtualKey(p_event.key.key);
		if (!key) {
			break;
		}
		m_key = key == VK_BACK || key == VK_TAB || key == VK_RETURN || key == VK_ESCAPE ? key : key << 8;
		m_unk0x1c = key;
		s_keyboardHeld |= KeyHeldMask(key);
		ComposeHeldInput(this);
		break;
	}

	case SDL_EVENT_KEY_UP: {
		unsigned int key = (unsigned int) Platform_KeyToVirtualKey(p_event.key.key);
		if (!key) {
			break;
		}
		s_keyboardHeld &= ~KeyHeldMask(key);
		ComposeHeldInput(this);
		break;
	}

	case SDL_EVENT_TEXT_INPUT:
		m_key = (unsigned char) p_event.text.text[0];
		return 0;

	case SDL_EVENT_MOUSE_BUTTON_DOWN:
		ClaimMousePointer();
		if (!UpdatePointer(this, p_event.button.x, p_event.button.y)) {
			return 0;
		}
		if (p_event.button.button == SDL_BUTTON_LEFT) {
			m_button |= 1;
			s_mouseHeld |= 0x20;
			if (firstKey1 == VK_LBUTTON) {
				s_mouseHeld |= 0x4000;
			}
			if (secondKey1 == VK_LBUTTON) {
				s_mouseHeld |= 0x8000;
			}
		}
		else if (p_event.button.button == SDL_BUTTON_RIGHT) {
			m_button |= 4;
			s_mouseHeld |= 0x40;
			if (firstKey1 == VK_RBUTTON) {
				s_mouseHeld |= 0x4000;
			}
			if (secondKey1 == VK_RBUTTON) {
				s_mouseHeld |= 0x8000;
			}
		}
		else if (p_event.button.button == SDL_BUTTON_MIDDLE) {
			m_button |= 2;
		}
		ComposeHeldInput(this);
		return 0;

	case SDL_EVENT_MOUSE_BUTTON_UP: {
		ClaimMousePointer();
		bool inside = UpdatePointer(this, p_event.button.x, p_event.button.y) != 0;
		if (p_event.button.button == SDL_BUTTON_LEFT) {
			s_mouseHeld &= ~0x20;
			if (inside) {
				m_button |= 8;
			}
			if (firstKey1 == VK_LBUTTON && firstClearForButtonUp) {
				s_mouseHeld &= ~0x4000;
			}
			if (secondKey1 == VK_LBUTTON && secondClearForButtonUp) {
				s_mouseHeld &= ~0x8000;
			}
		}
		else if (p_event.button.button == SDL_BUTTON_RIGHT) {
			s_mouseHeld &= ~0x40;
			if (inside) {
				m_button |= 0x10;
			}
			if (firstKey1 == VK_RBUTTON && firstClearForButtonUp) {
				s_mouseHeld &= ~0x4000;
			}
			if (secondKey1 == VK_RBUTTON && secondClearForButtonUp) {
				s_mouseHeld &= ~0x8000;
			}
		}
		ComposeHeldInput(this);
		return 0;
	}

	case SDL_EVENT_MOUSE_WHEEL:
		ClaimMousePointer();
		m_wheel = (int) p_event.wheel.y;
		return 0;

	case SDL_EVENT_GAMEPAD_AXIS_MOTION:
	case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
	case SDL_EVENT_GAMEPAD_BUTTON_UP:
		if (GamepadPointerEvent(p_event)) {
			s_pointerSource = POINTER_GAMEPAD;
		}
		break;

	default:
		break;
	}

	return 0;
}

void INPUT_AS::ApplyGamepad(unsigned int p_delta)
{
	PLATFORM_GAMEPAD_STATE state = Platform_GamepadState();
	const bool generationChanged = state.m_generation != s_gamepadGeneration;
	if (generationChanged) {
		s_gamepadGeneration = state.m_generation;
		s_gamepadHeld = 0;
		s_previousGamepadButtons = 0;
		s_leftTriggerHeld = false;
		s_rightTriggerHeld = false;
		ComposeHeldInput(this);
	}
	if (!state.m_connected || !Map || !(Map->m_flag & 8)) {
		s_gamepadHeld = 0;
		s_previousGamepadButtons = state.m_buttons;
		s_leftTriggerHeld = false;
		s_rightTriggerHeld = false;
		ComposeHeldInput(this);
		return;
	}

	GRAPH_CORE* graph = (GRAPH_CORE*) Graph;
	if (!graph) {
		return;
	}
	if (!s_pointerInitialized) {
		s_pointerInitialized = true;
		UpdatePointer(
			this,
			0.5f * (graph->m_viewXMin + graph->m_viewXMax),
			0.5f * (graph->m_viewYMin + graph->m_viewYMax)
		);
	}

	const STICK_STATE left = ApplyRadialDeadzone(state.m_leftX, state.m_leftY, state.m_deadzone);
	const STICK_STATE right = ApplyRadialDeadzone(state.m_rightX, state.m_rightY, state.m_deadzone);
	const bool dpadLeft = IsGamepadButtonDown(state.m_buttons, SDL_GAMEPAD_BUTTON_DPAD_LEFT);
	const bool dpadRight = IsGamepadButtonDown(state.m_buttons, SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
	const bool dpadUp = IsGamepadButtonDown(state.m_buttons, SDL_GAMEPAD_BUTTON_DPAD_UP);
	const bool dpadDown = IsGamepadButtonDown(state.m_buttons, SDL_GAMEPAD_BUTTON_DPAD_DOWN);

	if (s_rightTriggerHeld) {
		s_rightTriggerHeld = state.m_rightTrigger > 0.4f;
	}
	else {
		s_rightTriggerHeld = state.m_rightTrigger >= 0.55f;
	}
	if (s_leftTriggerHeld) {
		s_leftTriggerHeld = state.m_leftTrigger > 0.4f;
	}
	else {
		s_leftTriggerHeld = state.m_leftTrigger >= 0.55f;
	}

	const bool confirm = IsGamepadButtonDown(state.m_buttons, state.m_confirm);
	const bool primary = s_rightTriggerHeld || confirm;
	const bool secondary = s_leftTriggerHeld || IsGamepadButtonDown(state.m_buttons, SDL_GAMEPAD_BUTTON_WEST);
	const bool oldPrimary = (s_gamepadHeld & 0x20) != 0;
	const bool oldSecondary = (s_gamepadHeld & 0x40) != 0;

	unsigned int held = 0;
	const float movementThreshold = 0.38268343f * left.m_magnitude;
	if (left.m_x < -movementThreshold || dpadLeft) {
		held |= 0x80;
	}
	if (left.m_x > movementThreshold || dpadRight) {
		held |= 0x100;
	}
	if (left.m_y > movementThreshold || dpadDown) {
		held |= 0x200;
	}
	if (left.m_y < -movementThreshold || dpadUp) {
		held |= 0x400;
	}
	if (primary) {
		held |= 0x4020;
	}
	if (secondary) {
		held |= 0x8040;
	}
	s_gamepadHeld = held;
	if (primary && !oldPrimary) {
		m_button |= 1;
	}
	else if (!primary && oldPrimary) {
		m_button |= 8;
	}
	if (secondary && !oldSecondary) {
		m_button |= 4;
	}
	else if (!secondary && oldSecondary) {
		m_button |= 0x10;
	}
	ComposeHeldInput(this);

	const uint64_t pressed = state.m_buttons & ~s_previousGamepadButtons;
	if ((pressed & GamepadButton(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER)) &&
		!(pressed & GamepadButton(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER))) {
		m_wheel = 1;
	}
	else if (
		(pressed & GamepadButton(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER)) &&
		!(pressed & GamepadButton(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER))
	) {
		m_wheel = -1;
	}
	if (pressed & (GamepadButton(state.m_cancel) | GamepadButton(SDL_GAMEPAD_BUTTON_START))) {
		m_key = VK_ESCAPE;
	}
	else if (pressed & GamepadButton(SDL_GAMEPAD_BUTTON_NORTH)) {
		m_key = 'f';
	}
	else if (pressed & GamepadButton(SDL_GAMEPAD_BUTTON_BACK)) {
		m_key = VK_TAB;
	}
	else if ((pressed & GamepadButton(state.m_confirm)) && !MenuCursorMode()) {
		m_key = ' ';
	}
	s_previousGamepadButtons = state.m_buttons;

	const bool menu = MenuCursorMode();
	if (generationChanged && (right.m_magnitude > 0.0f ||
							  (menu && (left.m_magnitude > 0.0f || dpadLeft || dpadRight || dpadUp || dpadDown)))) {
		s_pointerSource = POINTER_GAMEPAD;
	}
	if (s_pointerSource != POINTER_GAMEPAD) {
		return;
	}
	if (menu) {
		float cursorX = right.m_x + left.m_x + (dpadRight ? 1.0f : 0.0f) - (dpadLeft ? 1.0f : 0.0f);
		float cursorY = right.m_y + left.m_y + (dpadDown ? 1.0f : 0.0f) - (dpadUp ? 1.0f : 0.0f);
		const float cursorMagnitude = std::sqrt(cursorX * cursorX + cursorY * cursorY);
		if (cursorMagnitude > 1.0f) {
			cursorX /= cursorMagnitude;
			cursorY /= cursorMagnitude;
		}
		if (cursorX != 0.0f || cursorY != 0.0f) {
			const float seconds = std::min(p_delta, 71u) * 0.001f;
			const float speed = std::min(graph->m_width, graph->m_height) * 0.8f * state.m_cursorSpeed;
			const float x = std::clamp(m_x + cursorX * speed * seconds, graph->m_viewXMin, graph->m_viewXMax - 1.0f);
			const float y = std::clamp(m_y + cursorY * speed * seconds, graph->m_viewYMin, graph->m_viewYMax - 1.0f);
			UpdatePointer(this, x, y);
		}
		return;
	}

	if (right.m_magnitude <= 0.0f || !Map->m_player[0]) {
		return;
	}
	MAN* player = Map->Flagman(0);
	if (!player) {
		return;
	}
	const float radius = 0.35f * std::min(graph->m_width, graph->m_height);
	const float directionX = right.m_x / right.m_magnitude;
	const float directionY = right.m_y / right.m_magnitude;
	const float playerX = player->m_x - Map->m_shiftX;
	const float playerY = player->m_y - player->m_z - Map->m_shiftY;
	const float x = std::clamp(playerX + directionX * radius, graph->m_viewXMin, graph->m_viewXMax - 1.0f);
	const float y = std::clamp(playerY + directionY * radius, graph->m_viewYMin, graph->m_viewYMax - 1.0f);
	UpdatePointer(this, x, y);
}

// FUNCTION: ALIEN 0x426330
int INPUT_AS::Save(STREAM* p_stream) const
{
	return p_stream->Write(this, 0x20);
}

// FUNCTION: ALIEN 0x426350
int INPUT_AS::Load(STREAM* p_stream)
{
	return p_stream->Read(this, 0x20);
}

// FUNCTION: ALIEN 0x426370
int INPUT_AS::GetKeyByName(STRING p_name)
{
	p_name = p_name.ToUpper();
	if (!strcmp(
			p_name.m_str,
			// STRING: ALIEN 0x482330
			"LBUTTON"
		)) {
		return VK_LBUTTON;
	}
	else if (!strcmp(
				 p_name.m_str,
				 // STRING: ALIEN 0x482320
				 "RBUTTON"
			 )) {
		return VK_RBUTTON;
	}
	else if (!strcmp(
				 p_name.m_str,
				 // STRING: ALIEN 0x481a04
				 "["
			 )) {
		return VK_LWIN;
	}
	else if (!strcmp(
				 p_name.m_str,
				 // STRING: ALIEN 0x48230c
				 "]"
			 )) {
		return VK_RWIN;
	}
	else if (!strcmp(
				 p_name.m_str,
				 // STRING: ALIEN 0x483768
				 "LEFT"
			 )) {
		return VK_LEFT;
	}
	else if (!strcmp(
				 p_name.m_str,
				 // STRING: ALIEN 0x483760
				 "RIGHT"
			 )) {
		return VK_RIGHT;
	}
	else if (!strcmp(
				 p_name.m_str,
				 // STRING: ALIEN 0x48375c
				 "UP"
			 )) {
		return VK_UP;
	}
	else if (!strcmp(
				 p_name.m_str,
				 // STRING: ALIEN 0x483754
				 "DOWN"
			 )) {
		return VK_DOWN;
	}
	else if (!strcmp(
				 p_name.m_str,
				 // STRING: ALIEN 0x48374c
				 "INSERT"
			 )) {
		return VK_INSERT;
	}
	else if (!strcmp(
				 p_name.m_str,
				 // STRING: ALIEN 0x483744
				 "DELETE"
			 )) {
		return VK_DELETE;
	}
	else if (!strcmp(
				 p_name.m_str,
				 // STRING: ALIEN 0x48373c
				 "HOME"
			 )) {
		return VK_HOME;
	}
	else if (!strcmp(
				 p_name.m_str,
				 // STRING: ALIEN 0x483738
				 "END"
			 )) {
		return VK_END;
	}
	else if (!strcmp(
				 p_name.m_str,
				 // STRING: ALIEN 0x483730
				 "PGUP"
			 )) {
		return VK_PRIOR;
	}
	else if (!strcmp(
				 p_name.m_str,
				 // STRING: ALIEN 0x483728
				 "PGDN"
			 )) {
		return VK_NEXT;
	}
	else if (!strcmp(
				 p_name.m_str,
				 // STRING: ALIEN 0x483720
				 "SHIFT"
			 )) {
		return VK_SHIFT;
	}
	else if (!strcmp(
				 p_name.m_str,
				 // STRING: ALIEN 0x483718
				 "CTRL"
			 )) {
		return VK_CONTROL;
	}
	else if (!strcmp(
				 p_name.m_str,
				 // STRING: ALIEN 0x483714
				 "F1"
			 )) {
		return VK_F1;
	}
	else if (!strcmp(
				 p_name.m_str,
				 // STRING: ALIEN 0x483710
				 "F2"
			 )) {
		return VK_F2;
	}
	else if (!strcmp(
				 p_name.m_str,
				 // STRING: ALIEN 0x48370c
				 "F3"
			 )) {
		return VK_F3;
	}
	else if (!strcmp(
				 p_name.m_str,
				 // STRING: ALIEN 0x483708
				 "F4"
			 )) {
		return VK_F4;
	}
	else if (!strcmp(
				 p_name.m_str,
				 // STRING: ALIEN 0x483704
				 "F5"
			 )) {
		return VK_F5;
	}
	else if (!strcmp(
				 p_name.m_str,
				 // STRING: ALIEN 0x483700
				 "F6"
			 )) {
		return VK_F6;
	}
	else if (!strcmp(
				 p_name.m_str,
				 // STRING: ALIEN 0x4836fc
				 "F7"
			 )) {
		return VK_F7;
	}
	else if (!strcmp(
				 p_name.m_str,
				 // STRING: ALIEN 0x4836f8
				 "F8"
			 )) {
		return VK_F8;
	}
	else if (!strcmp(
				 p_name.m_str,
				 // STRING: ALIEN 0x4836f4
				 "F9"
			 )) {
		return VK_F9;
	}
	else if (!strcmp(
				 p_name.m_str,
				 // STRING: ALIEN 0x4836f0
				 "F10"
			 )) {
		return VK_F10;
	}
	else if (!strcmp(
				 p_name.m_str,
				 // STRING: ALIEN 0x4836ec
				 "F11"
			 )) {
		return VK_F11;
	}
	else if (!strcmp(
				 p_name.m_str,
				 // STRING: ALIEN 0x4836e8
				 "F12"
			 )) {
		return VK_F12;
	}
	return p_name.m_str[0];
}

// FUNCTION: ALIEN 0x426d40
void INPUT_AS::Tact()
{
	unsigned int result = s_keyboardHeld | s_mouseHeld | s_gamepadHeld;
	m_button = result;
	m_key = 0;
	m_unk0x1c = 0;
	m_wheel = 0;
	if (!firstClearForButtonUp) {
		result &= 0xffffbfff;
		m_button = result;
	}
	if (!secondClearForButtonUp) {
		m_button &= 0xffff7fff;
	}
}

// FUNCTION: ALIEN 0x426d70
void INPUT_AS::ClearLClick()
{
	unsigned int result = m_button & 0xfffffffe;
	m_button = result;
	if (firstKey1 == 1) {
		result &= 0xffffbfff;
		m_button = result;
	}
	if (secondKey1 == 1) {
		m_button &= 0xffff7fff;
	}
}

// FUNCTION: ALIEN 0x426da0
void INPUT_AS::ClearRClick()
{
	unsigned int result = m_button & 0xfffffffb;
	m_button = result;
	if (firstKey1 == 2) {
		result &= 0xffffbfff;
		m_button = result;
	}
	if (secondKey1 == 2) {
		m_button &= 0xffff7fff;
	}
}
