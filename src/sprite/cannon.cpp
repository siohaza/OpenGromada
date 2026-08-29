#include "sprite/cannon.h"

#include <math.h>

#include "game/const.h"
#include "game/gametime.h"
#include "game/map.h"

#include "sprite/ex_sprite_data.h"
#include "video/vid.h"

#include "sprite/sprite.h"

// FUNCTION: ALIEN 0x447fe0
CANNON::CANNON(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent)
	: SPRITE(p_vid, p_x, p_y, p_z, p_dir, p_parent)
{
	m_unk0x70 |= 1;
	VID* vid = m_vid;
	if ((vid->m_flag & 0x400000) && vid->m_unk0x30 != 0.0f) {
		if (rand() % 2) {
			float za = m_vid->m_unk0x30;
			m_unk0x24 = ((float) rand() * za) * 3.0518509e-5f;
		} else {
			float za = m_vid->m_unk0x30;
			m_unk0x24 = ((float) rand() * za) * -0.000030518509f;
		}
	} else if (p_parent && p_parent->m_ani >= 15 && vid->m_unk0x60 < 0.0f) {
		double za = vid->m_unk0x30;
		m_unk0x24 = (float) za;
		double speed = p_parent->m_speed;
		m_speed = (m_exData ? m_exData->m_unk0x20 : vid->m_unk0x2c) + speed;
	} else {
		double za = vid->m_unk0x30;
		m_unk0x24 = (float) za;
	}
	if ((m_vid->m_flag & 0x400) && m_vid->m_unk0x2c != 0.0f) {
		float speed = m_vid->m_unk0x2c;
		EX_SPRITE_DATA* data = m_exData;
		data->m_unk0x20 = ((float) rand() * speed) * 3.0518509e-5f;
	}
	StartMove();
}

static inline int InMap(float p_x, float p_y)
{
	if (p_x < 0.0f)
		return 0;
	if (p_x >= Map->m_w)
		return 0;
	if (p_y < 0.0f)
		return 0;
	if (p_y >= Map->m_h)
		return 0;
	return 1;
}

inline int SPRITE::IsXYCross(const VID* p_vid, float p_x, float p_y) const
{
	return (float) fabs(m_x - p_x) < m_vid->m_unk0x384 + p_vid->m_unk0x384
		&& (float) fabs(m_y - p_y) < m_vid->m_unk0x388 + p_vid->m_unk0x388;
}

inline ANGLE SPRITE::RotateToGoal(int p_time)
{
	return Rotate(DirectionTo(m_goal) + (unsigned char) (m_speed < 0.0f ? 0x80 : 0), p_time);
}

