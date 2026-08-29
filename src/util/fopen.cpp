#include <stdio.h>

// FUNCTION: ALIEN 0x439d90
FILE* FOpen(char** p_name, const char* p_mode)
{
	if (**p_name)
		return fopen(*p_name, p_mode);
	return 0;
}
