#include "audio/sfx.h"
#include "audio/sound.h"
#include "game/const.h"
#include "game/engine.h"
#include "game/game_descriptor.h"
#include "game/gametime.h"
#include "game/map.h"
#include "game/region.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "sprite/ex_sprite_data.h"
#include "sprite/linker.h"
#include "sprite/sprite.h"
#include "ui/mouse.h"
#include "util/game_random.h"
#include "util/polar.h"
#include "util/resource.h"
#include "video/vid.h"
#include "video/vid_exdata.h"
#include "world/hash_map.h"

#include <bit>
#include <math.h>
#include <stdlib.h>

// FUNCTION: ALIEN 0x40ec40
void SPRITE::Draw()
{
	m_vid->Draw(this);
}

// FUNCTION: ALIEN 0x43a840
ANGLE SPRITE::Direction()
{
	return ANGLE(m_dir);
}

// FUNCTION: ALIEN 0x43f200
SPRITE::SPRITE(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent)
{
	m_uiScale = p_parent ? (p_parent->m_uiScale & 0xff) : 0;
	m_invulnerable = 0;
	m_flag = 0;
	if (p_vid->m_flag & 0x100) {
		p_x = (float) (8 - GameRand() % 17) + p_x;
		p_y = (float) (8 - GameRand() % 17) + p_y;
	}
	m_vid = p_vid;
	m_x = p_x;
	m_y = p_y;
	if (p_vid->m_flag & 0x200) {
		float ground = Map->GetGroundZ_vid(p_vid, p_x, p_y);
		m_z = ground;
		if (p_parent) {
			m_z += p_z - p_parent->Z();
		}
	}
	else {
		m_z = p_z;
	}

	int army;
	if (p_parent) {
		army = (p_parent->m_flag >> 11) & 3;
	}
	else if (m_vid->m_weapon || !m_vid->m_linkVid) {
		army = m_vid->m_exData->m_army;
	}
	else {
		army = m_vid->m_linkVid->m_exData->m_army;
	}
	m_flag = (m_flag & 0xffffe7ff) | ((army & 3) << 11);

	int phase;
	if (m_vid->m_flag & 0x1000000) {
		phase = 0;
	}
	else {
		phase = m_vid->m_defaultAniPeriod;
		phase = GameRand() % (phase + 1);
	}
	m_tactTime = CurrentTime - phase;
	m_createTime = CurrentTime;
	m_noRef = 0;
	m_parent = 0;
	m_child = 0;
	m_goal = 0;
	army = (m_flag >> 11) & 3;
	int hidden = (m_vid->m_flag & 0x20000) != 0 && ((m_flag >> 11) & 3) != (int) Map->m_curArmy;
	m_flag = (m_flag & 0xfffe1800) | ((hidden & 1) << 16);
	AngleAssign((ANGLE*) &m_dir, ANGLE(0));
	m_unk0x50 = 0;
	m_unk0x54 = m_vid->m_maxHp[(m_flag >> 11) & 3];
	m_speed = 0.0f;
	m_unk0x24 = 0.0f;
	m_unk0x04 = 0;
	m_exData = m_vid->m_unk0x478 ? new EX_SPRITE_DATA(this) : 0;

	VID* vid = m_vid;
	if (m_vid->m_noAnimCadr[0] || m_vid->m_noAnimCadr[2] || !m_vid->m_noAnimCadr[15]) {
		if (!m_vid->m_noAnimCadr[14] || (Map->m_flag & 0x20)) {
			m_ani = 0;
		}
		else {
			m_ani = 14;
		}
	}
	else {
		m_ani = 15;
	}
	m_begCadr = m_vid->m_aniBegCadr[m_ani];
	m_noCadr = m_begCadr;
	m_endCadr = m_vid->m_aniDirCadrs[m_ani] - 1;
	ChangeDirection(p_dir);
	if (m_ani == 0) {
		int frames = m_endCadr - m_begCadr;
		if (frames > 0 && !(m_vid->m_flag & 0x1000000)) {
			m_noCadr += GameRand() % (frames + 1);
		}
	}
	if (m_vid != EmptyVid) {
		++m_noRef;
		if (m_vid != EmptyVid) {
			Insert();
		}
	}
	CreateLink();
	// LoadVid draws its progress sprite before MAP has built the spatial hash.
	// The retail x86 call happened to be harmless for that non-spatial VID, but
	// invoking a member through a null Hash pointer is undefined in C++.
	if (Hash) {
		Hash->Insert(this);
	}
	army = (m_flag >> 11) & 3;
	vid = m_vid;
	vid->m_unk0x458 = RealCurrentTime;
	++vid->m_entitiesNumber[army];
	if (m_ani != 14 && !(Map->m_flag & 0x20)) {
		CreateChildAndPlaySFX(14);
	}
	if (m_vid->m_nLinkDots) {
		m_vid->SetGridZ(this);
	}
}

// FUNCTION: ALIEN 0x43f500
void* SPRITE::ScalarDeletingDestructor(unsigned int p_flags)
{
	SPRITE* result = this;
	this->~SPRITE();
	if (p_flags & 1) {
		operator delete(result);
	}
	return result;
}

// FUNCTION: ALIEN 0x43f520
SPRITE::~SPRITE()
{
	int fn = m_vid->m_unk0x408[17];
	if (fn >= 0 && !(Game_IsZS1() && ENGINE::globaldeleting)) {
		Map->ScriptRun(fn, this, 0, 0);
	}
	if (m_vid->m_nLinkDots) {
		m_vid->ResetGridZ(this);
	}
	if (m_vid == EmptyVid && m_noRef) {
		MYERROR::Error(
			::Error,
			"SPRITE %i",
			4,
			// STRING: ALIEN 0x4843bc
			"noRef for SPRITE with EmptyVid",
			m_noRef,
			m_vid ? m_vid->m_idx : -1
		);
	}
	if (m_noRef > 1) {
		if (Hash) {
			Hash->Delete(this);
		}
		if (m_noRef > 1) {
			Map->DeletePointerToSprite(this);
		}
	}
	unsigned int army = (m_flag >> 11) & 3;
	if (m_vid->m_entitiesNumber[army]) {
		--m_vid->m_entitiesNumber[army];
	}
	SetGoal(0);
	m_unk0x6c = 0;
	while (m_child) {
		m_child->ScalarDeletingDestructor(1);
	}
	if (m_parent) {
		m_parent->m_child = 0;
	}
	if (m_vid != EmptyVid) {
		Remove();
		--m_noRef;
	}
	if (m_noRef) {
		MYERROR::Error(
			::Error,
			"SPRITE %i",
			10,
			// STRING: ALIEN 0x484394
			"Reference count non zero after delete",
			m_noRef,
			m_vid ? m_vid->m_idx : -1
		);
	}
	if (m_exData) {
		delete m_exData;
	}
}

// FUNCTION: ALIEN 0x43f720
void SPRITE::DeletePointerToSprite(SPRITE* p_sprite)
{
	if (!p_sprite) {
		return;
	}
	if (m_child) {
		m_child->DeletePointerToSprite(p_sprite);
	}
	if (m_goal == p_sprite) {
		if (m_ani == 8) {
			SetCommand(4, p_sprite->m_x, p_sprite->m_y, p_sprite->m_z);
		}
		else {
			SetCommand(0, 0);
		}
	}
	for (int i = 0; i < m_actions.m_n; ++i) {
		if ((SPRITE*) m_actions.m_data[i].m_a == p_sprite &&
			(m_actions.m_data[i].m_cmd == 32 || m_actions.m_data[i].m_cmd == 34 || m_actions.m_data[i].m_cmd == 74 ||
			 m_actions.m_data[i].m_cmd == 150 || m_actions.m_data[i].m_cmd == 151 || m_actions.m_data[i].m_cmd == 152 ||
			 m_actions.m_data[i].m_cmd == 75)) {
			m_actions.m_data[i].m_cmd = 255;
			m_actions.m_data[i].m_a = 0;
		}
	}
}

// FUNCTION: ALIEN 0x43f820
void SPRITE::MoveTact()
{
	if (!m_vid->m_canMove) {
		return;
	}
	float x, y, z;
	MoveTactCalcCoor(&x, &y, &z);
	float curGround = Map->GetGroundZ_ff(m_x, m_y);
	float newGround = Map->GetGroundZ_ff(x, y);
	VID* vid = m_vid;
	float cruise = newGround + vid->m_unk0x60;
	if ((vid->m_unk0x0c & 0x200) && (vid->m_flag & 2) && z <= newGround && m_z >= curGround) {
		z = m_z;
	}
	else if (m_z != z && !(vid->m_flag & 2) && cruise != 0.0f) {
		if (m_z < cruise) {
			if (z >= cruise) {
				z = cruise;
				m_unk0x24 = 0.0f;
			}
		}
		else if (m_z > cruise) {
			if (z < cruise) {
				z = cruise;
				m_unk0x24 = 0.0f;
			}
		}
		else if (!(vid->m_flag & 0x8000000)) {
			m_unk0x24 = 0.0f;
		}
	}
	if (m_goal && m_speed != 0.0f && (m_flag & 0x4000) && (m_flag & 0x8000)) {
		Stop();
	}
	if (m_x != x || m_y != y) {
		if (CanPlaceWithCrush(x, y, z)) {
			m_unk0x24 = 0.0f;
			m_speed = 0.0f;
			m_flag |= 0x400;
		}
		else {
			ChangeCoor(x, y, z);
		}
	}
	if (m_z != z) {
		ChangeZ(z);
	}
}

// FUNCTION: ALIEN 0x43fa00
int SPRITE::MoveTactMapLimit(float p_x, float p_y)
{
	if (p_x < 0.0f) {
		Rotate(ANGLE(64), CurrentTime - PrevCurrentTime);
		return 1;
	}
	if (p_y < 0.0f) {
		Rotate(ANGLE(128), CurrentTime - PrevCurrentTime);
		return 1;
	}
	if (Map->m_w <= p_x) {
		Rotate(ANGLE(192), CurrentTime - PrevCurrentTime);
		return 1;
	}
	if (Map->m_h <= p_y) {
		Rotate(ANGLE(0), CurrentTime - PrevCurrentTime);
		return 1;
	}
	return 0;
}

inline static int NearBetween(float p_value, float p_a, float p_b, float p_pad)
{
	if (p_a < p_b) {
		if (p_value >= p_a - p_pad) {
			return p_value <= p_b + p_pad;
		}
		return 0;
	}
	if (p_value < p_b - p_pad) {
		return 0;
	}
	return p_value <= p_a + p_pad;
}

