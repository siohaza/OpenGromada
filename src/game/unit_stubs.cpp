#include "game/game_descriptor.h"
#include "game/gametime.h"
#include "game/map.h"
#include "game/unit.h"
#include "sprite/ex_sprite_data.h"
#include "util/game_random.h"
#include "util/myerror.h"
#include "util/stream.h"
#include "video/vid.h"
#include "video/vid_exdata.h"

#include <stdlib.h>

void LIST_INT::Expand(int p_max)
{
	if (p_max > m_max) {
		int* old = m_data;
		m_data = new int[p_max];
		if (!m_data) {
			MYERROR::LogExit(::Error, "!!!ERROR!!!::LIST: Not enough memory %i", p_max);
		}
		if (old) {
			for (int i = 0; i < m_max; ++i) {
				m_data[i] = old[i];
			}
			delete[] old;
		}
		m_max = p_max;
	}
}

void LIST_INT::Read(STREAM* p_stream)
{
	p_stream->Read(&m_n, 4);
	Expand(m_n);
	p_stream->Read(m_data, 4 * m_n);
}

void LIST_SHORT::Expand(int p_max)
{
	if (p_max > m_max) {
		short* old = m_data;
		m_data = new short[p_max];
		if (!m_data) {
			MYERROR::LogExit(::Error, "!!!ERROR!!!::LIST: Not enough memory %i", p_max);
		}
		if (old) {
			for (int i = 0; i < m_max; ++i) {
				m_data[i] = old[i];
			}
			delete[] old;
		}
		m_max = p_max;
	}
}

void LIST_SHORT::Read(STREAM* p_stream)
{
	p_stream->Read(&m_n, 4);
	Expand(m_n);
	p_stream->Read(m_data, 2 * m_n);
}

// FUNCTION: ALIEN 0x447130
UNIT::UNIT(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent)
	: TERRAIN(p_vid, p_x, p_y, p_z, p_dir, p_parent)
{
	m_unk0x84 = -1;
	m_unk0x78 = 0;
	m_unk0x7c = 0;
	m_turn = 0;
	VID* vid = m_vid;
	VID* linkVid = vid->m_linkVid;
	m_unk0x8c = m_child && m_child->m_vid == m_vid->m_linkVid && m_child->m_vid->m_weaponVid && m_child->m_vid->m_weapon
					? m_vid->m_linkVid->m_exData->m_defaultBehavior
					: m_vid->m_exData->m_defaultBehavior;
	m_ammo = (m_vid->m_linkVid && m_vid->m_linkVid->m_weaponVid && m_vid->m_linkVid->m_weapon
				  ? m_vid->m_linkVid->m_exData->m_maxAmmo
				  : m_vid->m_exData->m_maxAmmo)
			 << 6;
}

// FUNCTION: ALIEN 0x447220
void* UNIT::ScalarDeletingDestructor(unsigned int p_flags)
{
	UNIT* result = this;
	this->~UNIT();
	if (p_flags & 1) {
		operator delete(result);
	}
	return result;
}

// FUNCTION: ALIEN 0x447240
UNIT::~UNIT()
{
	Map->DeletePointerToSprite(this);
}

// FUNCTION: ALIEN 0x447260
void UNIT::MoveTact()
{
	int sprClass = m_vid->m_sprClass;
	if (sprClass != 2 && sprClass != 4) {
		SPRITE::MoveTact();
		return;
	}
	float x, y, z;
	MoveTactCalcCoor(&x, &y, &z);
	if (m_turn) {
		int fps = Map->m_fps;
		if (abs(m_turn) > fps) {
			if (m_turn > 0) {
				m_turn = fps / 2;
			}
			else {
				m_turn = fps / -2;
			}
		}
		unsigned char sign = m_turn > 0 ? 0x40 : 0xc0;
		Rotate(Direction() + ANGLE(sign), CurrentTime - PrevCurrentTime);
		if (m_turn < 0) {
			m_turn++;
		}
		else {
			m_turn--;
		}
	}
	else if (m_goal && m_speed != 0.0f) {
		int t = m_speed < 0.0f ? 0x80 : 0;
		char glide = (char) t;
		Rotate(GlideDirection(DirectionTo(Goal()) + ANGLE((unsigned char) glide)), CurrentTime - PrevCurrentTime);
		if ((m_flag & 0x4000) && (m_flag & 0x8000)) {
			Stop();
		}
	}
	if (m_x != x || m_y != y || m_z != z) {
		if (!CanPlaceWithCrushAndGlide(&x, &y, &z)) {
			MoveTactMapLimit(x, y);
			ChangeCoor(x, y, z);
			return;
		}
		if (!m_turn) {
			if (GameRand() % 2 != 0) {
				m_turn = Map->m_fps / 2;
				return;
			}
			m_turn = Map->m_fps / -2;
		}
	}
}

