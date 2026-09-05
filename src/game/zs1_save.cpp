#include "game/zs1_save.h"

#include "platform/paths.h"
#include "platform/save_file.h"
#include "util/myerror.h"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <set>







namespace {
std::string UserPath(int slot, int level = -1)
{
	char name[80];
	if (level == -1) std::snprintf(name, sizeof(name), "userdata/user%03d.dat", slot + 1);
	else std::snprintf(name, sizeof(name), "userdata/user%03d_%02d.dat", slot + 1, level);
	return name;
}

bool HasMarker(int slot, const char* suffix)
{
	const std::string path = std::string(Platform_PrefPath()) + UserPath(slot) + suffix;
	FILE* file = Platform_FOpen(path.c_str(), "rb");
	if (!file) return false;
	fclose(file);
	return true;
}

bool HasTombstone(int slot) { return HasMarker(slot, ".opengromada-deleted"); }
bool SuppressBaseLevels(int slot) { return HasMarker(slot, ".opengromada-ignore-base-levels"); }


FILE* OpenNative(const std::string& name, bool allowBase)
{
	const std::string pref = std::string(Platform_PrefPath()) + name;
	if (FILE* file = Platform_FOpen(pref.c_str(), "rb")) return file;
	if (!allowBase) return nullptr;
	return Platform_FOpen((std::string(Platform_BasePath()) + name).c_str(), "rb");
}

int ReadLines(const std::string& name, std::vector<std::string>& lines, bool allowBase = true)
{
	FILE* file = OpenNative(name, allowBase);
	if (!file) return 0;
	std::string line;
	bool valid = true;
	size_t bytes = 0;
	for (int c; (c = fgetc(file)) != EOF;) {
		if (++bytes > 4 * 1024 * 1024 || c == 0) { valid = false; break; }
		if (c == '\n') {
			if (!line.empty() && line.back() == '\r') line.pop_back();
			lines.push_back(line);
			line.clear();
		}
		else {
			line.push_back(char(c));
			if (line.size() > 998) { valid = false; break; }
		}
	}
	if (ferror(file) || !line.empty()) valid = false;
	fclose(file);
	if (!valid) {
		MYERROR::Log(::Error, "ZS1 native save %s byte %zu: truncated/invalid line; file left untouched",
			name.c_str(), bytes);
		return -1;
	}
	return 1;
}

bool ParseInt(const std::string& value, int& out)
{
	const char* begin = value.data();
	const char* end = begin + value.size();
	const auto result = std::from_chars(begin, end, out);
	return result.ec == std::errc() && result.ptr == end;
}

bool DecodeBank(const std::vector<std::string>& lines, size_t first, std::vector<ZS1_ARG>& bank)
{
	std::vector<ZS1_ARG> decoded;
	for (size_t i = first; i < lines.size(); ++i) {
		const std::string& line = lines[i];
		const size_t split = line.find('=');
		if (split == std::string::npos) continue;
		if (split < 2 || (line[0] != 'i' && line[0] != 's')) return false;
		ZS1_ARG arg;
		arg.m_key = line.substr(1, split - 1);
		arg.m_isString = line[0] == 's';
		if (arg.m_isString) arg.m_str = line.substr(split + 1);
		else if (!ParseInt(line.substr(split + 1), arg.m_int)) return false;
		auto old = std::find_if(decoded.begin(), decoded.end(), [&](const ZS1_ARG& a) { return a.m_key == arg.m_key; });
		if (old == decoded.end()) decoded.push_back(std::move(arg));
		else *old = std::move(arg);
	}
	bank = std::move(decoded);
	return true;
}

bool LoadLevels(ZS1_USER& user)
{
	const std::string base = UserPath(user.m_slot);
	const std::string prefix = base.substr(base.find('/') + 1, 7) + "_";
	std::set<int> levels;
	for (const char* root : {Platform_BasePath(), Platform_PrefPath()}) {
		if (root == Platform_BasePath() && SuppressBaseLevels(user.m_slot)) continue;
		std::error_code error;
		const auto folder = std::filesystem::path(root) / "userdata";
		if (!std::filesystem::exists(folder, error)) continue;
		std::filesystem::directory_iterator it(folder, error), end;
		while (!error && it != end) {
			const std::string name = it->path().filename().string();
			if (name.size() > 12 && name.compare(0, 8, prefix) == 0 && name.substr(name.size()-4) == ".dat") {
				int level;
				if (ParseInt(name.substr(8, name.size()-12), level) && level >= 0) levels.insert(level);
			}
			it.increment(error);
		}
		if (error) return false;
	}
	for (int level : levels) {
		std::vector<std::string> lines;
		std::vector<ZS1_ARG> bank;
		if (ReadLines(UserPath(user.m_slot, level), lines, !SuppressBaseLevels(user.m_slot)) < 0 ||
			!DecodeBank(lines, 0, bank)) return false;
		user.m_levels.emplace_back(level, std::move(bank));
	}
	return true;
}

bool AddLine(std::string& bytes, const std::string& line)
{
	if (line.size() > 997 || line.find_first_of("\r\n") != std::string::npos ||
		line.find('\0') != std::string::npos) return false;
	bytes += line;
	bytes += "\r\n";
	return true;
}

bool EncodeBank(const std::vector<ZS1_ARG>& bank, std::string& bytes)
{
	for (const auto& arg : bank) {
		if (arg.m_key.empty() || arg.m_key.find('=') != std::string::npos) return false;
		if (!AddLine(bytes, (arg.m_isString ? "s" : "i") + arg.m_key + "=" +
			(arg.m_isString ? arg.m_str : std::to_string(arg.m_int)))) return false;
	}
	return true;
}

int RecordHash(const ZS1_RECORD& record)
{
	if (record.m_unverified) return -999;
	const std::string text = record.m_name + " " + std::to_string(record.m_score) + " " + std::to_string(record.m_type);
	uint32_t a = 1, b = 0;
	for (unsigned char c : text) { a = (a + c) % 65521; b = (b + a) % 65521; }
	const uint32_t hash = a | (b << 16);
	return hash <= INT32_MAX ? int(hash) : int(int64_t(hash) - 0x100000000LL);
}
}

