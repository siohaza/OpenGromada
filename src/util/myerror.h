#ifndef MYERROR_H
#define MYERROR_H

#include "util/decomp.h"

#include <stdio.h>

// VTABLE: ALIEN 0x47a2fc
class MYERROR {
public:
	MYERROR(int p_overwrite);
	virtual ~MYERROR();

	void* m_hwnd;        // 0x04 (HWND for the message box)
	FILE* m_file;        // 0x08
	char m_fname[0x400]; // 0x0c

	static int Error(
		MYERROR* p_handler,
		const char* p_format,
		int p_type,
		const char* p_error,
		int p_size,
		...
	) DECOMP_PRINTF(2, 6);
	static int Window(MYERROR* p_handler, const char* p_fmt, ...) DECOMP_PRINTF(2, 3);
	static int LogStatus(MYERROR* p_handler, const char* p_fmt, ...) DECOMP_PRINTF(2, 3);
	static int Log(MYERROR* p_handler, const char* p_fmt, ...) DECOMP_PRINTF(2, 3);
	static void LogExit(MYERROR* p_handler, const char* p_fmt, ...) DECOMP_PRINTF(2, 3);
};

// SYNTHETIC: ALIEN 0x408750
// MYERROR::`scalar deleting destructor'

extern MYERROR* Error;

#endif
