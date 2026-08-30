#include "logic/logic.h"

// FUNCTION: ALIEN 0x43a060
void LOGIC::PushInt(int p_value)
{
	m_stack.Insert(LOGICSTACK(p_value));
}

// FUNCTION: ALIEN 0x43a0f0
void LOGIC::PushObject(const void* p_object)
{
	LOGICSTACK value(p_object);
	m_stack.Insert(value);
}
