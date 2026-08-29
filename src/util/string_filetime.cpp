#define DECOMP_INLINE_STRING_COPY_LIFETIME
#include "util/string.h"

#include <windows.h>

// FUNCTION: ALIEN 0x406900
STRING FFileTime(const STRING& p_name)
{
	FILETIME creation;
	FILETIME access;
	FILETIME write;
	STRING result;
	const char* name = p_name.m_str;
	HANDLE file = CreateFileA(name, 0x80000000, 0, 0, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, 0);
	GetFileTime(file, &creation, &access, &write);
	CloseHandle(file);

	FILETIME local;
	SYSTEMTIME system;
	char text[80];

	// STRING: ALIEN 0x481858
	result += "Cr-";
	FileTimeToLocalFileTime(&write, &local);
	FileTimeToSystemTime(&local, &system);
	GetDateFormatA(LOCALE_USER_DEFAULT, 0, &system,
		// STRING: ALIEN 0x48184c
		"yyyy-MM-dd", text, 80);
	result += text;
	result += " ";
	GetTimeFormatA(LOCALE_USER_DEFAULT, 0, &system,
		// STRING: ALIEN 0x481840
		"hh:mm:ss", text, 80);
	result += text;

	// STRING: ALIEN 0x481838
	result += " La-";
	FileTimeToLocalFileTime(&write, &local);
	FileTimeToSystemTime(&local, &system);
	GetDateFormatA(LOCALE_USER_DEFAULT, 0, &system, "yyyy-MM-dd", text, 80);
	result += text;
	result += " ";
	GetTimeFormatA(LOCALE_USER_DEFAULT, 0, &system, "hh:mm:ss", text, 80);
	result += text;

	// STRING: ALIEN 0x481830
	result += " Lw-";
	FileTimeToLocalFileTime(&write, &local);
	FileTimeToSystemTime(&local, &system);
	GetDateFormatA(LOCALE_USER_DEFAULT, 0, &system, "yyyy-MM-dd", text, 80);
	result += text;
	result += " ";
	GetTimeFormatA(LOCALE_USER_DEFAULT, 0, &system, "hh:mm:ss", text, 80);
	result += text;
	return result;
}
