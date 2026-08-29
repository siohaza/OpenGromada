#define DECOMP_INLINE_STRING_CHARP_NONNULL
#define DECOMP_INLINE_STRING_DTOR
#include "util/string.h"

#include <io.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

// GLOBAL: ALIEN 0x4905e8
char empty_str[4];

// GLOBAL: ALIEN 0x490608
char STRING::EMPTY[16];

// FUNCTION: ALIEN 0x401840
char* STRING::Copy(const char* p_src, unsigned int p_size)
{
	m_str = (char*) ::operator new((p_size & 0xfffffff0) + 16);
	memcpy(m_str, p_src, p_size);
	char* result = m_str;
	result[p_size] = 0;
	return result;
}

// FUNCTION: ALIEN 0x401880
STRING::STRING(const char* p_src)
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

// FUNCTION: ALIEN 0x4061b0
STRING::STRING(const char* p_a, const char* p_b)
{
	unsigned int lenA = strlen(p_a);
	unsigned int lenB = strlen(p_b);
	m_str = (char*) operator new(((lenA + lenB) & 0xfffffff0) + 16);
	memcpy(m_str, p_a, lenA);
	memcpy(m_str + lenA, p_b, lenB + 1);
}

// FUNCTION: ALIEN 0x406230
STRING& STRING::operator=(const STRING& p_other)
{
	if (this != &p_other) {
		char* str = m_str;
		unsigned int srcLen = strlen(p_other.m_str);
		if (str == EMPTY) {
			if (srcLen != 0) {
				m_str = (char*) operator new((srcLen & 0xfffffff0) + 16);
				strncpy(m_str, p_other.m_str, srcLen + 1);
			}
			return *this;
		}
		if (srcLen != 0) {
			if (((strlen(str) & 0xfffffff0) + 16) != ((srcLen & 0xfffffff0) + 16)) {
				operator delete(str);
				m_str = (char*) operator new((srcLen & 0xfffffff0) + 16);
			}
			strncpy(m_str, p_other.m_str, srcLen + 1);
			return *this;
		}
		operator delete(str);
		m_str = EMPTY;
	}
	return *this;
}

// FUNCTION: ALIEN 0x406300
STRING& STRING::operator=(const char* p_src)
{
	char* str = m_str;
	unsigned int srcLen = strlen(p_src);
	if (str == EMPTY) {
		if (srcLen != 0) {
			m_str = (char*) operator new((srcLen & 0xfffffff0) + 16);
			strncpy(m_str, p_src, srcLen + 1);
		}
		return *this;
	}
	if (srcLen != 0) {
		if (((strlen(str) & 0xfffffff0) + 16) != ((srcLen & 0xfffffff0) + 16)) {
			operator delete(str);
			m_str = (char*) operator new((srcLen & 0xfffffff0) + 16);
		}
		strncpy(m_str, p_src, srcLen + 1);
		return *this;
	}
	operator delete(str);
	m_str = EMPTY;
	return *this;
}

// FUNCTION: ALIEN 0x4063c0
STRING& STRING::operator+=(const STRING& p_other)
{
	if (*p_other.m_str) {
		char* str = m_str;
		unsigned int curLen = strlen(m_str);
		unsigned int srcLen = strlen(p_other.m_str);
		if (str == EMPTY || ((curLen + srcLen) & 0xfffffff0) + 16 != (curLen & 0xfffffff0) + 16) {
			char* buf = (char*) operator new(((curLen + srcLen) & 0xfffffff0) + 16);
			m_str = buf;
			strncpy(buf, str, curLen);
			if (str != EMPTY)
				operator delete(str);
		}
		strncpy(&m_str[curLen], p_other.m_str, srcLen + 1);
		return *this;
	}
	return *this;
}

// FUNCTION: ALIEN 0x406470
STRING& STRING::operator+=(const char* p_src)
{
	if (*p_src) {
		char* str = m_str;
		unsigned int curLen = strlen(m_str);
		unsigned int srcLen = strlen(p_src);
		if (str == EMPTY || ((curLen + srcLen) & 0xfffffff0) + 16 != (curLen & 0xfffffff0) + 16) {
			char* buf = (char*) operator new(((curLen + srcLen) & 0xfffffff0) + 16);
			m_str = buf;
			strncpy(buf, str, curLen);
			if (str != EMPTY)
				operator delete(str);
		}
		strncpy(&m_str[curLen], p_src, srcLen + 1);
		return *this;
	}
	return *this;
}

