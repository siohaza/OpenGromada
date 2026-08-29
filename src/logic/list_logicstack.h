#ifndef LIST_LOGICSTACK_H
#define LIST_LOGICSTACK_H

#include "util/decomp.h"
#include "logic/logicstack.h"

// VTABLE: ALIEN 0x47a540

class LIST_LOGICSTACK {
public:
#ifdef DECOMP_INLINE_LIST_LOGICSTACK_SPECIAL_MEMBERS

	// FUNCTION: ALIEN 0x412ca0
	LIST_LOGICSTACK()
		: m_data(0)
	{
		m_n = 0;
		m_max = 0;
	}

	// FUNCTION: ALIEN 0x412cc0
	~LIST_LOGICSTACK()
	{
		LOGICSTACK* data = (LOGICSTACK*) m_data;
		if (data)
			delete[] data;
		m_data = 0;
		m_n = 0;
	}

	// FUNCTION: ALIEN 0x412eb0
	virtual void* ScalarDeletingDestructor(unsigned int p_flags)
	{
		LIST_LOGICSTACK* result = this;
		this->~LIST_LOGICSTACK();
		if (p_flags & 1)
			operator delete(result);
		return result;
	}
#else
	LIST_LOGICSTACK();
	~LIST_LOGICSTACK();
	virtual void* ScalarDeletingDestructor(unsigned int p_flags);
#endif

	int m_n; // 0x04
	int m_max; // 0x08
	int* m_data; // 0x0c

	int IsLastString();
	int GetNo() const;
	LOGICSTACK* Pop();
	void Insert(class LOGICSTACK p_item);
	void Push(const LOGICSTACK& p_item);
	void Expand(int p_max);
	int DeletePointerToObject(void* p_object);
	int PopObject();

	void Release()
	{
		LOGICSTACK* data = (LOGICSTACK*) m_data;
		m_max = 0;
		m_n = 0;
		if (data)
			delete[] data;
		m_data = 0;
	}
};

#endif