// STUB: ALIEN 0x43fb10
void SPRITE::MoveTactCalcCoor(float* p_x, float* p_y, float* p_z)
{
	m_flag &= ~0x400;
	*p_x = X();
	*p_y = Y();
	*p_z = Z();
	if ((m_flag & 0x7c) == 4 && !(m_flag & 0x80)) {
		MYERROR::Error(
			::Error,
			"SPRITE %i",
			10,
			// STRING: ALIEN 0x4843f0
			"Move without StartMove()",
			0,
			m_vid ? m_vid->m_idx : -1
		);
		StartMove();
	}
	if ((m_flag & 0x7c) == 4 && !m_goal) {
		MYERROR::Error(
			::Error,
			"SPRITE %i",
			10,
			// STRING: ALIEN 0x4843dc
			"Move without goal",
			0,
			m_vid ? m_vid->m_idx : -1
		);
		Stop();
	}
	unsigned int flag = m_flag;
	if (m_flag & 0x80) {
		EX_SPRITE_DATA* exData = m_exData;
		float target = exData ? exData->m_unk0x20 : m_vid->m_unk0x2c;
		if (target > m_speed) {
			VID* vid = m_vid;
			if (vid->m_unk0x34 == 999999.0f) {
				m_speed = exData ? exData->m_unk0x20 : vid->m_unk0x2c;
			}
			else {
				int dt = CurrentTime - PrevCurrentTime;
				m_speed = (float) dt * vid->m_unk0x34 + m_speed;
				float limit = exData ? exData->m_unk0x20 : vid->m_unk0x2c;
				if (m_speed >= limit) {
					m_speed = limit;
				}
			}
		}
	}
	else if (m_speed > 0.0f) {
		VID* vid = m_vid;
		if (vid->m_unk0x38 == 999999.0f) {
			m_speed = 0.0f;
		}
		else {
			int dt = CurrentTime - PrevCurrentTime;
			m_speed = m_speed - (float) dt * vid->m_unk0x38;
			if (m_speed < 0.0f) {
				m_speed = 0.0f;
			}
		}
	}
	if (m_speed == 0.0f && (Graph->m_windForce == 0.0f || !(m_vid->m_flag & 0x1000))) {
	}
	else if (m_speed == 999999.0f) {
		SPRITE* goal = m_goal;
		if (goal && (!(flag & 0x4000) || !(flag & 0x8000))) {
			float x = goal->m_x;
			float y = goal->m_y;
			float z = goal->m_z;
			int hit = AskLine(&x, &y, &z);
			ChangeCoor(x, y, z);
			if (hit) {
				m_flag |= 0x400;
			}
			*p_x = x;
			*p_y = y;
			*p_z = z;
			m_flag |= 0xc000;
			return;
		}
		m_flag = flag | 0xc000;
	}
	else {
		int dt = CurrentTime - PrevCurrentTime;
		float dist = (float) dt * m_speed;
		unsigned char dir;
		if ((m_vid->m_flag & 0x100000) && m_goal) {
			dir = Decart2Polar_f(m_goal->m_x - m_x, m_goal->m_y - m_y).m_dir;
		}
		else {
			dir = m_dir;
		}
		*p_x = ANGLE::SinTable[dir] * dist + *p_x;
		*p_y = *p_y - ANGLE::CosTable[dir] * dist;
		if (m_vid->m_flag & 0x1000) {
			float wind = (float) (int) (CurrentTime - PrevCurrentTime) * Graph->m_windForce;
			*p_x = ANGLE::SinTable[Graph->m_windDirection] * wind + *p_x;
			*p_y = *p_y - ANGLE::CosTable[Graph->m_windDirection] * wind;
		}
	}
	unsigned int vflag = m_vid->m_flag;
	if (vflag & 2) {
		m_unk0x24 = m_unk0x24 - (float) (int) (CurrentTime - PrevCurrentTime) * Const->m_unk0x08;
	}
	else if (vflag & 4) {
		m_unk0x24 = m_unk0x24 - (float) (int) (CurrentTime - PrevCurrentTime) * Const->m_unk0x0c;
	}
	*p_z = (float) (int) (CurrentTime - PrevCurrentTime) * m_unk0x24 + *p_z;

	SPRITE* goal = m_goal;
	if (!goal) {
		return;
	}
	if (NearBetween(goal->m_x, m_x, *p_x, 0.5f)) {
		m_flag |= 0x4000;
	}
	if (NearBetween(goal->m_y, m_y, *p_y, 0.5f)) {
		m_flag |= 0x8000;
	}
}

// FUNCTION: ALIEN 0x440030
void SPRITE::ChangeCoor(float p_x, float p_y, float p_z)
{
	float dx = p_x - m_x;
	float dy = p_y - m_y;
	float dz = p_z - m_z;
	for (SPRITE* sprite = this; sprite; sprite = sprite->m_child) {
		VID* vid = sprite->m_vid;
		if (vid->m_nLinkDots) {
			vid->ResetGridZ(this);
		}
		if (sprite->m_vid->m_flag & 0x40) {
			Hash->ChangeCoor(sprite, sprite->m_x + dx, sprite->m_y + dy);
		}
		if (sprite->m_exData && sprite->m_exData->m_coorTime != RealCurrentTime) {
			sprite->m_exData->m_coorTime = RealCurrentTime;
			sprite->m_exData->m_x = sprite->m_x;
			sprite->m_exData->m_y = sprite->m_y;
			sprite->m_exData->m_z = sprite->m_z;
		}
		vid = sprite->m_vid;
		sprite->m_x += dx;
		sprite->m_y += dy;
		sprite->m_z += dz;
		if (vid->m_nLinkDots) {
			vid->SetGridZ(this);
		}
	}
}

// STUB: ALIEN 0x440110
void SPRITE::Tact()
{
	VID* vid = m_vid;
	if (vid->m_unk0x47c & 0xf) {
		EX_SPRITE_DATA* exData = m_exData;
		VID_EXDATA* vex = vid->m_exData;
		unsigned int elapsed = CurrentTime - exData->m_time;
		unsigned int total = vex->m_unk0x40;
		int aged = 0;
		if (total == 999999) {
			if (m_ani >= 15 && m_endCadr != m_begCadr) {
				total = (m_endCadr - m_begCadr) * vid->m_aniDuration[m_ani];
			}
			else if (exData && exData->m_unk0x10 != 999999) {
				total = exData->m_unk0x10 + elapsed;
			}
			else {
				aged = 1;
			}
		}
		if (!aged && total) {
			float* stages = vex->m_unk0x44;
			int stage = (int) exData->m_unk0x1c + 1;
			while (stage < 7) {
				if ((float) elapsed < (float) total * stages[stage]) {
					break;
				}
				++stage;
			}
			--stage;
			exData->m_unk0x1c = (float) stage + ((float) elapsed - (float) total * stages[stage]) /
													((stages[stage + 1] - stages[stage]) * (float) total);
		}
	}
	if (!m_parent) {
		unsigned int saved = m_tactTime;
		m_tactTime = PrevCurrentTime;
		MoveTact();
		m_tactTime = saved;
	}
	if (m_vid->m_nLinkDots) {
		m_vid->SetGridZ(this);
	}

	VID* childVid = (VID*) m_vid->m_aniChildVid[m_ani];

	if (childVid) {
		if (m_vid->m_sprClass == 23 // B_REGION
			&& m_vid->m_unk0x140[m_ani] == 0.0f && m_vid->m_unk0x184[m_ani] == 0.0f) {
			REGION* region = (REGION*) this;
			float w = (region->m_flag & 8) ? Map->m_w : region->m_w;
			float h = (region->m_flag & 8) ? Map->m_h : region->m_h;
			unsigned int density = (unsigned int) (h * w / childVid->m_footprintWidth / childVid->m_footprintHeight);
			if (density && CurrentTime != PrevCurrentTime) {
				int odds = 1000 / (CurrentTime - PrevCurrentTime) / density;
				if (!(GameRand() % (odds + 1))) {
					CreateChild();
				}
			}
		}
		else if (childVid->m_flag & 0x80) {
			EX_SPRITE_DATA* exData = m_exData;
			if (CurrentTime - CurrentTime % exData->m_unk0x14 > PrevCurrentTime) {
				float vx = ANGLE::SinTable[m_dir] * m_speed;
				float vy = m_unk0x24 - childVid->m_unk0x30 + ANGLE::CosTable[m_dir] * m_speed;
				if (childVid->m_flag & 0x1000) {
					vx = vx - ANGLE::SinTable[Graph->m_windDirection] * Graph->m_windForce;
					vy = vy - ANGLE::CosTable[Graph->m_windDirection] * Graph->m_windForce;
				}
				float px = (vx != 0.0f) ? childVid->m_footprintWidth / (vx < 0.0f ? -vx : vx) : 30000.0f;
				float py = (vy != 0.0f) ? childVid->m_footprintHeight / (vy < 0.0f ? -vy : vy) : 30000.0f;
				if (px < py) {
					py = px;
				}
				exData->m_unk0x14 = (int) py;
				if (m_ani == 8 && (m_vid->m_exData->m_unk0x04 & 0x10)) {
					(unsigned int&) m_exData->m_unk0x14 >>= 1;
				}
				if ((unsigned int) m_exData->m_unk0x14 > 30000) {
					m_exData->m_unk0x14 = 30000;
				}
				if (!m_exData->m_unk0x14) {
					m_exData->m_unk0x14 = 1;
				}
				CreateChild();
			}
		}
	}

	if (m_createTime == CurrentTime) {
		return;
	}
	if (CurrentTime - CurrentTime % m_vid->m_aniDuration[m_ani] <= PrevCurrentTime) {
		return;
	}

	if (m_flag & 0x7c) {
		int cmd = (m_flag >> 2) & 0x1f;
		if (cmd < 16 && !m_goal) {
			MYERROR::Error(
				::Error,
				"SPRITE %i",
				10,
				// STRING: ALIEN 0x48440c
				"command need goal, but goal==NULL",
				cmd,
				m_vid ? m_vid->m_idx : -1
			);
			SetCommand(0, 0);
		}
	}
	if (m_unk0x50) {
		if (CurrentTime - m_tactTime >= m_unk0x50) {
			int cmd = m_flag & 0x7c;
			m_unk0x50 = 0;
			if (cmd == 72) {
				SetCommand(0, 0);
			}
		}
		else {
			m_unk0x50 = m_unk0x50 - CurrentTime + m_tactTime;
		}
	}

	for (int script = m_vid->m_unk0x408[m_ani]; script >= 0; script = m_vid->m_unk0x408[m_ani]) {
		if (m_noCadr != m_begCadr && !(m_vid->m_flag & 0x10) &&
			!(Game_IsZS1() && (m_vid->m_flag & 0x400))) {
			break;
		}
		int ani = m_ani;
		if (Map->ScriptRun(script, this, 0, 0)) {
			return;
		}
		if (ani == m_ani) {
			break;
		}
	}

	int sfx = m_vid->m_aniSfx[m_ani];
	if (sfx) {
		if (!(m_flag & 0x200) || Sound->IsLooped(sfx)) {
			m_flag |= 0x200;
			PlaySFX(m_vid->m_aniSfx[m_ani]);
		}
	}

	childVid = (VID*) m_vid->m_aniChildVid[m_ani];
	if (childVid && !(childVid->m_flag & 0x80)) {
		if (m_ani == 8 && (m_vid->m_exData->m_unk0x04 & 0x10)) {
			unsigned int mid = (m_endCadr + m_begCadr + 1) / 2;
			if (m_noCadr == mid || m_noCadr == (unsigned int) ((m_vid->m_flag & 0x40000) ? m_endCadr : m_begCadr)) {
				CreateChild();
			}
		}
		else if ((m_vid->m_flag & 0x10) || m_noCadr == (int) ((m_vid->m_flag & 0x40000) ? m_endCadr : m_begCadr)) {
			CreateChild();
		}
	}

	int frame = ++m_noCadr;
	int ani = m_ani;
	if (ani >= 15 && (frame > m_endCadr || !m_vid->m_noAnimCadr[ani])) {
		m_noCadr = m_endCadr;
		Action(15, 0, 0, 0);
		ScalarDeletingDestructor(1);
		return;
	}
	int cmd = m_flag & 0x7c;
	if (cmd == 68 || cmd == 72) {
		if (ani < 15 && ani != 10 && frame > m_endCadr) {
			if (m_speed != 0.0f) {
				if (ani != 2) {
					ChangeAnimation(2); // ANI_GO
				}
			}
			else if (ani == 2 || ani >= 7) {
				ChangeAnimation(0); // ANI_STAND
			}
		}
	}
	else if (frame > m_endCadr || (m_vid->m_flag & 0x10)) {
		if (!m_actions.m_n || cmd) {
			Action(130, 0, 0, 0); // ACT_NEXT_COMMAND
		}
		else {
			if (((ani < 15 && ani >= 7 && ani != 10) || (ani == 2 && m_speed == 0.0f)) &&
				m_actions.m_data[m_actions.m_n - 1].m_cmd >= 17) {
				ChangeAnimation(0);
			}
			int topCmd = m_actions.m_data[m_actions.m_n - 1].m_cmd;
			if (Game_IsZS1() && topCmd == 79) {
				ACT* wait = &m_actions.m_data[m_actions.m_n - 1];
				if (Map->m_logic.GetActionN((int) wait->m_a) == (int) wait->m_b) {
					--m_actions.m_n;
				}
			}
			else if (topCmd != 73) {
				ACT* act = &m_actions.m_data[--m_actions.m_n];
				if (act->m_cmd < 17) {
					ChangeAnimation(act->m_cmd);
				}
				else {
					Action(act->m_cmd, act->m_a, act->m_b, act->m_c);
				}
			}
		}
	}
	EX_SPRITE_DATA* exData = m_exData;
	if (exData) {
		unsigned int lifetime = exData->m_unk0x10;
		if (lifetime != 999999 && m_ani < 15) {
			if (CurrentTime - m_tactTime < lifetime) {
				exData->m_unk0x10 = lifetime + m_tactTime - CurrentTime;
			}
			else {
				ChangeAnimation(15); // ANI_DEATH
			}
		}
		unsigned int aging = m_vid->m_exData->m_unk0x40;
		if (aging != 999999) {
			if (CurrentTime - m_exData->m_time > aging) {
				m_exData->m_time = CurrentTime;
			}
		}
	}
	int wrap = m_noCadr > m_endCadr;
	m_tactTime = CurrentTime;
	if (wrap) {
		m_noCadr = m_begCadr;
	}
	return;
}

