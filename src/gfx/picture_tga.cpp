#include "gfx/picture_tga.h"

#include "platform/paths.h"
#include "util/myerror.h"

#include <ctype.h>

// FUNCTION: ALIEN 0x427ef0
int PICTURE_TGA::LoadHeader(TGA_HEADER* p_header)
{
	if (!m_file) {
		MYERROR::Error(
			::Error,
			"PICTURE '%s'",
			7,
			// STRING: ALIEN 0x483874
			"tga cadr",
			m_frame,
			m_name.m_str
		);
		return 1;
	}
	if (fread(p_header, sizeof(*p_header), 1, m_file) != 1) {
		Close();
		MYERROR::Error(::Error, "PICTURE '%s'", 5, "truncated tga header", 0, m_name.m_str);
		return 1;
	}
	m_imageType = p_header->m_imageType;
	m_unk0x428 = p_header->m_idLength + sizeof(*p_header);
	if (!m_imageType) {
		Close();
		MYERROR::Error(
			::Error,
			"PICTURE '%s'",
			10,
			// STRING: ALIEN 0x483860
			"not image in tga",
			0,
			m_name.m_str
		);
		return 1;
	}
	if (p_header->m_colorMapType == 1) {
		Close();
		MYERROR::Error(
			::Error,
			"PICTURE '%s'",
			10,
			// STRING: ALIEN 0x48383c
			"not supported ColorMapType in tga",
			0,
			m_name.m_str
		);
		return 1;
	}
	if ((m_imageType & 3) == 1) {
		Close();
		MYERROR::Error(
			::Error,
			"PICTURE '%s'",
			10,
			// STRING: ALIEN 0x48381c
			"not supported ColorMap in tga",
			0,
			m_name.m_str
		);
		return 1;
	}
	if ((m_imageType != 2 && m_imageType != 3 && m_imageType != 10 && m_imageType != 11) || !p_header->m_width ||
		!p_header->m_height ||
		(p_header->m_depth != 8 && p_header->m_depth != 16 && p_header->m_depth != 24 && p_header->m_depth != 32)) {
		Close();
		MYERROR::Error(::Error, "PICTURE '%s'", 10, "unsupported tga parameters", 0, m_name.m_str);
		return 1;
	}
	if (m_pixels &&
		(m_width != p_header->m_width || m_height != p_header->m_height || m_bpp != p_header->m_depth >> 3)) {
		MYERROR::Error(
			::Error,
			"PICTURE '%s'",
			10,
			// STRING: ALIEN 0x4837f0
			"TGA parameters different from first cadr",
			m_frame,
			m_name.m_str
		);
		return 1;
	}
	return 0;
}

// FUNCTION: ALIEN 0x428010
int PICTURE_TGA::Load(const STRING& p_name)
{
	TGA_HEADER header;
	if (PICTURE_BASE::Load(p_name)) {
		return 1;
	}
	if (LoadHeader(&header)) {
		return 1;
	}
	m_noFrames = 1;
	char* temporary;
	char** result = m_name.Before(&temporary, ".tga");
	int hasNumber = isdigit((char) (**result ? (*result)[strlen(*result) - 1] : 0));
	if (temporary != STRING::EMPTY) {
		operator delete(temporary);
	}
	if (hasNumber) {
		while (1) {
			char* name = *m_name.Add(&temporary, m_noFrames);
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

// FUNCTION: ALIEN 0x428150
int PICTURE_TGA::Rewind()
{
	int result = 0;
	if (m_pixels) {
		if (m_file) {
			fclose(m_file);
		}
		m_file = *m_name.m_str ? Platform_FOpen(m_name.m_str, "rb") : 0;
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
		char* numbered;
		char* numberedName = *m_name.Add(&numbered, m_frame);
		m_file = *numberedName ? Platform_FOpen(numberedName, "rb") : 0;
		if (numbered != STRING::EMPTY) {
			operator delete(numbered);
		}
	}
	TGA_HEADER header;
	if (!LoadHeader(&header)) {
		fseek(m_file, m_unk0x428, 0);
		if (m_imageType & 8) {
			// Decode directly because RLE packets may exceed the image size and cross rows.
			const size_t pixelCount = (size_t) m_width * (size_t) m_height;
			size_t decoded = 0;
			unsigned char pixel[4];
			while (decoded < pixelCount) {
				unsigned char packet;
				if (fread(&packet, 1, 1, m_file) != 1) {
					return 0;
				}
				const size_t count = (size_t) (packet & 0x7f) + 1;
				if (count > pixelCount - decoded) {
					return 0;
				}
				const bool repeated = (packet & 0x80) != 0;
				if (repeated && fread(pixel, 1, m_bpp, m_file) != (size_t) m_bpp) {
					return 0;
				}
				for (size_t i = 0; i < count; ++i) {
					if (!repeated && fread(pixel, 1, m_bpp, m_file) != (size_t) m_bpp) {
						return 0;
					}
					const size_t sourceIndex = decoded + i;
					const size_t x = sourceIndex % (size_t) m_width;
					const size_t y = (size_t) m_height - 1 - sourceIndex / (size_t) m_width;
					memcpy(m_pixels + m_bpp * (x + y * (size_t) m_width), pixel, m_bpp);
				}
				decoded += count;
			}
		}
		else {
			const size_t rowSize = (size_t) m_width * (size_t) m_bpp;
			for (int y = m_height - 1; y >= 0; --y) {
				if (fread(m_pixels + (size_t) y * rowSize, 1, rowSize, m_file) != rowSize) {
					return 0;
				}
			}
		}
	}
	// The original fell off the end here; non-zero means success, matching
	// PICTURE_BASE::NextFrame() and Rewind().
	return 1;
}
