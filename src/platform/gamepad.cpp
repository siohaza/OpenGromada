#include "platform/gamepad.h"

#include "platform/paths.h"
#include "platform/portable_config.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <string>

namespace
{

std::map<SDL_JoystickID, SDL_Gamepad*> s_gamepads;
std::map<SDL_JoystickID, uint64_t> s_lastUse;
SDL_JoystickID s_active;
uint64_t s_generation;
uint64_t s_useSerial;
bool s_initialized;
bool s_enabled;
bool s_suppressed;
float s_deadzone = 0.2f;
float s_cursorSpeed = 1.0f;
std::string s_confirm = "auto";

int ClampConfig(int p_value, int p_minimum, int p_maximum)
{
	return std::clamp(p_value, p_minimum, p_maximum);
}

std::string MappingPath(const char* p_root)
{
	std::string path = p_root && *p_root ? p_root : "./";
	if (path.back() != '/' && path.back() != '\\') {
		path += '/';
	}
	path += "gamecontrollerdb.txt";
	return path;
}

void LoadMappings(const std::string& p_path)
{
	SDL_PathInfo info;
	if (SDL_GetPathInfo(p_path.c_str(), &info)) {
		SDL_AddGamepadMappingsFromFile(p_path.c_str());
		SDL_ClearError();
	}
}

void SelectGamepad(SDL_JoystickID p_id)
{
	if (p_id) {
		s_lastUse[p_id] = ++s_useSerial;
	}
	if (s_active != p_id) {
		s_active = p_id;
		++s_generation;
	}
}

void OpenGamepad(SDL_JoystickID p_id)
{
	if (!p_id || s_gamepads.find(p_id) != s_gamepads.end()) {
		return;
	}
	SDL_Gamepad* gamepad = SDL_OpenGamepad(p_id);
	if (!gamepad) {
		SDL_ClearError();
		return;
	}
	s_gamepads.emplace(p_id, gamepad);
	s_lastUse.emplace(p_id, 0);
	if (!s_active) {
		SelectGamepad(p_id);
		s_suppressed = true;
	}
}

void CloseGamepad(SDL_JoystickID p_id)
{
	auto it = s_gamepads.find(p_id);
	if (it == s_gamepads.end()) {
		return;
	}
	SDL_CloseGamepad(it->second);
	s_gamepads.erase(it);
	s_lastUse.erase(p_id);
	if (s_active == p_id) {
		SDL_JoystickID fallback = 0;
		uint64_t lastUse = 0;
		for (const auto& gamepad : s_gamepads) {
			const uint64_t used = s_lastUse[gamepad.first];
			if (!fallback || used > lastUse) {
				fallback = gamepad.first;
				lastUse = used;
			}
		}
		SelectGamepad(fallback);
		s_suppressed = true;
	}
}

float NormalizeAxis(Sint16 p_value)
{
	return p_value < 0 ? (float) p_value / 32768.0f : (float) p_value / 32767.0f;
}

bool MeaningfulAxis(const SDL_GamepadAxisEvent& p_axis)
{
	if (p_axis.axis == SDL_GAMEPAD_AXIS_LEFT_TRIGGER || p_axis.axis == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) {
		return p_axis.value > 8192;
	}
	auto it = s_gamepads.find(p_axis.which);
	if (it == s_gamepads.end()) {
		return false;
	}
	SDL_GamepadAxis xAxis;
	SDL_GamepadAxis yAxis;
	if (p_axis.axis == SDL_GAMEPAD_AXIS_LEFTX || p_axis.axis == SDL_GAMEPAD_AXIS_LEFTY) {
		xAxis = SDL_GAMEPAD_AXIS_LEFTX;
		yAxis = SDL_GAMEPAD_AXIS_LEFTY;
	}
	else if (p_axis.axis == SDL_GAMEPAD_AXIS_RIGHTX || p_axis.axis == SDL_GAMEPAD_AXIS_RIGHTY) {
		xAxis = SDL_GAMEPAD_AXIS_RIGHTX;
		yAxis = SDL_GAMEPAD_AXIS_RIGHTY;
	}
	else {
		return false;
	}
	float x = NormalizeAxis(SDL_GetGamepadAxis(it->second, xAxis));
	float y = NormalizeAxis(SDL_GetGamepadAxis(it->second, yAxis));
	if (p_axis.axis == xAxis) {
		x = NormalizeAxis(p_axis.value);
	}
	else {
		y = NormalizeAxis(p_axis.value);
	}
	return std::hypot(x, y) > s_deadzone;
}

float NormalizeTrigger(Sint16 p_value)
{
	return std::clamp((float) p_value / 32767.0f, 0.0f, 1.0f);
}

bool StateIsNeutral(const PLATFORM_GAMEPAD_STATE& p_state)
{
	return p_state.m_buttons == 0 && std::hypot(p_state.m_leftX, p_state.m_leftY) <= s_deadzone &&
		   std::hypot(p_state.m_rightX, p_state.m_rightY) <= s_deadzone && p_state.m_leftTrigger < 0.1f &&
		   p_state.m_rightTrigger < 0.1f;
}

SDL_GamepadButton ConfirmButton(SDL_Gamepad* p_gamepad)
{
	if (!SDL_strcasecmp(s_confirm.c_str(), "east")) {
		return SDL_GAMEPAD_BUTTON_EAST;
	}
	if (!SDL_strcasecmp(s_confirm.c_str(), "south")) {
		return SDL_GAMEPAD_BUTTON_SOUTH;
	}
	return SDL_GetGamepadButtonLabel(p_gamepad, SDL_GAMEPAD_BUTTON_SOUTH) == SDL_GAMEPAD_BUTTON_LABEL_B
			   ? SDL_GAMEPAD_BUTTON_EAST
			   : SDL_GAMEPAD_BUTTON_SOUTH;
}

} // namespace

