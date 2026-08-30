#ifndef NAMED_LIST_LOGICVAR_BASE_H
#define NAMED_LIST_LOGICVAR_BASE_H

#include "util/decomp.h"
#include "util/named_list_struct_logicvar.h"

#include <stdint.h>

inline static void WriteLogicVarRecord(STREAM* p_stream, const LOGICVAR& p_var)
{
	const uint8_t flagAndPadding[4] = {(uint8_t) p_var.m_flag, 0, 0, 0};
	const uint32_t legacyStringToken = 0;
	const int32_t a = (int32_t) p_var.m_a;
	const int32_t type = (int32_t) p_var.m_type;
	const int32_t extra = (int32_t) p_var.m_extra;
	p_stream->Write(flagAndPadding, sizeof(flagAndPadding));
	p_stream->Write(&legacyStringToken, sizeof(legacyStringToken));
	p_stream->Write(&a, sizeof(a));
	p_stream->Write(&type, sizeof(type));
	p_stream->Write(&extra, sizeof(extra));
}

inline static void ReadLogicVarRecord(STREAM* p_stream, LOGICVAR* p_var)
{
	uint8_t flagAndPadding[4] = {};
	uint32_t legacyStringToken = 0;
	int32_t a = 0;
	int32_t type = 0;
	int32_t extra = 0;
	p_stream->Read(flagAndPadding, sizeof(flagAndPadding));
	p_stream->Read(&legacyStringToken, sizeof(legacyStringToken));
	p_stream->Read(&a, sizeof(a));
	p_stream->Read(&type, sizeof(type));
	p_stream->Read(&extra, sizeof(extra));
	(void) legacyStringToken;
	p_var->m_flag = (char) flagAndPadding[0];
	p_var->m_a = (int) a;
	p_var->m_type = (int) type;
	p_var->m_extra = (int) extra;
}

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
		if (m_data) {
			delete[] m_data;
		}
		m_data = 0;
		m_n = 0;
	}

	// FUNCTION: ALIEN 0x412f20
	virtual void* ScalarDeletingDestructor(unsigned int p_flags)
	{
		NAMED_LIST_LOGICVAR_BASE* result = this;
		this->~NAMED_LIST_LOGICVAR_BASE();
		if (p_flags & 1) {
			operator delete(result);
		}
		return result;
	}

	void Write(STREAM* p_stream) const
	{
		if (p_stream) {
			p_stream->Write(&m_n, sizeof(m_n));
			for (int i = 0; i < m_n; ++i) {
				const STRING& name = m_data[i].m_name;
				p_stream->Write(name.m_str, strlen(name.m_str) + 1);
				WriteLogicVarRecord(p_stream, m_data[i].m_var);
			}
		}
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

	char* GetName(int p_index) { return m_data[p_index].m_name.m_str; }

	void Release()
	{
		NAMED_LIST_STRUCT_LOGICVAR* data = m_data;
		m_max = 0;
		m_n = 0;
		if (data) {
			delete[] data;
		}
		m_data = 0;
	}

	void Expand(int p_max);

	int m_n;                            // 0x04
	int m_max;                          // 0x08
	NAMED_LIST_STRUCT_LOGICVAR* m_data; // 0x0c
};

#endif
