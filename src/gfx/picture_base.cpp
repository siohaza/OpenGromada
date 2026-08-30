#include "gfx/picture_base.h"

#include "platform/paths.h"
#include "util/myerror.h"
#include "util/string.h"

#include <string.h>

// FUNCTION: ALIEN 0x426dd0
PICTURE_BASE::PICTURE_BASE()
{
	m_noFrames = 0;
	m_frame = 0;
	m_bpp = 0;
	m_unk0x0c = 71;
	m_width = 0;
	m_height = 0;
	m_file = 0;
	m_pixels = 0;
}

// FUNCTION: ALIEN 0x426e10
void* PICTURE_BASE::ScalarDeletingDestructor(unsigned int p_flags)
{
	PICTURE_BASE* result = this;
	this->~PICTURE_BASE();
	if (p_flags & 1) {
		operator delete(result);
	}
	return result;
}

// FUNCTION: ALIEN 0x426e50
PICTURE_BASE::PICTURE_BASE(int p_width, int p_height, int p_bpp)
{
	m_frame = 0;
	m_bpp = 0;
	m_width = 0;
	m_height = 0;
	m_file = 0;
	m_pixels = 0;
	m_unk0x0c = 71;
	m_noFrames = 1;
	SetSize(p_width, p_height, p_bpp);
}

// FUNCTION: ALIEN 0x426f50
int PICTURE_BASE::Close()
{
	if (m_pixels) {
		operator delete(m_pixels);
	}
	FILE* file = m_file;
	int result = 0;
	m_pixels = 0;
	if (file) {
		result = fclose(file);
	}
	m_file = 0;
	m_noFrames = 0;
	m_width = 0;
	m_height = 0;
	return result;
}

// FUNCTION: ALIEN 0x426f90
int PICTURE_BASE::SaveTGA(const STRING& p_name, int p_x, int p_y, int p_w, int p_h)
{
	unsigned char header[18] = {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 24, 1};
	if (!m_pixels) {
		return MYERROR::Error(
			::Error,
			// STRING: ALIEN 0x483770
			"PICTURE '%s'",
			10,
			// STRING: ALIEN 0x4837a0
			"SaveTGA-not picture",
			0,
			m_name.m_str
		);
	}
	if (p_h == -1) {
		p_h = m_height;
	}
	if (p_w == -1) {
		p_w = m_width;
	}
	if (p_x + p_w > m_width || p_y + p_h > m_height) {
		return MYERROR::Error(
			::Error,
			"PICTURE '%s'",
			4,
			// STRING: ALIEN 0x483790
			"size in SaveTGA",
			0,
			m_name.m_str
		);
	}
	FILE* file = 0;
	if (*p_name.m_str) {
		file = Platform_FOpen(
			p_name.m_str,
			// STRING: ALIEN 0x481814
			"wb"
		);
	}
	if (!file) {
		return MYERROR::Error(::Error, "PICTURE '%s'", 7, p_name.m_str, 0, m_name.m_str);
	}
	if (m_bpp == 4) {
		header[16] = m_bpp * 8;
	}
	header[12] = (unsigned char) p_w;
	header[13] = (unsigned char) ((unsigned int) p_w >> 8);
	header[14] = (unsigned char) p_h;
	header[15] = (unsigned char) ((unsigned int) p_h >> 8);
	header[0] = 0;
	header[2] &= ~8;
	fwrite(header, 18, 1, file);
	for (int y = p_h - 1; y >= 0; --y) {
		for (int x = 0; x < p_w; ++x) {
			if (m_bpp == 4) {
				int pixel;
				int value = *GetPixel(&pixel, x + p_x, y + p_y);
				fwrite(&value, 1, 4, file);
			}
			else {
				int pixel;
				int value = *GetPixel(&pixel, x + p_x, y + p_y) & 0xffffff;
				fwrite(&value, 1, 3, file);
			}
		}
	}
	return fclose(file);
}

// FUNCTION: ALIEN 0x4271b0
int PICTURE_BASE::GetData(int p_x, int p_y)
{
	if (p_x < 0 || p_y < 0 || p_x >= m_width || p_y >= m_height) {
		return 0;
	}
	switch (m_bpp) {
	case 4:
		return ((int*) m_pixels)[m_width * p_y + p_x];
	case 3: {
		const unsigned char* pixel = m_pixels + (m_width * p_y + p_x) * 3;
		return (int) pixel[0] | ((int) pixel[1] << 8) | ((int) pixel[2] << 16);
	}
	case 2:
		return ((unsigned short*) m_pixels)[m_width * p_y + p_x];
	case 1:
		return m_pixels[m_width * p_y + p_x];
	}
	return 0;
}