int ZS1_FreeNativeSlot(const ZS1_STORE& store)
{
	for (int slot = 0; slot < 100; ++slot) {
		bool used = false;
		for (const auto& user : store.m_users) used |= user.m_slot == slot;
		if (!used) return slot;
	}
	return -1;
}

int ZS1_LoadNativeStore(ZS1_STORE& store)
{
	ZS1_STORE decoded = store;
	std::vector<ZS1_USER> users;
	std::vector<std::string> lines;
	bool profilesPresent = false;
	int currentSlot = -1;
	int status = ReadLines("userdata/_global.dat", lines);
	if (status < 0) return -1;
	if (status) {
		profilesPresent = true;
		if (!DecodeBank(lines, 0, decoded.m_global)) return -1;
		for (const auto& arg : decoded.m_global) if (arg.m_key == "m_iCurUser" && !arg.m_isString) currentSlot = arg.m_int;
	}
	for (int slot = 0; slot < 100; ++slot) {
		if (HasTombstone(slot)) { profilesPresent = true; continue; }
		lines.clear();
		status = ReadLines(UserPath(slot), lines);
		if (status < 0) return -1;
		if (!status) continue;
		profilesPresent = true;
		ZS1_USER user;
		user.m_slot = slot;
		if (lines.empty() || lines.front().empty() || !DecodeBank(lines, 1, user.m_args)) return -1;
		user.m_name = lines.front();
		if (!LoadLevels(user)) return -1;
		users.push_back(std::move(user));
	}
	if (profilesPresent) {
		decoded.m_users = std::move(users);
		decoded.m_current = -1;
		for (size_t i = 0; i < decoded.m_users.size(); ++i)
			if (decoded.m_users[i].m_slot == currentSlot) decoded.m_current = int(i);
	}
	else {

		if (decoded.m_users.size() > 100) return -1;
		for (size_t i = 0; i < decoded.m_users.size(); ++i) decoded.m_users[i].m_slot = int(i);
	}
	lines.clear();
	status = ReadLines("userdata/_records.dat", lines);
	if (status < 0) return -1;
	const bool recordsPresent = status != 0;
	if (!status && decoded.m_records.empty()) status = ReadLines("Maps/_records.dat_default", lines);
	if (status < 0) return -1;
	if (status) {
		if (lines.size() % 4) return -1;
		decoded.m_records.clear();
		for (size_t i = 0; i < lines.size(); i += 4) {
			ZS1_RECORD record;
			int hash = 0;
			record.m_name = lines[i];
			if (!ParseInt(lines[i+1], record.m_score) || !ParseInt(lines[i+2], record.m_type) ||
				!ParseInt(lines[i+3], hash)) return -1;
			if (RecordHash(record) != hash) {
				if (hash != -999) record.m_score = record.m_type = 0;
				record.m_unverified = true;
			}
			decoded.m_records.push_back(std::move(record));
		}
	}
	store = std::move(decoded);
	return profilesPresent || recordsPresent ? 1 : 0;
}

