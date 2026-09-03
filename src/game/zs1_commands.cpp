
#include "game/zs1_commands.h"

#include "game/const.h"
#include "game/map.h"
#include "logic/logicstack.h"
#include "logic/logicvar.h"
#include "platform/paths.h"
#include "sprite/sprite.h"
#include "util/myerror.h"
#include "util/string.h"
#include "video/vid.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <utility>
#include <vector>

namespace
{

struct ZS1_ARG {
	std::string m_key;
	bool m_isString = false;
	int m_int = 0;
	std::string m_str;
};

struct ZS1_USER {
	std::string m_name;
	std::vector<ZS1_ARG> m_args;
	std::vector<std::pair<int, std::vector<ZS1_ARG>>> m_levels;
};

struct ZS1_RECORD {
	std::string m_name;
	int m_score = 0;
	int m_type = 0;
};

struct ZS1_STORE {
	bool m_loaded = false;
	int m_current = -1;
	std::vector<ZS1_USER> m_users;
	std::vector<ZS1_ARG> m_global;
	std::vector<ZS1_ARG> m_level;
	std::vector<ZS1_RECORD> m_records;
};

ZS1_STORE g_store;

const char* PROFILE_FILE = "zs1_profiles.dat";
const char* RECORDS_FILE = "zs1_records.dat";

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

void WriteBank(FILE* p_file, const char* p_tag, const std::vector<ZS1_ARG>& p_bank)
{
	for (const ZS1_ARG& arg : p_bank) {
		if (arg.m_isString) {
			fprintf(p_file, "%ss\t%s\t%s\n", p_tag, arg.m_key.c_str(), arg.m_str.c_str());
		}
		else {
			fprintf(p_file, "%si\t%s\t%d\n", p_tag, arg.m_key.c_str(), arg.m_int);
		}
	}
}

void SaveStore()
{
	if (FILE* file = Platform_FOpen(PROFILE_FILE, "wb")) {
		fprintf(file, "ZS1USERS\t1\ncurrent\t%d\n", g_store.m_current);
		for (const ZS1_USER& user : g_store.m_users) {
			fprintf(file, "user\t%s\n", user.m_name.c_str());
			WriteBank(file, "arg0", user.m_args);
			for (const auto& level : user.m_levels) {
				fprintf(file, "level\t%d\n", level.first);
				WriteBank(file, "larg", level.second);
			}
		}
		fclose(file);
	}
	if (FILE* file = Platform_FOpen(RECORDS_FILE, "wb")) {
		fprintf(file, "ZS1RECORDS\t1\n");
		for (const ZS1_RECORD& record : g_store.m_records) {
			fprintf(file, "rec\t%d\t%d\t%s\n", record.m_score, record.m_type, record.m_name.c_str());
		}
		fclose(file);
	}
	Platform_InvalidatePathCache();
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
	if (FILE* file = Platform_FOpen(PROFILE_FILE, "rb")) {
		char line[1024];
		ZS1_USER* user = 0;
		std::vector<ZS1_ARG>* levelBank = 0;
		while (fgets(line, sizeof(line), file)) {
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
	if (FILE* file = Platform_FOpen(RECORDS_FILE, "rb")) {
		char line[1024];
		while (fgets(line, sizeof(line), file)) {
			line[strcspn(line, "\r\n")] = 0;
			int score;
			int type;
			int offset;
			if (sscanf(line, "rec\t%d\t%d\t%n", &score, &type, &offset) == 2) {
				ZS1_RECORD record;
				record.m_score = score;
				record.m_type = type;
				record.m_name = line + offset;
				g_store.m_records.push_back(record);
			}
		}
		fclose(file);
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
		if (*p_str1) {
			ZS1_USER user;
			user.m_name = p_str1;
			g_store.m_users.push_back(user);
			SaveStore();
			*p_outInt = 1;
		}
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
			g_store.m_users.erase(g_store.m_users.begin() + p_var1);
			if (g_store.m_current == p_var1) {
				g_store.m_current = -1;
			}
			else if (g_store.m_current > p_var1) {
				--g_store.m_current;
			}
			SaveStore();
			*p_outInt = 1;
		}
		return ZS1_CMD_INT;
	case 13:
		if (p_var1 >= 0 && p_var1 < (int) g_store.m_users.size() && *p_str1) {
			g_store.m_users[p_var1].m_name = p_str1;
			SaveStore();
			*p_outInt = 1;
		}
		return ZS1_CMD_INT;
	case 14:
		g_store.m_current = p_var1 >= 0 && p_var1 < (int) g_store.m_users.size() ? p_var1 : -1;
		SaveStore();
		return ZS1_CMD_INT;
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
		SaveStore();
		*p_outInt = (int) pos;
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
		SaveStore();
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
	case 32:
		g_store.m_records.clear();
		SaveStore();
		return ZS1_CMD_INT;
	case 33:
		for (size_t i = 0; i < g_store.m_users.size(); ++i) {
			if (g_store.m_users[i].m_name == p_str1) {
				g_store.m_current = (int) i;
				SaveStore();
				*p_outInt = 1;
				return ZS1_CMD_INT;
			}
		}
		if (*p_str1) {
			ZS1_USER user;
			user.m_name = p_str1;
			g_store.m_users.push_back(user);
			g_store.m_current = (int) g_store.m_users.size() - 1;
			SaveStore();
			*p_outInt = 1;
		}
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
				break;
			}
		}
		return ZS1_CMD_INT;
	}
	case 48: {
		if (g_store.m_current < 0 || g_store.m_current >= (int) g_store.m_users.size()) {
			return ZS1_CMD_INT;
		}
		ZS1_USER& user = g_store.m_users[g_store.m_current];
		for (auto& level : user.m_levels) {
			if (level.first == p_var1) {
				level.second = g_store.m_level;
				SaveStore();
				*p_outInt = 1;
				return ZS1_CMD_INT;
			}
		}
		user.m_levels.push_back({p_var1, g_store.m_level});
		SaveStore();
		*p_outInt = 1;
		return ZS1_CMD_INT;
	}
	case 49:
		g_store.m_level.clear();
		return ZS1_CMD_INT;
	default:
		MYERROR::Log(::Error, "ZS1: SendCommand2 id %i not implemented (var1=%i var2=%i)", p_id, p_var1, p_var2);
		return ZS1_CMD_INT;
	}
}