// FUNCTION: ALIEN 0x427250
void PICTURE_BASE::PutData(int p_x, int p_y, unsigned int p_data)
{
	if (p_x < 0 || p_y < 0 || p_x >= m_width || p_y >= m_height) {
		return;
	}
	switch (m_bpp) {
	case 4:
		((unsigned int*) m_pixels)[p_x + p_y * m_width] = p_data;
		break;
	case 3: {
		unsigned char* pixel = m_pixels + 3 * (p_x + p_y * m_width);
		pixel[0] = (unsigned char) p_data;
		pixel[1] = (unsigned char) (p_data >> 8);
		pixel[2] = (unsigned char) (p_data >> 16);
		break;
	}
	case 2:
		((unsigned short*) m_pixels)[p_x + p_y * m_width] = (unsigned short) p_data;
		break;
	case 1:
		m_pixels[p_x + p_y * m_width] = (unsigned char) p_data;
		break;
	}
}

// FUNCTION: ALIEN 0x427330
void PICTURE_BASE::PutPixel(int p_x, int p_y, COLOR p_color)
{
	if (p_x >= 0 && p_y >= 0 && p_x < m_width && p_y < m_height) {
		switch (m_bpp) {
		case 4:
			((unsigned int*) m_pixels)[p_x + p_y * m_width] = p_color.m_value;
			break;
		case 3: {
			unsigned char* pixel = m_pixels + 3 * (p_x + p_y * m_width);
			pixel[0] = (unsigned char) p_color.m_value;
			pixel[1] = (unsigned char) (p_color.m_value >> 8);
			pixel[2] = (unsigned char) (p_color.m_value >> 16);
			break;
		}
		case 2:
			((unsigned short*) m_pixels)[p_x + p_y * m_width] =
				(unsigned short) ((p_color.m_value >> 9 & 0x7c00) | (p_color.m_value >> 6 & 0x3e0) |
								  (p_color.m_value >> 3 & 0x1f));
			break;
		case 1:
			m_pixels[p_x + p_y * m_width] = (unsigned char) p_color.m_value;
			break;
		}
	}
}

// FUNCTION: ALIEN 0x427440
int* PICTURE_BASE::GetPixel(int* p_out, int p_x, int p_y)
{
	if (p_x < 0) {
		*p_out = 0xff000000;
		return p_out;
	}
	if (p_y < 0) {
		*p_out = 0xff000000;
		return p_out;
	}
	int width = m_width;
	if (p_x >= width || p_y >= m_height) {
		*p_out = 0xff000000;
		return p_out;
	}
	int bpp = m_bpp;
	if (bpp == 1) {
		*p_out = m_palette[m_pixels[p_x + p_y * width]];
		return p_out;
	}
	int index;
	--bpp;
	switch (bpp) {
	case 3:
		index = p_x + p_y * width;
		*p_out = ((unsigned int*) m_pixels)[index];
		return p_out;
	case 2: {
		index = p_x + p_y * width;
		const unsigned char* pixel = m_pixels + 3 * index;
		*p_out = (int) pixel[0] | ((int) pixel[1] << 8) | ((int) pixel[2] << 16);
		return p_out;
	}
	case 1: {
		index = p_x + p_y * width;
		const RGB555* rgb = &((RGB555*) m_pixels)[index];
		*p_out = COLOR(*rgb).m_value;
		return p_out;
	}
	case 0:
		*p_out = COLOR(m_pixels[p_x + p_y * width], m_pixels[p_x + p_y * width], m_pixels[p_x + p_y * width]).m_value;
		return p_out;
	}
	*p_out = 0xff000000;
	return p_out;
}

// FUNCTION: ALIEN 0x4275b0
int PICTURE_BASE::Rewind()
{
	int result = 0;
	if (m_pixels) {
		if (m_file) {
			fseek(m_file, m_unk0x428, 0);
		}
		m_frame = -1;
		result = NextFrame();
	}
	return result;
}

// FUNCTION: ALIEN 0x4275f0
int PICTURE_BASE::NextFrame()
{
	int result = m_noFrames;
	if (result) {
		if (++m_frame >= result) {
			return Rewind();
		}
	}
	return result;
}

// FUNCTION: ALIEN 0x427610
int PICTURE_BASE::Load(const STRING& p_name)
{
	Close();
	const char* name = p_name.m_str;
	if (!strcmp(name, empty_str)) {
		MYERROR::Error(
			::Error,
			"PICTURE '%s'",
			4,
			// STRING: ALIEN 0x4837b4
			"filename",
			0,
			m_name.m_str
		);
		return 1;
	}
	m_name = name;
	m_file = *p_name.m_str ? Platform_FOpen(p_name.m_str, "rb") : 0;
	if (!m_file) {
		MYERROR::Error(::Error, "PICTURE '%s'", 7, empty_str, 0, m_name.m_str);
		return 1;
	}
	return 0;
}

// FUNCTION: ALIEN 0x42c010
char** PICTURE_BASE::GetName_impl(char** p_out)
{
	const char* name = m_name.m_str;
	if (*name) {
		unsigned int len = strlen(name);
		char* buf = (char*) operator new((len & 0xfffffff0) + 16);
		*p_out = buf;
		memcpy(buf, name, len);
		(*p_out)[len] = 0;
		return p_out;
	}
	*p_out = STRING::EMPTY;
	return p_out;
}
