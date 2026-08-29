#ifndef NAMED_LIST_LOGICVAR_BASE_H
#define NAMED_LIST_LOGICVAR_BASE_H

#include "util/decomp.h"
#include "util/named_list_struct_logicvar.h"

// VTABLE: ALIEN 0x47a338

class NAMED_LIST_LOGICVAR_BASE {
public:
	NAMED_LIST_LOGICVAR_BASE()
	{
		m_data = 0;
		m_n = 0;
		m_max = 0;
	}

	// FUNCTION: ALIEN 0x412d30
	~NAMED_LIST_LOGICVAR_BASE()
	{
		if (m_data)
			delete[] m_data;
		m_data = 0;
		m_n = 0;
	}

	// FUNCTION: ALIEN 0x412f20
	virtual void* ScalarDeletingDestructor(unsigned int p_flags)
	{
		NAMED_LIST_LOGICVAR_BASE* result = this;
		this->~NAMED_LIST_LOGICVAR_BASE();
		if (p_flags & 1)
			operator delete(result);
		return result;
	}

	void Write(STREAM* p_stream) const
	{
		if (p_stream) {
			p_stream->Write(&m_n, sizeof(m_n));
			for (int i = 0; i < m_n; ++i) {
				const STRING& name = m_data[i].m_name;
				p_stream->Write(name.m_str, strlen(name.m_str) + 1);
				p_stream->Write(&m_data[i].m_var.m_flag, 20);
			}
		}
	}

	int Location(const STRING& p_name) const
	{
		int i = m_n;
		while (i) {
			--i;
			if (!strcmp(m_data[i].m_name.m_str, p_name.m_str))
				return i;
		}
		return -1;
	}

	char* GetName(int p_index) { return m_data[p_index].m_name.m_str; }

	void Release()
	{
		NAMED_LIST_STRUCT_LOGICVAR* data = m_data;
		m_max = 0;
		m_n = 0;
		if (data)
			delete[] data;
		m_data = 0;
	}

	void Expand(int p_max);

	int m_n; // 0x04
	int m_max; // 0x08
	NAMED_LIST_STRUCT_LOGICVAR* m_data; // 0x0c
};

DECOMP_SIZE_ASSERT(NAMED_LIST_LOGICVAR_BASE, 0x10)

#ifdef DECOMP_INLINE_NAMED_LIST_LOGICVAR_EXPAND
#include "util/myerror.h"

inline void NAMED_LIST_LOGICVAR_BASE::Expand(int p_max)
{
	if (p_max > m_max) {
		NAMED_LIST_STRUCT_LOGICVAR* oldData = m_data;
		NAMED_LIST_STRUCT_LOGICVAR* newData = new NAMED_LIST_STRUCT_LOGICVAR[p_max];
		m_data = newData;
		if (!newData)
			MYERROR::LogExit(::Error,
				"!!!ERROR!!!::LIST: Not enough memory %i", p_max);
		if (oldData) {
			for (int i = 0; i < m_max; ++i)
				m_data[i] = oldData[i];
		#pragma inline_depth(0)
			delete[] oldData;
		#pragma inline_depth(8)
		}
		m_max = p_max;
	}
}
#endif

#endif
