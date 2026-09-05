#pragma once

#include <string>
#include <utility>
#include <vector>

struct ZS1_ARG {
	std::string m_key;
	bool m_isString = false;
	int m_int = 0;
	std::string m_str;
};
struct ZS1_USER {
	std::string m_name;
	int m_slot = -1;
	std::vector<ZS1_ARG> m_args;
	std::vector<std::pair<int, std::vector<ZS1_ARG>>> m_levels;
};
struct ZS1_RECORD {
	std::string m_name;
	int m_score = 0;
	int m_type = 0;
	bool m_unverified = false;
};
struct ZS1_STORE {
	bool m_loaded = false;
	bool m_writeBlocked = false;
	int m_current = -1;
	std::vector<ZS1_USER> m_users;
	std::vector<ZS1_ARG> m_global;
	std::vector<ZS1_ARG> m_level;
	std::vector<ZS1_RECORD> m_records;
};


int ZS1_LoadNativeStore(ZS1_STORE& p_store);
bool ZS1_SaveNativeStore(ZS1_STORE& p_store);
int ZS1_LoadNativeLevel(int p_slot, int p_level, std::vector<ZS1_ARG>& p_bank);
bool ZS1_DeleteNativeUser(int p_slot);
int ZS1_FreeNativeSlot(const ZS1_STORE& p_store);
