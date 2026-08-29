#define DECOMP_INLINE_STRING_CHARP_CTOR
#define DECOMP_INLINE_STRING_COPY_LIFETIME
#define DECOMP_STRING_TOUPPER_OUT_OF_LINE
#include "util/string.h"

// FUNCTION: ALIEN 0x439bf0
STRING STRING::ToLower() const
{
	STRING result(m_str);
	_strlwr(result.m_str);
	return result;
}

// FUNCTION: ALIEN 0x439cc0
STRING STRING::ToUpper() const
{
	STRING result(m_str);
	_strupr(result.m_str);
	return result;
}
