#include "platform/save_file.h"
#include "platform/paths.h"
#include "util/myerror.h"

#include <SDL3/SDL.h>
#include <atomic>
#include <cstdio>
#include <filesystem>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

namespace fs = std::filesystem;
namespace {
std::atomic<unsigned> g_serial{0};

std::string UniqueSuffix()
{
	return ".opengromada-" + std::to_string(SDL_GetTicksNS()) + "-" + std::to_string(++g_serial);
}

bool SavePath(const std::string& name, fs::path& path)
{
	std::string normalized = name;
	for (char& c : normalized) if (c == '\\') c = '/';
	if (Platform_IsAbsolutePath(normalized.c_str())) return false;
	fs::path relative = fs::u8path(normalized);
	if (relative.empty() || relative.is_absolute() || relative.has_root_name()) return false;
	for (const auto& part : relative) if (part == "..") return false;
	path = fs::u8path(Platform_PrefPath()) / relative;
	return true;
}

bool AtomicReplace(const fs::path& temp, const fs::path& target)
{
#ifdef _WIN32
	return MoveFileExW(temp.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
	return std::rename(temp.c_str(), target.c_str()) == 0;
#endif
}

std::string Utf8Path(const fs::path& path)
{
	const std::u8string utf8 = path.u8string();
	return std::string(reinterpret_cast<const char*>(utf8.data()), utf8.size());
}

FILE* OpenStageFile(const fs::path& path, bool create)
{
#ifdef _WIN32
	return _wfopen(path.c_str(), create ? L"wbx" : L"r+b");
#else
	return std::fopen(path.c_str(), create ? "wbx" : "r+b");
#endif
}

bool SyncStageFile(const fs::path& path)
{
	FILE* file = OpenStageFile(path, false);
	if (!file) return false;
#ifdef _WIN32
	bool ok = _commit(_fileno(file)) == 0;
#else
	bool ok = fsync(fileno(file)) == 0;
#endif
	if (std::fclose(file) != 0) ok = false;
	return ok;
}

bool StagedPath(const std::string& name, fs::path& target, fs::path& parent)
{


	if (name.find('\0') != std::string::npos || name.find(':') != std::string::npos || !SavePath(name, target) ||
		target.filename().empty() || target.filename() == "." || target.filename() == "..") return false;
	std::error_code error;
	target = fs::absolute(target, error);
	if (error) return false;
	const fs::path absoluteRoot = fs::absolute(fs::u8path(Platform_PrefPath()), error);
	if (error) return false;
	const fs::path root = fs::weakly_canonical(absoluteRoot, error);
	if (error) return false;
	parent = fs::weakly_canonical(target.parent_path(), error);
	if (error) return false;
	auto child = parent.begin();
	for (auto base = root.begin(); base != root.end(); ++base, ++child) {
		if (child == parent.end() || *child != *base) return false;
	}
	const fs::file_status status = fs::symlink_status(target, error);
	if (error && error != std::errc::no_such_file_or_directory) return false;
	if (!fs::exists(status)) return true;

	const fs::perms writes = fs::perms::owner_write | fs::perms::group_write | fs::perms::others_write;
	return fs::is_regular_file(status) && (status.permissions() & writes) != fs::perms::none;
}
}

bool Platform_WriteSaveAtomic(const std::string& name, const std::string& bytes)
{
	fs::path target;
	if (!SavePath(name, target)) return false;
	std::error_code error;
	fs::create_directories(target.parent_path(), error);
	if (error) return false;
	const fs::path temp = target.string() + UniqueSuffix() + ".tmp";
	FILE* file = Platform_FOpen(temp.string().c_str(), "wb");
	if (!file) return false;
	bool ok = fwrite(bytes.data(), 1, bytes.size(), file) == bytes.size() && fflush(file) == 0;
#ifndef _WIN32
	if (ok) ok = fsync(fileno(file)) == 0;
#endif
	if (fclose(file) != 0) ok = false;
	if (ok && fs::exists(target, error)) {

		FILE* old = Platform_FOpen(target.string().c_str(), "rb");
		if (old) {
			std::string previous(bytes.size() + 1, '\0');
			const size_t read = fread(previous.data(), 1, previous.size(), old);
			const bool same = !ferror(old) && read == bytes.size() &&
				previous.compare(0, read, bytes) == 0;
			fclose(old);
			if (same) {
				fs::remove(temp, error);
				return true;
			}
		}
		fs::copy_file(target, target.string() + UniqueSuffix() + ".bak", error);
		ok = !error;
	}
	if (ok) {
		ok = AtomicReplace(temp, target);
	}
	if (!ok) {
		MYERROR::Log(::Error, "Save replacement failed for %s; original preserved", name.c_str());
		fs::remove(temp, error);
	}
	Platform_InvalidatePathCache();
	return ok;
}

bool Platform_ArchiveSave(const std::string& name)
{
	fs::path target;
	if (!SavePath(name, target)) return false;
	std::error_code error;
	if (!fs::exists(target, error)) return !error;
	fs::rename(target, target.string() + UniqueSuffix() + ".deleted", error);
	Platform_InvalidatePathCache();
	return !error;
}

Platform_StagedSave::~Platform_StagedSave()
{
	if (!m_temp.empty()) {
		std::error_code error;
		fs::remove(fs::u8path(m_temp), error);
		Platform_InvalidatePathCache();
	}
}

bool Platform_StagedSave::Begin(const std::string& name)
{
	if (!m_temp.empty()) return false;
	fs::path target, parent;
	if (!StagedPath(name, target, parent)) return false;
	std::error_code error;
	fs::create_directories(target.parent_path(), error);
	if (error) return false;


	fs::path checkedTarget, checkedParent;
	if (!StagedPath(name, checkedTarget, checkedParent) || target != checkedTarget || parent != checkedParent)
		return false;
	for (int attempt = 0; attempt < 8; ++attempt) {
		fs::path temp = target;
		temp += UniqueSuffix() + ".tmp";
		FILE* file = OpenStageFile(temp, true);
		if (!file) continue;
		if (std::fclose(file) != 0) {
			fs::remove(temp, error);
			return false;
		}
		m_relative = name;
		m_target = Utf8Path(target);
		m_parent = Utf8Path(parent);
		m_temp = Utf8Path(temp);
		Platform_InvalidatePathCache();
		return true;
	}
	return false;
}

bool Platform_StagedSave::Commit()
{
	if (m_temp.empty()) return false;
	fs::path target, parent;
	bool ok = StagedPath(m_relative, target, parent) && target == fs::u8path(m_target) && parent == fs::u8path(m_parent);
	std::error_code error;
	const fs::path temp = fs::u8path(m_temp);
	if (ok) {
		const fs::file_status status = fs::symlink_status(temp, error);
		ok = !error && fs::is_regular_file(status);
	}


	if (ok) ok = SyncStageFile(temp);
	if (ok) {
		const bool exists = fs::exists(target, error);
		ok = !error;
		if (ok && exists) {
			fs::path backup = target;
			backup += UniqueSuffix() + ".bak";
			fs::copy_file(target, backup, error);
			ok = !error;
			if (!ok) {


				std::error_code cleanup;
				fs::remove(backup, cleanup);
			}
			else {
				ok = SyncStageFile(backup);
			}
		}
	}
	if (ok) ok = AtomicReplace(temp, target);
	if (ok) {
		m_temp.clear();
	}
	else {
		MYERROR::Log(::Error, "Staged save replacement failed for %s; original preserved", m_relative.c_str());
	}
	Platform_InvalidatePathCache();
	return ok;
}
