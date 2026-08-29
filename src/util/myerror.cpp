#define DECOMP_UNINITIALIZED_STRING_DEFAULT_CTOR
#define DECOMP_INLINE_STRING_COPY_LIFETIME
#define DECOMP_INLINE_STRING_CHARP_CONVERSION
#include "util/myerror.h"

#include <stdarg.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "util/string.h"

extern STRING Date2Str();
extern STRING Time2Str();

typedef STRING* (__cdecl *STRING_SRET_FN)(STRING*);
typedef STRING* (__cdecl *STRING_SRET_REF_FN)(STRING*, const STRING&);

// GLOBAL: ALIEN 0x49061c
int Error;

// GLOBAL: ALIEN 0x481a48
char NewFilename[] = "logs\\error.log";

// GLOBAL: ALIEN 0x481a6c
char FileName[] = "error.log";

// FUNCTION: ALIEN 0x4084a0
MYERROR::MYERROR(int p_overwrite)
{
	m_unk0x04 = 0;
	STRING fname((
					 // STRING: ALIEN 0x481a60
					 "logs\\error" + Date2Str() +
					 " " + Time2Str())
					 .m_str,
		// STRING: ALIEN 0x481a58
		".log");
	fname.Replace(
		// STRING: ALIEN 0x47f730
		":",
		// STRING: ALIEN 0x47f734
		"h");
	fname.Replace(":",
		"m");
	const char* mode;
	if (!p_overwrite)
		fname = NewFilename;
	mode = p_overwrite ?
		// STRING: ALIEN 0x481a40
		"wt" :
		// STRING: ALIEN 0x481a44
		"at";
	m_file = *fname.m_str ? fopen(fname.m_str, mode) : 0;
	if (!m_file) {
		char* after;
		fname = *(STRING*) fname.After(&after,
			// STRING: ALIEN 0x481a38
			"logs\\");
		if (after != STRING::EMPTY)
			operator delete(after);
		mode = p_overwrite ? "wt" : "at";
		m_file = *fname.m_str ? fopen(fname.m_str, mode) : 0;
	}
	strcpy(m_fname, fname.m_str);
	{
	STRING exe;
	if (_pgmptr && *_pgmptr)
		exe.Copy(_pgmptr, strlen(_pgmptr));
	else
		exe.m_str = STRING::EMPTY;
	STRING version;
	STRING time;
	char* timeText = ((STRING_SRET_FN) Time2Str)(&time)->m_str;
	STRING date;
	char* dateText = ((STRING_SRET_FN) Date2Str)(&date)->m_str;
	Log((int) this,
		// STRING: ALIEN 0x481a14
		"----< %s %s >----< %s (%s) >----",
		dateText, timeText, _pgmptr,
		((STRING_SRET_REF_FN) FFileTime)(&version, exe)->m_str);
	}
}

// FUNCTION: ALIEN 0x408770
MYERROR::~MYERROR()
{
	if (m_file)
		fclose(m_file);
	m_file = 0;
	if (strcmp(m_fname, NewFilename) && strcmp(m_fname, FileName)) {
		remove(NewFilename);
		if (rename(m_fname, NewFilename)) {
			remove(FileName);
			rename(m_fname, FileName);
		}
	}
}

// FUNCTION: ALIEN 0x408840
char* MYERROR::Window(int p_handler, const char* p_fmt, ...)
{
	char text[1024];
	va_list args;
	va_start(args, p_fmt);
	if (!p_fmt)
		return (char*) p_fmt;
	vsprintf(text, p_fmt, args);
	if (*(FILE**) (p_handler + 8)) {
		fputs(text, *(FILE**) (p_handler + 8));
		fputs("\n", *(FILE**) (p_handler + 8));
		fflush(*(FILE**) (p_handler + 8));
	}
	if (!*(int*) (p_handler + 4))
		*(int*) (p_handler + 4) = (int) GetForegroundWindow();
	return (char*) MessageBoxA(*(HWND*) (p_handler + 4), text,
		// STRING: ALIEN 0x481a78
		"Error", 0);
}

// FUNCTION: ALIEN 0x4088d0
char* MYERROR::LogStatus(int p_handler, const char* p_fmt, ...)
{
	char buffer[1024];
	va_list args;
	va_start(args, p_fmt);
	char* result = (char*) p_fmt;
	if (p_fmt && *(FILE**) (p_handler + 8)) {
		vsprintf(buffer, p_fmt, args);
		int pos = ftell(*(FILE**) (p_handler + 8));
		fputs(buffer, *(FILE**) (p_handler + 8));
		// STRING: ALIEN 0x481a80
		fputs("\n", *(FILE**) (p_handler + 8));
		fflush(*(FILE**) (p_handler + 8));
		result = (char*) fseek(*(FILE**) (p_handler + 8), pos, 0);
	}
	return result;
}

