#include "util/named_list_logicvar.h"

#include "logic/logicvar.h"
#include "util/myerror.h"
#include "util/named_list_struct_logicvar.h"

// FUNCTION: ALIEN 0x40b0a0
NAMED_LIST_LOGICVAR::NAMED_LIST_LOGICVAR()
{
}

int NAMED_LIST_LOGICVAR::GetNo() const
{
	return m_n;
}

// FUNCTION: ALIEN 0x424a20
void NAMED_LIST_LOGICVAR::Insert(STRING p_name, LOGICVAR p_value)
{
	if (m_n >= m_max) {
		int newMax = 2 * m_max + 4;
		if (newMax > m_max) {
			NAMED_LIST_STRUCT_LOGICVAR* oldData = m_data;
			NAMED_LIST_STRUCT_LOGICVAR* newData = new NAMED_LIST_STRUCT_LOGICVAR[newMax];
			m_data = newData;
			if (!newData) {
				MYERROR::LogExit(::Error, "!!!ERROR!!!::LIST: Not enough memory %i", newMax);
			}
			if (oldData) {
				for (int i = 0; i < m_max; ++i) {
					m_data[i] = oldData[i];
				}
				delete[] oldData;
			}
			m_max = newMax;
		}
	}
	m_data[m_n].m_name = p_name;
	LOGICVAR& entry = m_data[m_n++].m_var;
	entry.m_flag = p_value.m_flag;
	entry.m_value = p_value.m_value;
	entry.m_a = p_value.m_a;
	entry.m_type = p_value.m_type;
	entry.m_b = p_value.m_b;
	entry.m_c = p_value.m_c;
	entry.m_d = p_value.m_d;
	entry.m_extra = p_value.m_extra;
}
