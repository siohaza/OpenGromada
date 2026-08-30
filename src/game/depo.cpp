#include "game/depo.h"

#include "game/const.h"
#include "game/engine.h"
#include "game/gametime.h"
#include "game/map.h"
#include "sprite/ex_sprite_data.h"
#include "sprite/sprite.h"
#include "ui/mouse.h"
#include "util/myerror.h"
#include "video/vid.h"
#include "world/hash_map.h"

extern char g_depoCantCreate[];

// FUNCTION: ALIEN 0x44c040
DEPO::DEPO(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent)
	: UNIT(p_vid, p_x, p_y, p_z, p_dir, p_parent)
{
	m_unk0x90 = 0;
	m_unk0x47c = 0;
	m_queueLen = 0;
	m_queueMax = 10;
	int* extra = m_unk0x2ec;
	short* queue = m_queue;
	int n = 20;
	do {
		*queue = 0;
		extra[-100] = 0;
		*extra = 0;
		++queue;
		++extra;
		--n;
	} while (n);
}

// FUNCTION: ALIEN 0x44c0d0
void* DEPO::ScalarDeletingDestructor(unsigned int p_flags)
{
	DEPO* result = this;
	this->~DEPO();
	if (p_flags & 1) {
		operator delete(result);
	}
	return result;
}

// FUNCTION: ALIEN 0x44c0f0
DEPO::~DEPO()
{
}

// FUNCTION: ALIEN 0x44c100
void DEPO::MoveTact()
{
	if ((CurrentTime & 0xFFFFC000) > PrevCurrentTime) {
		m_flag &= ~2u;
	}
	AddHpPerSecond(Const->m_depoAddHp);
}

// STUB: ALIEN 0x44c130
decomp_intptr DEPO::Action(int p_action, decomp_intptr p_a, decomp_intptr p_b, decomp_intptr p_c)
{
	switch (p_action) {
	case 73:
		m_actions.Insert(ACT(p_action, p_a, p_b, p_c));
		Action(130, 0, 0, 0);
		return 0;
	case 70:
		if ((m_flag & 0x7c) == 0x40) {
			m_actions.Insert(ACT(35, (unsigned short) m_queue[0], 0, 0));
			SetCommand(0, 0);
			return 0;
		}
		break;
	case 35:
		if (p_a < 0 || p_a >= Map->m_noVid || !Map->m_vids[p_a]) {
			return 0;
		}
		AddUnitToQueue(p_a);
		BuildNextUnit();
		return 0;
	case 130:
		if (m_ani < 15) {
			if (m_ani == 13) {
				ChangeAnimation(0);
			}
			if (m_unk0x47c && !m_unk0x2ec[m_unk0x47c - 1]) {
				if (!m_unk0x50) {
					ActionBuildUnit(p_a, p_b);
					return 0;
				}
				if (m_ani != 1) {
					ChangeAnimation(1);
					return 0;
				}
			}
			else {
				if (m_ani) {
					ChangeAnimation(0);
				}
				if (m_flag & 0x7c) {
					SetCommand(0, 0);
				}
				Map->ScriptRun(EvFunctionNumber[14], this, 0, 0);
				return 0;
			}
		}
		return 0;
	case 85:
		if (p_a > 0) {
			int flag = m_flag;
			if (!(flag & 2) && p_b && ((flag ^ ((SPRITE*) p_b)->m_flag) & 0x1800)) {
				Map->ScriptRun(EvFunctionNumber[13], this, (SPRITE*) p_b, 0);
				m_flag |= 2;
			}
		}
		return UNIT::Action(p_action, p_a, p_b, p_c);
	case 97: {
		int army = (m_flag >> 11) & 3;
		Map->m_player[army]->DeletePointerToSprite(this);
		ChangeArmy(p_a);
		if (army != ((m_flag >> 11) & 3)) {
			Map->ScriptRun(EvFunctionNumber[15], this, 0, 0);
		}
		Map->m_player[(m_flag >> 11) & 3]->AddPointerToSprite(this);
		return 0;
	}
	case 81:
	case 200: {
		UNIT::Action(p_action, p_a, p_b, p_c);
		Map->m_player[(m_flag >> 11) & 3]->AddPointerToSprite(this);
		if (m_exData) {
			int order[14];
			order[0] = 5;
			order[1] = 10;
			order[2] = 20;
			order[3] = 25;
			order[4] = 80;
			order[5] = 85;
			order[6] = 45;
			order[7] = 30;
			order[8] = 35;
			order[9] = 82;
			order[10] = 97;
			order[11] = 90;
			order[12] = 75;
			order[13] = 62;
			int put = 0;
			int* want = order;
			int n = 14;
			do {
				int scan = put;
				if (put < m_exData->m_list.m_n) {
					int key = *want;
					do {
						int* data = m_exData->m_list.m_data;
						int found = data[scan];
						if (key == found) {
							int prev = data[put];
							data[put++] = found;
							m_exData->m_list.m_data[scan] = prev;
						}
						++scan;
					} while (scan < m_exData->m_list.m_n);
				}
				++want;
				--n;
			} while (n);
		}
		return 0;
	}
	case 80:
		if (!ActionStackHaveCommand(73)) {
			m_actions.Insert(ACT(73, 0, 0, 0));
		}
		break;
	}
	return UNIT::Action(p_action, p_a, p_b, p_c);
}

