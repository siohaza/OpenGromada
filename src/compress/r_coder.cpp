#include "compress/r_coder.h"

// FUNCTION: ALIEN 0x425260
int R_CODER::start_encoding(char p_byte, int p_cache, int p_file)
{
	int result = m_file;
	if (!result) {
		m_file = p_file;
		m_low = 0;
		m_range = 0x80000000;
		m_byte = p_byte;
		m_help = 0;
		m_cache = p_cache;
		result = p_cache;
	}
	return result;
}

// FUNCTION: ALIEN 0x4252a0
unsigned int R_CODER::EncodeShift(unsigned int p_cumFreq, unsigned int p_freq, unsigned int p_shift)
{
	while (m_range <= 0x800000) {
		int low = m_low;
		if ((unsigned int) low < 0x7F800000) {
			fputc(m_byte, (FILE*) m_file);
			for (; m_help; m_help--)
				fputc(255, (FILE*) m_file);
			low = m_low;
			m_byte = (unsigned char) ((unsigned int) low >> 23);
		}
		else if (low & 0x80000000) {
			fputc(m_byte + 1, (FILE*) m_file);
			for (; m_help; m_help--)
				fputc(0, (FILE*) m_file);
			low = m_low;
			m_byte = (unsigned char) ((unsigned int) low >> 23);
		}
		else {
			++m_help;
		}
		m_range <<= 8;
		m_low = (low & 0x7FFFFF) << 8;
		++m_cache;
	}
	unsigned int range = m_range;
	unsigned int result = range >> p_shift;
	m_low += p_freq * result;
	if ((p_cumFreq + p_freq) >> p_shift) {
		m_range = range - p_freq * result;
	}
	else {
		result *= p_cumFreq;
		m_range = result;
	}
	return result;
}

// FUNCTION: ALIEN 0x4253b0
int R_CODER::EndEncoding()
{
	if (!m_file)
		return -1;
	while (m_range <= 0x800000) {
		int low = m_low;
		if ((unsigned int) low < 0x7F800000) {
			fputc(m_byte, (FILE*) m_file);
			for (; m_help; m_help--)
				fputc(255, (FILE*) m_file);
			low = m_low;
			m_byte = (unsigned char) ((unsigned int) low >> 23);
		}
		else if (low & 0x80000000) {
			fputc(m_byte + 1, (FILE*) m_file);
			for (; m_help; m_help--)
				fputc(0, (FILE*) m_file);
			low = m_low;
			m_byte = (unsigned char) ((unsigned int) low >> 23);
		}
		else {
			++m_help;
		}
		m_range <<= 8;
		m_low = (low & 0x7FFFFF) << 8;
		++m_cache;
	}
	m_cache += 5;
	unsigned int tmp;
	if ((m_low & 0x7FFFFF) < (((unsigned int) m_cache >> 1) & 0x7FFFFF))
		tmp = (unsigned int) m_low >> 23;
	else
		tmp = ((unsigned int) m_low >> 23) + 1;
	unsigned char out = (unsigned char) tmp;
	if (tmp > 0xFF) {
		fputc(m_byte + 1, (FILE*) m_file);
		for (; m_help; m_help--)
			fputc(0, (FILE*) m_file);
	}
	else {
		fputc(m_byte, (FILE*) m_file);
		for (; m_help; m_help--)
			fputc(255, (FILE*) m_file);
	}
	fputc(out, (FILE*) m_file);
	fputc((m_cache >> 16) & 0xFF, (FILE*) m_file);
	fputc((m_cache >> 8) & 0xFF, (FILE*) m_file);
	fputc(m_cache & 0xFF, (FILE*) m_file);
	return m_cache;
}

// FUNCTION: ALIEN 0x425570
int R_CODER::StartDecoding(FILE* p_stream)
{
	if (m_file)
		return 0;
	m_file = (int) p_stream;
	int v4 = fgetc(p_stream);
	if (v4 == -1)
		return v4;
	int v5 = fgetc((FILE*) m_file);
	m_byte = v5;
	m_low = (unsigned char) v5 >> 1;
	m_range = 128;
	return v4 ? -1 : 0;
}

// FUNCTION: ALIEN 0x4255d0
unsigned int R_CODER::DecodeCulShift(unsigned int p_shift)
{
	while (m_range <= 0x800000) {
		m_low = (m_low << 8) | ((m_byte << 7) & 0xff);
		m_byte = fgetc((FILE*) m_file);
		m_low |= m_byte >> 1;
		m_range <<= 8;
	}
	m_help = m_range >> p_shift;
	unsigned int tmp = (unsigned int) m_low / (unsigned int) m_help;
	return (tmp >> p_shift) ? (1 << p_shift) - 1 : tmp;
}

// FUNCTION: ALIEN 0x425650
int R_CODER::DecodeUpdate(int p_cumFreq, int p_freq, unsigned int p_totFreq)
{
	int scale = m_help;
	int result = p_freq * scale;
	m_low -= result;
	if (p_cumFreq + p_freq < p_totFreq)
		m_range = p_cumFreq * scale;
	else
		m_range -= result;
	return result;
}
