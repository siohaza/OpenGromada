#include "gfx/picture_z.h"

#include "util/myerror.h"

struct Z_HEADER {
	unsigned int m_magic;
	int m_width;
	int m_height;
	int m_frames;
};

// FUNCTION: ALIEN 0x428810
int PICTURE_Z::Load(const STRING& p_name)
{
	Z_HEADER header;
	if (PICTURE_BASE::Load(p_name)) {
		return 1;
	}
	fread(&header, sizeof(header), 1, m_file);
	if (header.m_magic != 0x6675425a) {
		Close();
		MYERROR::Error(
			::Error,
			"PICTURE '%s'",
			4,
			// STRING: ALIEN 0x483908
			"Z format file",
			0,
			m_name.m_str
		);
		return 1;
	}
	m_noFrames = header.m_frames;
	m_unk0x428 = sizeof(header);
	m_unk0x0c = 71;
	SetSize(header.m_width, header.m_height, 2);
	Rewind();
	return 0;
}

// FUNCTION: ALIEN 0x4288c0
int PICTURE_Z::NextFrame()
{
	int y;
	if (!m_pixels) {
		return MYERROR::Error(::Error, "PICTURE '%s'", 10, "Picture has not opened", m_frame, m_name.m_str);
	}
	if (++m_frame >= m_noFrames) {
		return Rewind();
	}
	for (y = 0; y < m_height; ++y) {
		for (int x = 0; x < m_width;) {
			unsigned short count;
			fread(&count, sizeof(count), 1, m_file);
			if (count & 0x8000) {
				unsigned short* dst;
				int run = count & 0x7fff;
				dst = ((unsigned short*) m_pixels) + x + m_width * y;
				for (int i = 0; i < run; ++i) {
					dst[i] = 0x8000;
				}
			}
			else {
				fread(((unsigned short*) m_pixels) + x + m_width * y, count, sizeof(unsigned short), m_file);
			}
			x += count & 0x7fff;
		}
	}
	return 1;
}