inline static int AniStepTime(const VID* p_vid, int p_ani)
{
	unsigned int duration = p_vid->m_aniDuration[p_ani];
	unsigned int dt = CurrentTime - PrevCurrentTime;
	return (int) (dt > duration ? dt : duration);
}

inline static void ActionFloat(float* p_out, int p_value)
{
	float converted = (float) p_value;
	*p_out = converted;
}

// STUB: ALIEN 0x440830
decomp_intptr SPRITE::Action(int p_action, decomp_intptr p_a, decomp_intptr p_b, decomp_intptr p_c)
{

	switch (p_action & 0xff) {
	case 201:
		ChangeAnimation(p_a);
		ChangeDirection(ANGLE((unsigned char) p_b));
		return 0;
	case 202:
		return m_ani;
	case 203:
		ChangeCoor((float) p_a, (float) p_b, Z());
		return 0;
	case 204:
		ChangeCoor(m_x, m_y, (float) p_a);
		return 0;
	case 9:                     // ACT_SALUT
		if (m_ani != 8) {       // ANI_FIGHT
			ChangeAnimation(9); // ANI_SALUT
		}
		return 0;
	case 112:
		if (!m_exData) {
			m_exData = new EX_SPRITE_DATA(this);
		}
		m_exData->m_unk0x10 = p_a;
		return 0;
	case 118:
		if (Game_IsZS1()) {
			SetGamma(GAMMA(GAMMA::DECODE, (unsigned int) p_a));
		}
		return 0;
	case 119:
		if (Game_IsZS1()) {
			SetCommand((int) p_a, (SPRITE*) p_b);
		}
		return 0;
	case 138:
		if (Game_IsZS1()) {
			if (!m_exData) {
				m_exData = new EX_SPRITE_DATA(this);
			}
			m_exData->m_unk0x20 = (float) p_a * 0.001f;
			if (m_speed > m_exData->m_unk0x20) {
				m_speed = m_exData->m_unk0x20;
			}
		}
		return 0;
	case 134: {
		if (Game_IsZS1() && p_a == -1) {
			ScalarDeletingDestructor(1);
			return 0;
		}
		if (p_a < 0 || p_a >= Map->m_noVid || !Map->m_vids[p_a]) {
			return 0;
		}
		if (Game_IsZS1() && p_a > 0 && p_b == 0 && p_c == 0) {
			while (SPRITE* victim
				   = Map->FindNearestSprite(MAP::MakeVidQuery((int) p_a), 0.0f, 0.0f, 40000.0f, 0)) {
				victim->ScalarDeletingDestructor(1);
			}
			return 0;
		}
		SPRITE* found = Map->GetSpriteScr(MAP::MakeVidQuery((int) p_a), (float) p_b, (float) p_c);
		if (found) {
			found->ScalarDeletingDestructor(1);
		}
		return 0;
	}
	case 35: {
		int nvid = p_a;
		if (!nvid) {
			nvid = (int) Action(59, 4, 0, 0);
		}
		if (nvid <= 0 || nvid >= Map->m_noVid || !Map->m_vids[nvid]) {
			return 0;
		}

		float x;
		ActionFloat(&x, p_b);
		float y;
		ActionFloat(&y, p_c);
		if (p_b == 0 && p_c == 0) {
			x = m_x;
			y = m_y;
		}
		else {
			if (p_b < 0) {
				x = m_x - (float) p_b - (float) (2 * (GameRand() % (1 - p_b)));
			}
			if (p_c < 0) {
				y = m_y - (float) p_c - (float) (2 * (GameRand() % (1 - p_c)));
			}
		}

		SPRITE* child = Map->CreateSprite(Map->Vid(nvid), x, y, GetZ(), Direction(), this);
		if (!child) {
			return 0;
		}
		if (Game_IsZS1() && !ActionStackHaveCommand(73)) {
			return 0;
		}
		Action(75, (decomp_intptr) child, 0, 0);
		return 0;
	}
	case 101:
		return (decomp_intptr) m_child;
	case 103:
		return (decomp_intptr) m_parent;
	case 132:
		m_flag |= 0x100;
		Hash->Delete(this);
		Remove();
		return 0;
	case 43: {
		if (Map->Flagman((int) Map->m_curArmy)) {
			SPRITE* flagman = (SPRITE*) Map->Flagman((int) Map->m_curArmy);
			float dx = (float) fabs((float) p_a - flagman->m_x);
			float dy = (float) fabs((float) p_b - flagman->m_y);
			float dist = dx > dy ? dy * 0.5f + dx : dx * 0.5f + dy;
			if (dist < (float) p_c) {
				return 0;
			}
		}
		ACT act(p_action, p_a, p_b, p_c);
		m_actions.Insert(act);
		return 0;
	}
	case 133:
		m_flag &= ~0x100;
		Hash->Insert(this);
		Insert();
		return 0;
	case 98:
		if (p_a) {
			m_flag |= 0x10000;
			if (m_child) {
				m_child->InvisibleOn();
			}
		}
		else {
			m_flag &= ~0x10000;
			if (m_child) {
				m_child->InvisibleOff();
			}
		}
		return 0;
	case 109:
		return (int) (m_speed * 1000.0f);
	case 110:
		m_speed = (float) p_a * 0.001f;
		return 0;
	case 107:
		return (int) (m_unk0x24 * 1000.0f);
	case 108:
		m_unk0x24 = (float) p_a * 0.001f;
		return 0;
	case 73:
		m_actions.Insert(ACT(p_action, p_a, p_b, p_c));
		Action(130, 0, 0, 0);
		return 0;
	case 72:
		m_actions.Release();
		return 0;
	case 76:
		return m_actions.m_n;
	case 71:
		m_actions.SetNumber(p_a + 1);
		m_noCadr = m_endCadr;
		return 0;
	case 75: {
		SPRITE* dest = (SPRITE*) p_a;
		if (!dest || dest == this) {
			return 0;
		}
		dest->ResetActionStack();
		if (m_actions.m_n <= 0) {
			return 0;
		}
		for (int i = 0; i < m_actions.m_n; ++i) {
			ACT& src = m_actions.m_data[i];
			if (src.m_cmd == 0x49) {
				return 0;
			}
			ACT act(src.m_cmd, src.m_a, src.m_b, src.m_c);
			dest->m_actions.Insert(act);
		}
		return 0;
	}
	case 74:

		SetCommand((p_action >> 8) & 0xff, (SPRITE*) p_a);
		if (p_a) {
			((SPRITE*) p_a)->ReleaseRef();
		}
		return 0;
	case 70: {
		ACT act((((m_flag >> 2) & 0x1f) << 8) + 0x4a, (decomp_intptr) m_goal, 0, 0);
		m_actions.Insert(act);
		if (m_goal) {
			++m_goal->m_noRef;
		}
		return 0;
	}
	case 54:
		InsertItem(p_a);
		return 0;
	case 58:
		if (!m_exData || p_a < 0 || p_a >= m_exData->m_list.m_n) {
			return 0;
		}
		return m_exData->m_list.m_data[p_a];
	case 56: {
		if (!m_exData) {
			return 0;
		}
		int value = p_a;
		if (m_exData->m_list.Location(&value) < 0) {
			return 0;
		}
		return 1;
	}
	case 55: {
		if (!m_exData) {
			return 0;
		}
		int value = p_a;
		int idx = m_exData->m_list.Location(&value);
		if (m_exData->m_list.DeleteNumber(idx)) {
			return 0;
		}
		return 1;
	}
	case 57:
		if (m_exData) {
			LIST_INT& list = m_exData->m_list;
			int* data = list.m_data;
			list.m_max = 0;
			list.m_n = 0;
			delete[] data;
			list.m_data = 0;
		}
		return 0;
	case 59: {
		unsigned int mask = p_a ? (unsigned int) p_a : 0xffffff;
		EX_SPRITE_DATA* ex = m_exData;
		if (!ex) {
			return -1;
		}

		int count = 0;
		for (int i = 0; i < ex->m_list.m_n; ++i) {
			int idx = ex->m_list.m_data[i];
			VID* vid = idx >= 0 && idx < Map->m_noVid ? Map->m_vids[idx] : 0;
			if (!vid) {
				vid = EmptyVid;
			}
			if (vid->m_unk0x0c & mask) {
				++count;
			}
		}

		int pick = GameRand() % count;
		ex = m_exData;
		if (ex->m_list.m_n <= 0) {
			return -1;
		}
		for (int j = 0; j < ex->m_list.m_n; ++j) {
			int idx = ex->m_list.m_data[j];
			VID* vid = idx >= 0 && idx < Map->m_noVid ? Map->m_vids[idx] : 0;
			if (!vid) {
				vid = EmptyVid;
			}
			if ((vid->m_unk0x0c & mask) && --pick < 0) {
				return ex->m_list.m_data[j];
			}
		}
		return -1;
	}
	case 130: {
		if (m_ani >= 0xf) {
			return 0;
		}

		int sway;
		if (m_parent) {
			sway = 1;
		}
		else {
			float speed = m_exData ? m_exData->m_unk0x20 : m_vid->m_unk0x2c;
			sway = speed == 0.0f;
		}
		if (sway && (m_vid->m_flag & 0x1000) && Graph->m_windForce != 0.0f) {
			unsigned char windDir;
			Rotate(*(ANGLE*) Graph->WindDirection(&windDir), AniStepTime(m_vid, m_ani));
		}

		if (m_speed != 0.0f) {
			if (m_ani != 2) {
				ChangeAnimation(2);
			}
		}
		else {
			if (m_ani < 6 || m_ani == 0xa) {
				return 0;
			}
			ChangeAnimation(0);
		}
		return 0;
	}
	case 62: { // ACT_CHANGE_VID
		if (p_a < 0 || p_a >= Map->m_noVid) {
			return 0;
		}
		VID* newVid = Map->m_vids[p_a];
		if (!newVid) {
			return 0;
		}
		if (m_vid->m_idx == p_a) {
			return 0;
		}

		if (m_vid->m_sprClass != newVid->m_sprClass) {
			MYERROR::Error(
				::Error,
				"SPRITE %i",
				4,
				// STRING: ALIEN 0x484494
				"ACT_CHANGE_VID",
				p_a,
				m_vid ? m_vid->m_idx : -1
			);
		}

		for (VID* link = m_vid->m_linkVid; link; link = link->m_linkVid) {
			DestroyLink(link);
		}

		m_flag &= ~0x2000;
		Hash->Delete(this);
		Remove();
		int oldArmy = (m_flag >> 11) & 3;
		if (m_vid->m_entitiesNumber[oldArmy]) {
			--m_vid->m_entitiesNumber[oldArmy];
		}

		VID* target = p_a >= 0 && p_a < Map->m_noVid && Map->m_vids[p_a] ? Map->m_vids[p_a] : EmptyVid;
		m_vid = target;
		target->m_unk0x458 = RealCurrentTime;
		int newArmy = (m_flag >> 11) & 3;
		++target->m_entitiesNumber[newArmy];

		int anim = p_b >= 0 ? p_b : m_ani;
		unsigned char savedDir = m_dir;
		AngleAssign((ANGLE*) &m_dir, ANGLE(0));
		m_ani = 0;
		m_begCadr = 0;
		m_noCadr = 0;
		m_endCadr = m_vid->m_aniDirCadrs[0] - 1;

		if (m_vid->m_unk0x478) {
			if (!m_exData) {
				m_exData = new EX_SPRITE_DATA(this);
			}
			else {
				m_exData->m_unk0x10 = m_vid->m_unk0x6c;
			}
		}

		Insert();
		Hash->Insert(this);
		CreateLink();
		ChangeAnimation(anim);
		ChangeDirection(ANGLE(savedDir));
		return 0;
	}
	case 135:
		PlaySFX(p_a);
		return 0;
	case 131:
		if (Game_IsZS1()) {
			int value = (int) p_b;
			if ((int) p_c == 1) {
				value += Map->m_logic.GetActionN((int) p_a);
			}
			Map->m_logic.SetActionN((int) p_a, value);
			return 0;
		}
		Map->ScriptRun(p_a, this, 0, 0);
		return 0;
	case 42: // ACT_STOP
		SetCommand(0, 0);
		return 0;
	case 61:
		ChangeAnimation(p_a);
		return 0;
	case 60:
		ChangeDirection(ANGLE((unsigned char) p_a));
		return 0;
	case 63:
		if (HasUIScriptLayout() && Graph && Map) {
			GRAPH_CORE* graph = (GRAPH_CORE*) Graph;
			// Preserve the menu root's assigned anchor.
			UI_SCALING::AXIS_ANCHOR anchorX = UIAnchorX();
			UI_SCALING::AXIS_ANCHOR anchorY = UIAnchorY();
			UI_SCALING::MENU_POINT point = UI_SCALING::TransformAnchoredScriptPoint(
				(float) p_a - Map->m_shiftX,
				(float) p_b - Map->m_shiftY,
				(float) p_c,
				graph->m_width,
				graph->m_height,
				UIDrawScale(),
				anchorX,
				anchorY
			);
			ChangeCoor(point.m_x + Map->m_shiftX, point.m_y + Map->m_shiftY, point.m_z);
			SetUIScriptLayout(UIScale(), anchorX, anchorY);
		}
		else {
			ChangeCoor((float) p_a, (float) p_b, (float) p_c);
		}
		return 0;
	case 105:
		return (decomp_intptr) m_unk0x50;
	case 106:
		m_unk0x50 = p_a;
		return 0;
	case 97:
		ChangeArmy(p_a);
		return 0;
	case 96:
		return (m_flag >> 11) & 3;
	case 90:
		return (decomp_intptr) m_goal;
	case 91: {
		SPRITE* goal = new SPRITE(EmptyVid, (float) p_a, (float) p_b, (float) p_c, ANGLE(0), 0);
		SetGoal(goal);
		return 0;
	}
	case 92:
	case 93:
		if (!m_parent) {
			return 0;
		}
		return m_parent->Action(p_action, p_a, p_b, p_c);
	case 83: // ACT_SET_INVULNERABLE
		m_invulnerable = (int) p_a;
		return 0;
	case 84: // ACT_GET_INVULNERABLE
		return m_invulnerable;
	case 99: // ACT_GET_PARENT
		return (decomp_intptr) m_parent;
	case 111:
		return (m_flag >> 2) & 0x1f;
	case 100: {
		VID* vid = m_vid;
		SPRITE* child = m_child;
		if (child) {
			VID* childVid = child->m_vid;
			if (childVid == vid->m_linkVid && childVid->m_weaponVid && childVid->m_weapon &&
				vid->m_exData->m_unk0x18 == 0.0f) {
				return (int) childVid->m_exData->m_unk0x18;
			}
		}
		return (int) vid->m_exData->m_unk0x18;
	}
	case 87:
		return m_unk0x54;
	case 88:
		if (!p_a) {
			int army = (m_flag >> 11) & 3;
			ChangeHp(m_vid->m_maxHp[army] * p_b / 100);
		}
		else {
			ChangeHp(p_a);
		}
		return 0;
	case 89:
		return PercentHp() * 100 / 255;
	case 85:
		if (m_unk0x54 >= m_vid->m_maxHp[(m_flag >> 11) & 3] && p_a < 0) {
			return 1;
		}
		if (m_ani >= 15) {
			return 0;
		}
		if (m_vid->m_unk0x408[18] >= 0) {
			int hook = Map->ScriptRun(m_vid->m_unk0x408[18], this, (SPRITE*) p_b, p_a);
			if (Game_IsZS1()) {
				if (hook > 0) {
					return 0;
				}
				if (hook < 0) {
					p_a = -hook;
				}
			}
			else if (hook) {
				return 0;
			}
		}
		if (m_vid->m_defaultMaxHp) {
			ChangeHp(m_unk0x54 - p_a);
		}
		if (m_unk0x54 > m_vid->m_maxHp[(m_flag >> 11) & 3] && p_a < 0) {
			m_unk0x54 = m_vid->m_maxHp[(m_flag >> 11) & 3];
		}
		if (p_a <= 0) {
			return 0;
		}
		if (m_vid->m_noAnimCadr[7] && (m_ani == 0 || m_ani == 2)) {
			ChangeAnimation(7);
			return 0;
		}
		if (m_vid->m_unk0x408[7] >= 0 && Map->ScriptRun(m_vid->m_unk0x408[7], this, (SPRITE*) p_b, 0)) {
			return 0;
		}
		if (m_vid->m_aniSfx[7]) {
			PlaySFX(m_vid->m_aniSfx[7]);
		}
		if (m_vid->m_aniChildVid[7]) {
			p_c = m_ani;
			m_ani = 7;
			CreateChild();
			m_ani = p_c;
		}
		return 0;
	case 86: {
		DestroyLink(m_vid->m_unk0x284);
		int army = (m_flag >> 11) & 3;
		m_unk0x54 = m_vid->m_maxHp[army];
		if (m_child && m_child->m_vid == m_vid->m_linkVid) {
			m_child->Action(86, 0, 0, 0);
		}
		else {
			CreateLink();
		}
		return 0;
	}
	case 38: {
		if (GameRand() % 5 == 0) {
			ChangeAnimation(0);
			return 0;
		}
		if (GameRand() % 5 == 0) {
			ChangeAnimation(12);
			return 0;
		}

		if (m_ani == 4 && GameRand() % 3 != 0) {
			Rotate(ANGLE(m_dir) - (unsigned char) 0x40, AniStepTime(m_vid, m_ani));
			return 0;
		}
		if (m_ani == 5 && GameRand() % 3 != 0) {
			Rotate(ANGLE(m_dir) + (unsigned char) 0x40, AniStepTime(m_vid, m_ani));
			return 0;
		}
		if (GameRand() % 4 != 0) {
			return 0;
		}
		if (GameRand() % 2 != 0) {
			Rotate(ANGLE(m_dir) - (unsigned char) 0x40, AniStepTime(m_vid, m_ani));
		}
		else {
			Rotate(ANGLE(m_dir) + (unsigned char) 0x40, AniStepTime(m_vid, m_ani));
		}
		return 0;
	}
	case 40:
		if (m_speed != 0.0f) {
			Stop();
		}
		m_unk0x50 = p_a + GameRand() % (p_b + 1);
		if (m_unk0x50) {
			SetCommand(18, 0);
		}
		return 0;
	case 41: {
		unsigned int duration = m_vid->m_aniDuration[m_ani];
		if (CurrentTime - PrevCurrentTime <= duration) {
			Rotate(ANGLE((unsigned char) p_a), (int) duration);
		}
		return 0;
	}
	case 39:
		Stop();
		if (p_a) {
			m_speed = 0.0f;
		}
		return 0;
	case 33: {
		SPRITE* goal = new SPRITE(EmptyVid, (float) p_a, (float) p_b, (float) p_c, ANGLE(0), 0);
		Move(goal);
		return 0;
	}
	case 34: // ACT_MOVE
		Move((SPRITE*) p_a);
		return 0;
	case 32: // ACT_ATTACK
		Attack((SPRITE*) p_a);
		return 0;
	case 37: {
		int weaponType;
		if (m_child && m_child->m_vid == m_vid->m_linkVid && m_child->m_vid->m_weaponVid && m_child->m_vid->m_weapon) {
			weaponType = m_child->m_vid->m_exData->m_unk0x00;
		}
		else {
			weaponType = m_vid->m_exData->m_unk0x00;
		}

		float gz;
		if (weaponType == 8) {
			gz = Map->GetGroundZ_ff((float) p_a, (float) p_b + 80.0f) + 80.0f;
			p_b += (int) gz;
		}
		else {
			gz = Map->GetGroundZ_ff((float) p_a, (float) p_b) + 19.0f;
			p_b += (int) gz - 19;
		}

		SPRITE* marker = new SPRITE(EmptyVid, (float) p_a, (float) p_b, gz, ANGLE(0), 0);
		SetCommand(4, marker);
		return 0;
	}
	case 200: {
		RESOURCE* res = (RESOURCE*) p_a;
		short count = 0;
		res->Read(&count, 2);
		m_actions.m_n = count;
		m_actions.Expand(count);

		for (int i = 0; i < m_actions.m_n; ++i) {
			int rec[3] = {0, 0, 0};
			res->Read(rec, 12);
			m_actions.m_data[i].m_cmd = rec[0];
			m_actions.m_data[i].m_a = rec[1];
			m_actions.m_data[i].m_b = rec[2];
		}

		if (p_b < 7) {
			m_actions.Release();
		}

		for (int idx = 0; idx < m_actions.m_n; ++idx) {
			ACT& act = m_actions.m_data[idx];
			act.m_cmd &= 0xff;
			act.m_c = 0;
			if (act.m_cmd == 0x28) {
				act.m_cmd = 0x21;
			}
			else if (act.m_cmd == 0x27) {
				act.m_cmd = 0x20;
			}
			else if (act.m_cmd == 0x2f) {
				act.m_cmd = 0x49;
			}
			else {
				MYERROR::Error(
					::Error,
					"SPRITE %i",
					14,
					// STRING: ALIEN 0x48447c
					"actionStack.act restore",
					act.m_cmd,
					m_vid ? m_vid->m_idx : -1
				);
			}
			int cmd = act.m_cmd;
			if (cmd == 0x20 || cmd == 0x22 || cmd == 0x4a || cmd == 0x96 || cmd == 0x97 || cmd == 0x98 || cmd == 0x4b) {
				act.m_a = (decomp_intptr) Map->m_relation.Decode((const void*) act.m_a);
			}
		}

		if (p_b < 7) {
			return 0;
		}
		int army = 0;
		res->Read(&army, 1);
		ChangeArmy(army);
		return 0;
	}
	case 80: { // ACT_SAVE
		RESOURCE* res = (RESOURCE*) p_a;

		if (m_actions.m_n == 1 && m_actions.m_data[0].m_cmd == 0x49) {
			m_actions.Release();
		}

		res->Write(&m_actions.m_n, 4);
		for (int i = 0; i < m_actions.m_n; ++i) {
			const ACT& a = m_actions.m_data[i];
			int rec[4] = {a.m_cmd, (int) a.m_a, (int) a.m_b, (int) a.m_c};
			res->Write(rec, 16);
		}

		LIST_INT list;
		if (m_exData) {
			LIST_INT& src = m_exData->m_list;
			list.m_n = src.m_n;
			list.m_max = src.m_max;
			list.m_data = new int[src.m_max];
			if (!list.m_data) {
				MYERROR::LogExit(
					::Error,
					// STRING: ALIEN 0x48444c
					"!!!ERROR!!!::LIST: Not enough memory for = %i",
					src.m_max
				);
			}
			for (int i = 0; i < list.m_n; ++i) {
				list.m_data[i] = src.m_data[i];
			}
		}
		res->Write(&list.m_n, 4);
		res->Write(list.m_data, 4 * list.m_n);
		return 0;
	}
	case 81: { // ACT_RESTORE
		RESOURCE* res = (RESOURCE*) p_a;

		res->Read(&m_actions.m_n, 4);
		if (m_actions.m_n > m_actions.m_max) {
			ACT* old = m_actions.m_data;
			m_actions.m_data = new ACT[m_actions.m_n];
			if (!m_actions.m_data) {
				MYERROR::LogExit(::Error, "!!!ERROR!!!::LIST: Not enough memory %i", m_actions.m_n);
			}
			if (old) {
				for (int i = 0; i < m_actions.m_max; ++i) {
					m_actions.m_data[i] = old[i];
				}
				delete[] old;
			}
			m_actions.m_max = m_actions.m_n;
		}
		for (int i = 0; i < m_actions.m_n; ++i) {
			int rec[4] = {0, 0, 0, 0};
			res->Read(rec, 16);
			ACT& a = m_actions.m_data[i];
			a.m_cmd = rec[0];
			a.m_a = rec[1];
			a.m_b = rec[2];
			a.m_c = rec[3];
		}

		for (int idx = m_actions.m_n - 1; idx >= 0; --idx) {
			ACT& act = m_actions.m_data[idx];
			int cmd = act.m_cmd;
			if (cmd == 0x20 || cmd == 0x22 || cmd == 0x4a || cmd == 0x96 || cmd == 0x97 || cmd == 0x98 || cmd == 0x4b) {
				act.m_a = (decomp_intptr) Map->m_relation.Decode((const void*) act.m_a);
			}
			else if (idx && cmd == 0x49 && m_actions.m_data[idx - 1].m_cmd == 0x49 && idx < m_actions.m_n) {
				m_actions.m_data[idx] = m_actions.m_data[--m_actions.m_n];
			}
		}

		if (p_b < 12) {
			return 0;
		}

		LIST_INT list;
		res->Read(&list.m_n, 4);
		if (list.m_n > list.m_max) {
			int* old = list.m_data;
			list.m_data = new int[list.m_n];
			if (!list.m_data) {
				MYERROR::LogExit(::Error, "!!!ERROR!!!::LIST: Not enough memory %i", list.m_n);
			}
			if (old) {
				for (int i = 0; i < list.m_max; ++i) {
					list.m_data[i] = old[i];
				}
				delete[] old;
			}
			list.m_max = list.m_n;
		}
		res->Read(list.m_data, 4 * list.m_n);

		if (list.m_n) {
			if (!m_exData) {
				m_exData = new EX_SPRITE_DATA(this);
			}
			LIST_INT& dst = m_exData->m_list;
			delete[] dst.m_data;
			dst.m_n = list.m_n;
			dst.m_max = list.m_max;
			dst.m_data = new int[list.m_max];
			if (!dst.m_data) {
				MYERROR::LogExit(::Error, "!!!ERROR!!!::LIST: Not enough memory for = %i", dst.m_max);
			}
			for (int j = 0; j < dst.m_n; ++j) {
				dst.m_data[j] = list.m_data[j];
			}
		}
		if (Game_IsZS1() && p_b >= 13) {
			STRING name;
			name.Read_res(res);
			if (*name.m_str) {
				if (!m_exData) {
					m_exData = new EX_SPRITE_DATA(this);
				}
				m_exData->m_spriteName = name;
			}
		}
		return 0;
	}
	case 125: {
		static STRING emptyName;
		if (!Game_IsZS1()) {
			MYERROR::Error(::Error, "SPRITE %i", 10, "Action() have not this act", p_action, m_vid ? m_vid->m_idx : -1);
			return 0;
		}
		if (m_exData && *m_exData->m_spriteName.m_str) {
			return (decomp_intptr) &m_exData->m_spriteName;
		}
		emptyName = "";
		return (decomp_intptr) &emptyName;
	}
	case 15: {
		VID* vid = m_vid;
		if (!vid->m_fireDamage) {
			return 0;
		}
		m_unk0x54 = 0;

		float rx = vid->m_unk0x384 + vid->m_blastRadius;
		float ry = vid->m_unk0x388 + vid->m_blastRadius;
		float rz = 20.0f > vid->m_unk0x24 ? 20.0f : vid->m_unk0x24;

		for (SPRITE* t = Hash->FirstInBox(m_x - rx, m_y - ry, m_x + rx, m_y + ry); t; t = Hash->NextInBox()) {
			if (t == this) {
				continue;
			}
			VID* tvid = t->m_vid;
			if (!tvid->m_defaultMaxHp) {
				continue;
			}

			if ((m_vid->m_flag & 0x80000000) && !((t->m_flag ^ m_flag) & 0x1800)) {
				continue;
			}
			if ((float) fabs(m_x - t->m_x) >= rx + tvid->m_unk0x384) {
				continue;
			}
			if ((float) fabs(m_y - t->m_y) >= ry + tvid->m_unk0x388) {
				continue;
			}
			if ((float) fabs(m_z - t->m_z) >= rz + tvid->m_unk0x24) {
				continue;
			}

			if (Map->GetGroundZ_ff((t->m_x + m_x) * 0.5f, (t->m_y + m_y) * 0.5f) <= t->m_z + tvid->m_unk0x24) {
				if (Map->GetGroundZ_ff((m_x * 3.0f + t->m_x) * 0.25f, (m_y * 3.0f + t->m_y) * 0.25f) <=
					t->m_z + tvid->m_unk0x24) {
					if (Map->GetGroundZ_ff((t->m_x * 3.0f + m_x) * 0.25f, (t->m_y * 3.0f + m_y) * 0.25f) <=
						t->m_z + tvid->m_unk0x24) {

						if ((m_vid->m_flag & 0x4000000) && m_vid->m_blastRadius != 0.0f) {

							float dx = (float) fabs(t->m_x - m_x);
							float dy = (float) fabs(t->m_y - m_y);
							float dist = dx > dy ? dy * 0.5f + dx : dx * 0.5f + dy;
							if (dist > m_vid->m_blastRadius) {
								continue;
							}
							float fireDamage = (float) m_vid->m_fireDamage;
							int damage = (int) (fireDamage - fireDamage * dist / m_vid->m_blastRadius);
							t->Action(85, damage, (decomp_intptr) this, 0);
						}
						else {
							t->Action(85, m_vid->m_fireDamage, (decomp_intptr) this, 0);
						}
					}
				}
			}
		}
		return 0;
	}
	case 95:
		return 0;
	default:
		MYERROR::Error(
			::Error,
			"SPRITE %i",
			10,
			// STRING: ALIEN 0x484430
			"Action() have not this act",
			p_action,
			m_vid ? m_vid->m_idx : -1
		);
		return 0;
	}
}

