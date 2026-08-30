#ifndef R_CODER_H
#define R_CODER_H

#include "util/decomp.h"

#include <stdio.h>

class R_CODER {
public:
	int m_low;              // 0x00
	unsigned int m_range;   // 0x04
	int m_help;             // 0x08
	unsigned char m_byte;   // 0x0c
	undefined m_unk0x0d[3]; // 0x0d
	int m_cache;            // 0x10
	FILE* m_file;           // 0x14

	int start_encoding(char p_byte, int p_cache, FILE* p_file);
	unsigned int EncodeShift(unsigned int p_cumFreq, unsigned int p_freq, unsigned int p_shift);
	int EndEncoding();
	int StartDecoding(FILE* p_stream);
	unsigned int DecodeCulShift(unsigned int p_shift);
	int DecodeUpdate(int p_cumFreq, int p_freq, unsigned int p_totFreq);
};

#endif
