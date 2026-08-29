#ifndef MYERROR_H
#define MYERROR_H

#include <stdio.h>

#include "util/decomp.h"

struct _EXCEPTION_RECORD;

// VTABLE: ALIEN 0x47a2fc
class MYERROR {
public:
	MYERROR(int p_overwrite);
	virtual ~MYERROR();

	int m_unk0x04; // 0x04
	FILE* m_file; // 0x08
	char m_fname[0x400]; // 0x0c

	static char* Error(int p_handler, const char* p_format, int p_type, const char* p_error, int p_size, ...);
	static char* Window(int p_handler, const char* p_fmt, ...);
	static char* LogStatus(int p_handler, const char* p_fmt, ...);
	static char* Log(int p_handler, const char* p_fmt, ...);
	static void LogExit(int p_handler, const char* p_fmt, ...);
	int LogException(struct _EXCEPTION_RECORD* p_rec);
};

DECOMP_SIZE_ASSERT(MYERROR, 0x40c)

// SYNTHETIC: ALIEN 0x408750
// MYERROR::`scalar deleting destructor'

extern int Error;

#endif
