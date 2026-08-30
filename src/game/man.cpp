#include "game/man.h"

#include "game/gametime.h"
#include "game/map.h"
#include "gfx/graph.h"
#include "sprite/ex_sprite_data.h"
#include "video/vid.h"
#include "video/vid_exdata.h"

#include <math.h>
#include <string.h>

// GLOBAL: ALIEN 0x484b68
int g_damagePercent[3] = {50, 70, 90};

// FUNCTION: ALIEN 0x4545e0
MAN::MAN(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent)
	: UNIT(p_vid, p_x, p_y, p_z, p_dir, p_parent)
{
	InsertUniqueItem(261);
	int* ammo = &m_ammo[2];
	for (int i = 0; i < 8; ++i) {
		ammo[i] = 0;
	}
}

// FUNCTION: ALIEN 0x454640
void* MAN::ScalarDeletingDestructor(unsigned int p_flags)
{
	MAN* result = this;
	this->~MAN();
	if (p_flags & 1) {
		operator delete(result);
	}
	return result;
}

// FUNCTION: ALIEN 0x454660
MAN::~MAN()
{
}

// FUNCTION: ALIEN 0x454670
void MAN::MoveTact()
{
	float x, y, z;
	MoveTactCalcCoor(&x, &y, &z);
	if ((m_x != x || m_y != y) && x >= 0.0f && x < Map->m_w && y >= 0.0f && y < Map->m_h &&
		!CanPlaceWithCrushAndGlide(&x, &y, &z)) {
		ChangeCoor(x, y, z);
	}
	if (m_goal && (m_flag & 0x7c) == 4) {
		// Reversing flips the facing by half a turn.
		ANGLE glide((unsigned char) (m_speed < 0.0f ? 0x80 : 0));
		Rotate(GlideDirection(DirectionTo(Goal()) + glide), CurrentTime - PrevCurrentTime);
		if (!(m_flag & 0x4000) || !(m_flag & 0x8000)) {
			SPRITE* g = m_goal;
			float gx = g->m_x;
			if ((float) fabs(m_x - gx) < m_vid->m_unk0x384 + g->m_vid->m_unk0x384) {
				float gy = g->m_y;
				if ((float) fabs(m_y - gy) < m_vid->m_unk0x388 + g->m_vid->m_unk0x388) {
					Stop();
				}
			}
		}
		else {
			Stop();
		}
	}
}

inline static int HaveItemFromEnd(EX_SPRITE_DATA* p_ex, int p_item)
{
	int n = p_ex->m_list.m_n;
	int* p;
	if (n) {
		p = &p_ex->m_list.m_data[n];
		do {
			int item = *--p;
			n--;
			if (item != p_item) {
				continue;
			}
			if (n >= 0) {
				return 1;
			}
			break;
		} while (n);
	}
	return 0;
}

