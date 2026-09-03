#include "platform/paths.h"

#include <SDL3/SDL.h>
#include <ctype.h>
#include <map>
#include <string.h>
#include <string>
#include <vector>

#if defined(__linux__)
#include <unistd.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

namespace
{

std::string g_basePath;
std::string g_prefPath;
std::string g_resolved;

std::map<std::string, std::map<std::string, std::string>> g_dirCache;

std::string Lower(const std::string& p_text)
{
	std::string out(p_text);
	for (char& c : out) {
		c = (char) tolower((unsigned char) c);
	}
	return out;
}

bool Exists(const std::string& p_path)
{
	SDL_PathInfo info;
	return SDL_GetPathInfo(p_path.c_str(), &info);
}

const std::string* MatchInDirectory(const std::string& p_dir, const std::string& p_name)
{
	auto cached = g_dirCache.find(p_dir);
	if (cached == g_dirCache.end()) {
		std::map<std::string, std::string> entries;

		int count = 0;
		char** names = SDL_GlobDirectory(p_dir.empty() ? "." : p_dir.c_str(), nullptr, 0, &count);
		if (names) {
			for (int i = 0; i < count; ++i) {
				entries.emplace(Lower(names[i]), names[i]);
			}
			SDL_free(names);
		}
		cached = g_dirCache.emplace(p_dir, std::move(entries)).first;
	}

	auto hit = cached->second.find(Lower(p_name));
	return hit == cached->second.end() ? nullptr : &hit->second;
}

bool IsWriteMode(const char* p_mode)
{
	return p_mode && (*p_mode == 'w' || *p_mode == 'a');
}

void SetDirectoryPath(std::string& p_out, const char* p_path)
{
	p_out = p_path && *p_path ? p_path : "./";
	for (char& c : p_out) {
		if (c == '\\') {
			c = '/';
		}
	}
	if (p_out.back() != '/') {
		p_out += '/';
	}
}

void SetOverrideDirectoryPath(std::string& p_out, const char* p_path)
{
	if (Platform_IsAbsolutePath(p_path)) {
		SetDirectoryPath(p_out, p_path);
		return;
	}

	char* current = SDL_GetCurrentDirectory();
	std::string absolute = current && *current ? current : "./";
	SDL_free(current);
	if (absolute.back() != '/' && absolute.back() != '\\') {
		absolute += '/';
	}
	absolute += p_path ? p_path : "";
	SetDirectoryPath(p_out, absolute.c_str());
}

std::string JoinUnder(const char* p_root, const char* p_path)
{
	std::string path;
	if (!Platform_IsAbsolutePath(p_path)) {
		path = p_root;
	}
	path += p_path;
	for (char& c : path) {
		if (c == '\\') {
			c = '/';
		}
	}
	return path;
}

FILE* OpenUnder(const char* p_root, const char* p_path, const char* p_mode)
{
	const std::string path = JoinUnder(p_root, p_path);
	if (FILE* file = fopen(path.c_str(), p_mode)) {
		return file;
	}
	return fopen(Platform_ResolvePath(path.c_str()), p_mode);
}

void CreateParentDirectory(const std::string& p_path)
{
	size_t slash = p_path.find_last_of('/');
	if (slash == std::string::npos || slash == 0) {
		return;
	}
	SDL_CreateDirectory(p_path.substr(0, slash).c_str());
}

} // namespace

const char* Platform_BasePath()
{
	if (g_basePath.empty()) {
		const char* overridePath = SDL_getenv("ALIEN_SHOOTER_DATA_PATH");
		if (overridePath && *overridePath) {
			SetOverrideDirectoryPath(g_basePath, overridePath);
		}
		else {
			SetDirectoryPath(g_basePath, SDL_GetBasePath());
		}
	}
	return g_basePath.c_str();
}

static std::string g_prefApp = "AlienShooter";

void Platform_SetPrefApp(const char* p_app)
{
	if (p_app && *p_app) {
		g_prefApp = p_app;
	}
}