// FUNCTION: ALIEN 0x442660
void SPRITE::DrawSecondaryInfo()
{
	float y = ((GRAPH_CORE*) Graph)->m_viewYMin;
	int goalIdx = m_goal ? m_goal->m_vid->m_idx : 0;
	int hp = m_unk0x54;
	float spriteY = m_y;
	float spriteX = m_x;
	float viewX = ((GRAPH_CORE*) Graph)->m_viewXMin;
	GRAPH_CORE::PrintfXY(
		(GRAPH_CORE*) Graph,
		viewX + 22.0f,
		y,
		// STRING: ALIEN 0x484500
		"Ref=%-3i cmd=%1i ani=%-2i hp=%-3i AT=%i goal=%-3i spd=%-3i,%-3i timer=%i ammo=%i mvE=%1u%1u %i,%i,%i",
		m_noRef,
		(m_flag >> 2) & 0x1f,
		m_ani,
		hp,
		m_unk0x04,
		goalIdx,
		(int) (m_speed * 1000.0f),
		(int) (m_unk0x24 * 1000.0f),
		m_unk0x50,
		(int) Action(92, 0, 0, 0),
		(m_flag >> 14) & 1,
		(m_flag >> 15) & 1,
		(int) spriteX,
		(int) spriteY,
		(int) m_z
	);
	if (m_child && m_child->m_vid == m_vid->m_linkVid) {
		y = y + 12.0f;
		int childGoalIdx = m_child->m_goal ? m_child->m_goal->m_vid->m_idx : 0;
		hp = m_child->m_unk0x54;
		float childViewX = ((GRAPH_CORE*) Graph)->m_viewXMin;
		GRAPH_CORE::PrintfXY(
			(GRAPH_CORE*) Graph,
			childViewX + 22.0f,
			y,
			// STRING: ALIEN 0x4844bc
			"Ref=%-3i cmd=%1i ani=%-2i hp=%-3i AT=%i goal=%-3i timer=%i ammo=%i",
			m_child->m_noRef,
			(m_child->m_flag >> 2) & 0x1f,
			m_child->m_ani,
			hp,
			m_child->m_unk0x04,
			childGoalIdx,
			m_child->m_unk0x50,
			(int) m_child->Action(92, 0, 0, 0)
		);
	}
	if (m_actions.m_n) {
		y = y + 12.0f;
		STRING text;
		text += Printf(
			// STRING: ALIEN 0x4844b4
			"%i - ",
			m_actions.m_n
		);
		for (int i = m_actions.m_n - 1; i >= 0; --i) {
			text += Printf(
				// STRING: ALIEN 0x4844a4
				// The action arguments are pointer-wide; %i would print half.
				"%i(%lli,%lli,%lli) ",
				m_actions.m_data[i].m_cmd,
				(long long) m_actions.m_data[i].m_a,
				(long long) m_actions.m_data[i].m_b,
				(long long) m_actions.m_data[i].m_c
			);
		}
		COLOR white;
		white.m_value = 0xffffffff;
		((GRAPH_CORE*) Graph)->PutsXY(((GRAPH_CORE*) Graph)->GetViewXMin() + 30.0f, y, text, white);
	}
	DrawGoalLine();
}

