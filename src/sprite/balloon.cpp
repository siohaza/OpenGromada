#include "sprite/balloon.h"

#include <math.h>

#include "game/building.h"
#include "game/const.h"
#include "game/engine.h"
#include "game/gametime.h"
#include "game/map.h"
#include "video/vid.h"
#include "video/vid_exdata.h"
#include "world/hash_map.h"

// FUNCTION: ALIEN 0x44dd80
BALLOON::BALLOON(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent)
	: PLANE(p_vid, p_x, p_y, p_z, p_dir, p_parent)
{
	m_landingState = 0;
	m_unk0x90 = 0;
}

// FUNCTION: ALIEN 0x44ddd0
void BALLOON::MoveToNearestBase()
{
	SPRITE* best = 0;
	HASH_MAP* h = Hash;
	if (!h->m_list.m_n)
		return;
	int i = h->m_list.m_n - 1;
	h->m_iter = i;
	SPRITE* s = (SPRITE*) h->m_list.m_data[i];
	if (!s)
		return;
	for (;;) {
		if (s->m_vid->m_unk0x5c == m_vid && !((s->m_flag ^ m_flag) & 0x1800) &&
			(!s->m_child || s->m_child->m_vid != s->m_vid->m_unk0x5c) && !((ENGINE*) s)->m_unk0xd8) {
			if (!best)
				best = s;
			else {
				float tx = s->m_x;
				float a = (float) fabs(tx - m_x);
				float ty = s->m_y;
				float b = (float) fabs(ty - m_y);
				float d1;
				if (a > b) {
					d1 = b;
					d1 *= 0.5f;
					d1 += a;
				}
				else {
					d1 = a;
					d1 *= 0.5f;
					d1 += b;
				}
				float ux = best->m_x;
				float c = (float) fabs(ux - m_x);
				float uy = best->m_y;
				float d = (float) fabs(uy - m_y);
				float d2;
				if (c > d) {
					d2 = d;
					d2 *= 0.5f;
					d2 += c;
				}
				else {
					d2 = c;
					d2 *= 0.5f;
					d2 += d;
				}
				if (d1 < d2)
					best = s;
			}
		}
		h = Hash;
		if (h->m_iter > h->m_list.m_n)
			h->m_iter = h->m_list.m_n;
		if (--h->m_iter < 0)
			break;
		s = (SPRITE*) h->m_list.m_data[h->m_iter];
		if (!s)
			break;
	}
	if (best) {
		((ENGINE*) best)->m_unk0xd8 = 1;
		Move(best);
	}
}

// FUNCTION: ALIEN 0x44def0
void BALLOON::ConnectToBase()
{
	SPRITE* goal = m_goal;
	if (!m_goal ? 1 : m_goal->m_vid->m_linkVid != m_vid ? 1 : ((m_goal->m_flag ^ m_flag) & 0x1800)) {
		ChangeAnimation(15);
		return;
	}
	if (goal->m_child && goal->m_child->m_vid == goal->m_vid->m_linkVid) {
		SetCommand(0, 0);
		m_landingState = 1;
		return;
	}
	m_unk0x24 = 0.0f;
	ChangeCoor(m_goal->X() + m_goal->m_vid->m_unk0x4c, m_goal->Y() + m_goal->m_vid->m_unk0x50,
		m_goal->Z() + m_goal->m_vid->m_unk0x54);
	m_unk0x8c = m_goal->Action(94, 0, 0, 0);
	m_goal->AddLink(this);
	((ENGINE*) m_goal)->m_unk0xd8 = 1;
	SetCommand(0, 0);
}

