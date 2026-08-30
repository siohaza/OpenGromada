#include "sprite/plane.h"

#include "game/gametime.h"
#include "game/map.h"
#include "sprite/plane_internal.h"
#include "util/angle.h"
#include "util/game_random.h"
#include "util/polar.h"
#include "video/vid.h"

#include <stdlib.h>

void PLANE_INTERNAL::RetailExactEmptyCheck(void* p_object)
{
	((PLANE*) p_object)->PLANE::CheckFlightProperties();
}

// FUNCTION: ALIEN 0x44ce60
PLANE::PLANE(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent)
	: UNIT(p_vid, p_x, p_y, p_z, p_dir, p_parent)
{
	StartMove();
	m_unk0x24 = m_vid->m_unk0x30;
}

// FUNCTION: ALIEN 0x44ceb0
decomp_intptr PLANE::Action(int p_action, decomp_intptr p_a, decomp_intptr p_b, decomp_intptr p_c)
{
	switch (p_action) {
	case 0x82:
		PlaneNextCommand(p_a, p_b);
		return 0;
	case 0x55:
		return TERRAIN::Action(0x55, p_a, p_b, p_c);
	case 0x21: {
		float fy = (float) p_b;
		float fx = (float) p_a;
		float z = Map->GetGroundZ_vid(m_vid, fx, fy) + 80.0f;
		Move(new SPRITE(EmptyVid, fx, fy, z, ANGLE(0), 0));
		return 0;
	}
	}
	return UNIT::Action(p_action, p_a, p_b, p_c);
}

// FUNCTION: ALIEN 0x44cfb0
int PLANE::FreeFlight()
{
	if (m_ani == 4) {
		if (GameRand() % 3) {
			ChangeDirection(ANGLE((char) (Direction().m_dir - 0x20)));
		}
		else {
			ChangeAnimation(2);
		}
	}
	else if (m_ani == 5) {
		if (GameRand() % 3) {
			ChangeDirection(ANGLE((char) (Direction().m_dir + 0x20)));
		}
		else {
			ChangeAnimation(2);
		}
	}
	else if (m_ani == 2) {
		if (GameRand() % 11 == 0) {
			if (GameRand() % 2 != 0) {
				ChangeAnimation(4);
			}
			else {
				ChangeAnimation(5);
			}
		}
	}
	else {
		ChangeAnimation(2);
	}
	if (GameRand() % 21 == 0 && (m_unk0x8c & 1)) {
		SPRITE* enemy = SeekEnemy();
		if (enemy) {
			return SetCommand(4, enemy);
		}
	}
	return 0;
}
// FUNCTION: ALIEN 0x44d0a0
void PLANE::CheckFlightProperties()
{
}

// FUNCTION: ALIEN 0x44d0b0
int PLANE::FlightToTargetAdditionalActions()
{
	unsigned int prev = PrevCurrentTime;
	unsigned int dur = m_vid->m_aniDuration[m_ani];
	unsigned int dt = CurrentTime - prev;
	if (dt <= dur) {
		dt = dur;
	}
	int result = AttackTact(dt);
	m_unk0x04 = result;
	if (m_unk0x8c & 1) {
		if (result != 6 || GameRand() % 21 == 0) {
			SPRITE* enemy = SeekEnemy();
			if (enemy) {
				return SetCommand(4, enemy);
			}
		}
	}
	return 0;
}
// FUNCTION: ALIEN 0x44d120
int PLANE::FlightToTarget()
{
	ANGLE dir = DirectionTo(m_goal);
	unsigned int dur = m_vid->m_aniDuration[m_ani];
	unsigned int dt = CurrentTime - PrevCurrentTime;
	if (dt <= dur) {
		dt = dur;
	}
	if (Rotate(dir, dt).m_dir == 0) {
		ChangeAnimation(2);
	}
	return FlightToTargetAdditionalActions();
}

// FUNCTION: ALIEN 0x44d1c0
void PLANE::ZSpeedInitialization()
{
	float z = m_z;
	float groundZ = Map->GetGroundZ_vid(m_vid, m_x, m_y);
	if (groundZ + m_vid->m_unk0x60 - 10.0f > z) {
		m_unk0x24 = m_vid->m_unk0x30;
		return;
	}
	z = m_z;
	groundZ = Map->GetGroundZ_vid(m_vid, m_x, m_y);
	groundZ += m_vid->m_unk0x60;
	if (groundZ + 10.0f < z) {
		m_unk0x24 = -m_vid->m_unk0x30;
	}
	else {
		m_unk0x24 = 0;
	}
}

// FUNCTION: ALIEN 0x44d250
int PLANE::WayBlocked()
{
	float y = m_y;
	float x = ANGLE::CosTable[m_dir];
	float s = m_x;
	float c = ANGLE::SinTable[m_dir];
	return AskCell(s + c * 128.0f, y - x * 128.0f) != 0;
}

// FUNCTION: ALIEN 0x44d2b0
int PLANE::FlightIfWayBlocked()
{
	float y = m_y;
	float x = ANGLE::CosTable[(unsigned char) (m_dir + 0x10)];
	float s = m_x;
	float c = ANGLE::SinTable[(unsigned char) (m_dir + 0x10)];
	if (AskCell(s + c * 128.0f, y - x * 128.0f)) {
		ChangeAnimation(5);
		return ChangeDirection(ANGLE((char) (Direction().m_dir + 0x10)));
	}
	ChangeAnimation(4);
	return ChangeDirection(ANGLE((char) (Direction().m_dir - 0x10)));
}

// FUNCTION: ALIEN 0x44d370
int PLANE::PlaneNextCommand(int p_a, int p_b)
{
	int result = m_ani;
	if (result < 0xf && result != 0xc) {
		if (result >= 7 && result != 0xa) {
			ChangeAnimation(0);
		}
		ZSpeedInitialization();
		CheckFlightProperties();
		if (!m_parent) {
			if (!(m_flag & 0x80)) {
				m_flag |= 0x80;
			}
			if (m_goal) {
				return FlightToTargetAdditionalActions();
			}
			return FreeFlight();
		}
		// The original returned m_parent here as a truthy int.
		return 1;
	}
	return result;
}
