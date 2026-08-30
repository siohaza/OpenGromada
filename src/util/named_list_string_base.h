#ifndef NAMED_LIST_STRING_BASE_H
#define NAMED_LIST_STRING_BASE_H

#include "util/decomp.h"
#include "util/named_list_struct_string.h"

#include <string.h>

// VTABLE: ALIEN 0x47a33c

class NAMED_LIST_STRING_BASE {
public:
	NAMED_LIST_STRING_BASE()
	{
		m_data = 0;
		m_n = 0;
		m_max = 0;
	}

	// FUNCTION: ALIEN 0x412da0
	~NAMED_LIST_STRING_BASE()
	{
		if (m_data) {
			delete[] m_data;
		}
		m_data = 0;
		m_n = 0;
	}

	// FUNCTION: ALIEN 0x412fa0
	virtual void* ScalarDeletingDestructor(unsigned int p_flags)
	{
		NAMED_LIST_STRING_BASE* result = this;
		this->~NAMED_LIST_STRING_BASE();
		if (p_flags & 1) {
			operator delete(result);
		}
		return result;
	}

	int Location(const STRING& p_name) const
	{
		int i = m_n;
		while (i) {
			--i;
			if (!strcmp(m_data[i].m_name.m_str, p_name.m_str)) {
				return i;
			}
		}
		return -1;
	}

	char* GetValue(int p_index) { return m_data[p_index].m_value.m_str; }

	void Release()
	{
		m_max = 0;
		m_n = 0;
		if (m_data) {
			delete[] m_data;
		}
		m_data = 0;
	}

	int m_n;                          // 0x04
	int m_max;                        // 0x08
	NAMED_LIST_STRUCT_STRING* m_data; // 0x0c
};

#endif