// FUNCTION: ALIEN 0x406510
STRING* STRING::Read_file(FILE* p_stream)
{
	int i;
	int c;
	char Source[256];
	i = 0;
	*this = empty_str;
	do {
		if (i == 255) {
			i = 0;
			Source[255] = 0;
			*this += Source;
		}
		c = fgetc(p_stream);
		if (c < 0)
			c = 0;
		else if (c == 0x0a)
			c = 0;
		else if (c == 0x0d)
			continue;
		Source[i++] = c;
	} while (c > 0);
	return &(*this += Source);
}

// FUNCTION: ALIEN 0x406590
STRING* STRING::Read_res(STREAM* p_stream)
{
	int i;
	int c;
	char Source[256];
	i = 0;
	c = 0;
	*this = empty_str;
	do {
		if (i == 255) {
			i = 0;
			Source[255] = 0;
			*this += Source;
		}
		if (p_stream->Read(&c, 1))
			c = 0;
		else if (c == 0x0a)
			c = 0;
		else if (c == 0x0d)
			continue;
		Source[i++] = c;
	} while (c > 0);
	return &(*this += Source);
}

// FUNCTION: ALIEN 0x406630
size_t STRING::LoadFile(const STRING& p_name)
{
	FILE* file;
	if (*p_name.m_str)
		file = fopen(p_name.m_str, "rb");
	else
		file = 0;
	FILE*& stream = file;
	if (!stream) {
		*this = empty_str;
		return 0;
	}
	size_t size = _filelength(file->_file);
	if (m_str != STRING::EMPTY)
		operator delete(m_str);
	char* buf = (char*) operator new((size & 0xfffffff0) + 16);
	m_str = buf;
	if (!buf) {
		m_str = STRING::EMPTY;
		fclose(file);
		return 0;
	}
	fread(buf, 1, size, file);
	m_str[size] = 0;
	fclose(file);
	return size;
}

// FUNCTION: ALIEN 0x4066e0
STRING Printf(const char* p_format, ...)
{
	char buffer[4096];
	buffer[0] = empty_str[0];
	memset(&buffer[1], 0, 0xfff);
	va_list args;
	va_start(args, p_format);
	vsprintf(buffer, p_format, args);
	if (buffer && buffer[0])
		return STRING(buffer, STRING::COPY);
	return STRING();
}

// FUNCTION: ALIEN 0x4067a0
STRING Time2Str()
{
	time_t t;
	char buffer[256];
	time(&t);
	tm* lt = localtime(&t);
	// STRING: ALIEN 0x481818
	strftime(buffer, 0x100, "%H:%M:%S", lt);
	if (buffer && buffer[0])
		return STRING(buffer, STRING::COPY);
	return STRING();
}

// FUNCTION: ALIEN 0x406850
STRING Date2Str()
{
	time_t t;
	char buffer[256];
	time(&t);
	tm* lt = localtime(&t);
	// STRING: ALIEN 0x481824
	strftime(buffer, 0x100, "%Y-%m-%d", lt);
	if (buffer && buffer[0])
		return STRING(buffer, STRING::COPY);
	return STRING();
}

// FUNCTION: ALIEN 0x406b80
char** FTempFile(char** p_out, const char* p_dir, const char* p_prefix)
{
	char buffer[4096];
	char* tmp = _tempnam(p_dir, p_prefix);
	strcpy(buffer, tmp);
	free(tmp);
	if (buffer && buffer[0]) {
		unsigned int len = strlen(buffer);
		char* buf = (char*) operator new((len & 0xfffffff0) + 16);
		*p_out = buf;
		memcpy(buf, buffer, len);
		(*p_out)[len] = 0;
		return p_out;
	}
	*p_out = STRING::EMPTY;
	return p_out;
}

// FUNCTION: ALIEN 0x406c50
char* STRING::RemoveEndChars(const char* p_chars)
{
	char* p = &m_str[strlen(m_str) - 1];
	char* result = 0;
	while (p >= m_str) {
		result = strchr(p_chars, *p);
		if (!result)
			break;
		*p = 0;
		result = m_str;
		--p;
	}
	return result;
}

