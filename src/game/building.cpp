#include "game/building.h"

#include <stdlib.h>

#include "game/map.h"
#include "video/vid.h"
#include "video/vid_exdata.h"
#include "world/hash_map.h"

#include "game/const.h"

// FUNCTION: ALIEN 0x44ca00
BUILDING::BUILDING(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent)
	: UNIT(p_vid, p_x, p_y, p_z, p_dir, p_parent)
{
	m_unk0x9c = 0;
	m_buildVid = 0;
	m_unk0xa0 = 0;
}

// FUNCTION: ALIEN 0x44ca50
void BUILDING::MoveTact()
{
	if (m_vid->m_idx == 104)
		AddHpPerSecond(Const->m_buildingAddHp);
}

// FUNCTION: ALIEN 0x44ca70
decomp_intptr BUILDING::Action(int p_action, int p_a, int p_b, int p_c)
{
	switch (p_action) {
	case 0x49:
		m_actions.Insert(ACT(p_action, p_a, p_b, p_c));
		Action(0x82, 0, 0, 0);
		return 0;
	case 0x46:
		if ((m_flag & 0x7c) == 0x40 && m_buildVid) {
			m_actions.Insert(ACT(0x23, m_buildVid->m_idx, 0, 0));
			SetCommand(0, 0);
			return 0;
		}
		return UNIT::Action(p_action, p_a, p_b, p_c);
	case 0x23: {
		VID* v;
		if (p_a == 0)
			p_a = Action(0x3b, 4, 0, 0);
		if ((p_a > 0 && 0 != (m_flag & 0x7c)) || p_a < 0 || p_a >= Map->m_noVid ||
			!(v = Map->m_vids[p_a]))
			return 0;
		m_buildVid = v;
		m_unk0x94 = p_b;
		m_unk0x98 = p_c;
		m_unk0x50 = v->m_exData->m_buildTime;
		SetCommand(0x10, 0);
		if (m_ani < 0xf) {
			ChangeAnimation(1);
			return 0;
		}
		return 0;
	}
	case 0x82: {
		if (m_ani < 0xf) {
		if ((m_flag & 0x7c) == 0x40)
			ChangeAnimation(1);
		else if (m_ani != 0xa)
			ChangeAnimation(0);
		if ((m_flag & 0x7c) != 0x40 || m_unk0x50 || !m_buildVid)
			return 0;
		float fx = (float) m_unk0x94;
		float fy = (float) m_unk0x98;
		if (m_unk0x94 == 0 && m_unk0x98 == 0) {
			fx = m_x;
			fy = m_y;
		}
		if (m_unk0x94 < 0) {
			fx = m_x;
			int r = -m_unk0x94;
			r = rand() % (r + 1);
			fx = fx - m_unk0x94 - 2 * r;
		}
		if (m_unk0x98 < 0) {
			fy = m_y;
			int r = -m_unk0x98;
			r = rand() % (r + 1);
			fy = fy - m_unk0x98 - 2 * r;
		}
		float fz = m_z;
		if (!Hash->CanPlace(m_buildVid, fx, fy, fz)) {
			SetCommand(0, 0);
			SPRITE* s = Map->CreateSprite(m_buildVid, fx, fy, (float) GetZ(), Direction(), this);
			if (s)
				Action(0x4b, (int) s, 0, 0);
			m_buildVid = 0;
			ChangeAnimation(0);
		}
		}
		return 0;
	}
	case 0x50:
		if (!ActionStackHaveCommand(0x49))
			m_actions.Insert(ACT(0x49, 0, 0, 0));
		UNIT::Action(p_action, p_a, p_b, p_c);
		return 0;
	case 0xf:
		return UNIT::Action(p_action, p_a, p_b, p_c);
	}
	return UNIT::Action(p_action, p_a, p_b, p_c);
}
