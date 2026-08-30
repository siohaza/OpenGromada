#include "game/input_as.h"

#include "game/map.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "platform/keycodes.h"
#include "ui/mouse.h"

#include <SDL3/SDL.h>

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

int UpdatePointer(INPUT_AS* p_input, float p_x, float p_y)
{
	p_input->m_x = p_x;
	p_input->m_y = p_y;

	GRAPH_CORE* graph = (GRAPH_CORE*) Graph;
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

	p_input->m_worldX = worldX + Map->m_shiftX;
	p_input->m_worldY = worldY + Map->m_shiftY;
	Mouse->ChangeCoor(p_input->m_worldX, p_input->m_worldY, Mouse->Z());

	return p_input->m_x >= graph->m_viewXMin && p_input->m_x < graph->m_viewXMax && p_input->m_y >= graph->m_viewYMin &&
		   p_input->m_y < graph->m_viewYMax;
}

} // namespace

// FUNCTION: ALIEN 0x425d60
INPUT_AS::INPUT_AS()
{
	m_button = 0;
	m_wheel = 0;
	m_x = 0;
	m_worldX = 0;
	m_y = 0;
	m_worldY = 0;
	m_key = 0;
	m_unk0x1c = 0;
}

// FUNCTION: ALIEN 0x425d80
int INPUT_AS::ProcessEvent(const SDL_Event& p_event)
{
	switch (p_event.type) {
	case SDL_EVENT_MOUSE_MOTION:
		return UpdatePointer(this, p_event.motion.x, p_event.motion.y);

	case SDL_EVENT_WINDOW_FOCUS_LOST:
		// Drop held input across focus changes.
		m_button = 0;
		m_wheel = 0;
		m_key = 0;
		m_unk0x1c = 0;
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
		if (key == (unsigned int) g_keyScrollLeft2 || key == (unsigned int) g_keyScrollLeft) {
			m_button |= 0x80;
		}
		else if (key == (unsigned int) g_keyScrollRight2 || key == (unsigned int) g_keyScrollRight) {
			m_button |= 0x100;
		}
		else if (key == (unsigned int) g_keyScrollUp2 || key == (unsigned int) g_keyScrollUp) {
			m_button |= 0x400;
		}
		else if (key == (unsigned int) g_keyScrollDown2 || key == (unsigned int) g_keyScrollDown) {
			m_button |= 0x200;
		}
		else if (key == (unsigned int) firstKey1 || key == (unsigned int) firstKey2) {
			m_button |= 0x4000;
		}
		else if (key == (unsigned int) secondKey1 || key == (unsigned int) secondKey2) {
			m_button |= 0x8000;
		}
		else if (key == VK_SHIFT) {
			m_button |= 0x800;
		}
		else if (key == VK_CONTROL) {
			m_button |= 0x1000;
		}
		break;
	}

	case SDL_EVENT_KEY_UP: {
		unsigned int key = (unsigned int) Platform_KeyToVirtualKey(p_event.key.key);
		if (!key) {
			break;
		}
		if (key == (unsigned int) g_keyScrollLeft2 || key == (unsigned int) g_keyScrollLeft) {
			m_button &= ~0x80;
		}
		else if (key == (unsigned int) g_keyScrollRight2 || key == (unsigned int) g_keyScrollRight) {
			m_button &= ~0x100;
		}
		else if (key == (unsigned int) g_keyScrollUp2 || key == (unsigned int) g_keyScrollUp) {
			m_button &= ~0x400;
		}
		else if (key == (unsigned int) g_keyScrollDown2 || key == (unsigned int) g_keyScrollDown) {
			m_button &= ~0x200;
		}
		else if (key == (unsigned int) firstKey1 || key == (unsigned int) firstKey2) {
			m_button &= ~0x4000;
		}
		else if (key == (unsigned int) secondKey1 || key == (unsigned int) secondKey2) {
			m_button &= ~0x8000;
		}
		else if (key == VK_SHIFT) {
			m_button &= ~0x800;
		}
		else if (key == VK_CONTROL) {
			m_button &= ~0x1000;
		}
		break;
	}

	case SDL_EVENT_TEXT_INPUT:
		m_key = (unsigned char) p_event.text.text[0];
		return 0;

	case SDL_EVENT_MOUSE_BUTTON_DOWN:
		if (!UpdatePointer(this, p_event.button.x, p_event.button.y)) {
			return 0;
		}
		if (p_event.button.button == SDL_BUTTON_LEFT) {
			m_button |= 0x21;
			if (firstKey1 == VK_LBUTTON) {
				m_button |= 0x4000;
			}
			if (secondKey1 == VK_LBUTTON) {
				m_button |= 0x8000;
			}
		}
		else if (p_event.button.button == SDL_BUTTON_RIGHT) {
			m_button |= 0x44;
			if (firstKey1 == VK_RBUTTON) {
				m_button |= 0x4000;
			}
			if (secondKey1 == VK_RBUTTON) {
				m_button |= 0x8000;
			}
		}
		else if (p_event.button.button == SDL_BUTTON_MIDDLE) {
			m_button |= 2;
		}
		return 0;

	case SDL_EVENT_MOUSE_BUTTON_UP: {
		bool inside = UpdatePointer(this, p_event.button.x, p_event.button.y) != 0;
		if (p_event.button.button == SDL_BUTTON_LEFT) {
			m_button &= 0xffffffd7;
			if (inside) {
				m_button |= 8;
			}
			if (firstKey1 == VK_LBUTTON && firstClearForButtonUp) {
				m_button &= 0xffffbfff;
			}
			if (secondKey1 == VK_LBUTTON && secondClearForButtonUp) {
				m_button &= ~0x8000;
			}
		}
		else if (p_event.button.button == SDL_BUTTON_RIGHT) {
			m_button &= 0xffffffaf;
			if (inside) {
				m_button |= 0x10;
			}
			if (firstKey1 == VK_RBUTTON && firstClearForButtonUp) {
				m_button &= 0xffffbfff;
			}
			if (secondKey1 == VK_RBUTTON && secondClearForButtonUp) {
				m_button &= ~0x8000;
			}
		}
		return 0;
	}

	case SDL_EVENT_MOUSE_WHEEL:
		m_wheel = (int) p_event.wheel.y;
		return 0;

	default:
		break;
	}

	return 0;
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
	unsigned int result = m_button & 0xffffffe0;
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
