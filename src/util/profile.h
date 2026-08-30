#ifndef PROFILE_H
#define PROFILE_H

#include "util/string.h"

class PROFILE {
public:
	STRING m_name; // 0x00

	int Load(const STRING& p_name);
	unsigned int GetInt(const STRING& p_app, const STRING& p_key, int p_default);
	STRING GetString(const STRING& p_app, const STRING& p_key, const STRING& p_default);
};

extern PROFILE* Strings;

#endif
