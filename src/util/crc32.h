#ifndef CRC32_H
#define CRC32_H

#include "util/decomp.h"

class CRC32 {
public:
	unsigned int m_crc; // 0x00

	static unsigned int crc_table[256];

	CRC32(unsigned char* p_data, int p_len);
	int Add(unsigned char* p_data, int p_len);
};

#endif