// FUNCTION: ALIEN 0x442900
void SPRITE::DrawGoalLine()
{
	if (m_goal) {
		Graph->Line(
			(float) ScreenX(),
			(float) ScreenY(),
			(float) Goal()->ScreenX(),
			(float) Goal()->ScreenY(),
			GRAPH_CORE::GREEN
		);
	}
	if (m_child && m_child->m_goal) {
		Graph->Line(
			(float) Child()->ScreenX(),
			(float) Child()->ScreenY(),
			(float) Child()->Goal()->ScreenX(),
			(float) Child()->Goal()->ScreenY(),
			GRAPH_CORE::RED
		);
	}
}

inline static void AssignAngleValue(ANGLE* p_dst, const ANGLE& p_src)
{
	unsigned char value;
	if (p_dst != &p_src) {
		value = p_src.m_dir;
	}
	else {
		value = p_dst->m_dir;
	}
	p_dst->m_dir = value;
}

// FUNCTION: ALIEN 0x442be0
int SPRITE::ChangeDirection(ANGLE p_dir)
{
	unsigned char dir = p_dir.m_dir;
	ANGLE store = p_dir;
	if (m_unk0x24 != 0.0f && (m_vid->m_flag & 0x80000)) {
		float vert = (ANGLE::CosTable[p_dir.m_dir] * m_speed + m_unk0x24) * -1000000.0f;
		float horiz = ANGLE::SinTable[p_dir.m_dir] * m_speed * 1000000.0f;
		AssignAngleValue(&p_dir, Decart2Polar_f(horiz, vert));
	}
	if (m_dir != p_dir.m_dir) {
		if (m_child) {
			VID* vid = m_vid;
			VID* childVid = m_child->m_vid;
			if (childVid == vid->m_linkVid && !(childVid->m_flag & 0x2000000) &&
				(vid->m_unk0x4c != 0.0f || vid->m_unk0x50 != 0.0f)) {
				ANGLE stepped = m_vid->SteppedDirection(p_dir);
				float z = m_child->m_z;
				float y = ANGLE::SinTable2[stepped.m_dir] * m_vid->m_unk0x4c -
						  ANGLE::CosTable2[stepped.m_dir] * m_vid->m_unk0x50 + m_y;
				float x = ANGLE::CosTable[stepped.m_dir] * m_vid->m_unk0x4c -
						  -(ANGLE::SinTable[stepped.m_dir] * m_vid->m_unk0x50) + m_x;
				m_child->ChangeCoor(
					ANGLE::CosTable[stepped.m_dir] * m_vid->m_unk0x4c -
						-(ANGLE::SinTable[stepped.m_dir] * m_vid->m_unk0x50) + m_x,
					ANGLE::SinTable2[stepped.m_dir] * m_vid->m_unk0x4c -
						ANGLE::CosTable2[stepped.m_dir] * m_vid->m_unk0x50 + m_y,
					z
				);
			}
			else if (childVid->m_sprClass == 12 && !(childVid->m_flag & 0x2000000)) {
				ANGLE stepped = m_vid->SteppedDirection(p_dir);
				((LINKER*) m_child)->LinkRotate(stepped);
			}
			if (m_child->m_vid->m_unk0x3c == 0.0f) {
				m_child->ChangeDirection(store);
			}
		}
		VID* vid = m_vid;
		if (m_vid->m_noDir != 1) {
			int len = m_noCadr - m_begCadr;
			int dirCadrs = m_vid->m_aniDirCadrs[m_ani];
			m_begCadr = m_vid->m_aniBegCadr[m_ani];
			m_begCadr += m_vid->m_aniDirCadrs[m_ani] * (int) m_vid->RealDirection(p_dir);
			m_endCadr = m_begCadr + m_vid->m_aniDirCadrs[m_ani] - 1;
			m_noCadr = m_begCadr + len;
		}
		AngleAssign((ANGLE*) &m_dir, store);
	}
	return 0;
}

