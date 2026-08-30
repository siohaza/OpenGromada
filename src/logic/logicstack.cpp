#include "logic/logicstack.h"

#include "util/myerror.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// FUNCTION: ALIEN 0x41f7b0
LOGICSTACK::LOGICSTACK() : m_type(0), m_unk0x01{}, m_num(0), m_str()
{
}

LOGICSTACK::~LOGICSTACK() = default;

// FUNCTION: ALIEN 0x4243a0
LOGICSTACK::LOGICSTACK(int p_value) : m_type(2), m_unk0x01{}, m_num(p_value), m_str()
{
}

LOGICSTACK::LOGICSTACK(const STRING& p_value) : m_type(1), m_unk0x01{}, m_num(0), m_str(p_value)
{
}

// FUNCTION: ALIEN 0x4243c0
LOGICSTACK& LOGICSTACK::operator=(const LOGICSTACK& p_other)
{
	if (this != &p_other) {
		m_type = p_other.m_type;
		m_num = p_other.m_num;
		m_str = p_other.m_str;
	}
	return *this;
}

// FUNCTION: ALIEN 0x4243f0
LOGICSTACK::LOGICSTACK(const LOGICSTACK& p_other)
	: m_type(p_other.m_type), m_unk0x01{}, m_num(p_other.m_num), m_str(p_other.m_str)
{
}

// FUNCTION: ALIEN 0x4244b0
char LOGICSTACK::Read(STREAM* p_stream)
{
	p_stream->Read(this, 1);
	if ((m_type & 8) != 0) {
		if ((m_type & 1) != 0) {
			m_str.Read_res(p_stream);
			for (int i = 0; i < (int) strlen(m_str.m_str); ++i) {
				m_str.m_str[i] ^= 0x17;
			}
			return 1;
		}
		else {
			// The saved format is 32-bit; m_num is wider in memory.
			int stored = 0;
			char r = p_stream->Read(&stored, 4);
			m_num = stored;
			return r;
		}
	}
	return 1;
}

// FUNCTION: ALIEN 0x424520
char LOGICSTACK::Write(STREAM* p_stream) const
{
	p_stream->Write(this, 1);
	if ((m_type & 8) != 0) {
		if ((m_type & 1) != 0) {
			for (int i = 0; i < (int) strlen(m_str.m_str); ++i) {
				m_str.m_str[i] ^= 0x17;
			}
			p_stream->Write(m_str.m_str, strlen(m_str.m_str) + 1);
			for (int j = 0; j < (int) strlen(m_str.m_str); ++j) {
				m_str.m_str[j] ^= 0x17;
			}
			return 1;
		}
		else {
			int stored = (int) m_num;
			return p_stream->Write(&stored, 4);
		}
	}
	return 1;
}

// script: Applies binary operator opcodes 6 through 23 to the current stack value.
// FUNCTION: ALIEN 0x4245e0
void LOGICSTACK::BinarOperator(int p_operation, const LOGICSTACK& p_other)
{
	int value;
	int other;
	decomp_intptr otherValue;
	if (p_other.m_type & 1) {
		if (p_other.m_str.m_str[1] != 'x') {
			other = atoi(p_other.m_str.m_str);
		}
		else {
			sscanf(p_other.m_str.m_str, "%i", &value);
			other = value;
		}
		otherValue = other;
	}
	else {
		otherValue = p_other.m_num;
		other = (int) otherValue;
	}

	if (m_type & 1) {
		if (m_str.m_str[1] != 'x') {
			m_num = atoi(m_str.m_str);
		}
		else {
			sscanf(m_str.m_str, "%i", &value);
			m_num = value;
		}
	}

	switch (p_operation) {
	case 8:
		if ((p_other.m_type & 1) && (m_type & 1)) {
			m_str += p_other.m_str;
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
		if (other) {
			value = m_num / other;
		}
		else {
			value = 0xfffffff;
		}
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
		if (m_num && other) {
			value = 1;
		}
		else {
			value = 0;
		}
		m_num = value;
		m_type = 2;
		break;
	case 14:
		if (m_num || other) {
			value = 1;
		}
		else {
			value = 0;
		}
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
		if ((p_other.m_type & 1) && (m_type & 1)) {
			value = !strcmp(m_str.m_str, p_other.m_str.m_str);
		}
		else {
			value = m_num == otherValue;
		}
		m_num = value;
		m_type = 2;
		break;
	case 20:
		if ((p_other.m_type & 1) && (m_type & 1)) {
			value = strcmp(m_str.m_str, p_other.m_str.m_str) != 0;
		}
		else {
			value = m_num != otherValue;
		}
		m_num = value;
		m_type = 2;
		break;
	default:
		MYERROR::Log(
			::Error,
			// STRING: ALIEN 0x483640
			"!!!ERROE!!!LOGIC::Unknown Binary command %i",
			p_operation
		);
		m_type = 2;
		break;
	}
}

// FUNCTION: ALIEN 0x439dd0
LOGICSTACK::LOGICSTACK(const void* p_object) : m_type(18), m_unk0x01{}, m_num((intptr_t) p_object), m_str()
{
	if (!p_object) {
		m_type = 2;
	}
}

// FUNCTION: ALIEN 0x439df0
int LOGICSTACK::Int()
{
	if (m_type & 1) {
		if (m_str.m_str[1] != 'x') {
			return atoi(m_str.m_str);
		}

		int value;
		sscanf(m_str.m_str, "%i", &value);
		return value;
	}
	return m_num;
}

decomp_intptr LOGICSTACK::Value() const
{
	return (m_type & 1) ? m_str.Int() : m_num;
}

void LOGICSTACK::AssignValue(const LOGICSTACK& p_source)
{
	m_type &= 0xaf;
	m_num = p_source.Value();
	if (p_source.m_type & 0x10) {
		m_type |= 0x10;
	}
}

// FUNCTION: ALIEN 0x439e30
STRING* LOGICSTACK::String()
{
	return (m_type & 2) ? &(m_str = Int2Str((int) m_num)) : &m_str;
}

void LOGICSTACK::Inc()
{
	m_type &= ~0x10;
	if (m_type & 0x22) {
		++m_num;
	}
}

void LOGICSTACK::Dec()
{
	m_type &= ~0x10;
	if (m_type & 0x22) {
		--m_num;
	}
}
