#define DECOMP_INLINE_INT2STR
#define DECOMP_INLINE_STRING_DTOR
#include "logic/logicstack.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util/myerror.h"

// FUNCTION: ALIEN 0x41f7b0
LOGICSTACK::LOGICSTACK()
{
	m_type = 0;
	m_num = 0;
	m_str = STRING::EMPTY;
}

// FUNCTION: ALIEN 0x4243a0
LOGICSTACK::LOGICSTACK(int p_value)
{
	m_type = 2;
	m_num = p_value;
	m_str = STRING::EMPTY;
}

// FUNCTION: ALIEN 0x4243c0
LOGICSTACK& LOGICSTACK::operator=(const LOGICSTACK& p_other)
{
	m_type = p_other.m_type;
	m_num = p_other.m_num;
	*(STRING*) &m_str = *(const STRING*) &p_other.m_str;
	return *this;
}

// FUNCTION: ALIEN 0x4243f0
LOGICSTACK::LOGICSTACK(const LOGICSTACK& p_other)
{
	m_type = p_other.m_type;
	m_num = p_other.m_num;
	char* s = p_other.m_str;
	if (*s) {
		unsigned int len = strlen(s);
		m_str = (char*) operator new((len & 0xfffffff0) + 16);
		memcpy(m_str, s, len);
		m_str[len] = 0;
	}
	else
		m_str = STRING::EMPTY;
}

// FUNCTION: ALIEN 0x4244b0
char LOGICSTACK::Read(STREAM* p_stream)
{
	p_stream->Read(this, 1);
	if ((m_type & 8) != 0) {
		if ((m_type & 1) != 0) {
			STRING* v4 = (STRING*) &m_str;
			v4->Read_res(p_stream);
			for (int i = 0; i < (int) strlen(v4->m_str); ++i)
				v4->m_str[i] ^= 0x17;
		}
		else
			return p_stream->Read(&m_num, 4);
	}
}

// FUNCTION: ALIEN 0x424520
char LOGICSTACK::Write(STREAM* p_stream) const
{
	p_stream->Write(this, 1);
	if ((m_type & 8) != 0) {
		if ((m_type & 1) != 0) {
			char* str = m_str;
			for (int i = 0; i < (int) strlen(m_str); ++i)
				m_str[i] ^= 0x17;
			p_stream->Write(m_str, strlen(m_str) + 1);
			for (int j = 0; j < (int) strlen(m_str); ++j)
				m_str[j] ^= 0x17;
		}
		else
			return p_stream->Write(&m_num, 4);
	}
}

// script: Applies binary operator opcodes 6 through 23 to the current stack value.
// FUNCTION: ALIEN 0x4245e0
void LOGICSTACK::BinarOperator(int p_operation, const LOGICSTACK& p_other)
{
	int value;
	int other;
	if (p_other.m_type & 1) {
		if (p_other.m_str[1] != 'x')
			other = atoi(p_other.m_str);
		else {
			sscanf(p_other.m_str, "%i", &value);
			other = value;
		}
	}
	else
		other = p_other.m_num;

	if (m_type & 1) {
		if (m_str[1] != 'x')
			m_num = atoi(m_str);
		else {
			sscanf(m_str, "%i", &value);
			m_num = value;
		}
	}

	switch (p_operation) {
	case 8:
		if ((p_other.m_type & 1) && (m_type & 1)) {
			*(STRING*) &m_str += *(const STRING*) &p_other.m_str;
			m_type = 1;
		}
		else {
			m_num += other;
			m_type = 2;
		}
		break;
	case 9:
		m_num -= other;
		m_type = 2;
		break;
	case 19:
		value = m_num * other;
		m_num = value;
		m_type = 2;
		break;
	case 6:
		if (other)
			value = m_num / other;
		else
			value = 0xfffffff;
		m_num = value;
		m_type = 2;
		break;
	case 7:
		m_num %= other;
		m_type = 2;
		break;
	case 11:
		m_num |= other;
		m_type = 2;
		break;
	case 10:
		m_num ^= other;
		m_type = 2;
		break;
	case 12:
		m_num &= other;
		m_type = 2;
		break;
	case 23:
		m_num <<= other;
		m_type = 2;
		break;
	case 22:
		m_num >>= other;
		m_type = 2;
		break;
	case 21:
		if (m_num && other)
			value = 1;
		else
			value = 0;
		m_num = value;
		m_type = 2;
		break;
	case 14:
		if (m_num || other)
			value = 1;
		else
			value = 0;
		m_num = value;
		m_type = 2;
		break;
	case 16:
		m_num = m_num < other;
		m_type = 2;
		break;
	case 18:
		m_num = m_num <= other;
		m_type = 2;
		break;
	case 15:
		m_num = m_num > other;
		m_type = 2;
		break;
	case 17:
		m_num = m_num >= other;
		m_type = 2;
		break;
	case 13:
		if ((p_other.m_type & 1) && (m_type & 1))
			value = !strcmp(m_str, p_other.m_str);
		else
			value = m_num == other;
		m_num = value;
		m_type = 2;
		break;
	case 20:
		if ((p_other.m_type & 1) && (m_type & 1))
			value = strcmp(m_str, p_other.m_str) != 0;
		else
			value = m_num != other;
		m_num = value;
		m_type = 2;
		break;
	default:
		MYERROR::Log(::Error,
			// STRING: ALIEN 0x483640
			"!!!ERROE!!!LOGIC::Unknown Binary command %i", p_operation);
		m_type = 2;
		break;
	}
}

// FUNCTION: ALIEN 0x439dd0
LOGICSTACK::LOGICSTACK(const void* p_object)
{
	m_type = 18;
	m_num = (int) p_object;
	m_str = STRING::EMPTY;
	if (!p_object)
		m_type = 2;
}

// FUNCTION: ALIEN 0x439df0
int LOGICSTACK::Int()
{
	if (m_type & 1) {
		if (m_str[1] != 'x')
			return atoi(m_str);

		int value;
		sscanf(m_str, "%i", &value);
		return value;
	}
	return m_num;
}

// FUNCTION: ALIEN 0x439e30
STRING* LOGICSTACK::String()
{
	return (m_type & 2)
		? &(*(STRING*) &m_str = Int2Str(m_num))
		: (STRING*) &m_str;
}
