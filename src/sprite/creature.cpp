#include "sprite/creature.h"

#include <stdlib.h>

#include "game/gametime.h"
#include "game/map.h"
#include "game/region.h"
#include "video/vid.h"
#include "world/hash_map.h"

// FUNCTION: ALIEN 0x44d3e0
CREATURE::CREATURE(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent)
	: UNIT(p_vid, p_x, p_y, p_z, p_dir, p_parent)
{
	m_unk0x24 = m_vid->m_unk0x30;
	m_unk0x90 |= 1;
	StartMove();
	float y = m_y;
	float x = m_x;
	m_region = FindRegion(x, y);
}

// FUNCTION: ALIEN 0x44d460
void CREATURE::DeletePointerToSprite(SPRITE* p_sprite)
{
	if (m_region == p_sprite)
		m_region = 0;
	SPRITE::DeletePointerToSprite(p_sprite);
}

// FUNCTION: ALIEN 0x44d480
void CREATURE::MoveTact()
{
	float y, x, z;
	if ((m_unk0x90 & 1) && !m_region) {
		float fx, fy;
		m_unk0x90 &= ~1u;
		m_region = FindRegion(fx = m_x, fy = m_y);
	}
	MoveTactCalcCoor(&x, &y, &z);
	if (m_turn) {
		if (m_turn & 1) {
			if (m_ani == 4) {
				Rotate(Direction() - (unsigned char) 0x40, CurrentTime - PrevCurrentTime);
			}
			else if (m_ani != 5 && rand() % 2 == 0) {
				Rotate(Direction() - (unsigned char) 0x40, CurrentTime - PrevCurrentTime);
			}
			else {
				Rotate(Direction() + (unsigned char) 0x40, CurrentTime - PrevCurrentTime);
			}
		}
		if (m_turn < 0)
			m_turn++;
		else
			m_turn--;
	}
	else if (m_goal && m_speed != 0.0f) {
		Rotate(GlideDirection(DirectionTo(Goal()) + (m_speed < 0.0f ? 0x80 : 0)),
			CurrentTime - PrevCurrentTime);
		if ((m_flag & 0x4000) && (m_flag & 0x8000))
			Stop();
	}
	if (m_vid->m_flag & 0x200)
		z = Map->GetGroundZ_vid(m_vid, x, y);
	if (CanPlaceWithCrush(x, y, z)) {
		m_speed = 0;
		if (!m_turn)
			m_turn = 10;
	}
	else {
		REGION* r = (REGION*) m_region;
		float x0, w2, y0, h2;
		if (r != 0
			&& (x0 = r->m_x, w2 = r->m_w * 0.5f,
				x0 - w2 > x || x > x0 + w2
				|| (y0 = r->m_y, h2 = r->m_h * 0.5f,
					y0 - h2 > y || y > y0 + h2))) {
			SPRITE* nr = FindRegion(x, y);
			if (!nr || m_region->m_vid != nr->m_vid) {
				m_speed = 0;
				if (!m_turn)
					m_turn = 10;
				return;
			}
			m_region = nr;
		}
		MoveTactMapLimit(x, y);
		ChangeCoor(x, y, z);
	}
}

// FUNCTION: ALIEN 0x44d7b0
SPRITE* CREATURE::FindRegion(float p_x, float p_y)
{
	int iter = Map->m_layers[10].m_n;
	SPRITE* s = Map->NextSpriteByType(10, &iter, 0x40);
	if (!s)
		return 0;
	for (;;) {
		if (s->m_vid->m_sprClass == 0x17) {
			REGION* r = (REGION*) s;
			float x = r->m_x;
			float w = r->m_w;
			float w2 = w * 0.5f;
			if (x - w2 <= p_x && p_x <= x + w2) {
				float y = r->m_y;
				float h = r->m_h;
				float h2 = h * 0.5f;
				if (y - h2 <= p_y && p_y <= y + h2)
					return s;
			}
		}
		s = Map->NextSpriteByType(10, &iter, 0x40);
		if (!s)
			return 0;
	}
}

static inline unsigned int AniTactTime(unsigned int p_dur)
{
	return CurrentTime - PrevCurrentTime > p_dur ? CurrentTime - PrevCurrentTime : p_dur;
}

