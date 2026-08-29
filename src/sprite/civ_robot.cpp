#include "sprite/civ_robot.h"
#include <math.h>
#include "game/region.h"

#include "game/gametime.h"
#include "world/hash_map.h"

#include <math.h>

#include "game/map.h"
#include "misc.h"
#include "util/polar.h"
#include "util/string.h"

#include <stdlib.h>

#include "sprite/sprite.h"
#include "video/vid.h"

static inline unsigned int AniTactTime(unsigned int p_dur)
{
	return CurrentTime - PrevCurrentTime > p_dur ? CurrentTime - PrevCurrentTime : p_dur;
}

static inline void PtrSpriteAssign(PTR_SPRITE* p_dst, SPRITE* p_src)
{
	if (p_src)
		++p_src->m_noRef;
	if (p_dst->m_ptr)
		p_dst->m_ptr->Release();
	p_dst->m_ptr = p_src;
}

inline ANGLE::ANGLE(float p_x, float p_y)
{
	*this = Decart2Polar_f(p_x, p_y);
}

inline int SPRITE::IsXYCross(const VID* p_vid, float p_x, float p_y) const
{
	return (float) fabs(m_x - p_x) < m_vid->m_unk0x384 + p_vid->m_unk0x384
		&& (float) fabs(m_y - p_y) < m_vid->m_unk0x388 + p_vid->m_unk0x388;
}

static inline void CommitMove(CIV_ROBOT* p_robot, float p_x, const float& p_y, const float& p_z)
{
	p_robot->ChangeCoor(p_x, p_y, p_z);
}

// GLOBAL: ALIEN 0x484930
int CIV_ROBOT::RobotBuildingVids[16] = {
	170, 803, 805, 806, 807, 809, 810, 813, 846, 848, 861, 863, 864, 874, 875, 895
};

// GLOBAL: ALIEN 0x484970
char g_depoCantCreate[] = "Depo can't create unit";

// FUNCTION: ALIEN 0x44a590
CIV_ROBOT::CIV_ROBOT(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent)
	: CREATURE(p_vid, p_x, p_y, p_z, p_dir, p_parent)
	, m_target()
{
	m_state = 0;
	m_target = 0;
	m_unk0xa0 = 0;
}

// FUNCTION: ALIEN 0x44a650
void* CIV_ROBOT::ScalarDeletingDestructor(unsigned int p_flags)
{
	CIV_ROBOT* result = this;
	this->~CIV_ROBOT();
	if (p_flags & 1)
		operator delete(result);
	return result;
}

// FUNCTION: ALIEN 0x44a6b0
int CIV_ROBOT::IsRobotBuilding(const SPRITE* p_sprite)
{
	if (!p_sprite)
		return 0;
	int idx = p_sprite->m_vid->m_idx;
	const int* p = RobotBuildingVids;
	while (p < RobotBuildingVids + 16) {
		if (idx == *p)
			return 1;
		++p;
	}
	return 0;
}

// FUNCTION: ALIEN 0x44a6e0
SPRITE* CIV_ROBOT::FindRobotBuilding()
{
	SPRITE* result = 0;
	for (SPRITE* i = Hash->FirstInBox(X() - 200, Y() - 133,
									  X() + 200, Y() + Z() + 133);
		 i; i = Hash->NextInBox()) {
		VID* vid = i->m_vid;
		if ((vid->m_unk0x0c & 2) && vid->m_sprClass == 1) {
			if (IsRobotBuilding(i)) {
				SPRITE* region;
				if (!m_region
					|| (region = FindRegion(i->X(), i->Y())) != 0
						   && m_region->m_vid == region->m_vid) {
					result = i;
					if (rand() % 4 == 0)
						break;
				}
			}
		}
	}
	return result;
}