// FUNCTION: ALIEN 0x4433f0
// Returns the blocking sprite, or null when the position is free. The
// global Mouse is used as the sentinel for "blocked by terrain".
SPRITE* SPRITE::CanPlace(float p_x, float p_y, float p_z)
{
	VID* vid;
	if (!m_vid->m_unk0x18) {
		return 0;
	}
	if (m_vid->m_flag & 0x200) {
		if (Map->GetGroundZ_vid(m_vid, p_x, p_y) - p_z > m_vid->m_unk0x64) {
			return Mouse;
		}
		if (p_z - Map->GetGroundZ_vid(m_vid, p_x, p_y) > m_vid->m_unk0x68) {
			return Mouse;
		}
		vid = m_vid;
		if (vid->m_sprClass != 7) { // B_PLAYER
			float xr = vid->m_unk0x384 - 2.0f;
			float yr = vid->m_unk0x388 - 2.0f;
			float top = p_y - yr;
			float left = p_x - xr;
			if (Map->GetGroundZ_ff(left, top) - p_z > m_vid->m_unk0x64) {
				return Mouse;
			}
			if (p_z - Map->GetGroundZ_ff(left, top) > m_vid->m_unk0x68) {
				return Mouse;
			}
			float bot = yr + p_y;
			if (Map->GetGroundZ_ff(left, bot) - p_z > m_vid->m_unk0x64) {
				return Mouse;
			}
			if (p_z - Map->GetGroundZ_ff(left, bot) > m_vid->m_unk0x68) {
				return Mouse;
			}
			float right = xr + p_x;
			if (Map->GetGroundZ_ff(right, top) - p_z > m_vid->m_unk0x64 ||
				p_z - Map->GetGroundZ_ff(right, top) > m_vid->m_unk0x68 ||
				Map->GetGroundZ_ff(right, bot) - p_z > m_vid->m_unk0x64) {
				return Mouse;
			}
			if (p_z - Map->GetGroundZ_ff(right, bot) > m_vid->m_unk0x68) {
				return Mouse;
			}
		}
	}
	else {
		if (Map->GetGroundZ_vid(m_vid, p_x, p_y) > p_z) {
			return Mouse;
		}
		vid = m_vid;
		if (vid->m_sprClass != 7) { // B_PLAYER
			float xr = vid->m_unk0x384 - 2.0f;
			float yr = vid->m_unk0x388 - 2.0f;
			float top = p_y - yr;
			float left = p_x - xr;
			if (Map->GetGroundZ_ff(left, top) > p_z) {
				return Mouse;
			}
			float bot = yr + p_y;
			if (Map->GetGroundZ_ff(left, bot) > p_z) {
				return Mouse;
			}
			float right = xr + p_x;
			if (Map->GetGroundZ_ff(right, top) > p_z) {
				return Mouse;
			}
			if (Map->GetGroundZ_ff(right, bot) > p_z) {
				return Mouse;
			}
		}
	}
	VID* myVid = m_vid;
	SPRITE* s = Hash->FirstInBox(
		p_x - myVid->m_unk0x384,
		p_y - myVid->m_unk0x388,
		p_x + myVid->m_unk0x384,
		p_y + myVid->m_unk0x388
	);
	while (s) {
		if (s != this) {
			vid = m_vid;
			if (s->m_ani < 15) {
				VID* v = s->m_vid;
				if ((float) fabs(s->m_x - p_x) < v->m_unk0x384 + vid->m_unk0x384 &&
					(float) fabs(s->m_y - p_y) < v->m_unk0x388 + vid->m_unk0x388 && v->m_unk0x24 + s->m_z >= p_z &&
					p_z + vid->m_unk0x24 >= s->m_z && (v->m_unk0x18 & vid->m_unk0x18)) {
					return s;
				}
			}
		}
		s = Hash->NextInBox();
	}
	return 0;
}

// FUNCTION: ALIEN 0x443810
SPRITE* SPRITE::CanPlaceWithCrush(float p_x, float p_y, float p_z)
{
	if (!m_vid->m_unk0x18) {
		return 0;
	}
	if (m_vid->m_flag & 0x200) {
		if (Map->GetGroundZ_vid(m_vid, p_x, p_y) - p_z > m_vid->m_unk0x64) {
			return Mouse;
		}
		if (p_z - Map->GetGroundZ_vid(m_vid, p_x, p_y) > m_vid->m_unk0x68) {
			return Mouse;
		}
		if (m_vid->m_sprClass != 7) { // B_PLAYER
			float xr = m_vid->m_unk0x384 - 2.0f;
			float yr = m_vid->m_unk0x388 - 2.0f;
			float top = p_y - yr;
			float left = p_x - xr;
			if (Map->GetGroundZ_ff(left, top) - p_z > m_vid->m_unk0x64) {
				return Mouse;
			}
			if (p_z - Map->GetGroundZ_ff(left, top) > m_vid->m_unk0x68) {
				return Mouse;
			}
			float bot = yr + p_y;
			if (Map->GetGroundZ_ff(left, bot) - p_z > m_vid->m_unk0x64) {
				return Mouse;
			}
			if (p_z - Map->GetGroundZ_ff(left, bot) > m_vid->m_unk0x68) {
				return Mouse;
			}
			float right = xr + p_x;
			if (Map->GetGroundZ_ff(right, top) - p_z > m_vid->m_unk0x64 ||
				p_z - Map->GetGroundZ_ff(right, top) > m_vid->m_unk0x68 ||
				Map->GetGroundZ_ff(right, bot) - p_z > m_vid->m_unk0x64) {
				return Mouse;
			}
			if (p_z - Map->GetGroundZ_ff(right, bot) > m_vid->m_unk0x68) {
				return Mouse;
			}
		}
	}
	else {
		if (Map->GetGroundZ_vid(m_vid, p_x, p_y) > p_z) {
			return Mouse;
		}
		if (m_vid->m_sprClass != 7) { // B_PLAYER
			float xr = m_vid->m_unk0x384 - 2.0f;
			float yr = m_vid->m_unk0x388 - 2.0f;
			float top = p_y - yr;
			float left = p_x - xr;
			if (Map->GetGroundZ_ff(left, top) > p_z) {
				return Mouse;
			}
			float bot = yr + p_y;
			if (Map->GetGroundZ_ff(left, bot) > p_z) {
				return Mouse;
			}
			float right = xr + p_x;
			if (Map->GetGroundZ_ff(right, top) > p_z) {
				return Mouse;
			}
			if (Map->GetGroundZ_ff(right, bot) > p_z) {
				return Mouse;
			}
		}
	}
	VID* myVid = m_vid;
	SPRITE* s = Hash->FirstInBox(
		p_x - myVid->m_unk0x384,
		p_y - myVid->m_unk0x388,
		p_x + myVid->m_unk0x384,
		p_y + myVid->m_unk0x388
	);
	while (s) {
		if (s != this) {
			VID* vid = m_vid;
			if (s->m_ani < 15) {
				VID* v = s->m_vid;
				if ((float) fabs(s->m_x - p_x) < v->m_unk0x384 + vid->m_unk0x384 &&
					(float) fabs(s->m_y - p_y) < v->m_unk0x388 + vid->m_unk0x388 && v->m_unk0x24 + s->m_z >= p_z &&
					p_z + vid->m_unk0x24 >= s->m_z) {
					if (v->m_unk0x18 & vid->m_unk0x18) {
						return s;
					}
					if (v->m_flag & 0x4000) {
						s->Action(85, 5, 0, 0); // ACT_DAMAGE
					}
				}
			}
		}
		s = Hash->NextInBox();
	}
	return 0;
}

// GLOBAL: ALIEN 0x5da54c
int no_moved_sprite;

int SPRITE::IsXYCross(const VID* p_vid, float p_x, float p_y) const
{
	return (float) fabs(m_x - p_x) < m_vid->m_unk0x384 + p_vid->m_unk0x384 &&
		   (float) fabs(m_y - p_y) < m_vid->m_unk0x388 + p_vid->m_unk0x388;
}