int ZS1_LoadNativeLevel(int slot, int level, std::vector<ZS1_ARG>& bank)
{
	if (slot < 0 || slot >= 100 || level < 0) return -1;
	std::vector<std::string> lines;
	const int status = ReadLines(UserPath(slot, level), lines, !SuppressBaseLevels(slot));
	if (status < 0) return -1;
	if (!status) { bank.clear(); return 0; }
	return DecodeBank(lines, 0, bank) ? 1 : -1;
}

bool ZS1_SaveNativeStore(ZS1_STORE& store)
{
	if (store.m_writeBlocked) return false;

	std::vector<std::pair<std::string, std::string>> files;
	std::vector<ZS1_ARG> globals = store.m_global;
	const int current = store.m_current >= 0 && size_t(store.m_current) < store.m_users.size() ?
		store.m_users[store.m_current].m_slot : -1;
	bool hasCurrent = false;
	for (auto& arg : globals) if (arg.m_key == "m_iCurUser") {
		arg.m_isString = false; arg.m_int = current; hasCurrent = true;
	}
	if (!hasCurrent) globals.push_back({"m_iCurUser", false, current, {}});
	for (const auto& user : store.m_users) {
		if (user.m_slot < 0 || user.m_slot >= 100) return false;
		std::string bytes;
		if (!AddLine(bytes, user.m_name) || !EncodeBank(user.m_args, bytes)) return false;
		files.emplace_back(UserPath(user.m_slot), std::move(bytes));
		for (const auto& level : user.m_levels) {
			bytes.clear();
			if (level.first < 0 || !EncodeBank(level.second, bytes)) return false;
			files.emplace_back(UserPath(user.m_slot, level.first), std::move(bytes));
		}
	}
	std::string bytes;
	for (const auto& record : store.m_records) {
		if (!AddLine(bytes, record.m_name) || !AddLine(bytes, std::to_string(record.m_score)) ||
			!AddLine(bytes, std::to_string(record.m_type)) || !AddLine(bytes, std::to_string(RecordHash(record)))) return false;
	}
	files.emplace_back("userdata/_records.dat", std::move(bytes));
	bytes.clear();
	if (!EncodeBank(globals, bytes)) return false;
	files.emplace_back("userdata/_global.dat", std::move(bytes));
	for (const auto& file : files) if (!Platform_WriteSaveAtomic(file.first, file.second)) return false;
	for (const auto& user : store.m_users) {

		if (HasTombstone(user.m_slot) &&
			!Platform_ArchiveSave(UserPath(user.m_slot) + ".opengromada-deleted")) return false;
	}
	return true;
}

bool ZS1_DeleteNativeUser(int slot)
{
	if (slot < 0 || slot >= 100) return false;




	if (!Platform_WriteSaveAtomic(UserPath(slot) + ".opengromada-ignore-base-levels", "ignore\n")) return false;
	if (!Platform_WriteSaveAtomic(UserPath(slot) + ".opengromada-deleted", "deleted\n")) return false;
	const std::string base = UserPath(slot);
	const std::string prefix = base.substr(base.find('/') + 1, 7);
	std::error_code error;
	const auto directory = std::filesystem::path(Platform_PrefPath()) / "userdata";
	std::filesystem::directory_iterator it(directory, error), end;
	std::vector<std::string> files;
	while (!error && it != end) {
		const std::string name = it->path().filename().string();
		if (name == prefix + ".dat" || (name.size() > 12 && name.compare(0, 8, prefix + "_") == 0 &&
			name.substr(name.size() - 4) == ".dat" &&
			name.find_first_not_of("0123456789", 8) == name.size() - 4)) {
			files.push_back(name);
		}
		it.increment(error);
	}
	if (error) return false;
	for (const auto& name : files) if (!Platform_ArchiveSave("userdata/" + name)) return false;
	return true;
}
