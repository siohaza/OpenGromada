#ifndef NAMED_LIST_STRUCT_LOGICVAR_H
#define NAMED_LIST_STRUCT_LOGICVAR_H

#include "logic/logicvar.h"

class NAMED_LIST_STRUCT_LOGICVAR {
public:
	STRING m_name; // 0x00
	LOGICVAR m_var; // 0x04

#ifdef DECOMP_INLINE_NAMED_LIST_STRUCT_LOGICVAR_DTOR
	~NAMED_LIST_STRUCT_LOGICVAR()
	{
	}
#else
	~NAMED_LIST_STRUCT_LOGICVAR();
#endif
#ifdef DECOMP_INLINE_NAMED_LIST_STRUCT_LOGICVAR_CTOR
	NAMED_LIST_STRUCT_LOGICVAR()
	{
		m_var.m_flag = 0;
	}
#else
	NAMED_LIST_STRUCT_LOGICVAR();
#endif

#ifdef DECOMP_INLINE_NAMED_LIST_STRUCT_LOGICVAR_ASSIGN
	NAMED_LIST_STRUCT_LOGICVAR& operator=(const NAMED_LIST_STRUCT_LOGICVAR& p_other)
	{
		m_name = p_other.m_name;
		m_var.m_flag = p_other.m_var.m_flag;
		m_var.m_value = p_other.m_var.m_value;
		m_var.m_a = p_other.m_var.m_a;
		m_var.m_type = p_other.m_var.m_type;
		m_var.m_b = p_other.m_var.m_b;
		m_var.m_c = p_other.m_var.m_c;
		m_var.m_d = p_other.m_var.m_d;
		m_var.m_extra = p_other.m_var.m_extra;
		return *this;
	}
#else
	NAMED_LIST_STRUCT_LOGICVAR& operator=(const NAMED_LIST_STRUCT_LOGICVAR& p_other);
#endif
};

DECOMP_SIZE_ASSERT(NAMED_LIST_STRUCT_LOGICVAR, 0x18)

#endif