// STUB: ALIEN 0x443c40
SPRITE* SPRITE::CanPlaceWithCrushAndGlide(float* p_x, float* p_y, float* p_z)
{
	if (m_vid->m_unk0x18) {
		SPRITE* blocker = CanPlace(*p_x, *p_y, *p_z);
		if (!blocker) {
			goto placed;
		}
		{
			int script = m_vid->m_unk0x408[19];
			if (script >= 0 && Map->ScriptRun(script, this, blocker == Mouse ? 0 : blocker, 0)) {
				*p_x = X();
				*p_y = Y();
				*p_z = Z();
				return 0;
			}
		}
		if (blocker != Mouse && (blocker->m_vid->m_exData->m_unk0x04 & 0x40)) {
			SPRITE* other = blocker;
			float x2 = other->m_x + *p_x - m_x;
			float y2 = other->m_y + *p_y - m_y;
			float z2 = other->Z();
			++no_moved_sprite;
			if (no_moved_sprite < 5 && !other->CanPlaceWithCrushAndGlide(&x2, &y2, &z2)) {
				if (m_vid->m_exData->m_unk0x04 & 0x40) {
					unsigned char opposite = (unsigned char) (m_dir + 0x80);
					unsigned char d1 = (unsigned char) (other->m_dir - opposite);
					unsigned char d2 = (unsigned char) (opposite - other->m_dir);
					unsigned char delta = d1 < d2 ? d1 : d2;
					if (delta < 0x10) {
						if (m_dir < 0x80) {
							other->ChangeCoor(x2, y2, z2);
						}
						goto popDepth;
					}
				}
				other->ChangeCoor(x2, y2, z2);
			}
		popDepth:
			if (no_moved_sprite) {
				--no_moved_sprite;
			}
		}
		if (!CanPlace(X(), *p_y, *p_z)) {
			*p_x = X();
			goto placed;
		}
		if (!CanPlace(*p_x, Y(), *p_z)) {
			*p_y = Y();
			goto placed;
		}
		if (blocker != Mouse) {
			SPRITE* other = blocker;
			if (other->m_ani < 15) {
				VID* v = other->m_vid;
				VID* mine = m_vid;
				if (other->IsXYCross(mine, m_x, m_y) && v->m_unk0x24 + other->m_z >= m_z &&
					m_z + mine->m_unk0x24 >= other->m_z) {
					goto placed;
				}
			}
		}
		*p_x = X();
		*p_y = Y();
		*p_z = Z();
		return blocker;

	placed:
		if (m_vid->m_flag & 0x200) {
			*p_z = Map->GetGroundZ_vid(m_vid, *p_x, *p_y);
		}
	}
	return 0;
}

// STUB: ALIEN 0x443ed0
ANGLE SPRITE::GlideDirection(ANGLE p_dir)
{
	unsigned char a3 = p_dir.m_dir;
	if (a3 <= 0x60 || a3 >= 0xa0) {
		if (a3 > 0xe0 || a3 < 0x20) {
			float hw = m_vid->m_footprintWidth * 0.5f;
			float hh = m_vid->m_footprintHeight * 0.5f;
			if (CanPlace(
					m_x - (hw = m_vid->m_footprintWidth * 0.5f),
					m_y - (hh = m_vid->m_footprintHeight * 0.5f),
					m_z
				) &&
				!CanPlace(X() + (hw = m_vid->m_footprintWidth * 0.5f), m_y, m_z)) {
				return ANGLE((unsigned char) 40);
			}
			if (CanPlace(
					m_x + (hw = m_vid->m_footprintWidth * 0.5f),
					m_y - (hh = m_vid->m_footprintHeight * 0.5f),
					m_z
				) &&
				!CanPlace(X() - (hw = m_vid->m_footprintWidth * 0.5f), m_y, m_z)) {
				return ANGLE((unsigned char) -40);
			}
		}
		else if (a3 <= 0xb0 || a3 >= 0xd0) {
			if (a3 > 0x30 && a3 < 0x50) {
				float hw = m_vid->m_footprintWidth * 0.5f;
				float hh = m_vid->m_footprintHeight * 0.5f;
				if (CanPlace(m_x + hw, m_y + hh, m_z) && !CanPlace(m_x, m_y - hh, m_z)) {
					return ANGLE((unsigned char) 40);
				}
				if (CanPlace(m_x + hw, m_y - hh, m_z) && !CanPlace(m_x, m_y + hh, m_z)) {
					return ANGLE((unsigned char) 88);
				}
			}
		}
		else {
			float hw = m_vid->m_footprintWidth * 0.5f;
			float hh = m_vid->m_footprintHeight * 0.5f;
			if (CanPlace(m_x - hw, m_y + hh, m_z) && !CanPlace(m_x, m_y - hh, m_z)) {
				return ANGLE((unsigned char) -40);
			}
			if (CanPlace(m_x - hw, m_y - hh, m_z) && !CanPlace(m_x, m_y + hh, m_z)) {
				return ANGLE((unsigned char) -88);
			}
		}
	}
	else {
		float hw = m_vid->m_footprintWidth * 0.5f;
		float hh = m_y;
		if (CanPlace(X() - (hw = m_vid->m_footprintWidth * 0.5f), hh + m_vid->m_footprintHeight * 0.5f, m_z) &&
			!CanPlace(X() + (hw = m_vid->m_footprintWidth * 0.5f), m_y, m_z)) {
			return ANGLE((unsigned char) 88);
		}
		if (CanPlace(m_x + hw, (hh = m_y) + m_vid->m_footprintHeight * 0.5f, m_z) && !CanPlace(m_x - hw, m_y, m_z)) {
			return ANGLE((unsigned char) -88);
		}
	}
	return ANGLE((unsigned char) a3);
}

// FUNCTION: ALIEN 0x444750
int SPRITE::SetCommandWithoutLink(int p_cmd, SPRITE* p_goal)
{
	if ((m_flag & 0x7c) == 0x48 && p_cmd != 18) {
		m_unk0x50 = 0;
	}
	SetGoal(p_goal);
	if (p_cmd < 16 && !m_goal) {
		m_flag &= 0xffffff83;
		return 1;
	}
	m_flag = (m_flag & 0xffffff83) | ((p_cmd & 0x1f) << 2);
	return 0;
}

// STUB: ALIEN 0x444dd0
void SPRITE::CreateChild()
{
	VID* childVid = (VID*) m_vid->m_aniChildVid[m_ani];
	int count = m_vid->m_aniFireCount[m_ani];
	count = abs(count);
	int barrel = 0;
	if (!childVid) {
		return;
	}
	if (childVid->m_prop) {
		return;
	}
	ANGLE dir = (childVid->m_flag & 0x2000000) ? ANGLE(0) : m_vid->SteppedDirection(ANGLE(m_dir));
	if (count == 2 && (m_vid->m_exData->m_unk0x04 & 0x10)) {
		if (m_flag & 0x2000) {
			barrel = 1;
			m_flag &= ~0x200;
		}
		else {
			count = 1;
		}
		m_flag = (m_flag & ~0x2000u) | (~m_flag & 0x2000u);
	}
	for (; barrel < count; ++barrel) {
		float ox;
		float oy;

		float bx;
		float by;
		if (m_vid->m_unk0x20c[m_ani] < 0) {
			if (m_vid->m_sprClass == 23 // B_REGION
				&& m_vid->m_unk0x140[m_ani] == 0.0f && m_vid->m_unk0x184[m_ani] == 0.0f) {
				REGION* region = (REGION*) this;
				bx = 0.0f;
				by = 0.0f;
				if (region->m_flag & 8) {
					float mapW = (float) Map->m_w;
					float selfX = m_x;
					ox = (float) GameRand() * mapW * 3.0518509e-5f - selfX;
					float mapH = (float) Map->m_h;
					float selfY = m_y;
					float selfZ = m_z;
					oy = (float) GameRand() * mapH * 3.0518509e-5f - selfY + selfZ;
				}
				else {
					float randW = region->m_w;
					float randomOffset = (float) GameRand() * randW * 3.0518509e-5f;
					ox = region->m_w * 0.5f - randomOffset;
					float randH = region->m_h;
					randomOffset = (float) GameRand() * randH * 3.0518509e-5f;
					oy = region->m_h * 0.5f - randomOffset + m_z;
				}
			}
			else {
				float jx = m_vid->m_unk0x140[m_ani] + m_vid->m_unk0x140[m_ani];
				float randomOffset = (float) GameRand() * jx * 3.0518509e-5f;
				jx = m_vid->m_unk0x140[m_ani] - randomOffset;
				float jy = m_vid->m_unk0x184[m_ani] + m_vid->m_unk0x184[m_ani];
				randomOffset = (float) GameRand() * jy * 3.0518509e-5f;
				jy = m_vid->m_unk0x184[m_ani] - randomOffset;
				bx = -(ANGLE::CosTable[dir.m_dir] * jx);
				by = -(ANGLE::SinTable2[dir.m_dir] * jx);
				ox = ANGLE::SinTable[dir.m_dir] * jy + bx;
				oy = by - ANGLE::CosTable2[dir.m_dir] * jy;
			}
		}
		else {
			if (barrel == 1) {
				bx = -(ANGLE::CosTable[dir.m_dir] * m_vid->m_unk0x140[m_ani]);
				by = -(ANGLE::SinTable2[dir.m_dir] * m_vid->m_unk0x140[m_ani]);
				ox = ANGLE::SinTable[dir.m_dir] * m_vid->m_unk0x184[m_ani] + bx;
				oy = by - ANGLE::CosTable2[dir.m_dir] * m_vid->m_unk0x184[m_ani];
			}
			else if (barrel == 2) {
				by = 0.0f;
				ox = ANGLE::SinTable[dir.m_dir] * m_vid->m_unk0x184[m_ani];
				bx = 0.0f;
				oy = ANGLE::CosTable2[dir.m_dir] * m_vid->m_unk0x184[m_ani];
			}
			else {
				bx = ANGLE::CosTable[dir.m_dir] * m_vid->m_unk0x140[m_ani];
				by = ANGLE::SinTable2[dir.m_dir] * m_vid->m_unk0x140[m_ani];
				ox = ANGLE::SinTable[dir.m_dir] * m_vid->m_unk0x184[m_ani] + bx;
				oy = by - ANGLE::CosTable2[dir.m_dir] * m_vid->m_unk0x184[m_ani];
			}
		}
		if (childVid->m_sprClass == 2 && Hash->CanPlace(childVid, ox + m_x, oy + m_y, m_vid->m_unk0x1c8[m_ani] + m_z)) {
			continue;
		}
		if (m_ani == 8 && !m_goal) {
			continue;
		}
		SPRITE* child = Map->CreateSprite(
			childVid,
			ox + m_x,
			oy + m_y,
			m_vid->m_unk0x1c8[m_ani] + m_z,
			(childVid->m_flag & 1) ? ANGLE(GameRand() % 256) : ANGLE(m_dir),
			this
		);
		if (child && m_goal && m_ani == 8) {
			VID_EXDATA* vex = m_vid->m_exData;
			if ((vex->m_unk0x04 & 0x20) && m_goal->m_vid != EmptyVid) {
				child->Attack(m_goal);
				child->StartMove();
			}
			else {
				float radius = vex->m_unk0x1c;
				float tx = 0.0f;
				float ty = 0.0f;
				for (int t = 0; t < 5; ++t) {
					float randomOffset = (float) GameRand() * radius * 0.000061037019f;
					tx = radius + m_goal->m_x + bx - randomOffset;
					randomOffset = (float) GameRand() * radius * 0.000061037019f;
					ty = radius + m_goal->m_y + by - randomOffset;
					if (Map->GetGroundZ_ff(tx, ty) < m_goal->m_z) {
						if (Map->GetGroundZ_ff((m_goal->X() + tx) * 0.5f, (m_goal->Y() + ty) * 0.5f) < m_goal->m_z) {
							break;
						}
					}
				}
				float goalZ = m_goal->m_z;
				SPRITE* marker = new SPRITE(EmptyVid, tx, ty, goalZ, ANGLE(0), 0);
				child->Attack(marker);
				child->StartMove();
			}
		}
	}
	if (m_ani == 8 && !(m_flag & 0x2000)) {
		int cmd = m_flag & 0x7c;
		if (cmd == 16 || cmd == 20) {
			if ((!(m_vid->m_flag & 0x10) && !(childVid->m_flag & 0x80)) || m_noCadr == m_endCadr) {

				if (m_goal && m_parent && m_parent->m_goal == m_goal) {
					int pcmd = m_parent->m_flag & 0x7c;
					if (pcmd == 16 || pcmd == 20) {
						m_parent->SetCommand(0, 0);
					}
					if (m_parent->m_vid->m_sprClass == 21 && (m_parent->m_flag & 0x7c) == 0x74) {

						((ENGINE*) m_parent)->SetCommandToTrain(30, 0, 0, 0);
					}
				}
				SetCommand(0, 0);
			}
		}
	}
}

