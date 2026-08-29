#ifndef LOGICSTACK_H
#define LOGICSTACK_H

#include <stdlib.h>

#include "util/decomp.h"
#include "util/string.h"

// script: A VM stack value tagged as an integer, string, or object.
class LOGICSTACK {
public:
	unsigned char m_type; // 0x00
	undefined m_unk0x01[3]; // 0x01
	int m_num; // 0x04
#ifdef DECOMP_LOGICSTACK_STRING_MEMBER

	STRING m_str; // 0x08
#else
	char* m_str; // 0x08
#endif

#if defined(DECOMP_INLINE_LOGICSTACK_DEFAULT_CTOR) \
	&& defined(DECOMP_LOGICSTACK_STRING_MEMBER)

	LOGICSTACK()
		: m_type(0)
		, m_num(0)
	{
	}
#elif defined(DECOMP_INLINE_LOGICSTACK_DEFAULT_CTOR)
	LOGICSTACK()
	{
		m_type = 0;
		m_num = 0;
		m_str = STRING::EMPTY;
	}
#elif defined(DECOMP_INLINE_LOGICSTACK_CTORS)

	LOGICSTACK()
	{
	}
#else
	LOGICSTACK();
#endif
#ifndef DECOMP_LOGICSTACK_STRING_MEMBER
	~LOGICSTACK()
	{
		if (m_str != STRING::EMPTY)
			operator delete(m_str);
	}
#endif
#if defined(DECOMP_INLINE_LOGICSTACK_CTORS) || defined(DECOMP_INLINE_LOGICSTACK_COPY_CTOR)
	LOGICSTACK(const LOGICSTACK& p_other)
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
#else
	LOGICSTACK(const LOGICSTACK& p_other);
#endif
#ifdef DECOMP_LOGICSTACK_STRING_MEMBER

	LOGICSTACK(int p_value)
		: m_type(2)
		, m_num(p_value)
	{
	}
	LOGICSTACK(const STRING& p_value)
		: m_type(1)
		, m_str(p_value)
	{
	}
	#ifdef DECOMP_INLINE_LOGICSTACK_OBJECT_CTOR

	LOGICSTACK(const void* p_object)
		: m_type(18)
		, m_num((int) p_object)
	{
		if (!p_object)
			m_type = 2;
	}
	#else
	LOGICSTACK(const void* p_object);
	#endif
#elif defined(DECOMP_INLINE_LOGICSTACK_CTORS)
	LOGICSTACK(int p_value)
	{
		m_type = 2;
		m_num = p_value;
	#ifdef DECOMP_INLINE_LOGICSTACK_VALUE_LIFETIME

		char* s = STRING::EMPTY;
		if (*s) {
			unsigned int len = strlen(STRING::EMPTY);
			m_str = (char*) operator new((len & 0xfffffff0) + 16);
			memcpy(m_str, STRING::EMPTY, len);
			m_str[len] = 0;
		}
		else
			m_str = STRING::EMPTY;
	#else
		m_str = STRING::EMPTY;
	#endif
	}
	LOGICSTACK(const void* p_object)
	{
		m_type = 18;
		m_num = (int) p_object;
		m_str = STRING::EMPTY;
		if (!p_object)
			m_type = 2;
	}
#elif defined(DECOMP_INLINE_LOGICSTACK_INT_CTOR)

	LOGICSTACK(int p_value)
	{
		m_type = 2;
		m_num = p_value;
		m_str = STRING::EMPTY;
	}
	LOGICSTACK(const STRING& p_value)
	{
		m_type = 1;
		m_str = STRING::EMPTY;
		*(STRING*) &m_str = p_value;
	}
	LOGICSTACK(const void* p_object);
#else
	LOGICSTACK(int p_value);
	LOGICSTACK(const void* p_object);
#endif
#ifdef DECOMP_INLINE_LOGICSTACK_ASSIGN
	LOGICSTACK& operator=(const LOGICSTACK& p_other)
	{
		m_type = p_other.m_type;
		m_num = p_other.m_num;
		*(STRING*) &m_str = *(const STRING*) &p_other.m_str;
		return *this;
	}
#else
	LOGICSTACK& operator=(const LOGICSTACK& p_other);
#endif
	void BinarOperator(int p_operation, const LOGICSTACK& p_other);
	#ifdef DECOMP_INLINE_LOGICSTACK_INCDEC
	void Inc()
	{
		m_type &= ~0x10;
		if (m_type & 0x22)
			++m_num;
	}
	void Dec()
	{
		m_type &= ~0x10;
		if (m_type & 0x22)
			--m_num;
	}
	#endif
	#ifdef DECOMP_INLINE_LOGICSTACK_INT

	int Int() { return (m_type & 1) ? ((STRING*) &m_str)->Int() : m_num; }
	#else
	int Int();
	#endif
	#ifdef DECOMP_INLINE_LOGICSTACK_STRING
	STRING* String()
	{
		return (m_type & 2)
			? &(*(STRING*) &m_str = Int2Str(m_num))
			: (STRING*) &m_str;
	}
	#else
	STRING* String();
	#endif
	char Read(STREAM* p_stream);
	char Write(STREAM* p_stream) const;
};

// SYNTHETIC: ALIEN 0x4041e0
// LOGICSTACK::`vector deleting destructor'

#endif
