#define DECOMP_INLINE_STRING_COPY_LIFETIME
#define DECOMP_INLINE_INT2STR
#define DECOMP_INLINE_LOGICSTACK_STRING

#include "game/map.h"

// FUNCTION: ALIEN 0x43a390
STRING* MAP::PopStr()
{
	LOGICSTACK* value = (LOGICSTACK*) m_logic.m_stack.m_data +
		--m_logic.m_stack.m_n;
	return value->String();
}
