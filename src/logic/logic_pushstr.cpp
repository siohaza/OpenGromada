#include "logic/logic.h"

// FUNCTION: ALIEN 0x439ef0
void LOGIC::PushStr(const STRING& p_value)
{
	m_stack.Insert(LOGICSTACK(p_value));
}
