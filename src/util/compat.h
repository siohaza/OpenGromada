#ifndef COMPAT_H
#define COMPAT_H

// Compatibility shims for the original MSVC CRT and calling conventions

#include <stdio.h>

#if defined(__MINGW32__) || (defined(_MSC_VER) && _MSC_VER >= 1100)
#define COMPAT_MODE
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#define MSVC420_VERSION 1020

#if !defined(_MSC_VER)
#ifndef __forceinline
#define __forceinline inline
#endif
// Calling conventions do not affect the portable internal ABI
#ifndef __cdecl
#define __cdecl
#endif
#ifndef __stdcall
#define __stdcall
#endif
#ifndef __fastcall
#define __fastcall
#endif
#endif

// MSVC CRT extensions

#if defined(_MSC_VER)

#include <io.h>
#include <stdlib.h>
#include <string.h>

#else

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

inline char* _strlwr(char* p_str)
{
	for (char* p = p_str; *p; ++p) {
		*p = (char) tolower((unsigned char) *p);
	}
	return p_str;
}

inline char* _strupr(char* p_str)
{
	for (char* p = p_str; *p; ++p) {
		*p = (char) toupper((unsigned char) *p);
	}
	return p_str;
}

// Current radix-10 callers provide at least 128 bytes.
inline char* _itoa(int p_value, char* p_buffer, int p_radix)
{
	if (p_radix == 10) {
		sprintf(p_buffer, "%d", p_value);
		return p_buffer;
	}

	unsigned int value = (unsigned int) p_value;
	char tmp[36];
	int n = 0;

	if (value == 0) {
		tmp[n++] = '0';
	}
	while (value) {
		unsigned int digit = value % (unsigned int) p_radix;
		tmp[n++] = (char) (digit < 10 ? '0' + digit : 'a' + digit - 10);
		value /= (unsigned int) p_radix;
	}
	for (int i = 0; i < n; ++i) {
		p_buffer[i] = tmp[n - 1 - i];
	}
	p_buffer[n] = '\0';
	return p_buffer;
}

#define _strdup strdup
#define _stricmp strcasecmp
#define _strnicmp strncasecmp

inline char* _tempnam(const char* p_dir, const char* p_prefix)
{
	const char* dir = (p_dir && *p_dir) ? p_dir : getenv("TMPDIR");
	if (!dir || !*dir) {
		dir = "/tmp";
	}
	const char* prefix = p_prefix ? p_prefix : "tmp";

	size_t size = strlen(dir) + strlen(prefix) + 40;
	for (int attempt = 0; attempt < 4096; ++attempt) {
		char* out = (char*) malloc(size);
		if (!out) {
			return 0;
		}
		// Avoid a duplicate trailing separator.
		const size_t dirLen = dir ? strlen(dir) : 0;
		const char* sep = (dirLen && (dir[dirLen - 1] == '/' || dir[dirLen - 1] == '\\')) ? "" : "/";
		snprintf(out, size, "%s%s%s%d_%d", dir, sep, prefix, (int) getpid(), attempt);

		struct stat st;
		if (stat(out, &st) != 0) {
			return out;
		}
		free(out);
	}
	return 0;
}

#endif

inline long compat_filelength(FILE* p_file)
{
	if (!p_file) {
		return -1;
	}

	long cur = ftell(p_file);
	if (cur < 0) {
		return -1;
	}
	if (fseek(p_file, 0, SEEK_END) != 0) {
		return -1;
	}

	long size = ftell(p_file);
	fseek(p_file, cur, SEEK_SET);
	return size;
}

#endif
