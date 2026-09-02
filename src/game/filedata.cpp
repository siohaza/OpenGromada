#include "game/filedata.h"

#include "platform/ini.h"
#include "platform/paths.h"
#include "platform/portable_config.h"
#include "util/myerror.h"

#include <string>

namespace
{

INI_FILE& FileFor(const char* p_relative)
{
	static INI_FILE saveIni;
	static INI_FILE optionsIni;
	static bool loaded = false;
	if (!loaded) {
		loaded = true;
		const std::string root = std::string(Platform_PrefPath()) + "saves/";
		const char* names[] = {"save.ini", "options.ini"};
		INI_FILE* files[] = {&saveIni, &optionsIni};
		for (int i = 0; i < 2; ++i) {
			const std::string prefPath = root + names[i];
			if (!files[i]->Load(prefPath.c_str())) {
				INI_FILE shipped;
				if (shipped.Load((std::string("saves/") + names[i]).c_str())) {
					files[i]->MergeMissing(shipped);
					if (files[i]->Dirty()) {
						files[i]->Save();
					}
				}
			}
		}
		const int fullscreen = PortableConfig_GetInt("display", "FullScreen", -1);
		if (fullscreen != -1 && optionsIni.GetInt("graph", "FullScreen", -1) != fullscreen) {
			optionsIni.SetInt("graph", "FullScreen", fullscreen);
			optionsIni.Save();
		}
	}
	return p_relative[0] == 'o' ? optionsIni : saveIni;
}

INI_FILE* ResolveDataPath(const char* p_path, std::string* p_section, std::string* p_key)
{
	std::string path = p_path ? p_path : "";
	for (char& c : path) {
		if (c == '\\') {
			c = '/';
		}
	}

	const char* file = "save.ini";
	if (path.rfind("options://", 0) == 0) {
		file = "options.ini";
		path.erase(0, 10);
	}
	else if (path.rfind("save://", 0) == 0) {
		path.erase(0, 7);
	}
	else if (::Error) {
		MYERROR::Log(::Error, "!!!ERROR!!!FILEDATA: Unknown scheme in '%s'", p_path ? p_path : "");
	}

	const size_t slash = path.find('/');
	if (slash == std::string::npos) {
		*p_section = "";
		*p_key = path;
	}
	else {
		*p_section = path.substr(0, slash);
		*p_key = path.substr(slash + 1);
	}
	return &FileFor(file);
}

} // namespace

void FileData_Save(const char* p_path, const char* p_value)
{
	std::string section;
	std::string key;
	INI_FILE* file = ResolveDataPath(p_path, &section, &key);
	file->Set(section.c_str(), key.c_str(), p_value ? p_value : "");
	file->Save();
}

const char* FileData_Load(const char* p_path, const char* p_default)
{
	std::string section;
	std::string key;
	INI_FILE* file = ResolveDataPath(p_path, &section, &key);
	const char* value = file->Get(section.c_str(), key.c_str());
	return value ? value : p_default;
}

int FileData_FileExists(const char* p_filename)
{
	FILE* file = Platform_FOpen(p_filename, "rb");
	if (!file) {
		return 0;
	}
	fclose(file);
	return 1;
}

const char* FileData_SaveFolder()
{
	return "saves";
}
