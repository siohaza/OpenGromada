#define DECOMP_INLINE_NAMED_LIST_STRUCT_LOGICVAR_DTOR
#define DECOMP_INLINE_NAMED_LIST_STRUCT_LOGICVAR_CTOR
#define DECOMP_INLINE_NAMED_LIST_STRUCT_LOGICVAR_ASSIGN
#define DECOMP_INLINE_STRING_DTOR
#include "util/named_list_logicvar.h"

#include "logic/logicvar.h"
#include "util/named_list_struct_logicvar.h"
#include "util/myerror.h"

// STUB: ALIEN 0x425080
void NAMED_LIST_LOGICVAR::Expand(int p_max)
{
	int i = 0;
	if (p_max > m_max) {
		NAMED_LIST_STRUCT_LOGICVAR* oldData = m_data;
		NAMED_LIST_STRUCT_LOGICVAR* newData = new NAMED_LIST_STRUCT_LOGICVAR[p_max];
		m_data = newData;
		if (!newData)
			MYERROR::LogExit(::Error,
				"!!!ERROR!!!::LIST: Not enough memory %i", p_max);
		if (oldData) {
			for (; i < m_max; ++i)
				m_data[i] = oldData[i];
			delete[] oldData;
		}
		m_max = p_max;
	}
}
