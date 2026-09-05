
#include "game/zs1_commands.h"
#include "game/zs1_save.h"

#include "game/const.h"
#include "game/map.h"
#include "logic/logicstack.h"
#include "logic/logicvar.h"
#include "platform/paths.h"
#include "sprite/sprite.h"
#include "util/myerror.h"
#include "util/string.h"
#include "video/vid.h"

#include <SDL3/SDL.h>
#include <math.h>
#include <algorithm>
#include <stdio.h>
#include <string.h>
#include <string>
#include <utility>
#include <vector>

namespace
{

ZS1_STORE g_store;

const char* PROFILE_FILE = "zs1_profiles.dat";
const char* RECORDS_FILE = "zs1_records.dat";

FILE* OpenLegacyProgress(const char* name)
{
	const std::string local = std::string(Platform_PrefPath()) + name;
	if (FILE* file = Platform_FOpen(local.c_str(), "rb")) return file;
	if (FILE* file = Platform_FOpen(name, "rb")) return file;



	const char* overridePath = SDL_getenv("ALIEN_SHOOTER_PREF_PATH");
	if (overridePath && *overridePath) return nullptr;
	char* old = SDL_GetPrefPath("SigmaTeam", "AlienShooter");
	if (!old) return nullptr;
	const std::string path = std::string(old) + name;
	SDL_free(old);
	FILE* file = Platform_FOpen(path.c_str(), "rb");
	if (file) MYERROR::Log(::Error, "ZS1: importing legacy %s read-only; original retained", path.c_str());
	return file;
}

ZS1_ARG* FindArg(std::vector<ZS1_ARG>& p_bank, const char* p_key)
{
	for (ZS1_ARG& arg : p_bank) {
		if (arg.m_key == p_key) {
			return &arg;
		}
	}
	return 0;
}

std::vector<ZS1_ARG>* ScopeBank(int p_scope)
{
	if (p_scope == 2) {
		return &g_store.m_level;
	}
	if (p_scope != 0 || g_store.m_current < 0 || g_store.m_current >= (int) g_store.m_users.size()) {
		return &g_store.m_global;
	}
	return &g_store.m_users[g_store.m_current].m_args;
}

bool SaveStore()
{
	if (!ZS1_SaveNativeStore(g_store)) {
		MYERROR::Log(::Error, "ZS1 save failed; recoverable originals retained (check native backups)");
		return false;
	}
	return true;
}

bool AddUser(const char* name)
{
	if (!*name || g_store.m_users.size() >= 100 || g_store.m_writeBlocked) return false;
	ZS1_USER user;
	user.m_name = name;
	user.m_slot = ZS1_FreeNativeSlot(g_store);

	const auto place = std::lower_bound(g_store.m_users.begin(), g_store.m_users.end(), user.m_slot,
		[](const ZS1_USER& existing, int slot) { return existing.m_slot < slot; });
	const int previous = g_store.m_current;
	g_store.m_current = int(place - g_store.m_users.begin());
	g_store.m_users.insert(place, std::move(user));
	if (SaveStore()) return true;
	g_store.m_users.erase(g_store.m_users.begin() + g_store.m_current);
	g_store.m_current = previous;
	return false;
}

void ParseArgLine(std::vector<ZS1_ARG>* p_bank, bool p_isString, char* p_rest)
{
	if (!p_bank) {
		return;
	}
	char* value = strchr(p_rest, '\t');
	if (!value) {
		return;
	}
	*value++ = 0;
	ZS1_ARG arg;
	arg.m_key = p_rest;
	arg.m_isString = p_isString;
	if (p_isString) {
		arg.m_str = value;
	}
	else {
		arg.m_int = atoi(value);
	}
	p_bank->push_back(arg);
}

void LoadStore()
{
	if (g_store.m_loaded) {
		return;
	}
	g_store.m_loaded = true;
	if (FILE* file = OpenLegacyProgress(PROFILE_FILE)) {
		char line[1024];
		const bool header = fgets(line, sizeof(line), file) && !strncmp(line, "ZS1USERS\t1", 10);
		if (!header) g_store.m_writeBlocked = true;
		ZS1_USER* user = 0;
		std::vector<ZS1_ARG>* levelBank = 0;
		while (header && fgets(line, sizeof(line), file)) {
			if (!strchr(line, '\n')) { g_store.m_writeBlocked = true; break; }
			line[strcspn(line, "\r\n")] = 0;
			char* rest = strchr(line, '\t');
			if (!rest) {
				continue;
			}
			*rest++ = 0;
			if (!strcmp(line, "current")) {
				g_store.m_current = atoi(rest);
			}
			else if (!strcmp(line, "user")) {
				g_store.m_users.push_back(ZS1_USER());
				user = &g_store.m_users.back();
				user->m_name = rest;
				levelBank = 0;
			}
			else if (user && !strcmp(line, "arg0i")) {
				ParseArgLine(&user->m_args, false, rest);
			}
			else if (user && !strcmp(line, "arg0s")) {
				ParseArgLine(&user->m_args, true, rest);
			}
			else if (user && !strcmp(line, "level")) {
				user->m_levels.push_back({atoi(rest), {}});
				levelBank = &user->m_levels.back().second;
			}
			else if (user && !strcmp(line, "largi")) {
				ParseArgLine(levelBank, false, rest);
			}
			else if (user && !strcmp(line, "largs")) {
				ParseArgLine(levelBank, true, rest);
			}
		}
		fclose(file);
	}
	if (g_store.m_current >= (int) g_store.m_users.size()) {
		g_store.m_current = -1;
	}
	if (FILE* file = OpenLegacyProgress(RECORDS_FILE)) {
		char line[1024];
		const bool header = fgets(line, sizeof(line), file) && !strncmp(line, "ZS1RECORDS\t1", 12);
		if (!header) g_store.m_writeBlocked = true;
		while (header && fgets(line, sizeof(line), file)) {
			if (!strchr(line, '\n')) { g_store.m_writeBlocked = true; break; }
			line[strcspn(line, "\r\n")] = 0;
			int score;
			int type;
			int offset = 0;
			if (sscanf(line, "rec\t%d\t%d\t%n", &score, &type, &offset) == 2 && offset > 0) {
				ZS1_RECORD record;
				record.m_score = score;
				record.m_type = type;
				record.m_name = line + offset;
				g_store.m_records.push_back(record);
			}
		}
		fclose(file);
	}
	if (ZS1_LoadNativeStore(g_store) < 0) {
		g_store.m_writeBlocked = true;
		MYERROR::Log(::Error, "ZS1: invalid native progress; writes disabled to preserve the original files");
	}
}

}