// STUB: ALIEN 0x454800
decomp_intptr MAN::Action(int p_action, decomp_intptr p_a, decomp_intptr p_b, decomp_intptr p_c)
{
	switch (p_action) {
	case 0x25: {
		SPRITE* c = m_child;
		if (!c) {
			break;
		}
		VID* vid = m_vid;
		VID* cv = c->m_vid;
		if (cv != vid->m_unk0x5c || !cv->m_weaponVid || !cv->m_unk0x40 || (unsigned int) c->m_unk0x50 > 0x1388 ||
			(c->m_ani == 8 && c->m_noCadr <= c->m_endCadr)) {
			break;
		}
		float fx = (float) p_a;
		float sx = fx - Map->m_shiftX;
		float fy = (float) p_b;
		float sy = fy - Map->m_shiftY;
		float gz;
		if (sx >= Graph->m_viewXMin && sx < Graph->m_viewXMax && sy >= Graph->m_viewYMin && sy < Graph->m_viewYMax) {
			gz = (float) (*((unsigned short*) Graph->m_zbuffer + (int) sy * Graph->m_zpitch + (int) sx) >> 3) - 128.0f;
			if (gz > m_z + 70.0f) {
				gz = m_z + 50.0f;
			}
		}
		else {
			int wtype;
			if (c->m_vid->m_weaponVid && c->m_vid->m_unk0x40) {
				wtype = c->m_vid->m_exData->m_unk0x00;
			}
			else {
				wtype = vid->m_exData->m_unk0x00;
			}
			if (wtype == 8) {
				float fy2 = fy + 80.0f;
				gz = Map->GetGroundZ_ff(fx, fy2) + 80.0f;
			}
			else {
				gz = Map->GetGroundZ_ff(fx, fy) + 19.0f;
				p_b -= 19;
			}
		}
		c = m_child;
		c->SetCommand(4, new SPRITE(EmptyVid, fx, (float) p_b + gz, gz, ANGLE(0), 0));
		break;
	}
	case 0x82:
		if (m_ani >= 0xf) {
			break;
		}
		if (m_goal || (m_child && m_child->m_goal)) {
			unsigned int dur = m_vid->m_aniDuration[m_ani];
			unsigned int dt = CurrentTime - PrevCurrentTime;
			if (dt <= dur) {
				dt = dur;
			}
			m_unk0x04 = AttackTact(dt);
		}
		if (m_speed != 0.0f) {
			ChangeAnimation(2);
		}
		else {
			ChangeAnimation(0);
		}
		break;
	case 0x36:
		if (p_a == 301 || p_a == 235) {
			InsertItem(p_a);
			break;
		}
		if (InsertUniqueItem(p_a)) {
			break;
		}
		if (p_a < 260 || p_a > 269) {
			break;
		}
		if (UNIT::Action(0x5c, 0, 0, 0) && p_a - 260 <= m_vid->m_unk0x5c->m_idx - 10 && p_a != 260) {
			break;
		}
		ChangeWeapon(p_a - 260);
		break;
	case 0x5c:
		if (!p_a || p_a == m_vid->m_unk0x5c->m_idx - 10) {
			return UNIT::Action(0x5c, 0, 0, 0);
		}
		return m_ammo[p_a];
	case 0x5d:
		if (p_b > 9) {
			return 0;
		}
		if (!p_b || p_b == m_vid->m_unk0x5c->m_idx - 10) {
			return UNIT::Action(0x5d, p_a, 0, 0);
		}
		return m_ammo[p_b] += p_a;
	case 0x55: {
		int dmg = p_a;
		if (dmg > 0) {
			if (m_vid->m_idx != 350) {
				for (SPRITE* p = m_child; p; p = p->m_child) {
					int idx = p->m_vid->m_idx;
					if (idx == 203 || idx == 181) {
						return 0;
					}
				}
				for (SPRITE* c = m_child; c; c = c->m_child) {
					int idx = c->m_vid->m_idx;
					if (idx >= 200 && idx <= 202) {
						c->Action(0x55, (dmg * g_damagePercent[idx - 200] + 50) / 100, p_b, p_c);
						dmg += dmg * g_damagePercent[c->m_vid->m_idx - 200] / -100;
					}
				}
			}
			if (dmg >= m_unk0x54 && Action(0x38, 230, 0, 0)) {
				Action(0x37, 230, 0, 0);
				ChangeHp(m_vid->m_maxHp[(m_flag >> 11) & 3]);
				Map->CreateSprite(
					Map->Vid(181),
					(float) GetX(),
					(float) GetY(),
					(float) (GetZ() + 22.0f),
					ANGLE(0),
					this
				);
				break;
			}
		}
		return SPRITE::Action(p_action, dmg, p_b, p_c);
	}
	case 0x3e: {
		VID* vid = m_vid;
		if (vid->m_idx < 20) {
			m_ammo[vid->m_unk0x5c->m_idx - 10] = UNIT::Action(0x5c, 0, 0, 0);
		}
		UNIT::Action(p_action, p_a, p_b, p_c);
		if (m_vid->m_idx > 20) {
			VID* v = m_vid;
			int cap;
			VID* link = v->m_unk0x5c;
			if (link && link->m_weaponVid && link->m_unk0x40) {
				cap = link->m_exData->m_maxAmmo;
			}
			else {
				cap = v->m_exData->m_maxAmmo;
			}
			UNIT::Action(0x5d, cap - UNIT::Action(0x5c, 0, 0, 0), 0, 0);
			break;
		}
		if (m_exData && HaveItemFromEnd(m_exData, 204)) {
			Child()->AddLink(Map->CreateSprite(
				Map->Vid(200),
				(float) GetX(),
				(float) GetY(),
				(float) (GetZ() + GetVid()->m_unk0x54),
				Direction(),
				0
			));
		}
		else if (m_exData && HaveItemFromEnd(m_exData, 205)) {
			Child()->AddLink(Map->CreateSprite(
				Map->Vid(201),
				(float) GetX(),
				(float) GetY(),
				(float) (GetZ() + GetVid()->m_unk0x54),
				Direction(),
				0
			));
		}
		else if (m_exData && HaveItemFromEnd(m_exData, 206)) {
			Child()->AddLink(Map->CreateSprite(
				Map->Vid(202),
				(float) GetX(),
				(float) GetY(),
				(float) (GetZ() + GetVid()->m_unk0x54),
				Direction(),
				0
			));
		}
		VID* v = m_vid;
		UNIT::Action(0x5d, m_ammo[v->m_unk0x5c->m_idx - 10] - UNIT::Action(0x5c, 0, 0, 0), 0, 0);
		break;
	}
	default:
		return UNIT::Action(p_action, p_a, p_b, p_c);
	}
	return 0;
}

// FUNCTION: ALIEN 0x455070
int MAN::ChangeWeapon(int p_weapon)
{
	VID* vid = m_vid;
	VID* weapon = vid->m_unk0x5c;
	if (weapon->m_idx <= 20) {
		if (p_weapon == 10) {
			p_weapon = 0;
		}
		int item = p_weapon + 0x104;
		if (m_exData && m_exData->m_list.m_n) {
			int i = m_exData->m_list.m_n;
			int* p = &m_exData->m_list.m_data[i];
			while (i) {
				--p;
				--i;
				if (*p == item) {
					if (i >= 0 && m_child && m_child->m_vid == weapon) {
						m_ammo[vid->m_unk0x5c->m_idx - 10] = UNIT::Action(0x5c, 0, 0, 0);
						m_child->Action(0x3e, p_weapon + 10, 0, 0);
						VID* v;
						int n = p_weapon + 10;
						if (n < 0 || n >= Map->m_noVid || !(v = Map->m_vids[n])) {
							v = EmptyVid;
						}
						m_vid->m_unk0x5c = v;
						UNIT::Action(0x5d, m_ammo[p_weapon] - UNIT::Action(0x5c, 0, 0, 0), 0, 0);
						return 1;
					}
					return 0;
				}
			}
		}
	}
	return 0;
}
