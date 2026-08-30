#ifndef PLATFORM_PORTABLE_CONFIG_H
#define PLATFORM_PORTABLE_CONFIG_H

enum {
	PORTABLE_CONFIG_VERSION = 1
};

const char* PortableConfig_GetString(const char* p_section, const char* p_key);
int PortableConfig_GetInt(const char* p_section, const char* p_key, int p_default);
void PortableConfig_SetString(const char* p_section, const char* p_key, const char* p_value);
void PortableConfig_SetInt(const char* p_section, const char* p_key, int p_value);
void PortableConfig_Erase(const char* p_section, const char* p_key);
bool PortableConfig_Flush();

#endif