// FUNCTION: ALIEN 0x44a7d0
void CIV_ROBOT::PathIsBlocked()
{
	m_speed = 0;
	if (!m_turn)
		m_turn = (rand() % 2) ? 10 : -10;
	if ((m_flag & 0x7c) == 4 && !(rand() % 9)) {
		Stop();
		m_state = 0;
	}
}

// STUB: ALIEN 0x44a840
void CIV_ROBOT::MoveTact()
{
	if ((m_unk0x90 & 1) && !m_region) {
		m_unk0x90 &= ~1u;
		m_region = FindRegion(X(), Y());
	}

	float x;
	float y;
	float z;
	MoveTactCalcCoor(&x, &y, &z);
	if (m_vid->m_flag & 0x200)
		z = Map->GetGroundZ_vid(m_vid, x, y);
	if (z - Z() > m_vid->m_unk0x64 || Z() - z > *(float*) &m_vid->m_unk0x68)
		goto blocked;

	{
		REGION* region = (REGION*) m_region;
		if (region && !region->IsInsideXY(x, y)) {
			SPRITE* found = FindRegion(x, y);
			if (!found || m_region->m_vid != found->m_vid)
				goto blocked;
			m_region = found;
		}
	}

	if (x >= 0.0f && x < Map->m_w && y >= 0.0f && y < Map->m_h) {
		float bz;
		SPRITE* blocker = (SPRITE*) CanPlaceWithCrush(x, y, Z());
		if (!blocker) {
			ChangeCoor(x, y, z);
			return;
		}
		if (IsXYCross(blocker->m_vid, blocker->m_x, blocker->m_y)) {
			if ((bz = blocker->m_z, m_vid->m_unk0x24 + m_z >= bz)
				&& bz + blocker->m_vid->m_unk0x24 >= m_z) {
				ChangeCoor(x, y, z);
				return;
			}
		}

		if (IsRobotBuilding(blocker) && m_state == 14) {
			unsigned char aimDir = (unsigned char) (blocker->m_dir + 0x80);
			ANGLE approach;
			float dy = m_y - blocker->m_y;
			float dx = m_x - blocker->m_x;
			AngleAssign(&approach, Decart2Polar_f(dx, dy));
			unsigned char d1 = (unsigned char) (approach.m_dir - aimDir);
			unsigned char d2 = (unsigned char) (aimDir - approach.m_dir);
			unsigned char delta = (d1 >= d2) ? d2 : d1;
			if (delta < 30) {
				ChangeCoor(x, y, z);
				return;
			}
		}

		if (blocker->m_vid == m_vid
			&& (m_state == 15 || ((CIV_ROBOT*) blocker)->m_state == 15)) {
			CommitMove(this, x, y, z);
			return;
		}
	}

blocked:
	PathIsBlocked();
}

// FUNCTION: ALIEN 0x44ab40
void CIV_ROBOT::DeletePointerToSprite(SPRITE* p_sprite)
{
	if (m_target == p_sprite)
		m_target = 0;
	CREATURE::DeletePointerToSprite(p_sprite);
}

// FUNCTION: ALIEN 0x44abc0
void CIV_ROBOT::ChangeAnimation(int p_ani)
{
	SPRITE* child = m_child;
	if (child && child->m_vid == m_vid->m_unk0x5c && child->m_ani != p_ani &&
		child->m_noCadr >= child->m_endCadr)
		child->ChangeAnimation(p_ani);
}

// FUNCTION: ALIEN 0x44abf0
void CIV_ROBOT::RotateHead(ANGLE p_dir)
{
	SPRITE* child = m_child;
	if (child) {
		VID* vid = m_vid;
		if (child->m_vid == vid->m_linkVid && child->m_noCadr >= child->m_endCadr) {
			Child()->Rotate(p_dir, AniTactTime(vid->m_aniDuration[m_ani]));
		}
	}
}

