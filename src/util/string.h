#ifndef STRING_H
#define STRING_H

#include "util/decomp.h"
#include "util/stream.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char empty_str[4];

class STRING {
public:
	char* m_str; // 0x00

	static char EMPTY[16];

	STRING();
	STRING(const char* p_src);
	STRING(const STRING& p_other);
	STRING(const char* p_a, const char* p_b);
	~STRING();
	STRING& operator=(const STRING& p_other);
	STRING& operator=(const char* p_src);
	STRING& operator+=(const STRING& p_other);
	STRING& operator+=(const char* p_src);
	STRING operator+(const STRING& p_other) const { return STRING(m_str, p_other.m_str); }
	STRING operator+(const char* p_src) const { return STRING(m_str, p_src); }
	char* Copy(const char* p_src, unsigned int p_size);
	char* RemoveEndChars(const char* p_chars);
	int Int() const;
	STRING ToBase64(int p_add) const;
	operator const char*() const;
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

inline STRING operator+(const char* p_a, const STRING& p_b)
{
	return STRING(p_a, p_b.m_str);
}

STRING Int2Str(int p_value);
STRING Printf(const char* p_format, ...) DECOMP_PRINTF(1, 2);
STRING FFileTime(const STRING& p_name);
char** FTempFile(char** p_out, const char* p_dir, const char* p_prefix);

#endif