// FUNCTION: ALIEN 0x408950
char* MYERROR::Log(int p_handler, const char* p_fmt, ...)
{
	char buffer[1024];
	va_list args;
	va_start(args, p_fmt);
	char* result = (char*) p_fmt;
	if (p_fmt && *(FILE**) (p_handler + 8)) {
		vsprintf(buffer, p_fmt, args);
		fputs(buffer, *(FILE**) (p_handler + 8));
		fputs("\n", *(FILE**) (p_handler + 8));
		result = (char*) fflush(*(FILE**) (p_handler + 8));
	}
	return result;
}

// FUNCTION: ALIEN 0x4089b0
void MYERROR::LogExit(int p_handler, const char* p_fmt, ...)
{
	char buffer[1024];
	va_list args;
	va_start(args, p_fmt);
	vsprintf(buffer, p_fmt, args);
	Log(p_handler, buffer);
	if (*(FILE**) (p_handler + 8))
		fclose(*(FILE**) (p_handler + 8));
	*(FILE**) (p_handler + 8) = 0;
	exit(1);
}

// FUNCTION: ALIEN 0x408a10
int MYERROR::LogException(EXCEPTION_RECORD* p_rec)
{
	switch (p_rec->ExceptionCode) {
	case EXCEPTION_BREAKPOINT:
		Log((int) this,
			// STRING: ALIEN 0x481eec
			"!!!ERROR EXCEPTION 0x%X!!!: EXCEPTION_BREAKPOINT", p_rec->ExceptionAddress);
		break;
	case EXCEPTION_DATATYPE_MISALIGNMENT:
		Log((int) this,
			// STRING: ALIEN 0x481eb0
			"!!!ERROR EXCEPTION 0x%X!!!: EXCEPTION_DATATYPE_MISALIGNMENT", p_rec->ExceptionAddress);
		break;
	case EXCEPTION_SINGLE_STEP:
		Log((int) this,
			// STRING: ALIEN 0x481e7c
			"!!!ERROR EXCEPTION 0x%X!!!: EXCEPTION_SINGLE_STEP", p_rec->ExceptionAddress);
		break;
	case EXCEPTION_IN_PAGE_ERROR:
		Log((int) this,
			// STRING: ALIEN 0x481e48
			"!!!ERROR EXCEPTION 0x%X!!!: EXCEPTION_IN_PAGE_ERROR", p_rec->ExceptionAddress);
		break;
	case EXCEPTION_ACCESS_VIOLATION:
		if (p_rec->ExceptionInformation[0])
			Log((int) this,
				// STRING: ALIEN 0x481e0c
				"!!!ERROR EXCEPTION 0x%X!!!: Access violation write to 0x%X",
				p_rec->ExceptionAddress, p_rec->ExceptionInformation[1]);
		else
			Log((int) this,
				// STRING: ALIEN 0x481dd0
				"!!!ERROR EXCEPTION 0x%X!!!: Access violation read from 0x%X",
				p_rec->ExceptionAddress, p_rec->ExceptionInformation[1]);
		break;
	case EXCEPTION_ILLEGAL_INSTRUCTION:
		Log((int) this,
			// STRING: ALIEN 0x481d94
			"!!!ERROR EXCEPTION 0x%X!!!: EXCEPTION_ILLEGAL_INSTRUCTION", p_rec->ExceptionAddress);
		break;
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		Log((int) this,
			// STRING: ALIEN 0x481d58
			"!!!ERROR EXCEPTION 0x%X!!!: EXCEPTION_ARRAY_BOUNDS_EXCEEDED", p_rec->ExceptionAddress);
		break;
	case EXCEPTION_FLT_DENORMAL_OPERAND:
		Log((int) this,
			// STRING: ALIEN 0x481d1c
			"!!!ERROR EXCEPTION 0x%X!!!: EXCEPTION_FLT_DENORMAL_OPERAND", p_rec->ExceptionAddress);
		break;
	case EXCEPTION_INVALID_DISPOSITION:
		Log((int) this,
			// STRING: ALIEN 0x481ce0
			"!!!ERROR EXCEPTION 0x%X!!!: EXCEPTION_INVALID_DISPOSITION", p_rec->ExceptionAddress);
		break;
	case EXCEPTION_NONCONTINUABLE_EXCEPTION:
		Log((int) this,
			// STRING: ALIEN 0x481ca0
			"!!!ERROR EXCEPTION 0x%X!!!: EXCEPTION_NONCONTINUABLE_EXCEPTION", p_rec->ExceptionAddress);
		break;
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		Log((int) this,
			// STRING: ALIEN 0x481c70
			"!!!ERROR EXCEPTION 0x%X!!!:FLT divide by zero", p_rec->ExceptionAddress);
		break;
	case EXCEPTION_FLT_INEXACT_RESULT:
		Log((int) this,
			// STRING: ALIEN 0x481c34
			"!!!ERROR EXCEPTION 0x%X!!!: EXCEPTION_FLT_INEXACT_RESULT", p_rec->ExceptionAddress);
		break;
	case EXCEPTION_FLT_INVALID_OPERATION:
		Log((int) this,
			// STRING: ALIEN 0x481bf8
			"!!!ERROR EXCEPTION 0x%X!!!: EXCEPTION_FLT_INVALID_OPERATION", p_rec->ExceptionAddress);
		break;
	case EXCEPTION_FLT_OVERFLOW:
		Log((int) this,
			// STRING: ALIEN 0x481bc4
			"!!!ERROR EXCEPTION 0x%X!!!: EXCEPTION_FLT_OVERFLOW", p_rec->ExceptionAddress);
		break;
	case EXCEPTION_FLT_STACK_CHECK:
		Log((int) this,
			// STRING: ALIEN 0x481b8c
			"!!!ERROR EXCEPTION 0x%X!!!: EXCEPTION_FLT_STACK_CHECK", p_rec->ExceptionAddress);
		break;
	case EXCEPTION_FLT_UNDERFLOW:
		Log((int) this,
			// STRING: ALIEN 0x481b58
			"!!!ERROR EXCEPTION 0x%X!!!: EXCEPTION_FLT_UNDERFLOW", p_rec->ExceptionAddress);
		break;
	case EXCEPTION_INT_DIVIDE_BY_ZERO:
		Log((int) this,
			// STRING: ALIEN 0x481b28
			"!!!ERROR EXCEPTION 0x%X!!!:INT divide by zero", p_rec->ExceptionAddress);
		break;
	case EXCEPTION_INT_OVERFLOW:
		Log((int) this,
			// STRING: ALIEN 0x481af4
			"!!!ERROR EXCEPTION 0x%X!!!: EXCEPTION_INT_OVERFLOW", p_rec->ExceptionAddress);
		break;
	case EXCEPTION_PRIV_INSTRUCTION:
		Log((int) this,
			// STRING: ALIEN 0x481abc
			"!!!ERROR EXCEPTION 0x%X!!!: EXCEPTION_PRIV_INSTRUCTION", p_rec->ExceptionAddress);
		break;
	case EXCEPTION_STACK_OVERFLOW:
		Log((int) this,
			// STRING: ALIEN 0x481a84
			"!!!ERROR EXCEPTION 0x%X!!!: EXCEPTION_STACK_OVERFLOW", p_rec->ExceptionAddress);
		break;
	}
	return 1;
}

