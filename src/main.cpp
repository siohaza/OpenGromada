#define DECOMP_INLINE_STRING_CHARP_CTOR_CALLS_COPY
#define DECOMP_INLINE_STRING_COPY_LIFETIME

#include <windows.h>

#include <stdlib.h>
#include <string.h>

#include "game/const.h"
#include "game/map.h"
#include "game/settings.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "ui/dlgitem.h"
#include "util/registry.h"
#include "util/string.h"

extern STRING Date2Str();
extern STRING Time2Str();

extern char CurrentSav[];

// GLOBAL: ALIEN 0x4905ec
static unsigned char g_passwordInit;
// GLOBAL: ALIEN 0x4905f0
static char* g_password;

static void password_str_atexit();

static inline STRING SelectedMapName(MAP* p_map, int p_save, const char* p_mask)
{
	return p_map->GetMapFileName(p_save, p_mask);
}

// STUB: ALIEN 0x404260
int __stdcall OptionsDialogProc(HWND p_dlg, UINT p_msg, WPARAM p_wparam, LPARAM p_lparam)
{
	if (!(g_passwordInit & 1)) {
		g_passwordInit |= 1;
		g_password = STRING::EMPTY;
		atexit(password_str_atexit);
	}
	DLGITEM devItem = { p_dlg, 1000 };
	DLGITEM modeItem = { p_dlg, 1001 };
	DLGITEM winItem = { p_dlg, 1005 };
	DLGITEM pwItem = { p_dlg, 1013 };

	char* typed;
	char* entered;
	char* value;
	char* now;
	char* again;
	pwItem.GetText(&typed);
	EnableWindow(GetDlgItem(p_dlg, 1017), strcmp(g_password, typed) == 0);
	if (typed != STRING::EMPTY)
		operator delete(typed);

	if (strcmp(g_password, empty_str)) {
		ShowWindow(GetDlgItem(p_dlg, 1015), 5);
		ShowWindow(GetDlgItem(p_dlg, 1016), 0);
	} else {
		ShowWindow(GetDlgItem(p_dlg, 1015), 0);
		ShowWindow(GetDlgItem(p_dlg, 1016), 5);
	}

	switch (p_msg) {
	case WM_INITDIALOG: {
		SetWindowTextA(p_dlg, Settings.m_appName);

		if (!(Settings.m_flag & 4)) {
			ShowWindow(GetDlgItem(p_dlg, 1005), 0);
			ShowWindow(GetDlgItem(p_dlg, 1006), 0);
			ShowWindow(GetDlgItem(p_dlg, 1007), 0);
			ShowWindow(GetDlgItem(p_dlg, 1008), 0);
			ShowWindow(GetDlgItem(p_dlg, 1009), 0);
			ShowWindow(GetDlgItem(p_dlg, 1010), 0);
			ShowWindow(GetDlgItem(p_dlg, 1012), 0);
		}
		STRING pw = Registry->GetString(
			// STRING: ALIEN 0x47f700
			STRING("Password", STRING::CALL_COPY), STRING(empty_str, STRING::CALL_COPY));
		*(STRING*) &g_password = pw;
		SendDlgItemMessageA(p_dlg, 1012, BM_SETCHECK,
			Registry->GetInt(STRING("SoundHighQuality", STRING::CALL_COPY), 1) != 0, 0);
		((GRAPH_CORE*) Graph)->SelectDisplayMode(&devItem, &modeItem, &winItem);
		SendDlgItemMessageA(p_dlg, 1017, CB_ADDSTRING, 0,
			(LPARAM) STRING(
				// STRING: ALIEN 0x47f6f4
				"Green Blood", STRING::CALL_COPY)
						 .m_str);
		SendDlgItemMessageA(p_dlg, 1017, CB_ADDSTRING, 0,
			(LPARAM) STRING(
				// STRING: ALIEN 0x47f6e8
				"Red Blood", STRING::CALL_COPY)
						 .m_str);
		int blood = Registry->GetInt(
			// STRING: ALIEN 0x47f70c
			STRING("Blood", STRING::CALL_COPY), 0);
		SendDlgItemMessageA(p_dlg, 1017, CB_SETCURSEL, blood, 0);
		if (SendDlgItemMessageA(p_dlg, 1017, CB_GETCURSEL, 0, 0)) {
			SetDlgItemTextA((HWND) pwItem.m_hDlg, pwItem.m_id,
				(*(STRING*) &g_password = empty_str).m_str);
		}
		return 1;
	}

	case WM_COMMAND:
		switch (LOWORD(p_wparam)) {
		case 1000:
			if ((short) HIWORD(p_wparam) == 9)
				((GRAPH_CORE*) Graph)->SelectDisplayMode(&devItem, &modeItem, &winItem);
			break;

		case 1001:
			if ((short) HIWORD(p_wparam) == 9)
				((GRAPH_CORE*) Graph)->SelectDisplayMode(&devItem, &modeItem, &winItem);
			break;

		case 2:
			EndDialog(p_dlg, 0);
			break;

		case 1: {

			((GRAPH_CORE*) Graph)->SelectDisplayMode(&devItem, &modeItem, &winItem);
			Registry->SetInt(STRING("SoundHighQuality", STRING::CALL_COPY),
				SendDlgItemMessageA(p_dlg, 1012, BM_GETCHECK, 0, 0) == 1);
			Registry->SetInt(STRING("Blood", STRING::CALL_COPY),
				(int) SendDlgItemMessageA(p_dlg, 1017, CB_GETCURSEL, 0, 0));
			pwItem.GetText(&entered);
			int changed = strcmp(Registry->GetString(STRING("Password", STRING::CALL_COPY),
										STRING(empty_str, STRING::CALL_COPY))
									 .m_str,
								 entered)
				!= 0;
			if (entered != STRING::EMPTY)
				operator delete(entered);
			if (changed) {
				pwItem.GetText(&value);
				Registry->SetString(STRING("Password", STRING::CALL_COPY),
					*(STRING*) &value);
				if (value != STRING::EMPTY)
					operator delete(value);
			}
			EndDialog(p_dlg, 1);
			break;
		}

		case 1013: {
			pwItem.GetText(&now);
			STRING empty(empty_str, STRING::CALL_COPY);
			int changed;
			if (!strcmp(empty.m_str, now)) {
				changed = 0;
			} else {
				pwItem.GetText(&again);
				int mismatch = strcmp(g_password, again) != 0;
				if (again != STRING::EMPTY)
					operator delete(again);
				changed = mismatch ? 1 : 0;
			}
			if (now != STRING::EMPTY)
				operator delete(now);
			if (changed)
				SendDlgItemMessageA(p_dlg, 1017, CB_SETCURSEL, 0, 0);
			break;
		}

		case 1017:
			if ((short) HIWORD(p_wparam) == 9
				&& SendDlgItemMessageA(p_dlg, 1017, CB_GETCURSEL, 0, 0)) {
				SetDlgItemTextA((HWND) pwItem.m_hDlg, pwItem.m_id,
					(*(STRING*) &g_password = empty_str).m_str);
			}
			break;
		}
		break;
	}
	return 0;
}

