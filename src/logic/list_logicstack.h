#ifndef LIST_LOGICSTACK_H
#define LIST_LOGICSTACK_H

#include "logic/logicstack.h"
#include "util/decomp.h"

#include <stdint.h>

// VTABLE: ALIEN 0x47a540

class LIST_LOGICSTACK {
public:
	LIST_LOGICSTACK();
	~LIST_LOGICSTACK();
	virtual void* ScalarDeletingDestructor(unsigned int p_flags);

	int m_n;     // 0x04
	int m_max;   // 0x08
	int* m_data; // 0x0c

	int IsLastString();
	int GetNo() const;
	LOGICSTACK* Pop();
	void Insert(class LOGICSTACK p_item);
	void Push(const LOGICSTACK& p_item);
	void Expand(int p_max);
	int DeletePointerToObject(void* p_object);
	intptr_t PopObject();

	void Release()
	{
		LOGICSTACK* data = (LOGICSTACK*) m_data;
		m_max = 0;
		m_n = 0;
		if (data) {
			delete[] data;
		}
		m_data = 0;
	}
};

#endif
