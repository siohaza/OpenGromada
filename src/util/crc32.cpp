#include "util/crc32.h"

// FUNCTION: ALIEN 0x439db0
CRC32::CRC32(unsigned char* p_data, int p_len)
{
	m_crc = 0;
	Add(p_data, p_len);
}
