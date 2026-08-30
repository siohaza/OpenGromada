#ifndef PLATFORM_TYPES_H
#define PLATFORM_TYPES_H

#if defined(_WIN32)

#include <windows.h>

#else

struct RECT {
	int left;
	int top;
	int right;
	int bottom;
};

struct POINT {
	int x;
	int y;
};

typedef void* HINSTANCE;

#endif

static_assert(sizeof(RECT) == 16, "RECT must be four 32 bit fields");
static_assert(sizeof(POINT) == 8, "POINT must be two 32 bit fields");

#endif