bool Platform_GamepadInit()
{
	if (s_initialized) {
		return s_enabled;
	}
	s_initialized = true;
	s_enabled = PortableConfig_GetInt("gamepad", "Enabled", 1) != 0;
	if (!s_enabled) {
		return false;
	}

	s_deadzone = ClampConfig(PortableConfig_GetInt("gamepad", "Deadzone", 20), 0, 50) * 0.01f;
	s_cursorSpeed = ClampConfig(PortableConfig_GetInt("gamepad", "CursorSpeed", 100), 25, 300) * 0.01f;
	const char* confirm = PortableConfig_GetString("gamepad", "Confirm");
	if (confirm && *confirm) {
		s_confirm = confirm;
	}

	if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD)) {
		s_enabled = false;
		SDL_ClearError();
		return false;
	}

	const char* mappingHint = SDL_GetHint(SDL_HINT_GAMECONTROLLERCONFIG);
	const char* mappingFileHint = SDL_GetHint(SDL_HINT_GAMECONTROLLERCONFIG_FILE);
	if (!(mappingHint && *mappingHint) && !(mappingFileHint && *mappingFileHint)) {
		const std::string executableMappings = MappingPath(SDL_GetBasePath());
		const std::string dataMappings = MappingPath(Platform_BasePath());
		LoadMappings(executableMappings);
		if (dataMappings != executableMappings) {
			LoadMappings(dataMappings);
		}
	}

	int count = 0;
	SDL_JoystickID* ids = SDL_GetGamepads(&count);
	for (int i = 0; ids && i < count; ++i) {
		OpenGamepad(ids[i]);
	}
	SDL_free(ids);
	return true;
}

void Platform_GamepadShutdown()
{
	for (auto& gamepad : s_gamepads) {
		SDL_CloseGamepad(gamepad.second);
	}
	s_gamepads.clear();
	s_lastUse.clear();
	s_active = 0;
	s_useSerial = 0;
	s_suppressed = false;
	++s_generation;
	if (s_initialized && s_enabled) {
		SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
	}
	s_initialized = false;
	s_enabled = false;
}

