#include "util/fstream.h"

// FUNCTION: ALIEN 0x408390
int FSTREAM::Read(void* p_dest, int p_size)
{
	return p_size - fread(p_dest, 1, p_size, m_file);
}

// FUNCTION: ALIEN 0x4083c0
int FSTREAM::Write(const void* p_src, int p_size)
{
	return p_size - fwrite(p_src, 1, p_size, m_file);
}
