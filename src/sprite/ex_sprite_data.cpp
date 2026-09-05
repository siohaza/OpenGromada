#include "sprite/ex_sprite_data.h"

#include "game/game_descriptor.h"
#include "util/game_random.h"

#include "game/gametime.h"
#include "sprite/sprite.h"
#include "video/vid.h"

// FUNCTION: ALIEN 0x43f1a0
EX_SPRITE_DATA::EX_SPRITE_DATA(SPRITE* p_sprite) : m_unk0x24(0), m_unk0x28(0)
{
	m_unk0x14 = 1;


	m_unk0x10 = p_sprite->m_vid->DefaultLifetime();
	m_unk0x1c = 0;
	m_time = CurrentTime;
	m_unk0x20 = p_sprite->m_vid->m_unk0x2c;
	if (Game_IsZS1() && p_sprite->m_vid->m_randomSpeed != p_sprite->m_vid->m_unk0x2c) {
		VID* vid = p_sprite->m_vid;
		m_unk0x20 = vid->m_unk0x2c + (float) GameRand() * (vid->m_randomSpeed - vid->m_unk0x2c) * 3.0518509e-5f;
	}
	m_x = p_sprite->X();
	m_y = p_sprite->Y();
	m_z = p_sprite->Z();
}

// FUNCTION: ALIEN 0x446900
int LIST_INT::Location(int* p_value)
{
	int result = m_n;
	if (!result) {
		return -1;
	}
	int val = *p_value;
	int* p = m_data + result;
	while (1) {
		int cur = *--p;
		--result;
		if (cur == val) {
			return result;
		}
		if (!result) {
			return -1;
		}
	}
}

// FUNCTION: ALIEN 0x446bb0
int LIST_INT::DeleteNumber(int p_idx)
{
	if (p_idx < 0 || p_idx >= m_n) {
		return 1;
	}
	--m_n;
	m_data[p_idx] = m_data[m_n];
	return 0;
}
