#include "util/string.h"

#include "platform/paths.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// GLOBAL: ALIEN 0x4905e8
char empty_str[4];

// GLOBAL: ALIEN 0x490608
char STRING::EMPTY[16];

namespace
{

size_t StringAllocationSize(size_t p_length)
{
	return (p_length & ~(size_t) 15) + 16;
}

char* AllocateString(size_t p_length)
{
	return (char*) ::operator new(StringAllocationSize(p_length));
}

char* DuplicateString(const char* p_src, size_t p_length)
{
	if (!p_src || !p_length) {
		return STRING::EMPTY;
	}
	char* result = AllocateString(p_length);
	memcpy(result, p_src, p_length);
	result[p_length] = 0;
	return result;
}

void ReleaseString(char* p_text)
{
	if (p_text != STRING::EMPTY) {
		::operator delete(p_text);
	}
}

} // namespace

STRING::STRING() : m_str(EMPTY)
{
}

STRING::STRING(const char* p_src) : m_str(DuplicateString(p_src, p_src ? strlen(p_src) : 0))
{
}

STRING::STRING(const STRING& p_other) : m_str(DuplicateString(p_other.m_str, strlen(p_other.m_str)))
{
}

STRING::~STRING()
{
	ReleaseString(m_str);
}

// FUNCTION: ALIEN 0x401840
char* STRING::Copy(const char* p_src, unsigned int p_size)
{
	char* replacement = DuplicateString(p_src, p_src ? p_size : 0);
	ReleaseString(m_str);
	m_str = replacement;
	return m_str;
}

// FUNCTION: ALIEN 0x4061b0
STRING::STRING(const char* p_a, const char* p_b) : m_str(EMPTY)
{
	const size_t lenA = p_a ? strlen(p_a) : 0;
	const size_t lenB = p_b ? strlen(p_b) : 0;
	if (!lenA && !lenB) {
		return;
	}
	m_str = AllocateString(lenA + lenB);
	if (lenA) {
		memcpy(m_str, p_a, lenA);
	}
	if (lenB) {
		memcpy(m_str + lenA, p_b, lenB);
	}
	m_str[lenA + lenB] = 0;
}

// FUNCTION: ALIEN 0x406230
STRING& STRING::operator=(const STRING& p_other)
{
	if (this != &p_other) {
		Copy(p_other.m_str, (unsigned int) strlen(p_other.m_str));
	}
	return *this;
}

// FUNCTION: ALIEN 0x406300
STRING& STRING::operator=(const char* p_src)
{
	if (p_src != m_str) {
		Copy(p_src, p_src ? (unsigned int) strlen(p_src) : 0);
	}
	return *this;
}

// FUNCTION: ALIEN 0x4063c0
STRING& STRING::operator+=(const STRING& p_other)
{
	return *this += p_other.m_str;
}

// FUNCTION: ALIEN 0x406470
STRING& STRING::operator+=(const char* p_src)
{
	if (!p_src || !*p_src) {
		return *this;
	}
	const size_t curLen = strlen(m_str);
	const size_t srcLen = strlen(p_src);
	char* joined = AllocateString(curLen + srcLen);
	if (curLen) {
		memcpy(joined, m_str, curLen);
	}
	memcpy(joined + curLen, p_src, srcLen);
	joined[curLen + srcLen] = 0;
	ReleaseString(m_str);
	m_str = joined;
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
		if (c < 0) {
			c = 0;
		}
		else if (c == 0x0a) {
			c = 0;
		}
		else if (c == 0x0d) {
			continue;
		}
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
		if (p_stream->Read(&c, 1)) {
			c = 0;
		}
		else if (c == 0x0a) {
			c = 0;
		}
		else if (c == 0x0d) {
			continue;
		}
		Source[i++] = c;
	} while (c > 0);
	return &(*this += Source);
}

// FUNCTION: ALIEN 0x406630
size_t STRING::LoadFile(const STRING& p_name)
{
	FILE* file;
	if (*p_name.m_str) {
		file = Platform_FOpen(p_name.m_str, "rb");
	}
	else {
		file = 0;
	}
	FILE*& stream = file;
	if (!stream) {
		*this = empty_str;
		return 0;
	}
	size_t size = (size_t) compat_filelength(file);
	char* buf = AllocateString(size);
	ReleaseString(m_str);
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
	vsnprintf(buffer, sizeof(buffer), p_format, args);
	va_end(args);
	buffer[sizeof(buffer) - 1] = 0;
	if (buffer[0]) {
		return STRING(buffer);
	}
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
	if (buffer[0]) {
		return STRING(buffer);
	}
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
	if (buffer[0]) {
		return STRING(buffer);
	}
	return STRING();
}

// FUNCTION: ALIEN 0x406b80
char** FTempFile(char** p_out, const char* p_dir, const char* p_prefix)
{
	// The recovered caller supplies a Windows-only directory. Keep converter
	// output in the portable runtime's writable preference directory instead.
	(void) p_dir;
	char* tmp = _tempnam(Platform_PrefPath(), p_prefix);
	if (tmp && *tmp) {
		*p_out = DuplicateString(tmp, strlen(tmp));
		free(tmp);
		return p_out;
	}
	free(tmp);
	*p_out = STRING::EMPTY;
	return p_out;
}

