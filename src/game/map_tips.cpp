#define DECOMP_INLINE_STRING_DTOR
#define DECOMP_INLINE_INT2STR_CALL_CTOR
#include "game/map.h"

#include <stdlib.h>
#include <string.h>

#include "util/profile.h"

// STUB: ALIEN 0x40f460
STRING MAP::GetMouseTipsString() const
{
	STRING result(empty_str, STRING::CALL_COPY);
	if (!(m_flag & 0x10))
		result = m_player[m_curArmy]->GetMouseTipsString();
	if (!strcmp(result.m_str, empty_str) && m_menu.m_underCursor) {
		STRING key =
			// STRING: ALIEN 0x482904
			"MenuVid" + Int2Str(m_menu.NVidUnderCursor());
		result = Strings->GetString(
			// STRING: ALIEN 0x4828f8
			STRING("MouseTips", STRING::CALL_COPY),
			key +
				// STRING: ALIEN 0x4828f0
				"AllDir",
			STRING(empty_str, STRING::CALL_COPY));
		if (!strcmp(result.m_str, empty_str))
			result = Strings->GetString(STRING("MouseTips", STRING::CALL_COPY),
				key +
					// STRING: ALIEN 0x4828ec
					"Dir"
					+ Int2Str(m_menu.NDirUnderCursor()),
				STRING(empty_str, STRING::CALL_COPY));
	}
	return result;
}