// FUNCTION: ALIEN 0x44d880
decomp_intptr CREATURE::Action(int p_action, int p_a, int p_b, int p_c)
{
	switch (p_action) {
	default:
		return UNIT::Action(p_action, p_a, p_b, p_c);
	case 130: { // ACT_NEXT_COMMAND
		int ani = m_ani;
		if (ani < 15) {
			if (ani == 8 || ani == 13) // ANI_FIGHT / ANI_WOUND
				ChangeAnimation(0);
			if ((m_unk0x90 & 1) && !m_region) {
				float fy = m_y;
				m_unk0x90 &= ~1u;
				float fx = m_x;
				m_region = FindRegion(fx, fy);
			}
			if (m_goal && (m_flag & 0x7c) == 4) {
				if (m_speed == 0.0f) {
					ChangeAnimation(0); // ANI_STAND
				}
				else if (m_ani != 2) {
					ChangeAnimation(2); // ANI_GO
					return 0;
				}
			}
			else {
				int cmd = m_flag & 0x7c;
				if (cmd == 12 || cmd == 16
					|| CurrentTime - (CurrentTime & 0x7ff) > m_tactTime) {

					m_unk0x04 = AttackTact(AniTactTime(m_vid->m_aniDuration[m_ani]));
					unsigned int state = m_unk0x04;
					if (state == 1 && m_speed == 0.0f) {
						SPRITE* turret = m_child;
						if ((turret && turret->m_vid == m_vid->m_linkVid
								&& turret->m_vid->m_aniChildVid[8] && turret->m_vid->m_weapon
								&& turret->m_goal)
							|| m_goal)
							SetCommand(0, 0);
					}
					if ((state == 2 || state == 5) && (m_unk0x8c & 1)
						&& ((m_flag & 0x7c) == 0 || (m_flag & 0x7c) == 4)) {
						SPRITE* enemy = SeekEnemy();
						if (enemy)
							SetCommand(4, enemy);
					}
				}
				cmd = m_flag & 0x7c;
				if (cmd != 4 && cmd != 12 && cmd != 16) {
					if (m_ani == 4 && rand() % 3) { // ANI_L_ROTATE
						Rotate(Direction() - (unsigned char) 32, AniTactTime(m_vid->m_aniDuration[m_ani]));
					}
					if (m_ani == 5 && rand() % 3) { // ANI_R_ROTATE
						Rotate(Direction() + (unsigned char) 32, AniTactTime(m_vid->m_aniDuration[m_ani]));
					}
					if (CurrentTime - (CurrentTime & 0x7ff) > m_tactTime) {
						if (!(rand() % 4)) {
							StartMove();
						}
						else if (m_speed == 0.0f && !(rand() % 3)
							&& (m_vid->m_noAnimCadr[12] || m_vid->m_unk0x20c[12]
								|| m_vid->m_aniSfx[12])) {
							ChangeAnimation(12); // ANI_UNLOAD
						}
						else if (!(rand() % 4)) {
							if (rand() % 2) {
								Rotate(Direction() - (unsigned char) 32, AniTactTime(m_vid->m_aniDuration[m_ani]));
							}
							else {
								Rotate(Direction() + (unsigned char) 32, AniTactTime(m_vid->m_aniDuration[m_ani]));
							}
						}
						else {
							Stop();
						}
					}
				}
				if (m_ani == 13 || m_ani == 9) // ANI_WOUND / ANI_SALUT
					ChangeAnimation(0); // ANI_STAND
			}
		}
		return 0;
	}
	case 85: // ACT_DAMAGE
		if (p_a > 0) {
			for (SPRITE* s = Hash->FirstInBox(X() - 150.0f, Y() - 150.0f,
					X() + 150.0f, Y() + 150.0f);
				s; s = Hash->NextInBox()) {
				if (s->m_vid->m_sprClass == 25 // B_CREATURE
					&& NearDistanceTo(X() - s->m_x, Y() - s->m_y) < 150.0f)
					s->StartMove();
			}
		}
		return UNIT::Action(85, p_a, p_b, p_c);
	}
}
