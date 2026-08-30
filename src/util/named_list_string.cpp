#include "util/named_list_string.h"

#include "util/myerror.h"
#include "util/named_list_struct_string.h"

// FUNCTION: ALIEN 0x40b0c0
NAMED_LIST_STRING::NAMED_LIST_STRING()
{
}

// FUNCTION: ALIEN 0x424b70
void NAMED_LIST_STRING::Insert(STRING p_name, STRING p_value)
{
	if (m_n >= m_max) {
		int newMax = 2 * m_max + 4;
		if (newMax > m_max) {
			NAMED_LIST_STRUCT_STRING* oldData = m_data;
			NAMED_LIST_STRUCT_STRING* newData = new NAMED_LIST_STRUCT_STRING[newMax];
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
	m_data[m_n++].m_value = p_value;
}
