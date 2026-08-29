#define DECOMP_INLINE_STRING_COPY_LIFETIME
#define DECOMP_INLINE_LOGICSTACK_CTORS
#include "logic/logic.h"

// FUNCTION: ALIEN 0x439ef0
void LOGIC::PushStr(const STRING& p_value)
{
	LOGICSTACK value;
	value.m_type = 1;
	STRING copy(p_value);
	value.m_str = copy.m_str;
	copy.m_str = STRING::EMPTY;
	m_stack.Insert(value);
}