void Platform_GamepadProcessEvent(const SDL_Event& p_event)
{
	if (!s_enabled) {
		return;
	}
	switch (p_event.type) {
	case SDL_EVENT_GAMEPAD_ADDED:
		OpenGamepad(p_event.gdevice.which);
		break;
	case SDL_EVENT_GAMEPAD_REMOVED:
		CloseGamepad(p_event.gdevice.which);
		break;
	case SDL_EVENT_GAMEPAD_REMAPPED:
		OpenGamepad(p_event.gdevice.which);
		if (p_event.gdevice.which == s_active) {
			Platform_GamepadClearState();
		}
		break;
	case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
		if (s_gamepads.find(p_event.gbutton.which) != s_gamepads.end()) {
			SelectGamepad(p_event.gbutton.which);
		}
		break;
	case SDL_EVENT_GAMEPAD_AXIS_MOTION:
		if (MeaningfulAxis(p_event.gaxis) && s_gamepads.find(p_event.gaxis.which) != s_gamepads.end()) {
			SelectGamepad(p_event.gaxis.which);
		}
		break;
	case SDL_EVENT_WINDOW_FOCUS_LOST:
	case SDL_EVENT_WINDOW_FOCUS_GAINED:
	case SDL_EVENT_WILL_ENTER_BACKGROUND:
	case SDL_EVENT_DID_ENTER_BACKGROUND:
	case SDL_EVENT_DID_ENTER_FOREGROUND:
		Platform_GamepadClearState();
		break;
	default:
		break;
	}
}

void Platform_GamepadClearState()
{
	s_suppressed = true;
	++s_generation;
}

PLATFORM_GAMEPAD_STATE Platform_GamepadState()
{
	PLATFORM_GAMEPAD_STATE state = {};
	state.m_generation = s_generation;
	state.m_deadzone = s_deadzone;
	state.m_cursorSpeed = s_cursorSpeed;
	auto it = s_gamepads.find(s_active);
	if (!s_enabled || it == s_gamepads.end()) {
		return state;
	}

	SDL_Gamepad* gamepad = it->second;
	state.m_connected = true;
	for (int button = 0; button < SDL_GAMEPAD_BUTTON_COUNT && button < 64; ++button) {
		if (SDL_GetGamepadButton(gamepad, (SDL_GamepadButton) button)) {
			state.m_buttons |= uint64_t{1} << button;
		}
	}
	state.m_leftX = NormalizeAxis(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX));
	state.m_leftY = NormalizeAxis(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY));
	state.m_rightX = NormalizeAxis(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTX));
	state.m_rightY = NormalizeAxis(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTY));
	state.m_leftTrigger = NormalizeTrigger(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER));
	state.m_rightTrigger = NormalizeTrigger(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER));
	state.m_confirm = ConfirmButton(gamepad);
	state.m_cancel = state.m_confirm == SDL_GAMEPAD_BUTTON_SOUTH ? SDL_GAMEPAD_BUTTON_EAST : SDL_GAMEPAD_BUTTON_SOUTH;
	if (s_suppressed) {
		const bool neutral = StateIsNeutral(state);
		state.m_buttons = 0;
		state.m_leftX = 0.0f;
		state.m_leftY = 0.0f;
		state.m_rightX = 0.0f;
		state.m_rightY = 0.0f;
		state.m_leftTrigger = 0.0f;
		state.m_rightTrigger = 0.0f;
		if (neutral) {
			s_suppressed = false;
		}
	}
	return state;
}

bool Platform_GamepadEventIsActive(const SDL_Event& p_event)
{
	if (!s_enabled || !s_active) {
		return false;
	}
	switch (p_event.type) {
	case SDL_EVENT_GAMEPAD_AXIS_MOTION:
		return p_event.gaxis.which == s_active;
	case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
	case SDL_EVENT_GAMEPAD_BUTTON_UP:
		return p_event.gbutton.which == s_active;
	default:
		return false;
	}
}
