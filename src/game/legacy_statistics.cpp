#include "game/legacy_statistics.h"

#include "game/map.h"
#include "logic/logicstack.h"
#include "logic/logicvar.h"
#include "sprite/sprite.h"
#include "util/myerror.h"
#include "util/string.h"
#include "video/vid.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cerrno>
#include <cstdint>
#include <cstdlib>

namespace {






constexpr int NATIVE_VID_LIMIT = 4096;
using MONSTER_MASK = std::array<bool, NATIVE_VID_LIMIT>;

bool IsMonster(const MONSTER_MASK& p_mask, decomp_intptr p_vid)
{
	return p_vid >= 0 && p_vid < NATIVE_VID_LIMIT && p_mask[(size_t) p_vid];
}

unsigned CountDeathChain(const MONSTER_MASK& p_mask, const VID* p_vid)
{
	if (!p_vid || !p_vid->m_aniChildVid[15]) return 0;


	return unsigned(IsMonster(p_mask, p_vid->m_unk0x20c[15])) +
		unsigned(IsMonster(p_mask, p_vid->m_aniChildVid[15]->m_unk0x20c[15]));
}

bool ReadMonsterMask(LOGIC& p_logic, MONSTER_MASK& p_mask)
{
	const int variable = p_logic.m_variables.Location(STRING("MonstersVid"));
	if (variable < 0 || p_logic.m_variables.m_data[variable].m_var.m_flag != 1) {


		MYERROR::Log(::Error, "!!!ERROR!!! SCRIPT Can't find variable 'MonstersVid' in GetUnitInMap");
		return false;
	}
	const LOGICVAR& declaration = p_logic.m_variables.m_data[variable].m_var;
	const int count = std::min(declaration.m_extra, NATIVE_VID_LIMIT);
	if (declaration.m_a < 0 || count < 0 || declaration.m_a > p_logic.m_stack.m_n ||
		count > p_logic.m_stack.m_n - declaration.m_a) {
		p_logic.RuntimeError("GetUnitInMap MonstersVid outside variable storage", declaration.m_a);
		return false;
	}
	const LOGICSTACK* slots = (const LOGICSTACK*) p_logic.m_stack.m_data;
	for (int i = 0; i < count; ++i) {
		const LOGICSTACK& value = slots[declaration.m_a + i];
		decomp_intptr nvid;
		if (value.m_type & 2) {
			nvid = value.m_num;
		}
		else {
			const char* text = value.m_str.m_str;
			if (!*text) break;


			char* end;
			errno = 0;
			const bool hexadecimal = text[1] == 'x';
			const long parsed = std::strtol(text, &end, hexadecimal ? 0 : 10);
			if (errno == ERANGE || (hexadecimal && end == text)) {
				p_logic.RuntimeError("GetUnitInMap invalid MonstersVid value", i);
				return false;
			}
			nvid = parsed;
		}
		if (nvid < 0 || nvid >= NATIVE_VID_LIMIT) {


			p_logic.RuntimeError("GetUnitInMap MonstersVid index outside native range", i);
			return false;
		}
		p_mask[(size_t) nvid] = true;
	}
	return true;
}

}

int Legacy_CountUnitsInMap(MAP* p_map, int p_layerCount)
{
	if (!p_map) return 0;
	if (p_layerCount <= 0 || p_layerCount > MAP::MAX_LAYERS) {
		p_map->m_logic.RuntimeError("GetUnitInMap unsupported layer layout", p_layerCount);
		return 0;
	}
	MONSTER_MASK monsters = {};
	if (!ReadMonsterMask(p_map->m_logic, monsters)) return 0;
	uint32_t count = 0;
	for (int layer = 0; layer < p_layerCount; ++layer) {
		int position;
		for (SPRITE* sprite = p_map->FirstSprite(layer, &position); sprite;
			 sprite = p_map->NextSprite(layer, &position)) {
			const VID* vid = sprite->m_vid;
			if (vid && IsMonster(monsters, vid->m_idx)) {
				++count;
				count += CountDeathChain(monsters, vid);
			}


			for (int action = sprite->m_actions.m_n - 1; action >= 0; --action) {
				const ACT& act = sprite->m_actions.m_data[action];
				if (act.m_cmd == 73) break;
				if (act.m_cmd == 35 && act.m_a > 0 && IsMonster(monsters, act.m_a)) {
					++count;
					count += CountDeathChain(monsters, p_map->Vid((int) act.m_a));
				}
			}
		}
	}
	return std::bit_cast<int32_t>(count);
}
