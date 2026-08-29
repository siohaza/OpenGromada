#include "gfx/picture_tga.h"

#include "util/myerror.h"

#include <ctype.h>

// FUNCTION: ALIEN 0x427ef0
int PICTURE_TGA::LoadHeader(TGA_HEADER* p_header)
{
	if (!m_file) {
		MYERROR::Error(::Error,
			"PICTURE '%s'", 7,
			// STRING: ALIEN 0x483874
			"tga cadr", m_frame, m_name);
		return 1;
	}
	fread(p_header, sizeof(*p_header), 1, m_file);
	m_imageType = p_header->m_imageType;
	m_unk0x428 = p_header->m_idLength + sizeof(*p_header);
	if (!m_imageType) {
		Close();
		MYERROR::Error(::Error,
			"PICTURE '%s'", 10,
			// STRING: ALIEN 0x483860
			"not image in tga", 0, m_name);
		return 1;
	}
	if (p_header->m_colorMapType == 1) {
		Close();
		MYERROR::Error(::Error,
			"PICTURE '%s'", 10,
			// STRING: ALIEN 0x48383c
			"not supported ColorMapType in tga", 0, m_name);
		return 1;
	}
	if ((m_imageType & 3) == 1) {
		Close();
		MYERROR::Error(::Error,
			"PICTURE '%s'", 10,
			// STRING: ALIEN 0x48381c
			"not supported ColorMap in tga", 0, m_name);
		return 1;
	}
	if (m_pixels && (m_width != p_header->m_width || m_height != p_header->m_height || m_bpp != p_header->m_depth >> 3)) {
		MYERROR::Error(::Error,
			"PICTURE '%s'", 10,
			// STRING: ALIEN 0x4837f0
			"TGA parameters different from first cadr", m_frame, m_name);
		return 1;
	}
	return 0;
}

// FUNCTION: ALIEN 0x428010
int PICTURE_TGA::Load(const STRING& p_name)
{
	TGA_HEADER header;
	if (PICTURE_BASE::Load(p_name))
		return 1;
	if (LoadHeader(&header))
		return 1;
	m_noFrames = 1;
	char* temporary;
	char** result = ((STRING*) &m_name)->Before(&temporary, ".tga");
	int hasNumber = isdigit((char) (**result ? (*result)[strlen(*result) - 1] : 0));
	if (temporary != STRING::EMPTY)
		operator delete(temporary);
	if (hasNumber) {
		while (1) {
			char* name = *((STRING*) &m_name)->Add(&temporary, m_noFrames);
			int exists;
			if (!*name) {
				exists = 0;
			} else {
				FILE* file = fopen(name, "rb");
				FILE* opened = file;
				if (file)
					fclose(file);
				exists = opened != 0;
			}
			if (temporary != STRING::EMPTY)
				operator delete(temporary);
			if (!exists)
				break;
			++m_noFrames;
		}
	}
	SetSize(header.m_width, header.m_height, header.m_depth >> 3);
	Rewind();
	return 0;
}

// FUNCTION: ALIEN 0x428150
int PICTURE_TGA::Rewind()
{
	int result = (int) m_pixels;
	if (result) {
		if (m_file)
			fclose(m_file);
		m_file = *m_name.m_str ? fopen(m_name.m_str,
			"rb") : 0;
		TGA_HEADER header;
		result = LoadHeader(&header);
		if (!result) {
			fseek(m_file, m_unk0x428, 0);
			m_frame = -1;
			return NextFrame();
		}
	}
	return result;
}

// STUB: ALIEN 0x4281d0
int PICTURE_TGA::NextFrame()
{
	if (!m_pixels)
		return (int) MYERROR::Error(::Error,
			"PICTURE '%s'", 10,
			"Picture has not opened", m_frame, m_name);
	if (++m_frame >= m_noFrames)
		return Rewind();
	if (m_file)
		fclose(m_file);
	{
		char* numbered;
		char* numberedName = *((STRING*) &m_name)->Add(&numbered, m_frame);
		m_file = *numberedName ? fopen(numberedName, "rb") : 0;
		if (numbered != STRING::EMPTY)
			operator delete(numbered);
	}
	TGA_HEADER header;
	if (!LoadHeader(&header)) {
		fseek(m_file, m_unk0x428, 0);
		if (m_imageType & 8) {
			unsigned char* encoded = new unsigned char[m_width * m_height * m_bpp];
			if (!encoded) {
				return (int) MYERROR::Error(::Error,
					"PICTURE '%s'", 2,
					// STRING: ALIEN 0x483880
					"cadr2", m_frame, m_name);
			}
			fread(encoded, m_width * (m_height * m_bpp), 1, m_file);
			int offset;
			int y = m_height - 1;
			offset = 0;
			for (; y >= 0; --y) {
				for (int x = 0; x < m_width;) {
					int packet = encoded[offset];
					int rle = packet & 0x80;
					int count = (packet & 0x7f) + 1;
					++offset;
					for (int i = 0; i < count; ++i) {
						memcpy(m_pixels + m_bpp * (x + y * m_width), encoded + offset, m_bpp);
						if (!rle)
							offset += m_bpp;
						++x;
					}
					if (rle)
						offset += m_bpp;
				}
			}
			delete[] encoded;
		} else {
			for (int y = m_height - 1; y >= 0; --y)
				fread(m_pixels + y * m_width * m_bpp, m_width * m_bpp, 1, m_file);
		}
	}
}
