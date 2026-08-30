#ifndef TEXT_ENCODING_H
#define TEXT_ENCODING_H

#include <ctype.h>
#include <stddef.h>
#include <string.h>

namespace TEXT_ENCODING
{

struct UNIT {
	unsigned char m_glyph;
	size_t m_bytes;
};

inline unsigned char UnicodeToCp1251(unsigned int p_codepoint)
{
	static const unsigned short special[64] = {
		0x0402, 0x0403, 0x201a, 0x0453, 0x201e, 0x2026, 0x2020, 0x2021, 0x20ac, 0x2030, 0x0409, 0x2039, 0x040a,
		0x040c, 0x040b, 0x040f, 0x0452, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, 0xfffd, 0x2122,
		0x0459, 0x203a, 0x045a, 0x045c, 0x045b, 0x045f, 0x00a0, 0x040e, 0x045e, 0x0408, 0x00a4, 0x0490, 0x00a6,
		0x00a7, 0x0401, 0x00a9, 0x0404, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x0407, 0x00b0, 0x00b1, 0x0406, 0x0456,
		0x0491, 0x00b5, 0x00b6, 0x00b7, 0x0451, 0x2116, 0x0454, 0x00bb, 0x0458, 0x0405, 0x0455, 0x0457
	};

	if (p_codepoint < 0x80) {
		return (unsigned char) p_codepoint;
	}
	if (p_codepoint >= 0x0410 && p_codepoint <= 0x044f) {
		return (unsigned char) (0xc0 + p_codepoint - 0x0410);
	}
	for (unsigned int i = 0; i < 64; ++i) {
		if (special[i] == p_codepoint) {
			return (unsigned char) (0x80 + i);
		}
	}
	return (unsigned char) '?';
}

inline bool IsContinuation(unsigned char p_byte)
{
	return (p_byte & 0xc0) == 0x80;
}

inline UNIT DecodeUnit(const char* p_text, size_t p_size, size_t p_offset)
{
	UNIT result = {0, 0};
	if (!p_text || p_offset >= p_size) {
		return result;
	}

	const unsigned char* text = (const unsigned char*) p_text;
	unsigned char first = text[p_offset];
	unsigned int codepoint = first;
	size_t bytes = 1;
	bool valid = first < 0x80;

	if (first >= 0xc2 && first <= 0xdf && p_offset + 1 < p_size && IsContinuation(text[p_offset + 1])) {
		codepoint = ((unsigned int) (first & 0x1f) << 6) | (text[p_offset + 1] & 0x3f);
		bytes = 2;
		valid = true;
	}
	else if (
		first >= 0xe0 && first <= 0xef && p_offset + 2 < p_size && IsContinuation(text[p_offset + 1]) &&
		IsContinuation(text[p_offset + 2])
	) {
		unsigned char second = text[p_offset + 1];
		if ((first != 0xe0 || second >= 0xa0) && (first != 0xed || second < 0xa0)) {
			codepoint = ((unsigned int) (first & 0x0f) << 12) | ((unsigned int) (second & 0x3f) << 6) |
						(text[p_offset + 2] & 0x3f);
			bytes = 3;
			valid = true;
		}
	}
	else if (
		first >= 0xf0 && first <= 0xf4 && p_offset + 3 < p_size && IsContinuation(text[p_offset + 1]) &&
		IsContinuation(text[p_offset + 2]) && IsContinuation(text[p_offset + 3])
	) {
		unsigned char second = text[p_offset + 1];
		if ((first != 0xf0 || second >= 0x90) && (first != 0xf4 || second < 0x90)) {
			codepoint = ((unsigned int) (first & 7) << 18) | ((unsigned int) (second & 0x3f) << 12) |
						((unsigned int) (text[p_offset + 2] & 0x3f) << 6) | (text[p_offset + 3] & 0x3f);
			bytes = 4;
			valid = true;
		}
	}

	result.m_glyph = valid ? UnicodeToCp1251(codepoint) : first;
	result.m_bytes = bytes;
	return result;
}

inline int LegacyLength(const char* p_text)
{
	if (!p_text) {
		return 0;
	}
	size_t size = strlen(p_text);
	size_t offset = 0;
	int count = 0;
	while (offset < size) {
		UNIT unit = DecodeUnit(p_text, size, offset);
		offset += unit.m_bytes ? unit.m_bytes : 1;
		++count;
	}
	return count;
}

inline bool LegacyGlyphAt(const char* p_text, int p_index, unsigned char* p_glyph)
{
	if (!p_text || p_index < 0 || !p_glyph) {
		return false;
	}
	size_t size = strlen(p_text);
	size_t offset = 0;
	for (int index = 0; offset < size; ++index) {
		UNIT unit = DecodeUnit(p_text, size, offset);
		if (index == p_index) {
			*p_glyph = unit.m_glyph;
			return true;
		}
		offset += unit.m_bytes ? unit.m_bytes : 1;
	}
	return false;
}

struct METRICS {
	int m_units;
	int m_columns;
	int m_rows;
};

inline METRICS Measure(const char* p_text)
{
	METRICS result = {LegacyLength(p_text), 0, 1};
	if (!p_text) {
		return result;
	}

	size_t size = strlen(p_text);
	size_t offset = 0;
	int columns = 0;
	while (offset < size) {
		UNIT unit = DecodeUnit(p_text, size, offset);
		if (unit.m_glyph == '<' && size - offset >= 6 && !strncmp(p_text + offset, "<Font=", 6)) {
			const char* close = strchr(p_text + offset + 6, '>');
			if (close) {
				offset = (size_t) (close - p_text) + 1;
				continue;
			}
		}
		if (unit.m_glyph == 27) {
			offset += unit.m_bytes;
			if (offset < size) {
				UNIT font = DecodeUnit(p_text, size, offset);
				offset += font.m_bytes ? font.m_bytes : 1;
			}
			continue;
		}
		if (unit.m_glyph == '\n') {
			if (columns > result.m_columns) {
				result.m_columns = columns;
			}
			columns = 0;
			++result.m_rows;
		}
		else if (unit.m_glyph == '\r') {
			if (columns > result.m_columns) {
				result.m_columns = columns;
			}
			columns = 0;
		}
		else if (unit.m_glyph == '\t') {
			columns += 8;
		}
		else if (unit.m_glyph >= 0x20) {
			++columns;
		}
		offset += unit.m_bytes ? unit.m_bytes : 1;
	}
	if (columns > result.m_columns) {
		result.m_columns = columns;
	}
	return result;
}

inline bool IsLegacySpaceAt(const char* p_text, int p_index)
{
	unsigned char glyph;
	return LegacyGlyphAt(p_text, p_index, &glyph) && isspace((int) glyph) != 0;
}

} // namespace TEXT_ENCODING

#endif
