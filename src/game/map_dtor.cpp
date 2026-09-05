#include "audio/sound.h"
#include "game/const.h"
#include "game/map.h"
#include "gfx/graph.h"
#include "gfx/texture.h"
#include "platform/gamepad.h"
#include "platform/portable_config.h"
#include "platform/render.h"
#include "ui/mouse.h"
#include "util/myerror.h"
#include "util/profile.h"
#include "util/registry.h"
#include "video/vid.h"
#include "world/hash_map.h"

#include <SDL3/SDL.h>

// FUNCTION: ALIEN 0x40b180
MAP::~MAP()
{
	DiscardScriptFiles();
	ClearTerrainCamera();
	for (int i = 0; i < 17; ++i) {
		m_layers[i].DeleteAll();
	}
	if (Mouse) {
		Mouse->ScalarDeletingDestructor(1);
	}
	for (int p = 0; p < 4; ++p) {
		if (m_player[p]) {
			delete m_player[p];
		}
	}
	if (Hash) {
		delete Hash;
	}
	if (Strings) {
		delete Strings;
	}
	if (Sound) {
		delete Sound;
	}
	if (Const) {
		delete Const;
	}
	if (Registry) {
		delete Registry;
	}
	{
		for (int v = m_noVid - 1; v >= 0; --v) {
			if (m_vids[v]) {
				m_vids[v]->ScalarDeletingDestructor(1);
				m_vids[v] = 0;
			}
		}
		m_noVid = 0;
		MYERROR::Log(
			::Error,
			// STRING: ALIEN 0x4824ec
			"Vid    release %i %i",
			TextureMemoryInUse,
			VID::MemoryInUse
		);
	}
	if (Graph) {
		delete Graph;
	}
	if (m_groundz) {
		operator delete(m_groundz);
	}
	if (m_tempGroundz) {
		operator delete(m_tempGroundz);
	}
	if (m_weapon) {
		operator delete(m_weapon);
	}
	if (::Error) {
		delete ::Error;
	}
}

// STUB: ALIEN 0x40b930
int MAP::ProcessEvent(const SDL_Event& p_event)
{
	Platform_GamepadProcessEvent(p_event);
	if ((m_flag & 8) && m_input.ProcessEvent(p_event)) {
		return 1;
	}

	switch (p_event.type) {
	case SDL_EVENT_RENDER_DEVICE_RESET:
		if (Platform_RenderHandleDeviceReset()) {
			MYERROR::Log(::Error, "SDL renderer reset recovery failed: %s", SDL_GetError());
		}
		return 0;

	case SDL_EVENT_QUIT:
	case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
		if (!(((GRAPH_CORE*) Graph)->m_flags & 0x80) && m_window) {
			int x = 0;
			int y = 0;
			SDL_GetWindowPosition((SDL_Window*) m_window, &x, &y);
			PortableConfig_SetInt("window", "PositionX", x);
			PortableConfig_SetInt("window", "PositionY", y);
			PortableConfig_Flush();
		}
		m_quit = 1;
		return 0;

	// Focus drives the same pause the Win32 build hung off WM_ACTIVATEAPP:
	// audio stops and the hardware cursor is released while the game is in the
	// background.
	case SDL_EVENT_WINDOW_FOCUS_GAINED:
	case SDL_EVENT_DID_ENTER_FOREGROUND:
	case SDL_EVENT_WINDOW_FOCUS_LOST:
	case SDL_EVENT_WILL_ENTER_BACKGROUND:
	case SDL_EVENT_DID_ENTER_BACKGROUND: {
		int active = p_event.type == SDL_EVENT_WINDOW_FOCUS_GAINED || p_event.type == SDL_EVENT_DID_ENTER_FOREGROUND;
		m_flag = (m_flag & 0xfffffff7) | (8 * active);
		if (Sound && Mouse) {
			if (active) {
				Sound->Resume();
				Mouse->Enable();
			}
			else {
				Sound->Pause();
				Mouse->Disable();
			}
		}
		return 0;
	}
	}
	return 0;
}