inline static int UnitHaveArmedLink(UNIT* p_unit)
{
	return p_unit->m_child && p_unit->m_child->m_vid == p_unit->m_vid->m_linkVid &&
		   p_unit->m_child->m_vid->m_aniChildVid[8] && p_unit->m_child->m_vid->m_weapon;
}

// FUNCTION: ALIEN 0x447500
decomp_intptr UNIT::Action(int p_action, decomp_intptr p_a, decomp_intptr p_b, decomp_intptr p_c)
{

	int state;
	switch (p_action) {
	case 93: { // ACT_ADD_AMMO
		int max = m_vid->GetMaxAmmo();
		if (max == 999999) {
			m_ammo = 63999936;
			return max;
		}
		m_ammo += p_a << 6;
		state = m_ammo;
		if (m_ammo < 0) {
			m_ammo = 0;
		}
		if (m_unk0x74) {
			m_ammo = m_vid->GetMaxAmmo() << 6;
		}
		return state;
	}
	case 92: // ACT_GET_AMMO
		return m_ammo / 64;
	case 95: // ACT_SET_BEHAVE
		m_unk0x8c = p_a;
		if (m_child) {
			m_child->Action(p_action, p_a, p_b, p_c);
		}
		break;
	case 94: // ACT_GET_BEHAVE
		return m_unk0x8c;
	case 130: { // ACT_NEXT_COMMAND
		int ani = m_ani;
		if (ani < 15) {
			if (m_speed != 0.0f) {
				if (ani != 2) {
					ChangeAnimation(2); // ANI_GO
				}
			}
			else {
				ChangeAnimation(0); // ANI_STAND
			}
			if ((m_flag & 0x7c) == 0xc) {
				SPRITE* turret = m_child;
				if (turret && turret->m_vid == m_vid->m_linkVid && turret->m_vid->m_aniChildVid[8] &&
					turret->m_vid->m_weapon && m_goal && !turret->m_goal) {
					turret->SetCommand(3, m_goal);
				}
			}
			if ((m_flag & 0x7c) == 0x10) {
				SPRITE* turret = m_child;
				if (turret && turret->m_vid == m_vid->m_linkVid && turret->m_vid->m_aniChildVid[8] &&
					turret->m_vid->m_weapon && m_goal && !turret->m_goal) {
					turret->SetCommand(4, m_goal);
				}
			}
			if (UnitHaveArmedLink(this) || !m_turn) {
				{
					unsigned int dur = m_vid->m_aniDuration[m_ani];
					int dt = CurrentTime - PrevCurrentTime;
					if ((unsigned int) dt <= dur) {
						dt = (int) dur;
					}
					state = m_unk0x04 = AttackTact(dt);
				}
				if (state != 7) {
					if (state == 1) {
						float speed = m_exData ? m_exData->m_unk0x20 : m_vid->m_unk0x2c;
						if (speed == 0.0f) {
							if (UnitHaveArmedLink(this) && m_child->m_goal) {
								SetCommand(0, 0);
							}
							else if (m_goal) {
								SetCommand(0, 0);
							}
						}
						else if (m_speed == 0.0f) {
							StartMove();
						}
						int cmd = m_flag & 0x7c;
						if ((cmd == 12 || cmd == 16) && UnitHaveArmedLink(this) && (m_unk0x8c & 1) &&
							(m_child->m_unk0x50 || !(GameRand() % 4))) {
							SPRITE* enemy = SeekEnemy();
							if (enemy) {
								m_child->SetCommand(4, enemy);
							}
						}
						goto notAttacking;
					}
					if (!state && m_speed != 0.0f) {
						if (!UnitHaveArmedLink(this) || m_goal == m_child->m_goal) {
							Stop();
							goto notAttacking;
						}
					}
					if (state == 2) {
					patrol:
						if ((m_unk0x8c & 2) && m_speed == 0.0f) {
							StartMove();
							goto notAttacking;
						}
						goto seekTick;
					}
					if (state != 3) {
						goto notAttacking;
					}
				}

				SetCommand(0, 0);
			}
			else {
				state = 2;
				goto patrol;
			}
		notAttacking:
			if (state == 6) {
				if (!(m_unk0x8c & 1)) {
					goto lastGuard;
				}
				if (m_unk0x8c & 2) {
					SPRITE* enemy = SeekEnemy();
					if (enemy) {
						SetCommand(4, enemy);
					}
				}
				else {
					if (!UnitHaveArmedLink(this)) {
						goto lastGuard;
					}
					SPRITE* enemy = SeekEnemy();
					if (enemy) {
						m_child->SetCommand(4, enemy);
					}
				}
			}
			if (state == 2 || state == 5) {
			seekTick:
				if (m_unk0x8c & 1) {
					int cmd = m_flag & 0x7c;
					if (!cmd || cmd == 4 || cmd == 16) {
						if (m_endCadr > m_begCadr || (UnitHaveArmedLink(this) ? m_child->m_unk0x50 : m_unk0x50) ||
							!(GameRand() % 11)) {
							SPRITE* enemy = SeekEnemy();
							if (enemy) {
								SetCommand(4, enemy);
							}
						}
					}
				}
			}
		lastGuard:
			if (0 == (m_flag & 0x7c) && !m_goal && !m_unk0x50) {
				Stop();
			}
		}
		break;
	}
	case 85: { // ACT_DAMAGE
		decomp_intptr result = TERRAIN::Action(p_action, p_a, p_b, p_c);
		if (!Game_IsZS1() && p_a >= 0 && 0 == (m_flag & 0x7c) &&
			(m_exData ? m_exData->m_unk0x20 : m_vid->m_unk0x2c) != 0.0f) {
			unsigned char dir = GameRand() % 256;
			float z = m_z;
			float y = m_y;
			float c = ANGLE::CosTable[dir];
			float x = m_x;
			float s = ANGLE::SinTable[dir];
			Move(new SPRITE(EmptyVid, s * 64.0f + x, y - c * 64.0f, z, ANGLE(0), 0));
		}
		return result;
	}
	case 86: { // ACT_REPAIR
		VID* vid = m_vid;
		VID* link = vid->m_linkVid;
		int maxAmmo;
		if (link && link->m_aniChildVid[8] && link->m_weapon) {
			maxAmmo = link->m_exData->m_maxAmmo;
		}
		else {
			maxAmmo = vid->m_exData->m_maxAmmo;
		}
		m_ammo = maxAmmo << 6;
		return TERRAIN::Action(p_action, p_a, p_b, p_c);
	}
	case 80: { // ACT_SAVE
		TERRAIN::Action(p_action, p_a, p_b, p_c);
		((STREAM*) p_a)->Write(&m_unk0x8c, 4);
		break;
	}
	case 81: { // ACT_RESTORE
		TERRAIN::Action(p_action, p_a, p_b, p_c);
		STREAM* stream = (STREAM*) p_a;
		stream->Read(&m_unk0x8c, 4);
		if (p_b == 11) {
			LIST_INT list;
			list.Read(stream);
			for (int i = 0; i < list.m_n; ++i) {
				InsertItem(list.m_data[i]);
			}
		}
		else if (p_b < 11) {
			LIST_SHORT list;
			list.Read(stream);
			for (int i = 0; i < list.m_n; ++i) {
				InsertItem(list.m_data[i]);
			}
		}
		break;
	}
	case 200: {
		TERRAIN::Action(p_action, p_a, p_b, p_c);
		STREAM* stream = (STREAM*) p_a;
		if (p_b < 7) {
			state = 0;
			stream->Read(&state, 1);
			ChangeArmy(state);
		}
		stream->Read(&m_unk0x8c, 1);
		break;
	}
	case 97: { // ACT_SET_ARMY
		state = (m_flag >> 11) & 3;
		ChangeArmy(p_a);
		if (!Game_IsZS1() && state != ((m_flag >> 11) & 3) && m_vid->m_idx == 104) {
			Map->ScriptRun(EvFunctionNumber[15], this, 0, 0);
		}
		break;
	}
	default:
		return TERRAIN::Action(p_action, p_a, p_b, p_c);
	}
	return 0;
}

// FUNCTION: ALIEN 0x447f10
void UNIT::DrawSecondaryInfo()
{
	SPRITE::DrawSecondaryInfo();
}

// FUNCTION: ALIEN 0x447f20
int UNIT::AddAmmoTick(int p_ammo)
{
	VID* vid = m_vid;
	VID* linkVid = vid->m_unk0x5c;
	int maxAmmo;
	if (linkVid && linkVid->m_weaponVid && linkVid->m_unk0x40) {
		maxAmmo = linkVid->m_exData->m_maxAmmo;
	}
	else {
		maxAmmo = vid->m_exData->m_maxAmmo;
	}
	maxAmmo <<= 6;
	if (maxAmmo == 0 || maxAmmo <= m_ammo) {
		return 0;
	}
	if (!p_ammo || (m_ammo = m_ammo + maxAmmo / p_ammo, m_ammo > maxAmmo)) {
		m_ammo = maxAmmo;
	}
	return 1;
}
