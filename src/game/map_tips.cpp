#include "game/map.h"
#include "util/profile.h"

#include <stdlib.h>
#include <string.h>

// STUB: ALIEN 0x40f460
STRING MAP::GetMouseTipsString() const
{
	STRING result(empty_str);
	if (!(m_flag & 0x10)) {
		result = m_player[m_curArmy]->GetMouseTipsString();
	}
	if (!strcmp(result.m_str, empty_str) && m_menu.m_underCursor) {
		STRING key =
			// STRING: ALIEN 0x482904
			"MenuVid" + Int2Str(m_menu.NVidUnderCursor());
		result = Strings->GetString(
			// STRING: ALIEN 0x4828f8
			STRING("MouseTips"),
			key +
				// STRING: ALIEN 0x4828f0
				"AllDir",
			STRING(empty_str)
		);
		if (!strcmp(result.m_str, empty_str)) {
			result = Strings->GetString(
				STRING("MouseTips"),
				key +
					// STRING: ALIEN 0x4828ec
					"Dir" + Int2Str(m_menu.NDirUnderCursor()),
				STRING(empty_str)
			);
		}
	}
	return result;
}
