#include "gfx/picture_flic.h"

#include "util/myerror.h"
#include "util/packed.h"

#include <string.h>

struct FLIC_HEADER {
	unsigned int m_size;
	unsigned short m_type;
	unsigned short m_frames;
	unsigned short m_width;
	unsigned short m_height;
	unsigned short m_depth;
	unsigned short m_flags;
	unsigned int m_speed;
	unsigned char m_unk0x14[0x3c];
	unsigned int m_firstFrameOffset;
	unsigned char m_unk0x54[0x2c];
};

static_assert(sizeof(FLIC_HEADER) == 0x80, "FLIC_HEADER matches the ondisk FLIC header");

// FUNCTION: ALIEN 0x427780
int PICTURE_FLIC::Load(const STRING& p_name)
{
	FLIC_HEADER header;
	if (PICTURE_BASE::Load(p_name)) {
		return 1;
	}
	fread(&header, sizeof(header), 1, m_file);
	m_type = header.m_type;
	m_noFrames = header.m_frames;
	if (header.m_type == 0xaf11) {
		m_unk0x428 = sizeof(header);
		m_unk0x0c = 1000 * header.m_speed / 70;
	}
	else if (header.m_type == 0xaf12) {
		m_unk0x428 = header.m_firstFrameOffset;
		m_unk0x0c = header.m_speed;
	}
	else {
		Close();
		MYERROR::Error(
			::Error,
			"PICTURE '%s'",
			4,
			// STRING: ALIEN 0x4837c0
			"flic type",
			0,
			m_name.m_str
		);
		return 1;
	}
	SetSize(header.m_width, header.m_height, 1);
	Rewind();
	return 0;
}

// STUB: ALIEN 0x427890
int PICTURE_FLIC::NextFrame()
{
	if (!m_pixels) {
		return MYERROR::Error(
			::Error,
			"PICTURE '%s'",
			10,
			// STRING: ALIEN 0x4837d8
			"Picture has not opened",
			m_frame,
			m_name.m_str
		);
	}

	if (++m_frame >= m_noFrames) {
		return Rewind();
	}

	unsigned int frameSize;
	fread(&frameSize, sizeof(frameSize), 1, m_file);
	unsigned char* frame = (unsigned char*) operator new(frameSize);
	if (!frame) {
		return MYERROR::Error(
			::Error,
			"PICTURE '%s'",
			2,
			// STRING: ALIEN 0x482bb0
			"cadr",
			m_frame,
			m_name.m_str
		);
	}

	fread(frame, frameSize - sizeof(frameSize), 1, m_file);
	if (PackedRead<unsigned short>(frame) != 0xf1fa) {
		operator delete(frame);
		return MYERROR::Error(
			::Error,
			"PICTURE '%s'",
			4,
			// STRING: ALIEN 0x4837cc
			"chunk mark",
			m_frame,
			m_name.m_str
		);
	}

	unsigned int noChunks = PackedRead<unsigned short>(frame + 2);
	unsigned int chunkOffset = 12;
	for (unsigned int chunkNo = 0; chunkNo < noChunks; ++chunkNo) {
		unsigned int chunkSize = PackedRead<unsigned int>(frame + chunkOffset);
		unsigned char* data = frame + chunkOffset + 4;
		chunkOffset += chunkSize;
		unsigned char chunkType = *data;
		data += 2;

		switch (chunkType) {
		case 4:    // FLI_COLOR256
		case 11: { // FLI_COLOR64
			int noPackets = PackedRead<short>(data);
			data += 2;
			int paletteIndex = 0;
			for (int packet = 0; packet < noPackets; ++packet) {
				paletteIndex += *data++;
				int noColors = *data++;
				if (!noColors) {
					noColors = 256;
				}
				for (int color = 0; color < noColors; ++color) {
					int red;
					int green;
					int blue;
					if (chunkType == 11) {
						green = data[1] * 4;
						blue = data[2] * 4;
						red = data[0] * 4;
						m_palette[paletteIndex] = COLOR(red, green, blue).m_value;
					}
					else {
						green = data[1];
						blue = data[2];
						red = data[0];
						m_palette[paletteIndex] = COLOR(red, green, blue).m_value;
					}
					data += 3;
					++paletteIndex;
				}
			}
			break;
		}

		case 7: { // FLI_SS2
			int noLines = PackedRead<unsigned short>(data);
			data += 2;
			int dstOffset = 0;
			int line = 0;
			while (line < noLines) {

				int control;
				while (1) {
					control = PackedRead<short>(data);
					data += 2;
					if ((control & 0xc000) == 0xc000) {
						dstOffset -= m_width * control;
						line -= control;
						noLines -= control;
					}
					else if ((control & 0xc000) == 0x8000) {
						m_pixels[dstOffset + m_width - 1] = (unsigned char) control;
					}
					else {
						break;
					}
				}

				int lineOffset = dstOffset;
				if (!(control & 0xc000)) {
					for (int packet = 0; packet < control; ++packet) {
						dstOffset += *data++;
						int words = *(signed char*) data++;
						if (words < 0) {
							int word;
							while (++words <= 0) {
								PackedWrite<unsigned short>(m_pixels + dstOffset, PackedRead<unsigned short>(data));
								dstOffset += 2;
							}
							data += 2;
						}
						else {
							int bytes = 2 * words;
							memcpy(m_pixels + dstOffset, data, bytes);
							data += bytes;
							dstOffset += bytes;
						}
					}
				}
				dstOffset = lineOffset + m_width;
				++line;
			}
			break;
		}

		case 12: { // FLI_LC
			int line = PackedRead<unsigned short>(data);
			data += 2;
			int noLines = PackedRead<unsigned short>(data);
			data += 2;
			noLines += line;
			for (; line < noLines; ++line) {
				int noPackets = *data;
				int x = 0;
				++data;
				for (int packet = 0; packet < noPackets; ++packet) {
					x += *data++;
					int count = *(signed char*) data++;
					if (count < 0) {
						memset(m_pixels + m_width * line + x, *data++, -count);
						x -= count;
					}
					else {
						memcpy(m_pixels + m_width * line + x, data, count);
						x += count;
						data += count;
					}
				}
			}
			break;
		}

		case 13: // FLI_BLACK
			memset(m_pixels, 0, m_width * m_height);
			break;

		case 15: { // FLI_BRUN
			if (m_type == 0xaf11) {
				for (int y = 0; y < m_height; ++y) {
					int noPackets = *data;
					int x = 0;
					++data;
					for (int packet = 0; packet < noPackets; ++packet) {
						signed char count = *(signed char*) data;
						int literal;
						if (count < 0) {
							literal = x - count;
							while (x < literal) {
								m_pixels[y * m_width + x++] = *++data;
							}
						}
						else {
							++data;
							literal = (unsigned char) count + x;
							while (x < literal) {
								m_pixels[y * m_width + x++] = *data;
							}
						}
						++data;
					}
				}
			}
			else {
				int dstOffset = 0;
				for (int y = 0; y < m_height; ++y) {
					++data;
					int x = 0;
					while (x < m_width) {
						int count = *(signed char*) data++;
						if (count < 0) {
							memcpy(m_pixels + dstOffset, data, -count);
							dstOffset -= count;
							data -= count;
							x -= count;
						}
						else {
							memset(m_pixels + dstOffset, *data++, count);
							dstOffset += count;
							x += count;
						}
					}
				}
			}
			break;
		}

		case 16: // FLI_COPY
			memcpy(m_pixels, data, m_width * m_height);
			break;
		}
	}

	operator delete(frame);
	return 1;
}