const char* Platform_PrefPath()
{
	if (g_prefPath.empty()) {
		const char* overridePath = SDL_getenv("ALIEN_SHOOTER_PREF_PATH");
		if (overridePath && *overridePath) {
			SetOverrideDirectoryPath(g_prefPath, overridePath);
		}
		else {
			char* pref = SDL_GetPrefPath("SigmaTeam", g_prefApp.c_str());
			if (pref) {
				SetDirectoryPath(g_prefPath, pref);
				SDL_free(pref);
			}
			else {
				SetDirectoryPath(g_prefPath, Platform_BasePath());
			}
		}
	}
	return g_prefPath.c_str();
}

const char* Platform_ExecutablePath()
{
	static std::string path;
	if (!path.empty()) {
		return path.c_str();
	}

#if defined(__linux__)
	char buffer[4096];
	ssize_t n = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
	if (n > 0) {
		buffer[n] = 0;
		path = buffer;
	}
#elif defined(__APPLE__)
	char buffer[4096];
	uint32_t size = sizeof(buffer);
	if (_NSGetExecutablePath(buffer, &size) == 0) {
		path = buffer;
	}
#elif defined(_WIN32)
	char buffer[4096];
	DWORD n = GetModuleFileNameA(nullptr, buffer, sizeof(buffer));
	if (n > 0 && n < sizeof(buffer)) {
		path = buffer;
	}
#endif

	if (path.empty()) {
		path = Platform_BasePath();
	}
	return path.c_str();
}

bool Platform_IsAbsolutePath(const char* p_path)
{
	return p_path && (p_path[0] == '/' || p_path[0] == '\\' ||
					  (p_path[0] && p_path[1] == ':' && (p_path[2] == '/' || p_path[2] == '\\')));
}

const char* Platform_ResolvePath(const char* p_path)
{
	if (!p_path || !*p_path) {
		return p_path;
	}

	std::string path(p_path);
	for (char& c : path) {
		if (c == '\\') {
			c = '/';
		}
	}

	if (Exists(path)) {
		g_resolved = path;
		return g_resolved.c_str();
	}

	const bool absolute = path[0] == '/';
	std::string built = absolute ? "/" : "";

	size_t start = absolute ? 1 : 0;
	bool ok = true;

	while (start <= path.size()) {
		size_t slash = path.find('/', start);
		const bool last = slash == std::string::npos;
		std::string component = path.substr(start, last ? std::string::npos : slash - start);

		if (!component.empty() && component != ".") {
			std::string candidate = built + component;
			if (Exists(candidate)) {
				built = candidate;
			}
			else {
				std::string dir = built.empty() ? std::string(".") : built;
				if (dir.size() > 1 && dir.back() == '/') {
					dir.pop_back();
				}

				const std::string* match = MatchInDirectory(dir, component);
				if (!match) {
					ok = false;
					break;
				}
				built += *match;
			}
		}

		if (last) {
			break;
		}
		built += '/';
		start = slash + 1;
	}

	g_resolved = ok ? built : path;
	return g_resolved.c_str();
}

FILE* Platform_FOpen(const char* p_path, const char* p_mode)
{
	if (!p_path || !*p_path) {
		return nullptr;
	}

	if (IsWriteMode(p_mode)) {
		const std::string path = JoinUnder(Platform_PrefPath(), p_path);
		CreateParentDirectory(path);
		Platform_InvalidatePathCache();
		return fopen(path.c_str(), p_mode);
	}

	if (FILE* file = OpenUnder(Platform_BasePath(), p_path, p_mode)) {
		return file;
	}
	return OpenUnder(Platform_PrefPath(), p_path, p_mode);
}

int Platform_Rename(const char* p_from, const char* p_to)
{
	if (!p_from || !p_to) {
		return -1;
	}
	const std::string from = JoinUnder(Platform_PrefPath(), p_from);
	const std::string to = JoinUnder(Platform_PrefPath(), p_to);
	CreateParentDirectory(to);
	Platform_InvalidatePathCache();
	return rename(from.c_str(), to.c_str());
}

int Platform_Remove(const char* p_path)
{
	if (!p_path) {
		return -1;
	}
	const std::string path = JoinUnder(Platform_PrefPath(), p_path);
	Platform_InvalidatePathCache();
	return remove(path.c_str());
}

void Platform_InvalidatePathCache()
{
	g_dirCache.clear();
}
