#include "logic/list_logicstack.h"

#include "util/myerror.h"

#include <stdio.h>
#include <stdlib.h>

LIST_LOGICSTACK::LIST_LOGICSTACK() : m_n(0), m_max(0), m_data(0)
{
}

LIST_LOGICSTACK::~LIST_LOGICSTACK()
{
	delete[] (LOGICSTACK*) m_data;
	m_data = 0;
	m_n = 0;
}

void* LIST_LOGICSTACK::ScalarDeletingDestructor(unsigned int p_flags)
{
	LIST_LOGICSTACK* result = this;
	this->~LIST_LOGICSTACK();
	if (p_flags & 1) {
		operator delete(result);
	}
	return result;
}

// FUNCTION: ALIEN 0x424460
int LIST_LOGICSTACK::DeletePointerToObject(void* p_object)
{
	int result;
	int i;
	i = 0;
	result = 0;
	if (m_n > 0) {
		do {
			LOGICSTACK* e = &((LOGICSTACK*) m_data)[i];
			if ((e->m_type & 0x10) && e->m_num == (intptr_t) p_object) {
				e->m_num = 0;
				((LOGICSTACK*) m_data)[i].m_type &= ~0x10;
				++result;
			}
			++i;
		} while (i < m_n);
	}
	return result;
}

// FUNCTION: ALIEN 0x424980
int LIST_LOGICSTACK::GetNo() const
{
	return m_n;
}

// FUNCTION: ALIEN 0x424990
void LIST_LOGICSTACK::Push(const LOGICSTACK& p_item)
{
	Insert(p_item);
}

// FUNCTION: ALIEN 0x424e60
void LIST_LOGICSTACK::Insert(LOGICSTACK p_item)
{
	if (m_n >= m_max) {
		int newMax = 2 * m_max + 4;
		if (newMax > m_max) {
			LOGICSTACK* oldData = (LOGICSTACK*) m_data;
			LOGICSTACK* newData = new LOGICSTACK[newMax];
			m_data = (int*) newData;
			if (!newData) {
				MYERROR::LogExit(
					::Error,
					// STRING: ALIEN 0x47f7c8
					"!!!ERROR!!!::LIST: Not enough memory %i",
					newMax
				);
			}
			if (oldData) {
				for (int i = 0; i < m_max; ++i) {
					((LOGICSTACK*) m_data)[i] = oldData[i];
				}

				delete[] oldData;
			}
			m_max = newMax;
		}
	}
	LOGICSTACK* item = &((LOGICSTACK*) m_data)[m_n++];
	item->m_type = p_item.m_type;
	item->m_num = p_item.m_num;
	item->m_str = p_item.m_str;
}

// FUNCTION: ALIEN 0x439fd0
intptr_t LIST_LOGICSTACK::PopObject()
{
	LOGICSTACK* top = &((LOGICSTACK*) m_data)[m_n];
	if (top[-1].m_num && !(top[-1].m_type & 0x10)) {
		MYERROR::Error(Error, "LOGIC", 10, "this variable is not unit", 0);
	}
	LOGICSTACK* e = &((LOGICSTACK*) m_data)[--m_n];
	if (e->m_type & 1) {
		const char* str = e->m_str.m_str;
		if (str[1] != 'x') {
			return atoi(str);
		}
		int v;
		sscanf(str, "%i", &v);
		return v;
	}
	return e->m_num;
}

// FUNCTION: ALIEN 0x43a190
int LIST_LOGICSTACK::IsLastString()
{
	return m_n > 0 && (((LOGICSTACK*) m_data)[m_n - 1].m_type & 1);
}

// FUNCTION: ALIEN 0x43ab70
LOGICSTACK* LIST_LOGICSTACK::Pop()
{
	int v1 = m_n - 1;
	m_n = v1;
	return (LOGICSTACK*) m_data + v1;
}
