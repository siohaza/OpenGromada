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