// FUNCTION: ALIEN 0x44ac50
decomp_intptr CIV_ROBOT::Action(int p_action, int p_a, int p_b, int p_c)
{
	switch (p_action) {
	case 9: {
		if (m_ani == 8)
			break;
		SPRITE* child = m_child;
		if (child && child->m_vid == m_vid->m_linkVid)
			child->ChangeAnimation(9);
		else
			ChangeAnimation(9);
		break;
	}
	case 34: {
		SPRITE* b = (SPRITE*) p_a;
		if (!b)
			break;
		float by = b->m_y;
		float c = ANGLE::CosTable[b->m_dir];
		float bx = b->m_x;
		float s = ANGLE::SinTable[b->m_dir];
		float bz = b->Z() + b->m_vid->m_unk0x64;
		Move(new SPRITE(EmptyVid, s * 4.0f + bx, by - c * 4.0f, bz, ANGLE(0), 0));
		break;
	}
	case 85:
		if (p_a > 0) {
			for (SPRITE* i = Hash->FirstInBox(X() - 150.0f, Y() - 150.0f,
					 X() + 150.0f, Y() + 300.0f);
				 i; i = Hash->NextInBox()) {
				if (i->m_vid->m_sprClass == 20) {
					if (SPRITE::NearDistanceTo(i->m_x - m_x, i->m_y - m_y) < 150.0f) {
						((CIV_ROBOT*) i)->m_unk0xa0 = 1;
						((CIV_ROBOT*) i)->m_unk0xa4 = 1;
					}
				}
			}
		}
		return UNIT::Action(p_action, p_a, p_b, p_c);
	case 130: {
		if (m_ani >= 15)
			break;
		if ((m_unk0x90 & 1) && !m_region) {
			m_unk0x90 &= ~1u;
			m_region = FindRegion(X(), Y());
		}
		if ((m_flag & 0x4000) && (m_flag & 0x8000))
			Stop();
		if (m_turn) {
			if (m_turn > 0) {
				Rotate(Direction() - 32, AniTactTime(m_vid->m_aniDuration[m_ani]));
			}
			else {
				Rotate(Direction() + 32, AniTactTime(m_vid->m_aniDuration[m_ani]));
			}
			m_turn = m_turn < 0 ? m_turn + 1 : m_turn - 1;
		}
		else if (m_goal && (m_flag & 0x7c) == 4) {
			float dy = m_goal->m_y - m_y;
			float dx = m_goal->m_x - m_x;
			ANGLE d(dx, dy);
			if (m_speed < 0.0f)
				d.m_dir += 0x80;
			unsigned int duration = m_vid->m_aniDuration[m_ani];
			unsigned int dt = CurrentTime - PrevCurrentTime;
			if (CurrentTime - PrevCurrentTime <= duration)
				dt = duration;
			if (!Rotate(d, dt).m_dir)
				ChangeAnimation(2);
		}
		if (m_ani == 13)
			ChangeAnimation(0);
		int cmd = m_flag & 0x7c;
		if (cmd == 4 || cmd == 12)
			break;
		if (CurrentTime - (CurrentTime & 0x7ff) > m_tactTime)
			m_unk0xa4 = 1;
		if (m_unk0xa4) {
			m_unk0xa4 = 0;
			SPRITE* found = Map->FindNearestSprite(0x9015, X(), Y(), 350.0f, 0);
			PtrSpriteAssign(&m_target, found);
			if (m_state == 14) {
				if (!(m_flag & 0x80)) {
					if (!m_actions.m_n || m_actions.m_data[m_actions.m_n - 1].m_cmd == 73)
						m_state = 15;
				}
			}
			else if (m_state == 15 && !(rand() % 5)) {
				m_state = 5;
				float z = m_z;
				float y = m_y;
				float x = m_x;
				Move(new SPRITE(EmptyVid, x, y + 50.0f, z, ANGLE(0), 0));
			}
			else if (m_state == 7 && rand() % 2) {
				m_state = 7;
			}
			else if (m_unk0xa0) {
				if (rand() % 5)
					m_state = 7;
				else
					m_state = rand() % 3 ? 12 : 10;
			}
			else if (m_target.m_ptr && ((m_state != 9 && !(rand() % 3)) || !(rand() % 3)))
				m_state = 9;
			else if (m_target.m_ptr && !(rand() % 2))
				m_state = 11;
			else if (m_state == 5 && rand() % 3)
				m_state = 5;
			else if (m_state == 1 && rand() % 6)
				m_state = 1;
			else if (m_state == 4 && !(rand() % 2))
				m_state = 4;
			else if (!(rand() % 7))
				m_state = 4;
			else if (!(rand() % 6)) {
				PlaySFX(129);
				m_state = 3;
			}
			else if (!(rand() % 5)) {
				PlaySFX(129);
				m_state = 2;
			}
			else
				m_state = rand() % 5 ? 5 : 14;
			m_unk0xa0 = 0;
		}
		switch (m_state) {
		case 0:
			if (m_flag & 0x80)
				Stop();
			RotateHead(*(ANGLE*) &m_dir);
			break;
		case 13: {
			if (m_flag & 0x80)
				Stop();
			ANGLE d;
			d.m_dir = Direction().m_dir - 32;
			ChangeDirection(d);
			RotateHead(*(ANGLE*) &m_dir);
			break;
		}
		case 12:
			if (m_flag & 0x80)
				Stop();
			ChangeAnimation(7);
			break;
		case 11: {
			if (m_flag & 0x80)
				Stop();
			if (!m_target.m_ptr)
				break;
			SPRITE* t = (SPRITE*) (const char*) *(STRING*) &m_target;
			float dy = t->m_y - m_y;
			float dx = t->m_x - m_x;
			RotateHead(ANGLE(dx, dy));
			break;
		}
		case 2: {
			if (m_flag & 0x80)
				Stop();
			SPRITE* child = m_child;
			if (!child || child->m_vid != m_vid->m_linkVid)
				break;
			ANGLE d;
			d.m_dir = Child()->Direction().m_dir - 32;
			RotateHead(d);
			break;
		}
		case 3: {
			if (m_flag & 0x80)
				Stop();
			SPRITE* child = m_child;
			if (!child || child->m_vid != m_vid->m_linkVid)
				break;
			ANGLE d;
			d.m_dir = Child()->Direction().m_dir + 32;
			RotateHead(d);
			break;
		}
		case 4: {
			if (m_flag & 0x80)
				Stop();
			SPRITE* child = m_child;
			if (!child || child->m_vid != m_vid->m_linkVid)
				break;
			Child()->Rotate(Direction(), AniTactTime(m_vid->m_aniDuration[m_ani]));
			ChangeAnimation(12);
			break;
		}
		case 1: {
			SPRITE* g = m_goal;
			if (!g)
				break;
			float ax = (float) fabs(g->m_x - m_x);
			float ay = (float) fabs(g->m_y - m_y);
			float dist;
			if (ax > ay)
				dist = ax + ay * 0.5f;
			else
				dist = ax * 0.5f + ay;
			if (dist < 150.0f) {
				SetCommand(0, 0);
				if (!(rand() % 3)) {
					Rotate(ANGLE((unsigned char) Random(255)), AniTactTime(m_vid->m_aniDuration[m_ani]));
				}
				if (!(rand() % 10)) {
					ChangeAnimation(11);
					break;
				}
				if (!(rand() % 10)) {
					ChangeAnimation(9);
					break;
				}
				if (rand() % 10) {
					ChangeAnimation(0);
					break;
				}
				ChangeAnimation(6);
				break;
			}
			Move(m_goal);
			break;
		}
		case 5:
			if (!(m_flag & 0x80))
				StartMove();
			if (m_ani == 2 && !(rand() % 3)) {
				Rotate(Direction() - 32, AniTactTime(m_vid->m_aniDuration[m_ani]));
			}
			else if (m_ani == 2 && !(rand() % 3)) {
				Rotate(Direction() + 32, AniTactTime(m_vid->m_aniDuration[m_ani]));
			}
			else
				ChangeAnimation(2);
			RotateHead(*(ANGLE*) &m_dir);
			break;
		case 14:
			if (m_goal
				|| m_actions.m_n && m_actions.m_data[m_actions.m_n - 1].m_cmd != 73) {
				if (!(m_flag & 0x80)) {
					if (!m_actions.m_n || m_actions.m_data[m_actions.m_n - 1].m_cmd == 73)
						m_state = 15;
				}
				RotateHead(*(ANGLE*) &m_dir);
				break;
			}
			else {
				SPRITE* b = FindRobotBuilding();
				if (b) {
					float bz = b->m_z;
					float by = b->m_y;
					float c = ANGLE::CosTable[b->m_dir];
					float bx = b->m_x;
					float s = ANGLE::SinTable[b->m_dir];
					SPRITE* beacon = new SPRITE(EmptyVid, bx - s * 32.0f,
						c * 32.0f + by, bz, ANGLE(0), 0);
					Move(beacon);
					m_actions.InsertFirst(ACT(34, (int) b, 0, 0));
					break;
				}
				m_unk0xa4 = 1;
				break;
			}
		case 8: {
			if (!(m_flag & 0x80))
				StartMove();
			VID* vid = m_vid;
			if (m_speed >= vid->m_unk0x2c)
				m_speed = vid->m_unk0x2c + vid->m_unk0x2c;
			SPRITE* child = m_child;
			if (!child || child->m_vid != vid->m_linkVid)
				break;
			Child()->Rotate(Direction(), AniTactTime(vid->m_aniDuration[m_ani]));
			ChangeAnimation(11);
			break;
		}
		case 7: {
			if (!(m_flag & 0x80))
				StartMove();
			VID* vid = m_vid;
			if (m_speed >= vid->m_unk0x2c)
				m_speed = vid->m_unk0x2c + vid->m_unk0x2c;
			if (m_target.m_ptr) {
				Rotate(DirectionTo((SPRITE*) (const char*) *(STRING*) &m_target).Invert(), AniTactTime(vid->m_aniDuration[m_ani]));
			}
			SPRITE* child = m_child;
			if (!child || child->m_vid != m_vid->m_linkVid)
				break;
			Child()->Rotate(Direction(), AniTactTime(m_vid->m_aniDuration[m_ani]));
			ChangeAnimation(11);
			break;
		}
		case 9: {
			if (m_flag & 0x80)
				Stop();
			SPRITE* child = m_child;
			if (!child || child->m_vid != m_vid->m_linkVid)
				break;
			if (m_target.m_ptr) {
				Child()->Rotate(DirectionTo((SPRITE*) (const char*) *(STRING*) &m_target), AniTactTime(m_vid->m_aniDuration[m_ani]));
			}
			else {
				Child()->Rotate(Direction(), AniTactTime(m_vid->m_aniDuration[m_ani]));
			}
			if (rand() % 3)
				ChangeAnimation(9);
			else
				ChangeAnimation(11);
			break;
		}
		case 10: {
			if (m_flag & 0x80)
				Stop();
			SPRITE* child = m_child;
			if (!child || child->m_vid != m_vid->m_linkVid)
				break;
			if (m_target.m_ptr) {
				Child()->Rotate(DirectionTo((SPRITE*) (const char*) *(STRING*) &m_target), AniTactTime(m_vid->m_aniDuration[m_ani]));
			}
			else {
				Child()->Rotate(Direction(), AniTactTime(m_vid->m_aniDuration[m_ani]));
			}
			ChangeAnimation(6);
			break;
		}
		}
		break;
	}
	default:
		return CREATURE::Action(p_action, p_a, p_b, p_c);
	}
	return 0;
}