static int CountDeathChain(const unsigned char* p_mask, VID* p_vid)
{
	if (!p_vid || !p_vid->m_aniChildVid[15]) {
		return 0;
	}
	int count = 0;
	int direct = p_vid->m_unk0x20c[15];
	if (MAP::ValidVidIndex(direct) && p_mask[direct]) {
		++count;
	}
	int nested = p_vid->m_aniChildVid[15]->m_unk0x20c[15];
	if (MAP::ValidVidIndex(nested) && p_mask[nested]) {
		++count;
	}
	return count;
}

int ZS1_CountUnitsInMap(MAP* p_map)
{
	static unsigned char mask[MAP::MAX_VIDS];
	memset(mask, 0, sizeof(mask));

	LOGIC& logic = p_map->m_logic;
	int var = logic.m_variables.Location(STRING("MonstersVid"));
	if (var < 0 || logic.m_variables.m_data[var].m_var.m_flag != 1) {
		return 0;
	}
	const LOGICVAR& monsters = logic.m_variables.m_data[var].m_var;
	LOGICSTACK* slots = (LOGICSTACK*) logic.m_stack.m_data;
	for (int i = 0; i < monsters.m_extra && monsters.m_a + i < logic.m_stack.m_n; ++i) {
		int nvid = slots[monsters.m_a + i].Int();
		if (MAP::ValidVidIndex(nvid)) {
			mask[nvid] = 1;
		}
	}

	int count = 0;
	for (int layer = 0; layer < MAP::LayerCount(); ++layer) {
		for (int i = 0; i < p_map->m_layers[layer].m_n; ++i) {
			SPRITE* sprite = (SPRITE*) p_map->m_layers[layer].m_data[i];
			VID* vid = sprite->m_vid;
			if (vid && MAP::ValidVidIndex(vid->m_idx) && mask[vid->m_idx]) {
				++count;
				count += CountDeathChain(mask, vid);
			}
			int queued = sprite->m_actions.m_n;
			if (!queued || sprite->m_actions.m_data[queued - 1].m_cmd == 73) {
				continue;
			}
			for (int a = queued - 1; a >= 0; --a) {
				const ACT& act = sprite->m_actions.m_data[a];
				if (act.m_cmd == 73) {
					break;
				}
				if (act.m_cmd == 35 && act.m_a > 0 && MAP::ValidVidIndex((int) act.m_a) && mask[act.m_a]) {
					++count;
					count += CountDeathChain(mask, p_map->Vid((int) act.m_a));
				}
			}
		}
	}
	return count;
}