// FUNCTION: ALIEN 0x4454d0
void SPRITE::ChangeHp(int p_hp)
{
	if (p_hp <= 0 && m_vid->m_defaultMaxHp) {
		if (m_ani < 0xf) {
			VID* v = m_vid;
			v->m_deaths[(m_flag >> 11) & 3]++;
			int maxHp = m_vid->m_maxHp[(m_flag >> 11) & 3];
			int overkill = Game_IsZS1() ? 3 * maxHp / 2 : maxHp;
			int hasDeath2
				= Game_IsZS1() ? (m_vid->m_unk0x290 || m_vid->m_noAnimCadr[16]) : (m_vid->m_unk0x290 != 0);
			if (m_unk0x54 - p_hp > overkill && hasDeath2) {
				ChangeAnimation(0x10);
			}
			else {
				ChangeAnimation(0xf);
			}
		}
	}
	else {
		VID* v = m_vid;
		int half = v->m_maxHp[(m_flag >> 11) & 3] / 2;
		if (p_hp > half && m_unk0x54 <= half) {
			DestroyLink(v->m_unk0x284);
		}
		int half2 = m_vid->m_maxHp[(m_flag >> 11) & 3] / 2;
		if (p_hp <= half2 && m_unk0x54 > half2) {
			if (Game_IsZS1() && !(m_vid->m_noAnimCadr[13] && (m_ani == 0 || m_ani == 2))) {
				FireAniEvent(13, 0);
			}
			else {
				ChangeAnimation(0xd);
			}
		}
		m_unk0x54 = p_hp;
	}
}

// FUNCTION: ALIEN 0x445bb0
int SPRITE::ActionStackHaveCommand(int p_cmd)
{
	for (int i = 0; i < m_actions.m_n; i++) {
		if (m_actions.m_data[i].m_cmd == p_cmd) {
			return 1;
		}
	}
	return 0;
}

inline static int EnemyPriority(SPRITE* p_sprite)
{
	SPRITE* child = p_sprite->m_child;
	if (child && child->m_vid == p_sprite->m_vid->m_unk0x5c && child->m_vid->m_weaponVid && child->m_vid->m_unk0x40) {
		return child->m_vid->m_exData->m_unk0x38;
	}
	return p_sprite->m_vid->m_exData->m_unk0x38;
}

// FUNCTION: ALIEN 0x445c80
int SPRITE::IsBetterEnemy(float p_dist, float p_bestDist, SPRITE* p_cand, SPRITE* p_best)
{
	if (!p_best) {
		return 1;
	}
	VID_EXDATA* ex = m_vid->m_exData;
	if (ex->m_unk0x04 & 0x100) {
		return p_bestDist > p_dist;
	}
	SPRITE* forced = m_unk0x6c;
	if (forced) {
		if (p_best == forced) {
			return 0;
		}
		if (p_cand == forced) {
			return 1;
		}
	}
	int candFlag = p_cand->m_flag & 0x1800;
	if (candFlag == 0x1000 && (p_best->m_flag & 0x1800) != 0x1000) {
		return 0;
	}
	if ((p_best->m_flag & 0x1800) != 0x1000 && candFlag == 0x1000) {
		return 1;
	}
	if (p_best->m_vid->m_unk0x0c & 8) {
		if (!(p_cand->m_vid->m_unk0x0c & 8)) {
			return 0;
		}
		ex = m_vid->m_exData;
	}
	if (!(p_best->m_vid->m_unk0x0c & 8) && (p_cand->m_vid->m_unk0x0c & 8)) {
		return 1;
	}
	SPRITE* child = m_child;
	float range;
	if (child && child->m_vid == m_vid->m_unk0x5c && child->m_vid->m_weaponVid && child->m_vid->m_unk0x40 &&
		ex->m_unk0x18 == 0.0f) {
		range = child->m_vid->m_exData->m_unk0x18;
	}
	else {
		range = ex->m_unk0x18;
	}
	if (p_bestDist > range && p_dist <= range) {
		return 1;
	}
	int candSeek = 0;
	if (p_cand->m_vid->m_unk0x0c & 4) {
		candSeek = p_cand->Action(92, 0, 0, 0);
	}
	int bestSeek = 0;
	if (p_best->m_vid->m_unk0x0c & 4) {
		bestSeek = p_best->Action(92, 0, 0, 0);
	}
	if (!bestSeek && candSeek) {
		return 1;
	}
	if (bestSeek && !candSeek) {
		return 0;
	}
	if (EnemyPriority(p_cand) > EnemyPriority(p_best)) {
		return 1;
	}
	return EnemyPriority(p_cand) >= EnemyPriority(p_best) && p_bestDist > p_dist;
}

inline static int HasAttackWeapon(const VID* p_vid)
{
	return p_vid->m_aniChildVid[8] && p_vid->m_weapon;
}

inline static int IsAttackBusy(const SPRITE* p_sprite)
{
	return p_sprite->m_unk0x50 > 5000 || (p_sprite->m_ani == 8 && p_sprite->m_noCadr <= p_sprite->m_endCadr);
}

// STUB: ALIEN 0x4460b0
int SPRITE::AttackTact(int p_time)
{
	SPRITE* child = m_child;
	if (child) {
		VID* childVid = child->m_vid;
		if (childVid == m_vid->m_linkVid && childVid->m_aniChildVid[8] && childVid->m_weapon) {
			int result = child->AttackTact(p_time);
			if (result == 5 && m_goal) {
				return 6;
			}
			return result;
		}
	}
	VID* vid = m_vid;
	if (!HasAttackWeapon(vid)) {
		return 8;
	}
	SPRITE* goal = m_goal;
	if (!goal) {
		if (m_parent && !m_unk0x50) {
			Rotate(m_parent->Direction(), p_time);
		}
		return 5;
	}
	if (IsAttackBusy(this)) {
		int d;
		Rotate(DirectionTo(Goal(), &d), p_time);
		return 4;
	}
	int cmd = m_flag & 0x7c;
	if (cmd != 20 && cmd != 12 && cmd != 16) {
		if (m_parent && !m_unk0x50) {
			Rotate(m_parent->Direction(), p_time);
		}
		return 6;
	}
	VID_EXDATA* vex = vid->m_exData;
	float dx = (float) fabs(goal->m_x - m_x);
	float dy = (float) fabs(goal->m_y - m_y);
	float dist;
	if (dx > dy) {
		dist = dx + dy * 0.5f;
	}
	else {
		dist = dx * 0.5f + dy;
	}
	if (m_parent && cmd != 20 && (dist >= vex->m_unk0x18 || dist <= vex->m_unk0x3c)) {
		Rotate(m_parent->Direction(), p_time);
	}
	else {
		int d;
		ANGLE aim = DirectionTo(Goal(), &d);
		if (!Rotate(aim, p_time).m_dir || (vex->m_unk0x04 & 1)) {
			if (dist <= vex->m_unk0x18) {
				int cost = m_vid->m_aniFireCount[8];
				if ((int) Action(92, 0, 0, 0) >= abs(cost)) {
					Action(93, -cost, 0, 0);
					ChangeAnimation(8); // ANI_FIGHT
					m_unk0x50 = m_vid->m_exData->m_unk0x20 + 5000;
					return 0;
				}
				return 7;
			}
			if ((m_flag & 0x7c) == 12) {
				return 1;
			}
			if (vex->m_unk0x14 + vex->m_unk0x14 <= dist) {
				return 3;
			}
			return 2;
		}
	}
	if (!(dist <= vex->m_unk0x18)) {
		if ((m_flag & 0x7c) == 12) {
			return 1;
		}
		if (!(vex->m_unk0x14 + vex->m_unk0x14 <= dist)) {
			return 2;
		}
		return 3;
	}
	return 4;
}

// FUNCTION: ALIEN 0x446760
void SPRITE::InsertItem(int p_vidIdx)
{
	if (!m_exData) {
		m_exData = new EX_SPRITE_DATA(this);
	}

	LIST_INT* list = &m_exData->m_list;
	int max = list->m_max;
	int n = list->m_n;
	if (n >= max) {
		int newMax = 2 * max + 4;
		if (newMax > max) {
			int* oldData = list->m_data;
			list->m_data = new int[newMax];
			if (!list->m_data) {
				MYERROR::LogExit(::Error, "!!!ERROR!!!::LIST: Not enough memory %i", newMax);
			}
			if (oldData) {
				for (int i = 0; i < list->m_max; ++i) {
					list->m_data[i] = oldData[i];
				}
				delete[] oldData;
			}
			list->m_max = newMax;
		}
	}
	list->m_data[list->m_n++] = p_vidIdx;
}

// STUB: ALIEN 0x446810
int SPRITE::InsertUniqueItem(int p_vidIdx)
{
	if (!m_exData) {
		m_exData = new EX_SPRITE_DATA(this);
	}

	LIST_INT* list = &m_exData->m_list;
	int n = list->m_n;
	int i = n;
	if (i) {
		int* p = list->m_data + n;
		do {
			--i;
			if (*--p != p_vidIdx) {
				continue;
			}
			if (i >= 0) {
				return 1;
			}
			break;
		} while (i);
	}

	int max = list->m_max;
	if (list->m_n >= max) {
		int newMax = 2 * max + 4;
		if (newMax > max) {
			int* oldData = list->m_data;
			list->m_data = new int[newMax];
			if (!list->m_data) {
				MYERROR::LogExit(::Error, "!!!ERROR!!!::LIST: Not enough memory %i", newMax);
			}
			if (oldData) {
				for (int i = 0; i < list->m_max; ++i) {
					list->m_data[i] = oldData[i];
				}
				delete[] oldData;
			}
			list->m_max = newMax;
		}
	}
	list->m_data[list->m_n++] = p_vidIdx;
	return 0;
}

// FUNCTION: ALIEN 0x44bd10
int SPRITE::Release()
{
	int refs = m_noRef - 1;
	m_noRef = refs;
	if (refs <= 0) {
		if (refs < 0) {
			MYERROR::Error(::Error, "SPRITE %i", 4, "noRef at Release", refs, m_vid ? m_vid->m_idx : -1);
			return 0;
		}
		ScalarDeletingDestructor(1);
		return 0;
	}
	return refs;
}
