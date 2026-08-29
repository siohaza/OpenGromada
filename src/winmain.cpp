
#define DECOMP_INLINE_STRING_CHARP_CTOR
#define DECOMP_INLINE_STRING_DTOR
#define DECOMP_STRING_NEW_EXPR

#include <windows.h>

#include "game/map.h"
#include "game/map_steam.h"
#include "game/settings.h"
#include "util/myerror.h"
#include "util/string.h"

// STUB: ALIEN 0x404f00
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	__try {
		Map = new MAP_STEAM(hInstance, hPrevInstance, STRING(lpCmdLine), nShowCmd, &Settings);
		if (Map) {
			if (Map->m_flag & 4)
				while (!Map->Tact())
					;
			delete Map;
		}
	}
	__except (((MYERROR*) ::Error)->LogException(((LPEXCEPTION_POINTERS) GetExceptionInformation())->ExceptionRecord)) {
	}
	return 0;
}
