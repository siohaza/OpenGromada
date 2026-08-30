#include "util/myerror.h"

#include "platform/paths.h"
#include "util/string.h"

#include <SDL3/SDL.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

extern STRING Date2Str();
extern STRING FFileTime(const STRING&);
extern STRING Time2Str();

// GLOBAL: ALIEN 0x49061c
MYERROR* Error;

// GLOBAL: ALIEN 0x481a48
char NewFilename[] = "logs\\error.log";

// GLOBAL: ALIEN 0x481a6c
char FileName[] = "error.log";

// FUNCTION: ALIEN 0x4084a0
MYERROR::MYERROR(int p_overwrite)
{
	m_hwnd = 0;
	STRING fname(
		(
			// STRING: ALIEN 0x481a60
			"logs\\error" + Date2Str() + " " + Time2Str()
		)
			.m_str,
		// STRING: ALIEN 0x481a58
		".log"
	);
	fname.Replace(
		// STRING: ALIEN 0x47f730
		":",
		// STRING: ALIEN 0x47f734
		"h"
	);
	fname.Replace(":", "m");
	const char* mode;
	if (!p_overwrite) {
		fname = NewFilename;
	}
	mode = p_overwrite ?
					   // STRING: ALIEN 0x481a40
			   "wt"
					   :
					   // STRING: ALIEN 0x481a44
			   "at";
	m_file = *fname.m_str ? Platform_FOpen(fname.m_str, mode) : 0;
	if (!m_file) {
		char* after;
		fname.After(
			&after,
			// STRING: ALIEN 0x481a38
			"logs\\"
		);
		fname = after;
		if (after != STRING::EMPTY) {
			operator delete(after);
		}
		mode = p_overwrite ? "wt" : "at";
		m_file = *fname.m_str ? Platform_FOpen(fname.m_str, mode) : 0;
	}
	snprintf(m_fname, sizeof(m_fname), "%s", fname.m_str);
	{
		STRING exe(Platform_ExecutablePath());
		STRING version = FFileTime(exe);
		STRING time = Time2Str();
		STRING date = Date2Str();
		Log(this,
			// STRING: ALIEN 0x481a14
			"----< %s %s >----< %s (%s) >----",
			date.m_str,
			time.m_str,
			exe.m_str,
			version.m_str);
	}
}

// FUNCTION: ALIEN 0x408770
MYERROR::~MYERROR()
{
	if (m_file) {
		fclose(m_file);
	}
	m_file = 0;
	if (strcmp(m_fname, NewFilename) && strcmp(m_fname, FileName)) {
		Platform_Remove(NewFilename);
		if (Platform_Rename(m_fname, NewFilename)) {
			Platform_Remove(FileName);
			Platform_Rename(m_fname, FileName);
		}
	}
}

// FUNCTION: ALIEN 0x408840
int MYERROR::Window(MYERROR* p_handler, const char* p_fmt, ...)
{
	char text[1024];
	if (!p_fmt) {
		return 0;
	}
	va_list args;
	va_start(args, p_fmt);
	vsnprintf(text, sizeof(text), p_fmt, args);
	va_end(args);

	if (p_handler && p_handler->m_file) {
		fputs(text, p_handler->m_file);
		fputs("\n", p_handler->m_file);
		fflush(p_handler->m_file);
	}

	// m_hwnd is the SDL window when one exists; a null parent is fine.
	SDL_ShowSimpleMessageBox(
		SDL_MESSAGEBOX_ERROR,
		// STRING: ALIEN 0x481a78
		"Error",
		text,
		p_handler ? (SDL_Window*) p_handler->m_hwnd : 0
	);
	return 0;
}

// FUNCTION: ALIEN 0x4088d0
int MYERROR::LogStatus(MYERROR* p_handler, const char* p_fmt, ...)
{
	char buffer[1024];
	va_list args;
	va_start(args, p_fmt);
	int result = p_fmt ? 1 : 0;
	if (p_fmt && p_handler && p_handler->m_file) {
		vsnprintf(buffer, sizeof(buffer), p_fmt, args);
		int pos = ftell(p_handler->m_file);
		fputs(buffer, p_handler->m_file);
		// STRING: ALIEN 0x481a80
		fputs("\n", p_handler->m_file);
		fflush(p_handler->m_file);
		result = fseek(p_handler->m_file, pos, 0);
	}
	va_end(args);
	return result;
}