// FUNCTION: ALIEN 0x404c10
static void password_str_atexit()
{
	if (g_password != STRING::EMPTY)
		operator delete(g_password);
}

// FUNCTION: ALIEN 0x404c30
LRESULT __stdcall MainWndProc(HWND p_wnd, UINT p_msg, WPARAM p_wparam, LPARAM p_lparam)
{
	if (Map->WndProc(p_wnd, p_msg, p_wparam, p_lparam))
		return 1;
	if (p_msg != 0x111)
		return DefWindowProcA(p_wnd, p_msg, p_wparam, p_lparam);
	switch ((unsigned short) p_wparam) {
	case 0x9C42:
		if (Const && Const->m_mapName) {
			Map->SaveMap(STRING(CurrentSav));
			return 0;
		}
		break;
	case 0x9C41:
		if (Const && Const->m_mapName) {
			Map->Load(SelectedMapName(Map, 0,
										  // STRING: ALIEN 0x47f750
										  "Map Files"));
			return 0;
		}
		break;
	case 0x9C57: {
		STRING path =
			// STRING: ALIEN 0x47f744
			"Screens\\" + Date2Str() + " " + Time2Str() +
			".tga";
		path.Replace(":", "h");
		path.Replace(":",
					 // STRING: ALIEN 0x47f72c
					 "m");
		path.Replace(":",
					 // STRING: ALIEN 0x47f728
					 "s");
		Graph->ScreenShot(&path, 0, 0, (int) Graph->m_width, (int) Graph->m_height);
		break;
	}
	case 0x9C44:
		PostMessageA(p_wnd, 0x10, 0, 0);
		break;
	}
	return 0;
}
