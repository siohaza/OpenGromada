#include "util/registry.h"

#include "platform/portable_config.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <string>

// GLOBAL: ALIEN 0x492b68
REGISTRY* Registry;

namespace
{

std::string SectionOf(const STRING& p_path)
{
	static const char* const prefixes[] = {
		"HKEY_USERS\\",
		"HKEY_CURRENT_USER\\",
		"HKEY_CLASSES_ROOT\\",
		"HKEY_CURRENT_CONFIG\\",
		"HKEY_LOCAL_MACHINE\\",
	};

	const char* path = p_path.m_str;
	for (const char* prefix : prefixes) {
		size_t len = strlen(prefix);
		if (strncmp(path, prefix, len) == 0) {
			path += len;
			break;
		}
	}

	std::string section(path);
	for (char& c : section) {
		if (c == '\\') {
			c = '/';
		}
	}
	return section;
}

bool StartsWithI(const std::string& p_text, const char* p_prefix)
{
	size_t length = strlen(p_prefix);
	if (p_text.size() < length) {
		return false;
	}
	for (size_t i = 0; i < length; ++i) {
		if (tolower((unsigned char) p_text[i]) != tolower((unsigned char) p_prefix[i])) {
			return false;
		}
	}
	return true;
}

std::string LegacySection(const std::string& p_section, int p_kind)
{
	static const char* const canonical = "SOFTWARE/Gromada/AlienShooter";
	size_t length = strlen(canonical);
	if (!StartsWithI(p_section, canonical) || (p_section.size() != length && p_section[length] != '/')) {
		return std::string();
	}

	const std::string suffix = p_section.substr(length);
	if (p_kind == 0) {
		return std::string("SOFTWARE/Gromada/AlienShooter-portable") + suffix;
	}
	return std::string("SOFTWARE/Gromada/") + suffix;
}

const char* GetWithLegacyFallback(const std::string& p_section, const char* p_name)
{
	if (const char* value = PortableConfig_GetString(p_section.c_str(), p_name)) {
		return value;
	}
	for (int kind = 0; kind < 2; ++kind) {
		const std::string legacy = LegacySection(p_section, kind);
		if (!legacy.empty()) {
			if (const char* value = PortableConfig_GetString(legacy.c_str(), p_name)) {
				return value;
			}
		}
	}
	return nullptr;
}

} // namespace

int Registry_GetIntExact(const STRING& p_path, const STRING& p_name, int p_default)
{
	return PortableConfig_GetInt(SectionOf(p_path).c_str(), p_name.m_str, p_default);
}

char** REGISTRY::Path(char** p_subkey, void** p_hkey) const
{
	if (p_hkey) {
		*p_hkey = 0;
	}
	if (p_subkey) {
		std::string section = SectionOf(m_path);
		*p_subkey = (char*) operator new(section.size() + 1);
		memcpy(*p_subkey, section.c_str(), section.size() + 1);
	}
	return p_subkey;
}

// FUNCTION: ALIEN 0x42cf70
STRING REGISTRY::GetString(const STRING& p_name, const STRING& p_default)
{
	const char* value = GetWithLegacyFallback(SectionOf(m_path), p_name.m_str);
	return value ? STRING(value) : p_default;
}

// FUNCTION: ALIEN 0x42d130
int REGISTRY::GetInt(const STRING& p_name, int p_default)
{
	const char* value = GetWithLegacyFallback(SectionOf(m_path), p_name.m_str);
	return value ? atoi(value) : p_default;
}

// FUNCTION: ALIEN 0x42d1f0
void REGISTRY::SetString(const STRING& p_name, const STRING& p_value)
{
	PortableConfig_SetString(SectionOf(m_path).c_str(), p_name.m_str, p_value.m_str);
	PortableConfig_Flush();
}

// FUNCTION: ALIEN 0x42d280
void REGISTRY::SetInt(const STRING& p_name, int p_data)
{
	PortableConfig_SetInt(SectionOf(m_path).c_str(), p_name.m_str, p_data);
	PortableConfig_Flush();
}

// FUNCTION: ALIEN 0x42d310
void REGISTRY::Delete(const STRING& p_name)
{
	const std::string section = SectionOf(m_path);
	PortableConfig_Erase(section.c_str(), p_name.m_str);
	for (int kind = 0; kind < 2; ++kind) {
		const std::string legacy = LegacySection(section, kind);
		if (!legacy.empty()) {
			PortableConfig_Erase(legacy.c_str(), p_name.m_str);
		}
	}
	PortableConfig_Flush();
}
