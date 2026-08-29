#define DECOMP_INLINE_STRING_DTOR
#pragma inline_depth(1)
#include "game/input_as.h"
#pragma inline_depth(8)

#include <windows.h>

#include "game/map.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "ui/mouse.h"

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

// FUNCTION: ALIEN 0x425d60
INPUT_AS::INPUT_AS()
{
	m_button &= 0xffff2000;
	m_wheel = 0;
	m_x = 0;
	m_worldX = 0;
	m_y = 0;
	m_worldY = 0;
	m_key = 0;
	m_unk0x1c = 0;
}

// FUNCTION: ALIEN 0x425d80
int INPUT_AS::ProcessMessage(void* p_wnd, unsigned int p_msg, unsigned int p_wparam, unsigned int p_lparam)
{
	switch (p_msg) {
	case 0x84: { // WM_NCHITTEST
		RECT rect;
		GetWindowRect((HWND) p_wnd, &rect);
		g_windowPosX = rect.left;
		float sx = (float) (int) (p_lparam & 0xffff) - rect.left;
		float sy = (float) (int) (p_lparam >> 16) - rect.top;
		g_windowPosY = rect.top;
		m_x = sx;
		m_y = sy;
		GRAPH_CORE* g = (GRAPH_CORE*) Graph;
		if (((GRAPH_CORE*) Graph)->m_viewXMin > sx)
			sx = ((GRAPH_CORE*) Graph)->m_viewXMin;
		if (((GRAPH_CORE*) Graph)->m_viewXMax <= sx)
			sx = ((GRAPH_CORE*) Graph)->m_viewXMax - 1.0f;
		if ((double) ((GRAPH_CORE*) Graph)->m_viewYMin > sy)
			sy = ((GRAPH_CORE*) Graph)->m_viewYMin;
		if ((double) ((GRAPH_CORE*) Graph)->m_viewYMax <= sy)
			sy = ((GRAPH_CORE*) Graph)->m_viewYMax - 1.0f;
		m_worldX = sx + Map->m_shiftX;
		m_worldY = sy + Map->m_shiftY;
		Mouse->ChangeCoor(m_worldX, m_worldY, Mouse->Z());
		sx = m_x;
		sy = m_y;
		if (sx >= ((GRAPH_CORE*) Graph)->m_viewXMin && sx < ((GRAPH_CORE*) Graph)->m_viewXMax
			&& sy >= ((GRAPH_CORE*) Graph)->m_viewYMin && sy < ((GRAPH_CORE*) Graph)->m_viewYMax)
			return 1;
		break;
	}
	case 0x1c: // WM_ACTIVATEAPP
		if (!p_wparam)
			m_button &= 0xffffc7ff;
		break;
	case 0x3: { // WM_MOVE
		RECT rect;
		GetWindowRect((HWND) p_wnd, &rect);
		g_windowPosX = rect.left;
		g_windowPosY = rect.top;
		break;
	}
	case 0x100: // WM_KEYDOWN
		m_key = p_wparam << 8;
		m_unk0x1c = p_wparam;
		if (p_wparam == (unsigned int) g_keyScrollLeft2 || p_wparam == (unsigned int) g_keyScrollLeft)
			m_button |= 0x80;
		else if (p_wparam == (unsigned int) g_keyScrollRight2 || p_wparam == (unsigned int) g_keyScrollRight)
			m_button |= 0x100;
		else if (p_wparam == (unsigned int) g_keyScrollUp2 || p_wparam == (unsigned int) g_keyScrollUp)
			m_button |= 0x400;
		else if (p_wparam == (unsigned int) g_keyScrollDown2 || p_wparam == (unsigned int) g_keyScrollDown)
			m_button |= 0x200;
		else if (p_wparam == (unsigned int) firstKey1 || p_wparam == (unsigned int) firstKey2)
			m_button |= 0x4000;
		else if (p_wparam == (unsigned int) secondKey1 || p_wparam == (unsigned int) secondKey2)
			m_button |= 0x8000;
		else if (p_wparam == 16)
			m_button |= 0x800;
		else if (p_wparam == 17)
			m_button |= 0x1000;
		break;
	case 0x101: // WM_KEYUP
		if (p_wparam == (unsigned int) g_keyScrollLeft2 || p_wparam == (unsigned int) g_keyScrollLeft)
			m_button &= ~0x80;
		else if (p_wparam == (unsigned int) g_keyScrollRight2
			|| p_wparam == (unsigned int) g_keyScrollRight)
			m_button &= ~0x100;
		else if (p_wparam == (unsigned int) g_keyScrollUp2 || p_wparam == (unsigned int) g_keyScrollUp)
			m_button &= ~0x400;
		else if (p_wparam == (unsigned int) g_keyScrollDown2
			|| p_wparam == (unsigned int) g_keyScrollDown)
			m_button &= ~0x200;
		else if (p_wparam == (unsigned int) firstKey1 || p_wparam == (unsigned int) firstKey2)
			m_button &= ~0x4000;
		else if (p_wparam == (unsigned int) secondKey1 || p_wparam == (unsigned int) secondKey2)
			m_button &= ~0x8000;
		else if (p_wparam == 16)
			m_button &= ~0x800;
		else if (p_wparam == 17)
			m_button &= ~0x1000;
		break;
	case 0x102: // WM_CHAR
		m_key = p_wparam & 0xff;
		return 0;
	case 0x201: // WM_LBUTTONDOWN
		m_button |= 0x21;
		if (firstKey1 == 1)
			m_button |= 0x4000;
		if (secondKey1 == 1)
			m_button |= 0x8000;
		return 0;
	case 0x20a: // WM_MOUSEWHEEL
		m_wheel = (short) (p_wparam >> 16) / 120;
		return 0;
	case 0x204: // WM_RBUTTONDOWN
		m_button |= 0x44;
		if (firstKey1 == 2)
			m_button |= 0x4000;
		if (secondKey1 == 2)
			m_button |= 0x8000;
		return 0;
	case 0x207: // WM_MBUTTONDOWN
		m_button |= 2;
		return 0;
	case 0x202: { // WM_LBUTTONUP
		m_button = (m_button & 0xffffffd7) | 8;
		if (firstKey1 == 1 && firstClearForButtonUp)
			m_button &= 0xffffbfff;
		if (secondKey1 != 1 || !secondClearForButtonUp)
			return 0;
		m_button &= ~0x8000;
		return 0;
	}
	case 0x205: // WM_RBUTTONUP
		m_button = (m_button & 0xffffffaf) | 0x10;
		if (firstKey1 == 2 && firstClearForButtonUp)
			m_button &= 0xffffbfff;
		if (secondKey1 == 2 && secondClearForButtonUp)
			m_button &= ~0x8000;
		return 0;
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
	if (!strcmp(p_name.m_str,
			// STRING: ALIEN 0x482330
			"LBUTTON"))
		return VK_LBUTTON;
	else if (!strcmp(p_name.m_str,
				 // STRING: ALIEN 0x482320
				 "RBUTTON"))
		return VK_RBUTTON;
	else if (!strcmp(p_name.m_str,
				 // STRING: ALIEN 0x481a04
				 "["))
		return VK_LWIN;
	else if (!strcmp(p_name.m_str,
				 // STRING: ALIEN 0x48230c
				 "]"))
		return VK_RWIN;
	else if (!strcmp(p_name.m_str,
				 // STRING: ALIEN 0x483768
				 "LEFT"))
		return VK_LEFT;
	else if (!strcmp(p_name.m_str,
				 // STRING: ALIEN 0x483760
				 "RIGHT"))
		return VK_RIGHT;
	else if (!strcmp(p_name.m_str,
				 // STRING: ALIEN 0x48375c
				 "UP"))
		return VK_UP;
	else if (!strcmp(p_name.m_str,
				 // STRING: ALIEN 0x483754
				 "DOWN"))
		return VK_DOWN;
	else if (!strcmp(p_name.m_str,
				 // STRING: ALIEN 0x48374c
				 "INSERT"))
		return VK_INSERT;
	else if (!strcmp(p_name.m_str,
				 // STRING: ALIEN 0x483744
				 "DELETE"))
		return VK_DELETE;
	else if (!strcmp(p_name.m_str,
				 // STRING: ALIEN 0x48373c
				 "HOME"))
		return VK_HOME;
	else if (!strcmp(p_name.m_str,
				 // STRING: ALIEN 0x483738
				 "END"))
		return VK_END;
	else if (!strcmp(p_name.m_str,
				 // STRING: ALIEN 0x483730
				 "PGUP"))
		return VK_PRIOR;
	else if (!strcmp(p_name.m_str,
				 // STRING: ALIEN 0x483728
				 "PGDN"))
		return VK_NEXT;
	else if (!strcmp(p_name.m_str,
				 // STRING: ALIEN 0x483720
				 "SHIFT"))
		return VK_SHIFT;
	else if (!strcmp(p_name.m_str,
				 // STRING: ALIEN 0x483718
				 "CTRL"))
		return VK_CONTROL;
	else if (!strcmp(p_name.m_str,
				 // STRING: ALIEN 0x483714
				 "F1"))
		return VK_F1;
	else if (!strcmp(p_name.m_str,
				 // STRING: ALIEN 0x483710
				 "F2"))
		return VK_F2;
	else if (!strcmp(p_name.m_str,
				 // STRING: ALIEN 0x48370c
				 "F3"))
		return VK_F3;
	else if (!strcmp(p_name.m_str,
				 // STRING: ALIEN 0x483708
				 "F4"))
		return VK_F4;
	else if (!strcmp(p_name.m_str,
				 // STRING: ALIEN 0x483704
				 "F5"))
		return VK_F5;
	else if (!strcmp(p_name.m_str,
				 // STRING: ALIEN 0x483700
				 "F6"))
		return VK_F6;
	else if (!strcmp(p_name.m_str,
				 // STRING: ALIEN 0x4836fc
				 "F7"))
		return VK_F7;
	else if (!strcmp(p_name.m_str,
				 // STRING: ALIEN 0x4836f8
				 "F8"))
		return VK_F8;
	else if (!strcmp(p_name.m_str,
				 // STRING: ALIEN 0x4836f4
				 "F9"))
		return VK_F9;
	else if (!strcmp(p_name.m_str,
				 // STRING: ALIEN 0x4836f0
				 "F10"))
		return VK_F10;
	else if (!strcmp(p_name.m_str,
				 // STRING: ALIEN 0x4836ec
				 "F11"))
		return VK_F11;
	else if (!strcmp(p_name.m_str,
				 // STRING: ALIEN 0x4836e8
				 "F12"))
		return VK_F12;
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
	if (!secondClearForButtonUp)
		m_button &= 0xffff7fff;
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
	if (secondKey1 == 1)
		m_button &= 0xffff7fff;
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
	if (secondKey1 == 2)
		m_button &= 0xffff7fff;
}