// FUNCTION: ALIEN 0x44c5f0
void DEPO::AddUnitToQueue(int p_vid)
{
	if (m_queueLen >= m_queueMax) {
		return;
	}
	int money = Map->m_player[(m_flag >> 11) & 3]->m_money;
	VID* v;
	if (p_vid < 0 || p_vid >= Map->m_noVid || !(v = Map->m_vids[p_vid])) {
		v = EmptyVid;
	}
	if (money < v->GetBuildTime()) {
		return;
	}
	if (p_vid < 0 || p_vid >= Map->m_noVid || !(v = Map->m_vids[p_vid])) {
		v = EmptyVid;
	}
	PLAYER* player = Map->m_player[(m_flag >> 11) & 3];
	int cost = -v->GetBuildTime();
	player->m_money += cost;
	m_queue[m_queueLen] = (short) p_vid;
	v = Map->GetVid(p_vid);
	m_buildTicks[m_queueLen] = v->GetBuildTime() * Const->m_unk0x24;
	m_unk0x2ec[m_queueLen] = 0;
	m_queueLen++;
}

// FUNCTION: ALIEN 0x44c700
void DEPO::BuildNextUnit()
{
	if (m_unk0x47c) {
		return;
	}
	m_unk0x47c = 1;
	if (m_unk0x2ec[0]) {
		int max = m_queueMax;
		while (1) {
			int idx = m_unk0x47c;
			if (idx > max) {
				break;
			}
			m_unk0x47c = idx + 1;
			if (!m_unk0x2ec[m_unk0x47c - 1]) {
				break;
			}
		}
	}
	if (m_unk0x47c <= m_queueLen) {
		SetCommand(16, 0);
		int idx = m_unk0x47c;
		int vid = (unsigned short) m_queue[idx - 1];
		int ticks = m_buildTicks[idx - 1];
		if (ticks) {
			m_unk0x50 = ticks;
			return;
		}
		VID* v;
		if (vid < 0 || vid >= Map->m_noVid || !(v = Map->m_vids[vid])) {
			v = EmptyVid;
		}
		m_unk0x50 = v->GetBuildTime() * Const->m_unk0x24;
		return;
	}
	m_unk0x47c = 0;
	SetCommand(0, 0);
	ChangeAnimation(0);
}

// FUNCTION: ALIEN 0x44c7e0
int DEPO::ActionBuildUnit(int p_a, int p_b)
{
	SPRITE* blocker;
	for (;;) {
		float x = m_z;
		float y = m_y;
		float z = m_x;
		int vidId = (unsigned short) m_queue[m_unk0x47c - 1];
		VID* vid = (vidId >= 0 && vidId < Map->m_noVid && Map->m_vids[vidId]) ? Map->m_vids[vidId] : EmptyVid;
		blocker = Hash->CanPlace(vid, z, y, x);
		if (!blocker || blocker == (SPRITE*) Mouse) {
			break;
		}
		blocker->ScalarDeletingDestructor(1);
	}

	ANGLE dir;
	float sx;
	float sy;
	float sz;
	SPRITE* unit = Map->CreateSprite(
		Map->Vid((unsigned short) m_queue[m_unk0x47c - 1]),
		GetX(),
		GetY(),
		GetZ(),
		Direction(),
		this
	);
	if (unit) {
		if ((int) unit->m_vid->m_sprClass == 21) {
			static_cast<ENGINE*>(unit)->m_unk0xf4 = m_unk0x90;
		}

		--m_queueLen;
		for (int i = m_unk0x47c - 1; i < m_queueLen; ++i) {
			m_queue[i] = m_queue[i + 1];
			m_buildTicks[i] = m_buildTicks[i + 1];
			m_unk0x2ec[i] = m_unk0x2ec[i + 1];
		}
		m_unk0x47c = 0;

		unit->m_flag |= 1;
		Map->ScriptRun(EvFunctionNumber[23], unit, 0, 0);
		unit->PlaySFX(105);

		if (m_queueLen || (m_actions.m_n && m_actions.m_data[m_actions.m_n - 1].m_cmd != 73)) {
			BuildNextUnit();
		}
		else {
			if ((int) unit->m_vid->m_sprClass == 21) {
				static_cast<ENGINE*>(unit)->m_unk0xdc = 1;
			}
			++m_unk0x90;
		}
	}
	else {
		int idx = (unsigned short) m_queue[m_unk0x47c - 1];
		MYERROR::Error(
			::Error,
			// STRING: ALIEN 0x4825a8
			"SPRITE %i",
			10,
			g_depoCantCreate,
			idx,
			m_vid ? m_vid->m_idx : -1
		);
	}
	return 0;
}