// FUNCTION: ALIEN 0x406c90
int STRING::Replace(const char* p_find, const char* p_replace)
{
	int lenFind = strlen(p_find);
	int lenReplace = strlen(p_replace);
	char* found = strstr(m_str, p_find);
	if (!found)
		return 0;
	if (lenFind < lenReplace) {
		char* oldStr = m_str;
		int prefix = found - oldStr;
		m_str = (char*) operator new(strlen(oldStr) + (lenReplace - lenFind) + 1);
		strncpy(m_str, oldStr, prefix);
		strncpy(m_str + prefix, p_replace, lenReplace);
		strncpy(m_str + prefix + lenReplace, found + lenFind, strlen(found) - lenFind + 1);
		if (oldStr != EMPTY)
			operator delete(oldStr);
		return 1;
	}
	strncpy(found, p_replace, lenReplace);
	memmove(found + lenReplace, found + lenFind, strlen(found) - lenFind + 1);
	return 1;
}

// FUNCTION: ALIEN 0x406dd0
char** STRING::Before(char** p_out, const char* p_substr) const
{
	const char* found = strstr(m_str, p_substr);
	const char* s = m_str;
	unsigned int len;
	char* buf;
	if (found) {
		len = found - s;
		if (len == 0)
			goto empty;
		goto copy;
	} else if (*s) {
		len = strlen(s);
copy:
		buf = (char*) operator new((len & 0xfffffff0) + 16);
		*p_out = buf;
		memcpy(buf, s, len);
		(*p_out)[len] = 0;
		return p_out;
	}
empty:
	*p_out = EMPTY;
	return p_out;
}

static inline char** ConstructAfterResult(char** p_out, const char* p_source)
{
	if (p_source && *p_source) {
		unsigned int len = strlen(p_source);
		char* buf = (char*) operator new((len & 0xfffffff0) + 16);
		*p_out = buf;
		memcpy(buf, p_source, len);
		(*p_out)[len] = 0;
		return p_out;
	}
	*p_out = STRING::EMPTY;
	return p_out;
}

// FUNCTION: ALIEN 0x406e50
char** STRING::After(char** p_out, const char* p_substr) const
{
	char* found = strstr(m_str, p_substr);
	if (found)
		return ConstructAfterResult(p_out, found + strlen(p_substr));
	return ConstructAfterResult(p_out, empty_str);
}

// FUNCTION: ALIEN 0x406f20
char** STRING::BeforeLast(char** p_out, const char* p_substr) const
{
	char* found = strstr(m_str, p_substr);
	const char* source;
	unsigned int len;
	char* buf;
	if (found) {
		char* last;
		do {
			last = found;
			found = strstr(found + 1, p_substr);
		} while (found);
		if (last) {
			source = m_str;
			len = last - m_str;
			if (len) {
				buf = (char*) operator new((len & 0xfffffff0) + 16);
				*p_out = buf;
				memcpy(buf, source, len);
				(*p_out)[len] = 0;
				return p_out;
			}
			*p_out = STRING::EMPTY;
			return p_out;
		}
	}
	source = m_str;
	if (*m_str) {
		len = strlen(m_str);
		buf = (char*) operator new((len & 0xfffffff0) + 16);
		*p_out = buf;
		memcpy(buf, source, len);
		(*p_out)[len] = 0;
		return p_out;
	}
	*p_out = STRING::EMPTY;
	return p_out;
}

// FUNCTION: ALIEN 0x406fc0
char** STRING::AfterLast(char** p_out, const char* p_substr) const
{
	char* found = strstr(m_str, p_substr);
	if (found) {
		char* last;
		do {
			last = found;
			found = strstr(found + 1, p_substr);
		} while (found);
		if (last)
			return ConstructAfterResult(p_out, last + strlen(p_substr));
	}
	return ConstructAfterResult(p_out, empty_str);
}

static inline const char* AddSource(const STRING* p_string)
{
	return p_string->m_str;
}

