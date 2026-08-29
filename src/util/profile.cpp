#include "util/profile.h"

#include <string.h>
#include <windows.h>

// FUNCTION: ALIEN 0x407660
int PROFILE::Load(const STRING& p_name)
{
	m_name = empty_str;
	m_name = p_name;
	return 0;
}

// FUNCTION: ALIEN 0x407680
unsigned int PROFILE::GetInt(const STRING& p_app, const STRING& p_key, int p_default)
{
	const char* file = m_name.m_str;
	const char* key = p_key.m_str;
	const char* app = p_app.m_str;
	return GetPrivateProfileIntA(app, key, p_default, file);
}

// GLOBAL: ALIEN 0x490734
PROFILE* Strings;

// FUNCTION: ALIEN 0x4076a0
STRING PROFILE::GetString(const STRING& p_app, const STRING& p_key, const STRING& p_default)
{
	char buffer[32768];
	GetPrivateProfileStringA(p_app.m_str, p_key.m_str, p_default.m_str, buffer, 0x7fff,
		m_name.m_str);
	if (buffer && buffer[0])
		return STRING(buffer, STRING::COPY);
	return STRING();
}