// STUB: ALIEN 0x448130
void CANNON::MoveTact()
{
	if (!m_vid->m_canMove)
		return;

	float nx;
	float ny;
	float nz;
	MoveTactCalcCoor(&nx, &ny, &nz);
	if ((m_flag & 0x400) && m_ani < 15) {
		ChangeAnimation(15);
	}

	float curGroundZ = Map->GetGroundZ_ff(X(), Y());
	float newGroundZ = Map->GetGroundZ_ff(nx, ny);
	VID* vid = m_vid;
	float hoverZ = newGroundZ + vid->m_unk0x60;
	float savedZ = m_z;

	if (nz <= newGroundZ && m_z >= curGroundZ) {
		nz = Z();
		if (m_ani >= 15) {
			Stop();
			if (newGroundZ > curGroundZ)
				CreateChildAndPlaySFX(11);
		}
		else {
			if (!(vid->m_flag & 0x10000000)) {
				Stop();
				nz = curGroundZ;
				if (newGroundZ > curGroundZ)
					CreateChildAndPlaySFX(11);
				else
					CreateChildAndPlaySFX(12);
				ChangeAnimation(15);
			}
			else if (newGroundZ > curGroundZ) {
				ChangeAnimation(11);
				ChangeDirection((unsigned char) (Direction().m_dir + 0x80));
			}
			else if (m_unk0x24 < -0.022f) {
				ChangeAnimation(12);
				m_unk0x24 = m_unk0x24 * -0.5f;
			}
			else if (m_unk0x24 > 0.0049999999f) {
				m_unk0x24 = m_unk0x24 * 0.5f;
				m_speed = m_speed * 0.5f;
				ChangeDirection((unsigned char) (Direction().m_dir + 0x80));
			}
			else {
				m_unk0x24 = 0;
				ChangeAnimation(15);
			}
		}
		nx = X();
		ny = Y();
	}
	else if (m_z != nz) {
		unsigned int flag = vid->m_flag;
		if (!(flag & 2) && hoverZ != 0.0f) {
			if (m_z < hoverZ) {
				if (nz < hoverZ)
					goto move;
				nz = hoverZ;
			}
			else if (m_z > hoverZ) {
				if (nz >= hoverZ)
					goto move;
				nz = hoverZ;
			}
			else if (flag & 0x8000000)
				goto move;
			m_unk0x24 = 0;
		}
	}

move:
	if (m_x != nx || m_y != ny) {
		if (CanPlaceWithCrush(nx, ny, nz)) {
			if (m_ani < 15) {
				ChangeAnimation(15);
			}
		}
		else if (!InMap(nx, ny)) {
			ChangeCoor(nx, ny, nz);
			if (m_ani < 15 && !(m_vid->m_flag & 2)
				&& (m_x < -220.0f || m_y < -200.0f || X() > Map->m_w + 220.0f
					|| Y() > Map->m_h + 200.0f)) {
				ChangeAnimation(15);
			}
		}
		else
			ChangeCoor(nx, ny, nz);
	}

	if (m_z != nz)
		ChangeZ(nz);

	SPRITE* goal = m_goal;
	if (goal) {

		float dx = (float) fabs(goal->m_x - m_x);
		float dy = (float) fabs(goal->m_y - m_y);
		float dist = dx <= dy ? dx * 0.5f + dy : dy * 0.5f + dx;

		if (m_vid->m_flag & 0x8000000) {
			if (m_unk0x70 & 1) {
				curGroundZ = Map->GetGroundZ_ff(X(), Y());
				if ((Z() < curGroundZ + m_vid->m_unk0x60 || Z() < m_goal->Z())
					&& m_unk0x24 > 0.0f) {
					m_unk0x24 = m_unk0x24 - (CurrentTime - PrevCurrentTime) * Const->m_unk0x08;
				}
				else {
					m_unk0x70 &= ~1;
				}
			}
			else {
				ANGLE rot = RotateToGoal(CurrentTime - PrevCurrentTime);
				if (dist <= 10.0f || dist >= 30.0f || rot.m_dir <= 0x46) {
					SPRITE* g = m_goal;
					if (Z() <= g->Z() || dist == 0.0f) {
						m_unk0x24 = 0;
					}
					else {
						float vz = (g->m_z - m_z) / dist * 0.1f;
						m_unk0x24 = vz;
						float terminal = -m_vid->m_unk0x30;
						if (vz < terminal)
							m_unk0x24 = terminal;
					}
				}
				else {
					m_unk0x70 |= 1;
					StartMove();
				}
			}
		}
		else {
			if (dist > 100.0f) {
				unsigned char origDir = m_dir;
				RotateToGoal(CurrentTime - PrevCurrentTime);
				unsigned char d1 = (unsigned char) (origDir - m_dir);
				unsigned char d2 = (unsigned char) (m_dir - origDir);
				unsigned char turned = (d1 < d2) ? d1 : d2;
				if (turned > 0x64) {
					Stop();
					ChangeAnimation(15);
				}
			}
		}

		unsigned int flag = m_flag;
		if (((flag & 0x4000) && (flag & 0x8000))
			|| IsXYCross(m_goal->m_vid, m_goal->m_x, m_goal->m_y)) {
			float gz = m_goal->m_z;
			if (m_vid->m_unk0x24 + m_z >= gz && gz + m_goal->m_vid->m_unk0x24 >= m_z)
				goto hit;
			float z = m_z;
			gz = m_goal->m_z;
			if (savedZ < z) {
				if (gz >= savedZ && gz <= z)
					goto hit;
			}
			else if (gz >= z && gz <= savedZ) {
				goto hit;
			}
		}
		if ((m_vid->m_flag & 0x8000000) && (float) fabs(m_goal->m_z - m_z) < 20.0f
			&& (float) fabs(m_goal->m_x - m_x) < 10.0f
			&& (float) fabs(m_goal->m_y - m_y) < 10.0f) {
		hit:
			Stop();
			ChangeAnimation(15);
		}
	}
	else {
		VID* v = m_vid;
		if (v->m_unk0x60 < 0.0f && m_unk0x24 < 0.0f) {
			Rotate(Direction() + (unsigned char) 32, CurrentTime - PrevCurrentTime);
		}
	}
}

// FUNCTION: ALIEN 0x448950
decomp_intptr CANNON::Action(int p_action, int p_a, int p_b, int p_c)
{
	if (p_action != 0x82)
		return SPRITE::Action(p_action, p_a, p_b, p_c);
	int ani = m_ani;
	if (ani == 8) {
		ChangeAnimation(0xf);
		return 0;
	}
	if (ani < 0xf) {
		if (m_z < -100.0f) {
			ChangeAnimation(0x10);
			return 0;
		}
		if (m_speed != 0.0f) {
			ChangeAnimation(2);
			return 0;
		}
		if (ani >= 7 && ani != 0xa)
			ChangeAnimation(0);
	}
	return 0;
}

// FUNCTION: ALIEN 0x4489e0
void CANNON::DeletePointerToSprite(SPRITE* p_sprite)
{
	if (p_sprite && m_goal == p_sprite)
		SetGoal(new SPRITE(EmptyVid, (float) p_sprite->GetX(), (float) p_sprite->GetY(), (float) p_sprite->GetZ(), ANGLE(0), 0));
	SPRITE::DeletePointerToSprite(p_sprite);
}
