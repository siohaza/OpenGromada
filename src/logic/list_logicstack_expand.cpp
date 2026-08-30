#include "logic/list_logicstack.h"
#include "util/myerror.h"

// FUNCTION: ALIEN 0x424f70
void LIST_LOGICSTACK::Expand(int p_max)
{
	if (p_max > m_max) {
		LOGICSTACK* oldData = (LOGICSTACK*) m_data;
		LOGICSTACK* newData = new LOGICSTACK[p_max];
		m_data = (int*) newData;
		if (!newData) {
			MYERROR::LogExit(::Error, "!!!ERROR!!!::LIST: Not enough memory %i", p_max);
		}
		if (oldData) {
			for (int i = 0; i < m_max; ++i) {
				((LOGICSTACK*) m_data)[i] = oldData[i];
			}
			delete[] oldData;
		}
		m_max = p_max;
	}
}
