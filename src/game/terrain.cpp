#include "game/terrain.h"

#include "game/gametime.h"
#include "game/map.h"
#include "util/stream.h"
#include "video/vid.h"

extern VID* EmptyVid;

// FUNCTION: ALIEN 0x44bd60
TERRAIN::TERRAIN(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent)
	: SPRITE(p_vid, p_x, p_y, p_z, p_dir, p_parent)
{
	m_repairProgress = -1;
	m_unk0x74 = 0;
}

// STUB: ALIEN 0x44bdb0
SPRITE* TERRAIN::AskCell(float p_x, float p_y)
{
	if (p_x < 0.0f || p_x >= Map->m_w || p_y < 0.0f || p_y >= Map->m_h) {
		return this;
	}
	if (Map->GetGroundZ_vid(m_vid, m_x, m_y) < (double) m_z) {
		return CanPlace(p_x, p_y, Z());
	}
	return this;
}

// FUNCTION: ALIEN 0x44be50
void TERRAIN::AddHpPerSecond(int p_hp)
{
	if ((CurrentTime & 0xfffffc00) > PrevCurrentTime) {
		int hp = m_unk0x54;
		if (hp > 0) {
			int v = p_hp + hp;
			if (v > m_vid->m_maxHp[(m_flag >> 11) & 3]) {
				v = m_vid->m_maxHp[(m_flag >> 11) & 3];
			}
			ChangeHp(v);
		}
	}
}

// FUNCTION: ALIEN 0x44bea0
decomp_intptr TERRAIN::Action(int p_action, decomp_intptr p_a, decomp_intptr p_b, decomp_intptr p_c)
{
	switch (p_action) {
	case 0xc8:
		SPRITE::Action(0xc8, p_a, p_b, p_c);
		((STREAM*) p_a)->Read(&p_c, (p_b > 7) + 1);
		break;
	case 0x56:
		Repair(1);
		break;
	default:
		return SPRITE::Action(p_action, p_a, p_b, p_c);
	}
	return 0;
}

// FUNCTION: ALIEN 0x44bf10
int TERRAIN::Repair(int p_full)
{
	ChangeHp(m_vid->m_maxHp[(m_flag >> 11) & 3]);
	VID* vid = m_vid;
	VID* linkVid = vid->m_unk0x5c;
	if (linkVid && (!m_child || m_child->m_vid != linkVid) && p_full) {
		int army;
		int rebuild = 1;
		do {
			if (vid->m_idx == 35) {
				VID* v40 = (Map->m_noVid > 40 && Map->m_vids[40]) ? Map->m_vids[40] : EmptyVid;
				army = (m_flag >> 11) & 3;
				rebuild = v40->m_entitiesNumber[army];
				VID* v35 = (Map->m_noVid > 35 && Map->m_vids[35]) ? Map->m_vids[35] : EmptyVid;
				if (rebuild >= v35->m_entitiesNumber[army]) {
					break;
				}
			}
			CreateLink();
			Map->CreateSprite(Map->Vid(590), (float) GetX(), (float) GetY(), (float) GetZ(), ANGLE(0), this);
			return 1;
		} while (0);
	}
	if (m_child && m_child->m_vid == linkVid) {
		m_child->Action(0x56, 0, 0, 0);
	}
	return 1;
}
