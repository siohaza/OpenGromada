#ifndef STRING_H
#define STRING_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util/decomp.h"
#include "util/stream.h"

#ifdef DECOMP_INLINE_STRING_COPY_LIFETIME
#define DECOMP_INLINE_STRING_COPY_CTOR
#define DECOMP_INLINE_STRING_DTOR
#endif

extern char empty_str[4];

class STRING {
public:
	char* m_str; // 0x00

	static char EMPTY[16];

#ifdef DECOMP_UNINITIALIZED_STRING_DEFAULT_CTOR

	STRING() {}
#else
	STRING() { m_str = EMPTY; }
#endif
#ifdef DECOMP_INLINE_STRING_CHARP_CTOR
	STRING(const char* p_src)
	{
		if (p_src && *p_src) {
			unsigned int len = strlen(p_src);
#ifdef DECOMP_STRING_NEW_EXPR
			m_str = new char[(len & 0xfffffff0) + 16];
#else
			m_str = (char*) operator new((len & 0xfffffff0) + 16);
#endif
			memcpy(m_str, p_src, len);
			m_str[len] = 0;
		} else {
			m_str = EMPTY;
		}
	}
#elif defined(DECOMP_INLINE_STRING_CHARP_CTOR_CALLS_COPY)
	STRING(const char* p_src)
	{
		if (p_src && *p_src)
			Copy(p_src, strlen(p_src));
		else
			m_str = EMPTY;
	}
#else
	STRING(const char* p_src);
#endif

	enum COPY_TAG { COPY };
	STRING(const char* p_src, COPY_TAG)
	{
		unsigned int len = strlen(p_src);
		m_str = (char*) operator new((len & 0xfffffff0) + 16);
		memcpy(m_str, p_src, len);
		m_str[len] = 0;
	}

	enum INLINE_CHARP_TAG { INLINE_CHARP };
	STRING(const char* p_src, INLINE_CHARP_TAG)
	{
		if (p_src && *p_src) {
			unsigned int len = strlen(p_src);
			m_str = (char*) operator new((len & 0xfffffff0) + 16);
			memcpy(m_str, p_src, len);
			m_str[len] = 0;
		} else {
			m_str = EMPTY;
		}
	}
#ifdef DECOMP_INLINE_STRING_CHARP_NONNULL

	enum INLINE_CHARP_NONNULL_TAG { INLINE_CHARP_NONNULL };
	STRING(const char* p_src, INLINE_CHARP_NONNULL_TAG)
	{
		if (*p_src) {
			unsigned int len = strlen(p_src);
			m_str = (char*) operator new((len & 0xfffffff0) + 16);
			memcpy(m_str, p_src, len);
			m_str[len] = 0;
		} else {
			m_str = EMPTY;
		}
	}
#endif

	enum CALL_COPY_TAG { CALL_COPY };
	STRING(const char* p_src, CALL_COPY_TAG)
	{
		if (p_src && *p_src)
			Copy(p_src, strlen(p_src));
		else
			m_str = EMPTY;
	}

	enum CALL_COPY_NONNULL_TAG { CALL_COPY_NONNULL };
	STRING(const char* p_src, CALL_COPY_NONNULL_TAG)
	{
		if (*p_src)
			Copy(p_src, strlen(p_src));
		else
			m_str = EMPTY;
	}
#ifdef DECOMP_STRING_COPY_CTOR_CALL_COPY_NONNULL

	STRING(const STRING& p_other, CALL_COPY_NONNULL_TAG)
	{
		if (*p_other.m_str)
			Copy(p_other.m_str, strlen(p_other.m_str));
		else
			m_str = EMPTY;
	}
#endif
	#ifdef DECOMP_INLINE_STRING_COPY_CTOR
	STRING(const STRING& p_other)
	{
		if (*p_other.m_str) {
			unsigned int len = strlen(p_other.m_str);
			const char* src = p_other.m_str;
			m_str = (char*) operator new((len & 0xfffffff0) + 16);
			memcpy(m_str, src, len);
			m_str[len] = 0;
		} else {
			m_str = EMPTY;
		}
	}
	#else
	STRING(const STRING& p_other);
	#endif
	STRING(const char* p_a, const char* p_b);
	#ifdef DECOMP_INLINE_STRING_DTOR
	// FUNCTION: ALIEN 0x43ab50
	~STRING()
	{
		if (m_str != EMPTY)
#ifdef DECOMP_STRING_NEW_EXPR
			delete[] m_str;
#else
			::operator delete(m_str);
#endif
	}
	#else
	~STRING();
	#endif
	STRING& operator=(const STRING& p_other);
	STRING& operator=(const char* p_src);
	STRING& operator+=(const STRING& p_other);
	STRING& operator+=(const char* p_src);
	STRING operator+(const STRING& p_other) const { return STRING(m_str, p_other.m_str); }
	STRING operator+(const char* p_src) const { return STRING(m_str, p_src); }
	char* Copy(const char* p_src, unsigned int p_size);
	char* RemoveEndChars(const char* p_chars);
	int Int() const;
	int ToUnicode(unsigned short* p_dest, int p_size);
	STRING ToBase64(int p_add) const;
#ifdef DECOMP_INLINE_STRING_CHARP_CONVERSION

	operator const char*() const { return m_str; }
#else
	operator const char*() const;
#endif
	unsigned int Length() const;
	size_t LoadFile(const STRING& p_name);
	STRING* Read_file(FILE* p_stream);
	STRING* Read_res(STREAM* p_stream);
	int Write(STREAM* p_stream) const;
	unsigned int Write_file(void* p_stream) const;
	char** Before(char** p_out, const char* p_substr) const;
	char** After(char** p_out, const char* p_substr) const;
	char** BeforeLast(char** p_out, const char* p_substr) const;
	char** AfterLast(char** p_out, const char* p_substr) const;
	STRING ToLower() const;
	STRING ToUpper() const;
	char** Add(char** p_out, int p_add) const;
	int Replace(const char* p_find, const char* p_replace);
};

DECOMP_SIZE_ASSERT(STRING, 0x4)

inline STRING operator+(const char* p_a, const STRING& p_b)
{
	return STRING(p_a, p_b.m_str);
}

#ifndef DECOMP_STRING_TOUPPER_OUT_OF_LINE
inline STRING STRING::ToUpper() const
{
	STRING result(m_str);
	_strupr(result.m_str);
	return result;
}
#endif

#ifdef DECOMP_INLINE_STRING_INT
inline int STRING::Int() const
{
	if (m_str[1] != 'x')
		return atoi(m_str);

	int value;
	sscanf(m_str, "%i", &value);
	return value;
}
#endif

#ifdef DECOMP_INLINE_INT2STR

inline STRING Int2Str(int p_value)
{
	char buffer[128];
	char* s = _itoa(p_value, buffer, 10);
	if (s && *s)
		return STRING(s, STRING::COPY);
	return STRING();
}
#elif defined(DECOMP_INLINE_INT2STR_CALL_CTOR)

inline STRING Int2Str(int p_value)
{
	char buffer[128];
	return STRING(_itoa(p_value, buffer, 10));
}
#elif defined(DECOMP_INLINE_INT2STR_CALL_COPY)

inline STRING Int2Str(int p_value)
{
	char buffer[128];
	char* s = _itoa(p_value, buffer, 10);
	if (s && *s)
		return STRING(s, STRING::CALL_COPY);
	return STRING();
}
#else
STRING Int2Str(int p_value);
#endif
STRING Printf(const char* p_format, ...);
STRING FFileTime(const STRING& p_name);
char** FTempFile(char** p_out, const char* p_dir, const char* p_prefix);

#endif