// FUNCTION: ALIEN 0x408e00
char* MYERROR::Error(int p_handler, const char* p_format, int p_type, const char* p_error, int p_size, ...)
{
	char buffer[1024];
	va_list args;
	sprintf(buffer,
		// STRING: ALIEN 0x48207c
		"!!!ERROR %s!!!", (const char*) Time2Str());
	va_start(args, p_size);
	vsprintf(buffer + strlen(buffer), p_format, args);
	va_end(args);
	strcat(buffer,
		// STRING: ALIEN 0x482078
		": ");

	switch (p_type) {
	case 1:
		// STRING: ALIEN 0x482060
		strcat(buffer, "0x%X Couldn't copy %s");
		break;
	case 0:
		// STRING: ALIEN 0x482048
		strcat(buffer, "0x%X Couldn't lock %s");
		break;
	case 2:
		// STRING: ALIEN 0x482020
		strcat(buffer, "%i There was not enough memory for %s");
		break;
	case 13:
		// STRING: ALIEN 0x482010
		strcat(buffer, "%i Missing %s");
		break;
	case 3:
		// STRING: ALIEN 0x481ff4
		strcat(buffer, "0x%X Couldn't create the %s");
		break;
	case 4:
		// STRING: ALIEN 0x481fe4
		strcat(buffer, "0x%X Invalid %s");
		break;
	case 11:
		// STRING: ALIEN 0x481fc4
		strcat(buffer, "0x%X Section can't found (%s)");
		break;
	case 8:
		// STRING: ALIEN 0x481fa8
		strcat(buffer, "0x%X Couldn't set the %s");
		break;
	case 9:
		// STRING: ALIEN 0x481f8c
		strcat(buffer, "0x%X Couldn't get the %s");
		break;
	case 7:
		// STRING: ALIEN 0x481f74
		strcat(buffer, "0x%X Couldn't open '%s'");
		break;
	case 5:
		// STRING: ALIEN 0x481f64
		strcat(buffer, "0x%X Load %s");
		break;
	case 6:
		// STRING: ALIEN 0x481f54
		strcat(buffer, "0x%X Save %s");
		break;
	case 10:
		// STRING: ALIEN 0x481f4c
		strcat(buffer, "%i %s");
		break;
	case 12:
		// STRING: ALIEN 0x481f30
		strcat(buffer, "0x%X Unable initialize %s");
		break;
	case 14:
		// STRING: ALIEN 0x481f20
		strcat(buffer, "%i Unknownn %s");
		break;
	default:
		break;
	}
	return Log(p_handler, buffer, p_size, p_error);
}