// FUNCTION: ALIEN 0x44dfb0
void BALLOON::ZSpeedInitialization()
{
	float groundZ = Map->GetGroundZ_vid(m_vid, m_x, m_y);
	if (m_parent)
		m_landingState = 0;
	else if (m_goal && m_goal->m_vid->m_linkVid == m_vid && !((m_goal->m_flag ^ m_flag) & 0x1800))
		((ENGINE*) m_goal)->m_unk0xd8 = 1;
	switch ((unsigned char) m_landingState) {
	case 2: {
		m_unk0x24 = -m_vid->m_unk0x30;
		m_flag &= ~0x80u;
		m_speed = 0.0f;
		SPRITE* goal = m_goal;
		if (!goal || goal->m_vid->m_linkVid != m_vid || ((goal->m_flag ^ m_flag) & 0x1800) || goal->HaveFightLink()) {
			m_landingState = 1;
		}
		else {
			ChangeCoor(m_goal->X(), m_goal->Y(), Z());
			unsigned int duration = m_vid->m_aniDuration[m_ani];
			unsigned int time = CurrentTime - PrevCurrentTime;
			if (time <= duration)
				time = duration;
			Rotate(Goal()->Direction(), time);
			if (Z() <= m_goal->Z() + m_goal->m_vid->m_unk0x54)
				ConnectToBase();
		}
		break;
	}
	case 1:
		m_unk0x24 = m_vid->m_unk0x30;
		m_flag &= ~0x80u;
		m_speed = 0.0f;
		if (Z() >= groundZ + m_vid->m_unk0x60) {
			m_landingState = 0;
			m_flag |= 0x80;
		}
		break;
	case 0:
		if (!m_parent) {
			groundZ += m_vid->m_unk0x60;
			if (Z() < groundZ - 10.0f) {
				m_unk0x24 = m_vid->m_unk0x30;
				break;
			}
			if (Z() > groundZ + 10.0f) {
				m_unk0x24 = -m_vid->m_unk0x30;
				break;
			}
		}
		m_unk0x24 = 0.0f;
		break;
	}
}

// FUNCTION: ALIEN 0x44e1d0
int BALLOON::IsBalloonMoveFinished()
{
	SPRITE* goal = m_goal;
	if (goal && (float) fabs(goal->m_x - m_x) < 10.0f && (float) fabs(goal->m_y - m_y) < 10.0f)
		return 1;
	return (m_flag & 0x4000) && (m_flag & 0x8000);
}

// FUNCTION: ALIEN 0x44e220
void BALLOON::CheckFlightProperties()
{
	if (m_parent) {
		if ((CurrentTime & 0xfffffc00) > m_tactTime)
			AddAmmoTick(Const->m_balloonAddAmmo);
		SPRITE* goal = m_goal;
		if (!goal
			|| (m_parent->m_goal == goal
				&& ((m_parent->m_flag & 0x7c) == 108 || (m_parent->m_flag & 0x7c) == 104))
			|| NearDistanceTo(goal->m_x - m_parent->m_x, goal->m_y - m_parent->m_y)
				>= m_parent->m_vid->m_exData->m_unk0x18
			|| m_unk0x50) {
			unsigned int duration = m_vid->m_aniDuration[m_ani];
			unsigned int dt = CurrentTime - PrevCurrentTime > duration ? CurrentTime - PrevCurrentTime : duration;
			if (!Rotate(m_parent->Direction(), dt).m_dir)
				ChangeAnimation(0); // ANI_STAND
		} else {
			if ((m_flag & 0x7c) == 0x20) {
				SetCommand(3, m_goal);
				m_unk0x90 = 1;
			}
			DestroyLink(m_parent->m_vid->m_unk0x284);
			m_parent->m_child = 0;
			((ENGINE*) m_parent)->m_unk0xd8 = 0;
			SPRITE* target = m_goal;
			SPRITE* best = m_unk0x6c;
			if (best && best != target) {
				if (NearDistanceTo(best->m_x - m_x, best->m_y - m_y)
					< m_vid->m_exData->m_unk0x18)
					Attack(m_unk0x6c);
			}
			if (target && target->m_vid == EmptyVid) {
				m_parent->SetCommand(0, 0);
				float z = target->m_z;
				float y = target->m_y;
				float x = target->m_x;
				SPRITE* marker = new SPRITE(EmptyVid, x, y, z, ANGLE(0), 0);
				Attack(marker);
			}
			ChangeAnimation(2); // ANI_GO
			m_flag &= ~0x80;
			m_landingState = 1;
			m_parent = 0;
		}
		return;
	}

	SPRITE* best = m_unk0x6c;
	if (best && best != m_goal) {
		float dx = (float) fabs(best->m_x - m_x);
		float dy = (float) fabs(best->m_y - m_y);
		float dist = dx > dy ? dx + dy * 0.5f : dx * 0.5f + dy;
		if (dist < m_vid->m_exData->m_unk0x14 && m_ammo / 64 > 0)
			Attack(m_unk0x6c);
	}
	if (m_ammo / 64 <= 0 || !m_goal) {
		SPRITE* goal = m_goal;
		if (goal) {
			VID* linkVid = goal->m_vid->m_linkVid;
			if (linkVid == m_vid && !((m_flag ^ goal->m_flag) & 0x1800)) {
				SPRITE* child = goal->m_child;
				if (!child || child->m_vid != linkVid)
					goto keepPad;
			}
		}
		if (m_goal)
			SetCommand(0, 0);
		MoveToNearestBase();
	}
keepPad:
	{
		SPRITE* goal = m_goal;
		if (goal && goal->m_vid->m_linkVid == m_vid
			&& !((m_flag ^ goal->m_flag) & 0x1800)) {
			SPRITE* child = goal->m_child;
			if (child && child->m_vid == m_goal->m_vid->m_linkVid) {
				SetCommand(0, 0);
				MoveToNearestBase();
			}
		}
	}
	if (m_landingState != 2 && (m_flag & 0x7c) == 4) {
		if (IsBalloonMoveFinished()) {
			SPRITE* goal = m_goal;
			if (goal) {
				VID* linkVid = goal->m_vid->m_linkVid;
				if (linkVid == m_vid && !((m_flag ^ goal->m_flag) & 0x1800)) {
					SPRITE* child = goal->m_child;
					if (!child || child->m_vid != linkVid) {
						m_landingState = 2;
						ZSpeedInitialization();
					}
				}
			}
		}
	}
}

