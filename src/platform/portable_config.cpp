#include "platform/portable_config.h"

#include "platform/ini.h"
#include "platform/paths.h"

#include <string>

namespace
{

INI_FILE& Config()
{
	static INI_FILE ini;
	static bool loaded = false;
	if (!loaded) {
		loaded = true;
		const std::string root = Platform_PrefPath();
		ini.Load((root + "portable.ini").c_str());
		if (!ini.GetInt("meta", "SettingsImported", 0)) {
			INI_FILE legacy;
			if (legacy.Load((root + "settings.ini").c_str())) {
				ini.MergeMissing(legacy);
				ini.SetInt("meta", "SettingsImported", 1);
				ini.Save("portable.ini");
			}
		}
	}
	return ini;
}

} // namespace

const char* PortableConfig_GetString(const char* p_section, const char* p_key)
{
	return Config().Get(p_section, p_key);
}

const char* PortableConfig_GetRegistryString(const char* p_section, const char* p_key)
{
	return Config().GetRaw(p_section, p_key);
}

int PortableConfig_GetInt(const char* p_section, const char* p_key, int p_default)
{
	return Config().GetInt(p_section, p_key, p_default);
}

void PortableConfig_SetString(const char* p_section, const char* p_key, const char* p_value)
{
	Config().Set(p_section, p_key, p_value);
}

void PortableConfig_SetInt(const char* p_section, const char* p_key, int p_value)
{
	Config().SetInt(p_section, p_key, p_value);
}

void PortableConfig_Erase(const char* p_section, const char* p_key)
{
	Config().Erase(p_section, p_key);
}

bool PortableConfig_Flush()
{
	INI_FILE& ini = Config();
	return !ini.Dirty() || ini.Save("portable.ini");
}