// FUNCTION: ALIEN 0x406c50
char* STRING::RemoveEndChars(const char* p_chars)
{
	if (!p_chars) {
		return 0;
	}

	size_t len = strlen(m_str);
	char* result = 0;
	while (len) {
		if (!strchr(p_chars, m_str[len - 1])) {
			break;
		}
		m_str[--len] = 0;
		result = m_str;
	}
	return result;
}

// FUNCTION: ALIEN 0x406c90
int STRING::Replace(const char* p_find, const char* p_replace)
{
	if (!p_find) {
		return 0;
	}
	if (!p_replace) {
		p_replace = EMPTY;
	}
	const size_t lenFind = strlen(p_find);
	const size_t lenReplace = strlen(p_replace);
	char* found = strstr(m_str, p_find);
	if (!found) {
		return 0;
	}
	char* oldStr = m_str;
	const size_t oldLength = strlen(oldStr);
	const size_t prefixLength = (size_t) (found - oldStr);
	const size_t suffixLength = oldLength - prefixLength - lenFind;
	const size_t newLength = prefixLength + lenReplace + suffixLength;
	char* replaced = newLength ? AllocateString(newLength) : EMPTY;
	if (prefixLength) {
		memcpy(replaced, oldStr, prefixLength);
	}
	if (lenReplace) {
		memcpy(replaced + prefixLength, p_replace, lenReplace);
	}
	if (suffixLength) {
		memcpy(replaced + prefixLength + lenReplace, found + lenFind, suffixLength);
	}
	if (newLength) {
		replaced[newLength] = 0;
	}
	m_str = replaced;
	ReleaseString(oldStr);
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
		if (len == 0) {
			goto empty;
		}
		goto copy;
	}
	else if (*s) {
		len = strlen(s);
	copy:
		buf = AllocateString(len);
		*p_out = buf;
		memcpy(buf, s, len);
		(*p_out)[len] = 0;
		return p_out;
	}
empty:
	*p_out = EMPTY;
	return p_out;
}

inline static char** ConstructAfterResult(char** p_out, const char* p_source)
{
	if (p_source && *p_source) {
		unsigned int len = strlen(p_source);
		char* buf = AllocateString(len);
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
	if (found) {
		return ConstructAfterResult(p_out, found + strlen(p_substr));
	}
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
				buf = AllocateString(len);
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
		buf = AllocateString(len);
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
		if (last) {
			return ConstructAfterResult(p_out, last + strlen(p_substr));
		}
	}
	return ConstructAfterResult(p_out, empty_str);
}

inline static const char* AddSource(const STRING* p_string)
{
	return p_string->m_str;
}

// STUB: ALIEN 0x4070a0
char** STRING::Add(char** p_out, int p_add) const
{
	char* copy;
	int end = (int) strlen(m_str) - 1;
	if (*m_str) {
		unsigned int copyLength = strlen(m_str);
		copy = AllocateString(copyLength);
		memcpy(copy, m_str, copyLength);
		copy[copyLength] = 0;
	}
	else {
		copy = EMPTY;
	}
	if (!p_add) {
		if (*copy) {
			unsigned int copyLength = strlen(copy);
			*p_out = AllocateString(copyLength);
			memcpy(*p_out, copy, copyLength);
			(*p_out)[copyLength] = 0;
		}
		else {
			*p_out = EMPTY;
		}
	}
	else {
		--end;
		while (end >= 0 && !isdigit((unsigned char) AddSource(this)[end])) {
			--end;
		}
		if (end < 0) {
			if (*empty_str) {
				unsigned int copyLength = strlen(empty_str);
				*p_out = AllocateString(copyLength);
				memcpy(*p_out, empty_str, copyLength);
				(*p_out)[copyLength] = 0;
			}
			else {
				*p_out = EMPTY;
			}
		}
		else {
			int begin = end - 1;
			while (begin >= 0 && isdigit((unsigned char) AddSource(this)[begin])) {
				--begin;
			}
			if (begin < 0 || AddSource(this)[begin] != '-') {
				++begin;
			}
			unsigned int digits = end - begin + 1;
			const int value = (int) ((uint32_t) p_add + (uint32_t) atoi(&AddSource(this)[begin]));
			const size_t bufferLength = (size_t) digits + 32;
			char* buffer = AllocateString(bufferLength);
			// STRING: ALIEN 0x48185c
			const int formatted = snprintf(buffer, bufferLength + 1, "%0*i", (int) digits, value);
			if (formatted >= (int) digits) {
				memcpy(copy + begin, buffer, digits);
			}
			ReleaseString(buffer);
			if (*copy) {
				unsigned int copyLength = strlen(copy);
				*p_out = AllocateString(copyLength);
				memcpy(*p_out, copy, copyLength);
				(*p_out)[copyLength] = 0;
			}
			else {
				*p_out = EMPTY;
			}
		}
	}
	ReleaseString(copy);
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
		for (unsigned int j = 0; j < chunk; ++j) {
			line[j] = (char) (line[j] + p_add);
		}

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
	for (; pad > 0; pad--) {
		result.m_str[written - pad] = '=';
	}
	return STRING(result.m_str);
}

// FUNCTION: ALIEN 0x4242e0
int STRING::Int() const
{
	if (m_str[1] != 'x') {
		return atoi(m_str);
	}

	int value;
	sscanf(m_str, "%i", &value);
	return value;
}

// FUNCTION: ALIEN 0x424310
STRING Int2Str(int p_value)
{
	char buffer[128];
	char* s = _itoa(p_value, buffer, 10);
	if (s && *s) {
		return STRING(s);
	}
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
