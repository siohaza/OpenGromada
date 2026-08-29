#include "gfx/picture_base.h"

#include <string.h>

#include "util/myerror.h"
#include "util/string.h"

// FUNCTION: ALIEN 0x426eb0
void PICTURE_BASE::SetSize(int p_width, int p_height, int p_bpp)
{
	m_width = p_width;
	m_height = p_height;
	m_bpp = p_bpp;
	if (m_pixels)
		operator delete(m_pixels);
	m_pixels = (unsigned char*) operator new(m_height * m_bpp * m_width);
	if (m_pixels)
		memset(m_pixels, 0, m_height * m_bpp * m_width);
	else
		MYERROR::Error(::Error,
			"PICTURE '%s'", 2,
			// STRING: ALIEN 0x483780
			"picture buffer", m_height * m_bpp * m_width, m_name);
}