// FUNCTION: ALIEN 0x408950
int MYERROR::Log(MYERROR* p_handler, const char* p_fmt, ...)
{
	char buffer[1024];
	va_list args;
	va_start(args, p_fmt);
	int result = p_fmt ? 1 : 0;
	if (p_fmt && p_handler && p_handler->m_file) {
		vsnprintf(buffer, sizeof(buffer), p_fmt, args);
		fputs(buffer, p_handler->m_file);
		fputs("\n", p_handler->m_file);
		result = fflush(p_handler->m_file);
	}
	va_end(args);
	return result;
}

// FUNCTION: ALIEN 0x4089b0
void MYERROR::LogExit(MYERROR* p_handler, const char* p_fmt, ...)
{
	char buffer[1024];
	va_list args;
	va_start(args, p_fmt);
	if (p_fmt) {
		vsnprintf(buffer, sizeof(buffer), p_fmt, args);
	}
	else {
		buffer[0] = 0;
	}
	va_end(args);
	Log(p_handler, "%s", buffer);
	if (p_handler && p_handler->m_file) {
		fclose(p_handler->m_file);
	}
	if (p_handler) {
		p_handler->m_file = 0;
	}
	exit(1);
}

// FUNCTION: ALIEN 0x408e00
int MYERROR::Error(MYERROR* p_handler, const char* p_format, int p_type, const char* p_error, int p_size, ...)
{
	char buffer[1024];
	STRING time = Time2Str();
	int prefix = snprintf(
		buffer,
		sizeof(buffer),
		// STRING: ALIEN 0x48207c
		"!!!ERROR %s!!!",
		time.m_str
	);
	size_t used = prefix > 0 ? (size_t) prefix : 0;
	if (used >= sizeof(buffer)) {
		used = sizeof(buffer) - 1;
	}

	va_list args;
	va_start(args, p_size);
	if (p_format) {
		int written = vsnprintf(buffer + used, sizeof(buffer) - used, p_format, args);
		if (written > 0) {
			size_t available = sizeof(buffer) - used;
			used += (size_t) written < available ? (size_t) written : available - 1;
		}
	}
	va_end(args);
	if (used < sizeof(buffer) - 1) {
		int written = snprintf(
			buffer + used,
			sizeof(buffer) - used,
			// STRING: ALIEN 0x482078
			": "
		);
		if (written > 0) {
			size_t available = sizeof(buffer) - used;
			used += (size_t) written < available ? (size_t) written : available - 1;
		}
	}

	const char* detail = 0;
	switch (p_type) {
	case 1:
		// STRING: ALIEN 0x482060
		detail = "0x%X Couldn't copy %s";
		break;
	case 0:
		// STRING: ALIEN 0x482048
		detail = "0x%X Couldn't lock %s";
		break;
	case 2:
		// STRING: ALIEN 0x482020
		detail = "%i There was not enough memory for %s";
		break;
	case 13:
		// STRING: ALIEN 0x482010
		detail = "%i Missing %s";
		break;
	case 3:
		// STRING: ALIEN 0x481ff4
		detail = "0x%X Couldn't create the %s";
		break;
	case 4:
		// STRING: ALIEN 0x481fe4
		detail = "0x%X Invalid %s";
		break;
	case 11:
		// STRING: ALIEN 0x481fc4
		detail = "0x%X Section can't found (%s)";
		break;
	case 8:
		// STRING: ALIEN 0x481fa8
		detail = "0x%X Couldn't set the %s";
		break;
	case 9:
		// STRING: ALIEN 0x481f8c
		detail = "0x%X Couldn't get the %s";
		break;
	case 7:
		// STRING: ALIEN 0x481f74
		detail = "0x%X Couldn't open '%s'";
		break;
	case 5:
		// STRING: ALIEN 0x481f64
		detail = "0x%X Load %s";
		break;
	case 6:
		// STRING: ALIEN 0x481f54
		detail = "0x%X Save %s";
		break;
	case 10:
		// STRING: ALIEN 0x481f4c
		detail = "%i %s";
		break;
	case 12:
		// STRING: ALIEN 0x481f30
		detail = "0x%X Unable initialize %s";
		break;
	case 14:
		// STRING: ALIEN 0x481f20
		detail = "%i Unknownn %s";
		break;
	default:
		break;
	}
	if (detail && used < sizeof(buffer) - 1) {
		snprintf(buffer + used, sizeof(buffer) - used, detail, p_size, p_error ? p_error : "");
	}
	return Log(p_handler, "%s", buffer);
}
