#include "gfx/picture.h"

#include "gfx/picture_bmp.h"
#include "gfx/picture_flic.h"
#include "gfx/picture_tga.h"
#include "gfx/picture_z.h"
#include "util/myerror.h"

#include <string.h>

// FUNCTION: ALIEN 0x4289f0
PICTURE::PICTURE(int p_w, int p_h, int p_format)
{
	m_format = p_format;
	if (p_format <= 0) {
		return;
	}
	if (p_format > 2) {
		if (p_format == 5) {
			m_impl = new PICTURE_BASE(p_w, p_h, 2);
		}
	}
	else {
		m_impl = new PICTURE_BASE(p_w, p_h, 3);
	}
}

// FUNCTION: ALIEN 0x428a80
PICTURE::PICTURE()
{
	m_impl = new PICTURE_BASE();
}

// FUNCTION: ALIEN 0x428ac0
int PICTURE::Load(const char** p_name)
{
	if (!p_name || !*p_name) {
		return 1;
	}
	if (m_impl) {
		m_impl->ScalarDeletingDestructor(1);
	}
	if (strstr(*p_name, ".tga") || strstr(
									   *p_name,
									   // STRING: ALIEN 0x483960
									   ".TGA"
								   )) {
		m_format = 1;
		m_impl = new PICTURE_TGA;
	}
	else if (
		strstr(
			*p_name,
			// STRING: ALIEN 0x48395c
			".z"
		) ||
		strstr(
			*p_name,
			// STRING: ALIEN 0x483958
			".Z"
		)
	) {
		m_format = 5;
		m_impl = new PICTURE_Z;
	}
	else if (
		strstr(*p_name, ".flc") || strstr(
									   *p_name,
									   // STRING: ALIEN 0x483950
									   ".FLC"
								   )
	) {
		m_format = 3;
		m_impl = new PICTURE_FLIC;
	}
	else if (
		strstr(*p_name, ".bmp") || strstr(
									   *p_name,
									   // STRING: ALIEN 0x483948
									   ".BMP"
								   )
	) {
		m_format = 2;
		m_impl = new PICTURE_BMP;
	}
	else {
		if (strcmp(*p_name, empty_str)) {
			MYERROR::Log(
				::Error,
				// STRING: ALIEN 0x483918
				"!!!ERROR!!!PICTURE '%s': Unknown format file",
				*p_name
			);
		}
		m_format = 0;
		m_impl = new PICTURE_BASE;
	}
	if (m_format) {
		STRING name(*p_name);
		return m_impl->Load(name);
	}
	return 1;
}

// FUNCTION: ALIEN 0x430fa0
void PICTURE::PutPixel(int p_x, int p_y, COLOR p_color)
{
	m_impl->PutPixel(p_x, p_y, p_color);
}
