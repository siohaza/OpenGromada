#define DECOMP_INLINE_STRING_DTOR
#include "util/named_list_struct_logicvar.h"

// FUNCTION: ALIEN 0x424e00
NAMED_LIST_STRUCT_LOGICVAR::~NAMED_LIST_STRUCT_LOGICVAR()
{
}

// FUNCTION: ALIEN 0x4251e0
NAMED_LIST_STRUCT_LOGICVAR::NAMED_LIST_STRUCT_LOGICVAR()
{
	m_var.m_flag = 0;
}

// FUNCTION: ALIEN 0x425200
NAMED_LIST_STRUCT_LOGICVAR& NAMED_LIST_STRUCT_LOGICVAR::operator=(const NAMED_LIST_STRUCT_LOGICVAR& p_other)
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