// FUNCTION: ALIEN 0x44e5a0
int BALLOON::FlightToTargetAdditionalActions()
{
	SPRITE* goal = m_goal;
	if (!goal || goal->m_vid->m_unk0x5c != m_vid || ((goal->m_flag ^ m_flag) & 0x1800)) {
		unsigned int dur = m_vid->m_aniDuration[m_ani];
		unsigned int dt = CurrentTime - PrevCurrentTime;
		if (dt <= dur)
			dt = dur;
		m_unk0x04 = AttackTact(dt);
	}
	if (m_unk0x90 && m_goal && (float) fabs(m_goal->m_x - m_x) < 10.0f &&
		(float) fabs(m_goal->m_y - m_y) < 10.0f) {
		ChangeAnimation(0xf);
	}
	else {
		SPRITE* g = m_goal;
		if (g && g->m_vid->m_unk0x5c == m_vid && !((g->m_flag ^ m_flag) & 0x1800) &&
			m_ammo / 64 > 0 && (m_unk0x8c & 1)) {
			SPRITE* enemy = SeekEnemy();
			if (enemy)
				return SetCommand(4, enemy);
		}
	}

}

// FUNCTION: ALIEN 0x44e680
int BALLOON::SetCommand(int p_cmd, SPRITE* p_goal)
{
	SPRITE* goal = m_goal;
	m_unk0x90 = 0;
	if (goal && goal->m_vid->m_unk0x5c == m_vid && !((goal->m_flag ^ m_flag) & 0x1800))
		*(int*) ((char*) goal + 0xd8) = 0;
	if (p_goal && p_goal->m_vid->m_unk0x5c == m_vid && !((p_goal->m_flag ^ m_flag) & 0x1800))
		*(int*) ((char*) p_goal + 0xd8) = 1;
	return SPRITE::SetCommand(p_cmd, p_goal);
}

// FUNCTION: ALIEN 0x44e6f0
void BALLOON::MoveTact()
{
	float x;
	float y;
	float z;
	MoveTactCalcCoor(&x, &y, &z);
	if (m_landingState) {
		ChangeCoor(X(), Y(), z);
		return;
	}
	if (m_goal && m_speed != 0.0f) {
		Rotate(GlideDirection(DirectionTo(Goal())
			+ (m_speed < 0.0f ? 0x80 : 0)), CurrentTime - PrevCurrentTime);
	}
	if ((m_x != x || m_y != y) && !CanPlaceWithCrushAndGlide(&x, &y, &z)) {
		MoveTactMapLimit(x, y);
		ChangeCoor(x, y, z);
	}
}