// STUB: ALIEN 0x4070a0
char** STRING::Add(char** p_out, int p_add) const
{
	char* copy;
	int end = strlen(m_str) - 1;
	if (*m_str) {
		unsigned int copyLength = strlen(m_str);
		copy = (char*) operator new((copyLength & 0xfffffff0) + 16);
		memcpy(copy, m_str, copyLength);
		copy[copyLength] = 0;
	} else {
		copy = EMPTY;
	}
	if (!p_add) {
		if (*copy) {
			unsigned int copyLength = strlen(copy);
			*p_out = (char*) operator new((copyLength & 0xfffffff0) + 16);
			memcpy(*p_out, copy, copyLength);
			(*p_out)[copyLength] = 0;
		} else {
			*p_out = EMPTY;
		}
	} else {
		--end;
		while (end >= 0 && !isdigit(AddSource(this)[end]))
			--end;
		if (end < 0) {
			if (empty_str && *empty_str) {
				unsigned int copyLength = strlen(empty_str);
				*p_out = (char*) operator new((copyLength & 0xfffffff0) + 16);
				memcpy(*p_out, empty_str, copyLength);
				(*p_out)[copyLength] = 0;
			} else {
				*p_out = EMPTY;
			}
		} else {
			int begin = end - 1;
			while (begin >= 0 && isdigit(AddSource(this)[begin]))
				--begin;
			if (begin < 0 || AddSource(this)[begin] != '-')
				++begin;
			unsigned int digits = end - begin + 1;
			char buffer[128];
			// STRING: ALIEN 0x48185c
			sprintf(buffer, "%0*i", digits, p_add + atoi(&AddSource(this)[begin]));
			strncpy(copy + begin, buffer, digits);
			if (*copy) {
				unsigned int copyLength = strlen(copy);
				*p_out = (char*) operator new((copyLength & 0xfffffff0) + 16);
				memcpy(*p_out, copy, copyLength);
				(*p_out)[copyLength] = 0;
			} else {
				*p_out = EMPTY;
			}
		}
	}
	if (copy != EMPTY)
		operator delete(copy);
	return p_out;
}

// FUNCTION: ALIEN 0x4072e0
STRING STRING::ToBase64(int p_add) const
{
	char table[] =
		// STRING: ALIEN 0x481864
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	int written = 0;
	STRING result;
	unsigned int length = strlen(m_str);
	for (unsigned int pos = 0; pos < length; pos += 57) {
		unsigned int chunk = pos + 57 < length ? 57 : length - pos;
		char line[60];
		memcpy(line, m_str + pos, chunk);
		memset(line + chunk, 0, 57 - chunk);
		for (unsigned int j = 0; j < chunk; ++j)
			line[j] = (char) (line[j] + p_add);

		char out[80];
		int o = 0;
		for (unsigned int t = 0; t < chunk; t += 3) {
			unsigned int triple = (unsigned char) line[t];
			triple <<= 8;
			triple += (unsigned char) line[t + 1];
			triple <<= 8;
			triple += (unsigned char) line[t + 2];
			char* dst = out + o + 3;
			int count = 4;
			do {
				*dst = table[triple & 0x3f];
				triple >>= 6;
				dst--;
			} while (--count);
			o += 4;
		}
		written += o;
		out[o] = 0;
		result += out;
	}
	int pad = (3 - length % 3) % 3;
	for (; pad > 0; pad--)
		result.m_str[written - pad] = '=';
	return STRING(result.m_str, STRING::INLINE_CHARP_NONNULL);
}

// FUNCTION: ALIEN 0x407640
int STRING::ToUnicode(unsigned short* p_dest, int p_size)
{
	return MultiByteToWideChar(0, 0, m_str, -1, p_dest, p_size);
}

// FUNCTION: ALIEN 0x408430
STRING::STRING(const STRING& p_other)
{
	const char* src = p_other.m_str;
	if (*src) {
		unsigned int len = strlen(src);
		m_str = (char*) operator new((len & 0xfffffff0) + 16);
		memcpy(m_str, src, len);
		m_str[len] = 0;
	} else {
		m_str = EMPTY;
	}
}

// FUNCTION: ALIEN 0x4242e0
int STRING::Int() const
{
	if (m_str[1] != 'x')
		return atoi(m_str);

	int value;
	sscanf(m_str, "%i", &value);
	return value;
}

// FUNCTION: ALIEN 0x424310
STRING Int2Str(int p_value)
{
	char buffer[128];
	char* s = _itoa(p_value, buffer, 10);
	if (s && *s)
		return STRING(s, STRING::COPY);
	return STRING();
}

// FUNCTION: ALIEN 0x430f00
STRING::operator const char*() const
{
	return m_str;
}

// FUNCTION: ALIEN 0x439b70
unsigned int STRING::Length() const
{
	return strlen(m_str);
}

// FUNCTION: ALIEN 0x439b90
int STRING::Write(STREAM* p_stream) const
{
	return p_stream->Write(m_str, strlen(m_str) + 1);
}

// FUNCTION: ALIEN 0x439bc0
unsigned int STRING::Write_file(void* p_stream) const
{
	return fwrite(m_str, strlen(m_str) + 1, 1, (FILE*) p_stream);
}