ZS1_CMD_RESULT ZS1_SendCommand2(
	MAP* p_map,
	int p_id,
	int p_var1,
	int p_var2,
	const char* p_str1,
	const char* p_str2,
	int* p_outInt,
	const void** p_outObj,
	STRING* p_outStr
)
{
	LoadStore();
	*p_outInt = 0;

	switch (p_id) {
	case 1:
		*p_outInt = (int) sqrt((double) p_var1);
		return ZS1_CMD_INT;
	case 5:
		*p_outInt = (int) g_store.m_users.size();
		return ZS1_CMD_INT;
	case 6:
		if (p_var1 >= 0 && p_var1 < (int) g_store.m_users.size()) {
			*p_outStr = g_store.m_users[p_var1].m_name.c_str();
		}
		else {
			*p_outStr = "";
		}
		return ZS1_CMD_STR;
	case 7:
		for (const ZS1_USER& user : g_store.m_users) {
			if (user.m_name == p_str1) {
				return ZS1_CMD_INT;
			}
		}

		*p_outInt = AddUser(p_str1);
		return ZS1_CMD_INT;
	case 8:
	case 10: {
		std::vector<ZS1_ARG>* bank = ScopeBank(p_var1);
		if (!bank) {
			return ZS1_CMD_INT;
		}
		ZS1_ARG* arg = FindArg(*bank, p_str1);
		if (!arg) {
			bank->push_back(ZS1_ARG());
			arg = &bank->back();
			arg->m_key = p_str1;
		}
		arg->m_isString = p_id == 10;
		if (p_id == 10) {
			arg->m_str = p_str2;
		}
		else {
			arg->m_int = p_var2;
		}
		return ZS1_CMD_INT;
	}
	case 9: {
		std::vector<ZS1_ARG>* bank = ScopeBank(p_var1);
		ZS1_ARG* arg = bank ? FindArg(*bank, p_str1) : 0;
		*p_outInt = arg && !arg->m_isString ? arg->m_int : p_var2;
		return ZS1_CMD_INT;
	}
	case 11: {
		std::vector<ZS1_ARG>* bank = ScopeBank(p_var1);
		ZS1_ARG* arg = bank ? FindArg(*bank, p_str1) : 0;
		*p_outStr = arg && arg->m_isString ? arg->m_str.c_str() : p_str2;
		return ZS1_CMD_STR;
	}
	case 12:
		if (p_var1 >= 0 && p_var1 < (int) g_store.m_users.size()) {
			if (g_store.m_writeBlocked || !ZS1_DeleteNativeUser(g_store.m_users[p_var1].m_slot)) return ZS1_CMD_INT;
			g_store.m_users.erase(g_store.m_users.begin() + p_var1);
			if (g_store.m_current == p_var1) {
				g_store.m_current = -1;
			}
			else if (g_store.m_current > p_var1) {
				--g_store.m_current;
			}
			*p_outInt = SaveStore();
		}
		return ZS1_CMD_INT;
	case 13:
		if (p_var1 >= 0 && p_var1 < (int) g_store.m_users.size() && *p_str1) {
			const std::string previous = g_store.m_users[p_var1].m_name;
			g_store.m_users[p_var1].m_name = p_str1;
			*p_outInt = SaveStore();
			if (!*p_outInt) g_store.m_users[p_var1].m_name = previous;
		}
		return ZS1_CMD_INT;
	case 14: {
		const int previous = g_store.m_current;
		g_store.m_current = p_var1 >= 0 && p_var1 < (int) g_store.m_users.size() ? p_var1 : -1;
		*p_outInt = SaveStore();
		if (!*p_outInt) g_store.m_current = previous;
		return ZS1_CMD_INT;
	}
	case 15:
		*p_outInt = g_store.m_current;
		return ZS1_CMD_INT;
	case 16:
		MYERROR::Log(::Error, "ZS1: %s", p_str1);
		return ZS1_CMD_INT;
	case 17: {
		ZS1_RECORD record;
		record.m_name = p_str1;
		record.m_score = p_var1;
		record.m_type = p_var2;
		size_t pos = 0;
		while (pos < g_store.m_records.size() &&
			   !(record.m_score > g_store.m_records[pos].m_score ||
				 (record.m_score == g_store.m_records[pos].m_score &&
				  record.m_type > g_store.m_records[pos].m_type))) {
			++pos;
		}
		g_store.m_records.insert(g_store.m_records.begin() + pos, record);
		if (SaveStore()) *p_outInt = (int) pos;
		else {
			g_store.m_records.erase(g_store.m_records.begin() + pos);
			*p_outInt = -1;
		}
		return ZS1_CMD_INT;
	}
	case 18:
		*p_outInt = (int) g_store.m_records.size();
		return ZS1_CMD_INT;
	case 19:
		if (p_var1 >= 0 && p_var1 < (int) g_store.m_records.size()) {
			*p_outStr = g_store.m_records[p_var1].m_name.c_str();
		}
		else {
			*p_outStr = "";
		}
		return ZS1_CMD_STR;
	case 20:
		if (p_var1 >= 0 && p_var1 < (int) g_store.m_records.size()) {
			*p_outInt = g_store.m_records[p_var1].m_score;
		}
		return ZS1_CMD_INT;
	case 21:
		if (p_var1 >= 0 && p_var1 < (int) g_store.m_records.size()) {
			*p_outInt = g_store.m_records[p_var1].m_type;
		}
		return ZS1_CMD_INT;
	case 22:
		*p_outInt = SaveStore();
		return ZS1_CMD_INT;
	case 29: {
		VID* vid = p_map->GetVid(p_var1);
		if (vid != EmptyVid && vid->m_noDir) {
			*p_outInt = (unsigned char) ((p_var2 << 8) / vid->m_noDir);
		}
		return ZS1_CMD_INT;
	}
	case 30:
		*p_outObj = p_map->m_menu.m_underCursor;
		return ZS1_CMD_OBJ;
	case 31:
		*p_outInt = Const ? Const->m_debugMode : 0;
		return ZS1_CMD_INT;
	case 32: {
		auto previous = std::move(g_store.m_records);
		g_store.m_records.clear();
		if (!SaveStore()) g_store.m_records = std::move(previous);
		return ZS1_CMD_INT;
	}
	case 33:
		for (size_t i = 0; i < g_store.m_users.size(); ++i) {
			if (g_store.m_users[i].m_name == p_str1) {
				const int previous = g_store.m_current;
				g_store.m_current = (int) i;
				*p_outInt = SaveStore();
				if (!*p_outInt) g_store.m_current = previous;
				return ZS1_CMD_INT;
			}
		}
		*p_outInt = AddUser(p_str1);
		return ZS1_CMD_INT;
	case 34:
		MYERROR::Log(::Error, "ZS1: DBG_1");
		return ZS1_CMD_INT;
	case 47: {
		g_store.m_level.clear();
		if (g_store.m_current < 0 || g_store.m_current >= (int) g_store.m_users.size()) {
			return ZS1_CMD_INT;
		}
		ZS1_USER& user = g_store.m_users[g_store.m_current];
		for (const auto& level : user.m_levels) {
			if (level.first == p_var1) {
				g_store.m_level = level.second;
				*p_outInt = 1;
				return ZS1_CMD_INT;
			}
		}
		const int loaded = ZS1_LoadNativeLevel(user.m_slot, p_var1, g_store.m_level);
		if (loaded < 0) {
			g_store.m_writeBlocked = true;
			p_map->m_logic.RuntimeError("invalid ZS1 native level save", p_var1);
			return ZS1_CMD_INT;
		}
		*p_outInt = loaded;
		return ZS1_CMD_INT;
	}
	case 48: {
		if (g_store.m_current < 0 || g_store.m_current >= (int) g_store.m_users.size()) {
			return ZS1_CMD_INT;
		}
		ZS1_USER& user = g_store.m_users[g_store.m_current];
		for (auto& level : user.m_levels) {
			if (level.first == p_var1) {
				const auto previous = level.second;
				level.second = g_store.m_level;
				*p_outInt = SaveStore();
				if (!*p_outInt) level.second = previous;
				return ZS1_CMD_INT;
			}
		}
		user.m_levels.push_back({p_var1, g_store.m_level});
		*p_outInt = SaveStore();
		if (!*p_outInt) user.m_levels.pop_back();
		return ZS1_CMD_INT;
	}
	case 49:
		g_store.m_level.clear();
		return ZS1_CMD_INT;
	default:
		p_map->m_logic.RuntimeError("unsupported ZS1 SendCommand2", p_id);
		return ZS1_CMD_INT;
	}
}
