#ifndef LOGICVAR_H
#define LOGICVAR_H

#include "util/decomp.h"
#include "util/string.h"

// script: Describes a source variable and its parsed type/value metadata.
class LOGICVAR {
public:
	char m_flag; // 0x00

	STRING m_value; // 0x04
	union {         // 0x08
		int m_a;
		int m_b;
		int m_c;
		int m_d;
	};
	int m_type;  // 0x0c
	int m_extra; // 0x10

	LOGICVAR() {}

	LOGICVAR(char p_flag, int p_number)
	{
		m_flag = p_flag;
		m_a = p_number;
	}

	LOGICVAR(const LOGICVAR& p_other) : m_flag(p_other.m_flag), m_value(p_other.m_value)
	{
		m_a = p_other.m_a;
		m_type = p_other.m_type;
		m_extra = p_other.m_extra;
	}
};

#endif
