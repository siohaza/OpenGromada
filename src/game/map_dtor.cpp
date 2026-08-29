#define DECOMP_INLINE_STRING_DTOR
#define DECOMP_INLINE_LIST_SPRITE_SPECIAL_MEMBERS

#define DECOMP_INLINE_NAMED_LIST_STRUCT_STRING_DTOR
#define DECOMP_INLINE_NAMED_LIST_STRUCT_LOGICVAR_DTOR
#include "game/map.h"

#include "audio/sound.h"
#include "game/const.h"
#include "gfx/graph.h"
#include "gfx/texture.h"
#include "ui/mouse.h"
#include "util/myerror.h"
#include "util/profile.h"
#include "util/registry.h"
#include "video/vid.h"
#include "world/hash_map.h"

// FUNCTION: ALIEN 0x40b180
MAP::~MAP()
{
	for (int i = 0; i < 17; ++i)
		m_layers[i].DeleteAll();
	if (Mouse)
		Mouse->ScalarDeletingDestructor(1);
	for (int p = 0; p < 4; ++p) {
		if (m_player[p])
			delete m_player[p];
	}
	if (Hash)
		delete Hash;
	if (Strings)
		delete Strings;
	if (Sound)
		delete Sound;
	if (Const)
		delete Const;
	if (Registry)
		delete Registry;
	if (m_vids) {
		for (int v = m_noVid - 1; v >= 0; --v) {
			if (m_vids[v]) {
				m_vids[v]->ScalarDeletingDestructor(1);
				m_vids[v] = 0;
			}
		}
		m_noVid = 0;
		MYERROR::Log(::Error,
			// STRING: ALIEN 0x4824ec
			"Vid    release %i %i", TextureMemoryInUse, VID::MemoryInUse);
	}
	if (Graph)
		delete Graph;
	if (m_groundz)
		operator delete(m_groundz);
	if (m_tempGroundz)
		operator delete(m_tempGroundz);
	if (m_weapon)
		operator delete(m_weapon);
	if (::Error)
		delete (MYERROR*) ::Error;
	CoUninitialize();
	timeEndPeriod(1);
}

// STUB: ALIEN 0x40b930
int MAP::WndProc(void* p_wnd, unsigned int p_msg, unsigned int p_wparam, int p_lparam)
{
	if ((m_flag & 8) && m_input.ProcessMessage(p_wnd, p_msg, p_wparam, p_lparam))
		return 1;

	switch (p_msg) {
	case 0x2: // WM_DESTROY
		if (!(((GRAPH_CORE*) Graph)->m_flags & 0x80)) {
			RECT rect;
			GetWindowRect((HWND) m_hWnd, &rect);
			Registry->SetInt(
				STRING("WindowPositionX", STRING::INLINE_CHARP), rect.left);
			Registry->SetInt(
				STRING("WindowPositionY", STRING::INLINE_CHARP), rect.top);
		}
		m_hWnd = 0;
		PostQuitMessage(0);
		return 0;

	case 0xf: // WM_PAINT
		MYERROR::Log(::Error,
			// STRING: ALIEN 0x482508
			"WM_PAINT");
		return 0;

	case 0x1c: // WM_ACTIVATEAPP
		m_flag = (m_flag & 0xfffffff7) | (8 * ((p_wparam != 0) & 1));
		if (Sound && Mouse) {
			if (m_flag & 8) {
				Sound->Resume();
				Mouse->Enable();
			}
			else {
				Sound->Pause();
				Mouse->Disable();
			}
		}
		return 0;

	case 0x112: // WM_SYSCOMMAND
		switch (p_wparam) {
		case 0xf000: // SC_SIZE
		case 0xf010: // SC_MOVE
		case 0xf030: // SC_MAXIMIZE
		case 0xf170: // SC_MONITORPOWER
			if (((GRAPH_CORE*) Graph)->m_flags & 0x80)
				return 1;
			break;
		}
		break;

	case 0x211: // WM_ENTERMENULOOP
	case 0x231: // WM_ENTERSIZEMOVE
		((GRAPH_CORE*) Graph)->Pause();
		Sound->Pause();
		return 0;

	case 0x212: // WM_EXITMENULOOP
	case 0x232: // WM_EXITSIZEMOVE
		((GRAPH_CORE*) Graph)->Resume();
		Sound->Resume();
		return 0;
	}
	return 0;
}
