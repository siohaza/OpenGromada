#include "gfx/picture_bmp.h"

#include "platform/paths.h"
#include "util/myerror.h"

#include <ctype.h>

static __forceinline int LastChar(const char* p_value)
{
	if (*p_value) {
		return p_value[strlen(p_value) - 1];
	}
	return 0;
}

// FUNCTION: ALIEN 0x428400
int PICTURE_BMP::LoadHeader(BMP_HEADER* p_header)
{
	if (!m_file) {
		MYERROR::Error(
			::Error,
			"PICTURE '%s'",
			7,
			// STRING: ALIEN 0x4838fc
			"bmp cadr",
			m_frame,
			m_name.m_str
		);
		return 1;
	}
	fread(p_header, sizeof(*p_header), 1, m_file);
	if (p_header->m_compression) {
		Close();
		MYERROR::Error(
			::Error,
			"PICTURE '%s'",
			10,
			// STRING: ALIEN 0x4838d8
			"not supported bmp compression type",
			0,
			m_name.m_str
		);
		return 1;
	}
	if (p_header->m_depth < 8) {
		Close();
		MYERROR::Error(
			::Error,
			"PICTURE '%s'",
			10,
			// STRING: ALIEN 0x4838b4
			"not supported bmp 2 and 4 bit type",
			0,
			m_name.m_str
		);
		return 1;
	}
	m_unk0x428 = p_header->m_dataOffset;
	if (!m_pixels ||
		(m_width == p_header->m_width && m_height == p_header->m_height && m_bpp == p_header->m_depth >> 3)) {
		return 0;
	}
	MYERROR::Error(
		::Error,
		"PICTURE '%s'",
		10,
		// STRING: ALIEN 0x483888
		"BMP parameters different from first cadr",
		m_frame,
		m_name.m_str
	);
	return 1;
}

// FUNCTION: ALIEN 0x4284f0
int PICTURE_BMP::Load(const STRING& p_name)
{
	BMP_HEADER header;
	if (PICTURE_BASE::Load(p_name)) {
		return 1;
	}
	if (LoadHeader(&header)) {
		return 1;
	}
	m_noFrames = 1;
	char* temporary;
	char** result = p_name.Before(&temporary, ".bmp");
	int hasNumber = isdigit((char) LastChar(*result));
	if (temporary != STRING::EMPTY) {
		operator delete(temporary);
	}
	if (hasNumber) {
		while (1) {
			char* name = *p_name.Add(&temporary, m_noFrames);
			int exists;
			if (!*name) {
				exists = 0;
			}
			else {
				FILE* file = Platform_FOpen(name, "rb");
				FILE* opened = file;
				if (file) {
					fclose(file);
				}
				exists = opened != 0;
			}
			if (temporary != STRING::EMPTY) {
				operator delete(temporary);
			}
			if (!exists) {
				break;
			}
			++m_noFrames;
		}
	}
	SetSize(header.m_width, header.m_height, header.m_depth >> 3);
	Rewind();
	return 0;
}

// FUNCTION: ALIEN 0x428620
int PICTURE_BMP::Rewind()
{
	int result = 0;
	if (m_pixels) {
		if (m_file) {
			fclose(m_file);
		}
		m_file = *m_name.m_str ? Platform_FOpen(m_name.m_str, "rb") : 0;
		BMP_HEADER header;
		result = LoadHeader(&header);
		if (!result) {
			fseek(m_file, m_unk0x428, 0);
			m_frame = -1;
			return NextFrame();
		}
	}
	return result;
}

// FUNCTION: ALIEN 0x4286a0
int PICTURE_BMP::NextFrame()
{
	if (!m_pixels) {
		return MYERROR::Error(::Error, "PICTURE '%s'", 10, "Picture has not opened", m_frame, m_name.m_str);
	}
	if (++m_frame >= m_noFrames) {
		return Rewind();
	}
	if (m_file) {
		fclose(m_file);
	}
	{
		char* temporary;
		char* numberedName = *m_name.Add(&temporary, m_frame);
		m_file = *numberedName ? Platform_FOpen(numberedName, "rb") : 0;
		if (temporary != STRING::EMPTY) {
			operator delete(temporary);
		}
	}
	BMP_HEADER header;
	if (!LoadHeader(&header)) {
		fseek(m_file, m_unk0x428, 0);
		if (m_bpp == 1) {
			fread(m_palette, 0x400, 1, m_file);
		}
		int lineSize = m_width * m_bpp;
		unsigned char* line = m_pixels + m_height * lineSize;
		int padding = -lineSize & 3;
		fseek(m_file, m_unk0x428, 0);
		for (int lines = m_height; lines > 0; --lines) {
			line -= lineSize;
			fread(line, lineSize, 1, m_file);
			if (padding) {
				fseek(m_file, padding, 1);
			}
		}
	}
	// The original fell off the end here; non-zero means success, matching
	// PICTURE_BASE::NextFrame() and Rewind().
	return 1;
}
