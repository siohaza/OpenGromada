#ifndef NAMED_LIST_LOGICVAR_H
#define NAMED_LIST_LOGICVAR_H

#include "util/decomp.h"
#include "util/named_list_logicvar_base.h"

class NAMED_LIST_STRUCT_LOGICVAR;
class LOGICVAR;
class STRING;

// VTABLE: ALIEN 0x47a330

class NAMED_LIST_LOGICVAR : public NAMED_LIST_LOGICVAR_BASE {
public:
	NAMED_LIST_LOGICVAR();

	int GetNo() const;
	void Insert(STRING p_name, LOGICVAR p_value);
	void Expand(int p_max);

	void Read(STREAM* p_stream)
	{
		if (p_stream) {
			p_stream->Read(&m_n, sizeof(m_n));
			Expand(m_n);
			int i = 0;
			if (m_n > 0) {
				do {
					m_data[i].m_name.Read_res(p_stream);
					p_stream->Read(&m_data[i].m_var.m_flag, 20);
					++i;
				} while (i < m_n);
			}
		}
	}
};

DECOMP_SIZE_ASSERT(NAMED_LIST_LOGICVAR, 0x10)

// SYNTHETIC: ALIEN 0x424c90
// NAMED_LIST_STRUCT_LOGICVAR::`vector deleting destructor'

// LIBRARY: ALIEN 0x401000
// `vector constructor iterator'

#endif
