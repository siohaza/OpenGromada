#include "sprite/sprite.h"

#include "audio/sound.h"
#include "game/engine.h"
#include "game/gametime.h"
#include "game/map.h"
#include "game/viewport_math.h"
#include "gfx/gamma.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "sprite/ex_sprite_data.h"
#include "util/game_random.h"
#include "util/myerror.h"
#include "util/polar.h"
#include "util/string.h"
#include "video/vid.h"
#include "video/vid_exdata.h"
#include "world/hash_map.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

inline void GammaAssign(GAMMA* p_dst, const GAMMA& p_src)
{
	p_dst->m_a = p_src.m_a;
	p_dst->m_b = p_src.m_b;
}

// FUNCTION: ALIEN 0x410d40
float SPRITE::NearDistanceTo(float p_x, float p_y)
{
	float x = (float) fabs(p_x);
	float y = (float) fabs(p_y);
	if (x > y) {
		return x + y * 0.5f;
	}
	return x * 0.5f + y;
}

// FUNCTION: ALIEN 0x415040
float SPRITE::ScreenX()
{
	return m_x - Map->m_shiftX;
}

// FUNCTION: ALIEN 0x415050
float SPRITE::ScreenY()
{
	return m_y - m_z - Map->m_shiftY;
}

// FUNCTION: ALIEN 0x415060
float SPRITE::GetZ()
{
	return m_z;
}

// FUNCTION: ALIEN 0x41ef40
float SPRITE::GetX()
{
	return m_x;
}

// FUNCTION: ALIEN 0x41ef50
float SPRITE::GetY()
{
	return m_y;
}

// FUNCTION: ALIEN 0x43a850
VID* SPRITE::GetVid()
{
	return m_vid;
}

// FUNCTION: ALIEN 0x43a860
EX_SPRITE_DATA* SPRITE::ExData()
{
	return m_exData;
}

// FUNCTION: ALIEN 0x43a870
int SPRITE::IsClass(int p_class)
{
	return m_vid->m_sprClass == p_class;
}

// FUNCTION: ALIEN 0x43a890
int SPRITE::IsCommand(int p_command)
{
	return p_command == (int) ((m_flag >> 2) & 0x1f);
}

// FUNCTION: ALIEN 0x43f180
SPRITE* SPRITE::Child()
{
	return m_child;
}

// FUNCTION: ALIEN 0x442620
void SPRITE::InvisibleOn()
{
	m_flag |= SPRITE_FLAG_INVISIBLE;
	SPRITE* s = m_child;
	if (s) {
		do {
			s->m_flag |= SPRITE_FLAG_INVISIBLE;
			s = s->m_child;
		} while (s);
	}
}

// FUNCTION: ALIEN 0x442640
void SPRITE::InvisibleOff()
{
	m_flag &= ~SPRITE_FLAG_INVISIBLE;
	SPRITE* s = m_child;
	if (s) {
		do {
			s->m_flag &= ~SPRITE_FLAG_INVISIBLE;
			s = s->m_child;
		} while (s);
	}
}

// FUNCTION: ALIEN 0x4429e0
SPRITE* SPRITE::Goal()
{
	return m_goal;
}

// FUNCTION: ALIEN 0x4429f0
int SPRITE::Remove()
{
	SPRITE* child = m_child;
	if (child) {
		child->Remove();
	}
	return Map->RemoveSpriteFromLayer(this);
}

// FUNCTION: ALIEN 0x442a10
void SPRITE::Insert()
{
	SPRITE* child = m_child;
	if (child) {
		child->Insert();
	}
	Map->InsertSpriteToLayer(this);
}

// FUNCTION: ALIEN 0x442a30
unsigned int SPRITE::Write(STREAM* p_stream)
{
	unsigned int result = m_flag;
	if (!(result & 0x100)) {
		STREAM* stream = p_stream;
		unsigned int army = (result >> 11) & 3;
		// Save the low 32-bit identity token; RELATION resolves it on load.
		int id = (int) (decomp_intptr) this;
		stream->Write(&id, 4);
		stream->Write(&m_vid->m_idx, 4);
		stream->Write(&m_x, 4);
		stream->Write(&m_y, 4);
		stream->Write(&m_z, 4);
		int dir = m_dir;
		stream->Write(&dir, 4);
		return stream->Write(&army, 4);
	}
	return result;
}

// FUNCTION: ALIEN 0x442ad0
void SPRITE::ChangeArmy(int p_army)
{
	unsigned int oldFlag = m_flag;
	int oldArmy = (oldFlag >> 11) & 3;
	m_flag = (oldFlag & ~0x1800) | ((p_army & 3) << 11);
	if (m_child) {
		m_child->ChangeArmy((m_flag >> 11) & 3);
	}
	int newMax = m_vid->m_maxHp[(m_flag >> 11) & 3];
	int oldMax = m_vid->m_maxHp[oldArmy & 3];
	if (newMax != oldMax) {
		ChangeHp(m_unk0x54 * 256 / oldMax * newMax / 256);
	}
	if (m_vid->m_flag & 0x20000) {
		if (((m_flag >> 11) & 3) != Map->m_curArmy) {
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
	}
	VID* vid = m_vid;
	if (vid->m_entitiesNumber[oldArmy]) {
		vid->m_entitiesNumber[oldArmy]--;
	}
	int newArmy = (m_flag >> 11) & 3;
	vid = m_vid;
	vid->m_unk0x458 = RealCurrentTime;
	vid->m_entitiesNumber[newArmy]++;
}

// STUB: ALIEN 0x442ee0
void SPRITE::ChangeAnimation(int p_ani)
{
	if (p_ani >= 0x11) {
		MYERROR::Error(
			::Error,
			"SPRITE %i",
			4,
			// STRING: ALIEN 0x484568
			"new_animation in ChangeAnimation",
			p_ani,
			m_vid ? m_vid->m_idx : -1
		);
		return;
	}
	SPRITE* child = m_child;
	if (child) {
		VID* v = child->m_vid;
		if (v == m_vid->m_unk0x5c) {
			if ((!v->m_unk0x40 && !(v->m_flag & 0x1000) && child->m_ani != 8 && child->m_ani < 0xf) || p_ani == 0xf ||
				p_ani == 0x10) {
				child->ChangeAnimation(p_ani);
			}
		}
	}
	if (m_ani == p_ani) {
		return;
	}
	m_flag &= ~0x200u;
	ANGLE dir;
	if (m_unk0x24 != 0.0f && (m_vid->m_flag & 0x80000)) {
		float x = (ANGLE::CosTable[m_dir] * m_speed + m_unk0x24) * -1000000.0f;
		AngleAssign(&dir, Decart2Polar_f(ANGLE::SinTable[m_dir] * m_speed * 1000000.0f, x));
	}
	else {
		AngleAssign(&dir, ANGLE(m_dir));
	}
	VID* vid = m_vid;
	m_begCadr = vid->m_aniBegCadr[p_ani];
	m_begCadr += m_vid->RealDirection(dir) * vid->m_aniDirCadrs[p_ani];
	if (p_ani >= 0xd && !vid->m_noAnimCadr[p_ani]) {
		m_endCadr = m_begCadr;
		m_noCadr = m_begCadr;
		m_ani = p_ani;
	}
	else {
		m_endCadr = vid->m_aniDirCadrs[p_ani] + m_begCadr - 1;
		m_noCadr = m_begCadr;
		m_ani = p_ani;
	}
}

// FUNCTION: ALIEN 0x443070
void SPRITE::SetGamma(const GAMMA& p_gamma)
{
	if (!m_exData) {
		m_exData = new EX_SPRITE_DATA(this);
	}
	GammaAssign((GAMMA*) &m_exData->m_unk0x24, p_gamma);
}

static __forceinline int InterpolateGammaCurve(const int* p_values, float p_phase)
{
	int frame = (int) p_phase;
	int result;
	if (frame >= 7) {
		result = p_values[7];
	}
	else {
		result = (int) ((p_values[frame + 1] - p_values[frame]) * (p_phase - frame) + p_values[frame]);
	}
	return result;
}

// FUNCTION: ALIEN 0x4430b0
GAMMA SPRITE::GetGamma()
{
	EX_SPRITE_DATA* ex = m_exData;
	GAMMA* selected;
	int overrideA;
	if (!ex) {
		goto default_gamma;
	}
	overrideA = ((GAMMA*) &ex->m_unk0x24)->m_a;
	selected = (GAMMA*) &ex->m_unk0x24;
	if (overrideA || selected->m_b) {
		goto gamma_selected;
	}
default_gamma:
	selected = &m_vid->m_gamma[(m_flag >> 11) & 3];
gamma_selected:

	GAMMA base;
	base.m_a = selected->m_a;
	base.m_b = selected->m_b;
	if (m_vid->m_unk0x47c & 1) {
		VID_EXDATA* curves = m_vid->m_exData;
		int blue = InterpolateGammaCurve(curves->m_unk0xa4, ex->m_unk0x1c);
		int green = InterpolateGammaCurve(curves->m_unk0x84, ex->m_unk0x1c);
		int red = InterpolateGammaCurve(curves->m_unk0x64, ex->m_unk0x1c);
		int alpha = InterpolateGammaCurve(curves->m_unk0xc4, ex->m_unk0x1c);
		if (alpha < -255) {
			alpha = -255;
		}
		else if (alpha > 255) {
			alpha = 255;
		}

		GAMMA animated;
		animated.m_a = 0;
		animated.m_b = 0;
		if (alpha < 0) {
			animated.m_a = (-alpha) << 24;
		}
		else {
			animated.m_b = alpha << 24;
		}
		animated.SetRed(red);
		animated.SetGreen(green);
		animated.SetBlue(blue);

		GAMMA result;
		GAMMA* combined = result.Add(base, GAMMA(GAMMA::RAW_COPY, animated));
		base.m_a = combined->m_a;
		base.m_b = combined->m_b;
	}
	return GAMMA(GAMMA::RAW_COPY, base);
}

inline static int SpriteMaxHp(const SPRITE* p_sprite)
{
	return p_sprite->m_vid->m_maxHp[(p_sprite->m_flag >> 11) & 3];
}

// FUNCTION: ALIEN 0x444360
int SPRITE::PercentHp()
{
	if (m_child && m_child->m_vid == m_vid->m_unk0x5c && m_child->m_vid->m_weaponVid && m_child->m_vid->m_unk0x40) {
		if (SpriteMaxHp(m_child)) {
			return m_child->m_unk0x54 * 255 / SpriteMaxHp(m_child);
		}
	}
	if (SpriteMaxHp(this)) {
		return m_unk0x54 * 255 / SpriteMaxHp(this);
	}
	return 0;
}

// FUNCTION: ALIEN 0x4443f0
int SPRITE::StartMove()
{
	float speed = m_exData ? m_exData->m_unk0x20 : m_vid->m_unk0x2c;
	if (speed == 0.0f) {
		return 0;
	}
	if (m_goal) {
		if (m_x == m_goal->m_x && m_y == m_goal->m_y) {
			return 0;
		}
		int dist = 0;
		if (m_vid->m_noDir == 1 || (m_vid->m_flag & 1)) {
			float dy = m_goal->m_y - m_y;
			float dx = m_goal->m_x - m_x;
			ChangeDirection(ANGLE(dx, dy, &dist));
		}
		if (m_vid->m_unk0x0c & 0x200 || m_vid->m_sprClass == 5) {
			if (!dist) {
				float dy = m_goal->m_y - m_y;
				float dx = m_goal->m_x - m_x;
				ANGLE(dx, dy, &dist);
				if (!dist) {
					return 0;
				}
			}
			m_unk0x24 = m_vid->CalculateZSpeed(m_goal->m_z - m_z, (float) dist);
		}
		else if (m_goal->m_z > m_z) {
			speed = m_vid->m_unk0x30;
			m_unk0x24 = speed;
		}
		else if (m_goal->m_z < m_z) {
			m_unk0x24 = -m_vid->m_unk0x30;
		}
		else {
			m_unk0x24 = 0;
		}
	}
	m_flag = (m_flag & 0xffff3fff) | 0x80;
	if (m_ani < 0xf && m_ani != 3) {
		if (m_speed != 0.0f) {
			ChangeAnimation(2);
		}
		else {
			ChangeAnimation(3);
		}
	}
	if ((int&) m_vid->m_unk0x34 == 0x497423f0) {
		if (m_exData) {
			speed = m_exData->m_unk0x20;
			m_speed = speed;
			return 1;
		}
		speed = m_vid->m_unk0x2c;
		m_speed = speed;
	}
	return 1;
}

// FUNCTION: ALIEN 0x4445d0
void SPRITE::Stop()
{
	if ((m_flag & 0x7c) == 0 || (m_flag & 0x7c) == 4) {
		SetCommandWithoutLink(0, 0);
	}
	m_flag &= 0xffff3f7f;
	m_unk0x24 = 0;
	if (m_vid->m_unk0x38 == 999999.0f) {
		m_speed = 0;
	}
}

// FUNCTION: ALIEN 0x444620
int SPRITE::AskLine(float* p_x, float* p_y, float* p_z)
{
	return Hash->AskLine(m_vid, m_x, m_y, m_z, p_x, p_y, p_z);
}

// FUNCTION: ALIEN 0x444650
void SPRITE::SetGoal(SPRITE* p_sprite)
{
	if (m_goal != p_sprite) {
		if (m_goal) {
			m_goal->ReleaseRef();
		}
		m_goal = p_sprite;
		if (p_sprite) {
			++p_sprite->m_noRef;
		}
	}
}

// FUNCTION: ALIEN 0x4446c0
int SPRITE::SetCommand(int p_cmd, SPRITE* p_goal)
{
	if ((m_flag & 0x7c) == 0x48 && p_cmd != 0x12) {
		m_unk0x50 = 0;
	}
	SetGoal(p_goal);
	if (m_child && m_child->m_vid == m_vid->m_unk0x5c && m_child->m_vid->m_weaponVid && m_child->m_vid->m_unk0x40) {
		m_child->SetCommand(p_cmd, p_goal);
	}
	if (p_cmd < 0x10 && !m_goal) {
		m_flag &= ~0x7c;
		return 1;
	}
	m_flag = (m_flag & ~0x7c) | ((p_cmd & 0x1f) << 2);
	return 0;
}

// FUNCTION: ALIEN 0x4447b0
void SPRITE::Move(SPRITE* p_goal)
{
	if (SetCommandWithoutLink(1, p_goal) || StartMove()) {
		if (m_child && m_child->m_vid == m_vid->m_unk0x5c && m_child->m_vid->m_weaponVid && m_child->m_vid->m_unk0x40) {
			int cmd = m_child->m_flag & 0x7c;
			if (cmd != 0 && cmd != 0x10) {
				m_child->SetCommand(0, 0);
			}
		}
	}
	else {
		SetCommand(0, 0);
	}
}

inline static ANGLE DirDiff(const unsigned char* p_dir, unsigned char p_target)
{
	unsigned char a = *p_dir - p_target;
	unsigned char b = p_target - *p_dir;
	if (a < b) {
		b = a;
	}
	return ANGLE(b);
}

// STUB: ALIEN 0x444820
ANGLE SPRITE::Rotate(ANGLE p_target, int p_dt)
{
	if (p_target.m_dir == m_dir) {
		return ANGLE(0);
	}
	if ((int&) m_vid->m_unk0x3c == 0x497423f0) {
		ChangeDirection(p_target);
		return ANGLE(1);
	}
	int steps = (int) (p_dt * m_vid->m_unk0x3c + 0.5f);
	if (steps == 0) {
		return DirDiff(&m_dir, p_target.m_dir);
	}
	int adiff = abs(m_dir - p_target.m_dir);
	int rem = 0x100 - adiff;
	int fwd = 0;
	int bwd = 0;
	if (m_dir > p_target.m_dir) {
		if (adiff < rem) {
			fwd = 1;
		}
		else {
			bwd = 1;
		}
	}
	else if (m_dir < p_target.m_dir) {
		if (adiff <= rem) {
			bwd = 1;
		}
		else {
			fwd = 1;
		}
	}
	if (adiff >= rem) {
		adiff = rem;
	}
	if (steps >= adiff) {
		ChangeDirection(p_target);
		return ANGLE(0);
	}
	if (fwd) {
		ChangeDirection(ANGLE((unsigned char) (m_dir - steps)));
	}
	if (bwd) {
		ChangeDirection(ANGLE((unsigned char) (steps + m_dir)));
	}
	return DirDiff(&m_dir, p_target.m_dir);
}

// FUNCTION: ALIEN 0x444970
int SPRITE::AddLink(SPRITE* p_sprite)
{
	if (!p_sprite || p_sprite->m_parent) {
		return 1;
	}
	if (m_child) {
		m_child->m_parent = 0;
		p_sprite->AddLinkToLast(m_child);
	}
	m_child = p_sprite;
	p_sprite->m_parent = this;
	p_sprite->CopyUIScalingFrom(this);
	return 0;
}

// FUNCTION: ALIEN 0x4449c0
int SPRITE::AddLinkToLast(SPRITE* p_sprite)
{
	SPRITE* last = this;
	if (!p_sprite || p_sprite->m_parent) {
		return 1;
	}
	SPRITE* next = last;
	while (next->m_child) {
		next = next->m_child;
	}
	last = next;
	last->m_child = p_sprite;
	p_sprite->m_parent = last;
	p_sprite->CopyUIScalingFrom(last);
	return 0;
}

void SPRITE::SetUIScale(int p_scale)
{
	if (p_scale < 0 || p_scale > 3) {
		p_scale = 0;
	}
	for (SPRITE* sprite = this; sprite; sprite = sprite->m_child) {
		sprite->m_uiScale = p_scale;
	}
}

float SPRITE::UIDrawScale() const
{
	// A zero metadata word marks an ordinary world sprite. UIScale() retains
	// its legacy 1x fallback, but presentation compensation belongs only to
	// explicitly marked FRAME/STEXT UI roots and their children.
	if (!m_uiScale) {
		return 1.0f;
	}
	// VID746 is the authored full-screen pause/options backdrop. It must retain
	// the world's presentation enlargement so counter-scaled controls do not
	// expose live gameplay in strips around its 1024x768 canvas.
	if (m_vid && m_vid->m_idx == 746) {
		return (float) UIScale();
	}
	GRAPH_CORE* graph = (GRAPH_CORE*) Graph;
	return graph ? (float) UIScale() * graph->m_uiPresentationScale : (float) UIScale();
}

void SPRITE::SetUIScriptLayout(int p_scale, UI_SCALING::AXIS_ANCHOR p_anchorX, UI_SCALING::AXIS_ANCHOR p_anchorY)
{
	if (p_scale < 1 || p_scale > 3) {
		SetUIScale(p_scale);
		return;
	}
	int state = p_scale | 0x40 | ((int) p_anchorX << 2) | ((int) p_anchorY << 4);
	for (SPRITE* sprite = this; sprite; sprite = sprite->m_child) {
		unsigned int composition = (unsigned int) sprite->m_uiScale & ~0xffu;
		sprite->m_uiScale = (int) (composition | (unsigned int) state);
	}
}

void SPRITE::SetUIHorizontalGap(int p_width)
{
	if (p_width < 0) {
		p_width = 0;
	}
	if (p_width > 0x7fffff) {
		p_width = 0x7fffff;
	}
	m_uiScale = (int) (((unsigned int) m_uiScale & 0xffu) | ((unsigned int) p_width << 8));
}

void SPRITE::CopyUIScalingFrom(const SPRITE* p_sprite)
{
	int state = p_sprite ? (p_sprite->m_uiScale & 0xff) : 0;
	for (SPRITE* sprite = this; sprite; sprite = sprite->m_child) {
		sprite->m_uiScale = state;
	}
}

float SPRITE::ScriptX() const
{
	if (!HasUIScriptLayout() || !Graph || !Map) {
		return m_x;
	}
	GRAPH_CORE* graph = (GRAPH_CORE*) Graph;
	float screenX = m_x - Map->m_shiftX;
	return UI_SCALING::UntransformScriptAxis(screenX, graph->m_width, UIDrawScale(), UIAnchorX()) + Map->m_shiftX;
}

float SPRITE::ScriptY() const
{
	if (!HasUIScriptLayout() || !Graph || !Map) {
		return m_y;
	}
	GRAPH_CORE* graph = (GRAPH_CORE*) Graph;
	float projectedY = m_y - m_z - Map->m_shiftY;
	return UI_SCALING::UntransformScriptAxis(projectedY, graph->m_height, UIDrawScale(), UIAnchorY()) + m_z +
		   Map->m_shiftY;
}

// FUNCTION: ALIEN 0x444a00
void SPRITE::BreakLink()
{
	if (m_child) {
		m_child->m_parent = m_parent;
	}
	if (m_parent) {
		m_parent->m_child = m_child;
		m_parent = 0;
	}
	m_child = 0;
}

// FUNCTION: ALIEN 0x444a30
int SPRITE::DestroyLink(VID* p_vid)
{
	SPRITE* ent = this;
	while (ent->m_child) {
		if (ent->m_child->m_vid == p_vid) {
			SPRITE* c = ent->m_child;
			SPRITE* gc = c->m_child;
			ent->m_child = gc;
			if (gc) {
				gc->m_parent = ent;
			}
			c->m_child = 0;
			c->m_parent = 0;
			if (c) {
				c->ScalarDeletingDestructor(1);
			}
			return 1;
		}
		ent = ent->m_child;
	}
	return 0;
}

inline static int CanDescendFightLink(const SPRITE* p_sprite)
{
	SPRITE* c = p_sprite->m_child;
	return c && c->m_vid == p_sprite->m_vid->m_unk0x5c && c->m_vid->m_weaponVid && c->m_vid->m_unk0x40;
}

inline static ANGLE DirectionOf(const SPRITE* p_from, const SPRITE* p_to)
{
	float dy = p_to->m_y - p_from->m_y;
	float dx = p_to->m_x - p_from->m_x;
	return ANGLE(dx, dy);
}

inline static float NearDistanceOf(float p_x, float p_y)
{
	float x = (float) fabs(p_x);
	float y = (float) fabs(p_y);
	if (x > y) {
		return x + y * 0.5f;
	}
	return x * 0.5f + y;
}

inline static int IsNotCloserEnemy(const float& p_dist, const float& p_bestDist)
{
	return p_dist >= p_bestDist;
}

inline static SPRITE* LastHashSprite(HASH_MAP* p_hash, unsigned int p_n)
{
	return (SPRITE*) (p_hash->m_iter = --p_n, p_hash->m_list.m_data[p_n]);
}

// FUNCTION: ALIEN 0x444a80
SPRITE* SPRITE::SeekEnemy()
{
	SPRITE* s = this;
	SPRITE* best = 0;
	while (CanDescendFightLink(s)) {
		s = s->m_child;
	}
	VID_EXDATA* e = s->m_vid->m_exData;
	float range = e->m_unk0x14;
	float nearRange = e->m_unk0x18;
	int mask = e->m_unk0x00;
	float minRange = e->m_unk0x3c;
	int flags = e->m_unk0x04;
	int facing = flags & 2;
	int pick = flags & 8;
	if (range == 0.0f || !mask) {
		return 0;
	}
	float bestDist = range + 1.0f;
	SPRITE* c;
	if ((s->m_flag & 0x1800) == 0x800) {
		c = Map->Flagman(0);
	}
	else {
		unsigned int n = Hash->m_list.m_n;
		if (!n) {
			c = best;
		}
		else {
			c = LastHashSprite(Hash, n);
		}
	}
	while (c) {
		VID* cv = c->m_vid;
		if ((cv->m_unk0x0c & mask) && cv->m_defaultMaxHp &&
			(((c->m_flag ^ s->m_flag) & 0x1800) || (s->m_vid->m_exData->m_unk0x04 & 0x80)) && !(cv->m_flag & 0x20000) &&
			((c->m_flag & 0x1800) != 0x1000 || (s->m_vid->m_exData->m_unk0x04 & 0x80) ||
			 ((Map->m_flag & 2) && (s->m_flag & 0x1800) == 0x800 && cv->m_sprClass == 21)) &&
			(!(cv->m_unk0x0c & 8) || !c->m_parent)) {
			int ok = 1;
			if (facing) {
				ANGLE d = DirectionOf(s, c);
				unsigned char a = d.m_dir - s->m_dir;
				unsigned char b = s->m_dir - d.m_dir;
				if (a < b) {
					b = a;
				}
				if (b >= 0x20) {
					ok = 0;
				}
			}
			if (ok) {
				float dist = NearDistanceOf(c->m_x - s->m_x, c->m_y - s->m_y);
				if (dist <= range && dist >= minRange) {
					VID* sv = s->m_vid;
					if ((sv->m_unk0x0c & 8) && !s->m_unk0x6c) {
						if (IsNotCloserEnemy(dist, bestDist)) {
							ok = 0;
						}
					}
					else if (!s->CanSeekEnemy(c) || c->m_vid->m_idx == 104) {
						ok = 0;
					}
					else if (bestDist > nearRange && dist <= nearRange) {
						ok = 1;
					}
					else if (bestDist <= nearRange && dist > nearRange) {
						ok = 0;
					}
					else {
						ok = s->IsBetterEnemy(dist, bestDist, c, best);
					}
					if (ok) {
						if ((s->m_flag & 0x1800) || c->m_vid->m_sprClass != 21 || !((ENGINE*) c)->HaveArmy(0)) {
							bestDist = dist;
							best = c;
							if (pick) {
								if (!(GameRand() % 3)) {
									return c;
								}
							}
						}
					}
				}
			}
		}
		if ((s->m_flag & 0x1800) == 0x800) {
			break;
		}
		c = (SPRITE*) Hash->m_list.NextIterate((int*) &Hash->m_iter);
	}
	return best;
}

// FUNCTION: ALIEN 0x445470
void SPRITE::PlaySFX(int p_sfx) const
{
	float listenerX;
	float listenerY;
	Map->GetAudioListener(&listenerX, &listenerY);
	Sound->PlaySFXFromCoor(
		p_sfx,
		VIEWPORT_MATH::RelativeAudioAxis(m_x, listenerX),
		VIEWPORT_MATH::RelativeAudioAxis(m_y - m_z, listenerY)
	);
}

// FUNCTION: ALIEN 0x4455c0
void SPRITE::DrawRectangle()
{
	VID* vid = m_vid;
	if (vid->m_sprClass == 23) {
		DrawSecondaryInfo();
		return;
	}
	unsigned int type = vid->m_unk0x0c;
	COLOR color;
	if ((type & 1) && (vid->m_flag & 0x40)) {
		color = GRAPH_CORE::BLACK;
	}
	else if (vid->m_layer == 11) {
		color = GRAPH_CORE::WHITE;
	}
	else if (type & 4) {
		color = GRAPH_CORE::GREEN;
	}
	else if (type & 8) {
		color = GRAPH_CORE::LIGHTBLUE;
	}
	else if (type & 2) {
		color = GRAPH_CORE::YELLOW;
	}
	else if (type & 0x20) {
		color = GRAPH_CORE::RED;
	}
	else if (type & 0x200) {
		color = GRAPH_CORE::GRAY;
	}
	else if (vid->m_sprClass == 10) {
		color = GRAPH_CORE::WHITE;
	}
	else {
		color = GRAPH_CORE::RED;
	}

	Graph->Box(
		Map->ScrX(m_x - m_vid->m_unk0x384),
		Map->ScrY(m_y - m_z - m_vid->m_unk0x388),
		Map->ScrX(m_vid->m_unk0x384 + m_x),
		Map->ScrY(m_y - m_z + m_vid->m_unk0x388),
		color
	);
	if (m_vid->m_unk0x0c & 6) {
		Graph->Line(ScreenX(), Map->ScrY(m_y - m_z - m_vid->m_unk0x24), ScreenX(), ScreenY(), GRAPH_CORE::BLUE);
	}
	GRAPH_CORE::PrintfXY(
		(GRAPH_CORE*) Graph,
		m_x - Map->m_shiftX + 1.0f,
		m_y - m_z - Map->m_shiftY,
		"%i",
		m_vid->m_idx
	);
}

inline static float CreateLinkXOffset(const unsigned char& p_dir, const VID* p_vid)
{
	return ANGLE::CosTable[p_dir] * p_vid->m_unk0x4c - ANGLE::SinTable[p_dir] * -p_vid->m_unk0x50;
}

inline static float CreateLinkYOffset(const unsigned char& p_dir, const VID* p_vid)
{
	return ANGLE::SinTable2[p_dir] * p_vid->m_unk0x4c - ANGLE::CosTable2[p_dir] * p_vid->m_unk0x50;
}

// FUNCTION: ALIEN 0x445770
void SPRITE::CreateLink()
{
	VID* link = m_vid->m_unk0x5c;
	if (!link) {
		return;
	}
	SPRITE* child = m_child;
	if (child && child->m_vid == link) {
		return;
	}
	if (link->m_prop) {
		return;
	}
	float xoff, yoff;
	if (link->m_flag & 0x2000000) {
		xoff = m_vid->m_unk0x4c;
		yoff = -m_vid->m_unk0x50;
	}
	else {
		xoff = CreateLinkXOffset(m_dir, m_vid);
		yoff = CreateLinkYOffset(m_dir, m_vid);
	}
	AddLink(Map->CreateSprite(m_vid->m_unk0x5c, xoff + m_x, yoff + m_y, m_vid->m_unk0x54 + m_z, ANGLE(m_dir), 0));
	if (m_child) {
		m_child->ChangeArmy((m_flag >> 11) & 3);
	}
	else {
		MYERROR::Error(
			::Error,
			"SPRITE %i",
			3,
			// STRING: ALIEN 0x48458c
			"link",
			0,
			m_vid ? m_vid->m_idx : -1
		);
	}
}

// FUNCTION: ALIEN 0x4458a0
STRING SPRITE::GetTextActions() const
{
	STRING result;
	for (int i = 0; i < m_actions.m_n; ++i) {
		// STRING: ALIEN 0x484594
		result += Printf(
			"%c%i,%i,%i;",
			m_actions.m_data[i].m_cmd + 60,
			(int) m_actions.m_data[i].m_a,
			(int) m_actions.m_data[i].m_b,
			(int) m_actions.m_data[i].m_c
		);
	}
	return result;
}

// FUNCTION: ALIEN 0x445990
void SPRITE::SetTextActions(const STRING& p_text)
{
	struct PARSED_ACTION {
		int command;
		int c;
		int b;
		int a;
		char* after;
	} parsed;
	parsed.command = 0;
	STRING buffer(p_text);
	while (strcmp(buffer.m_str, empty_str)) {
		char command = 0;
		parsed.a = 0;
		parsed.b = 0;
		parsed.c = 0;
		// STRING: ALIEN 0x4845a0
		if (sscanf(buffer.m_str, "%c%i,%i,%i", &command, &parsed.a, &parsed.b, &parsed.c) != 4) {
			break;
		}
		parsed.command = (unsigned char) command;
		m_actions.Insert(ACT(parsed.command - 60, parsed.a, parsed.b, parsed.c));
		// STRING: ALIEN 0x481a10
		buffer.After(&parsed.after, ";");
		buffer = parsed.after;
		if (parsed.after != STRING::EMPTY) {
			operator delete(parsed.after);
		}
	}
}

// FUNCTION: ALIEN 0x445b10
void SPRITE::AddActionAfterStop(int p_cmd, decomp_intptr p_a, decomp_intptr p_b, decomp_intptr p_c)
{
	int location = m_actions.Location(ACT(73, 0, 0, 0));
	if (location >= 0) {
		m_actions.InsertBefore(location + 1, ACT(p_cmd, p_a, p_b, p_c));
	}
	else {
		m_actions.InsertFirst(ACT(p_cmd, p_a, p_b, p_c));
	}
}

// FUNCTION: ALIEN 0x445be0
void SPRITE::ResetActionStack()
{
	for (int i = 0; i < m_actions.m_n; ++i) {
		if (m_actions.m_data[i].m_cmdByte == 74) {
			if (m_actions.m_data[i].m_a) {
				((SPRITE*) m_actions.m_data[i].m_a)->Release();
			}
		}
	}
	m_actions.Release();
}

// FUNCTION: ALIEN 0x445f30
int SPRITE::CanAttackThisSprite(SPRITE* p_sprite)
{
	if (!p_sprite) {
		return 0;
	}
	VID* vid = m_vid;
	if (vid->m_weaponVid && vid->m_unk0x40 && (vid->m_exData->m_unk0x00 & p_sprite->m_vid->m_unk0x0c)) {
		return 1;
	}
	SPRITE* c = m_child;
	if (c && c->m_vid == vid->m_unk0x5c && c->m_vid->m_weaponVid && c->m_vid->m_unk0x40 &&
		(c->m_vid->m_exData->m_unk0x00 & p_sprite->m_vid->m_unk0x0c)) {
		return 1;
	}
	return 0;
}

// FUNCTION: ALIEN 0x445fb0
int SPRITE::Attack(SPRITE* p_target)
{
	VID* vid = m_vid;
	if (vid->m_sprClass == 0xc && vid->m_weaponVid) {
		int ani = m_ani;
		m_ani = 8;
		SetCommand(4, p_target);
		CreateChild();
		m_ani = ani;
		SetCommand(0, 0);
		return 0;
	}
	if (!p_target || p_target->m_vid == EmptyVid || CanAttackThisSprite(p_target) || m_vid->m_sprClass == 5 ||
		Action(0x9f, 0, 0, 0)) {
		SetCommand(3, p_target);
	}
	return 0;
}

// FUNCTION: ALIEN 0x446060
int SPRITE::CanSeekEnemy(SPRITE* p_sprite)
{
	return 1;
}

// FUNCTION: ALIEN 0x446070
int SPRITE::PrimitiveTact()
{
	unsigned int result = CurrentTime;
	if (CurrentTime - m_tactTime >= m_vid->m_aniDuration[m_ani]) {
		int frame = m_noCadr;
		int end = m_endCadr;
		m_tactTime = CurrentTime;
		if (++m_noCadr > m_endCadr) {
			m_noCadr = m_begCadr;
		}
	}
	return result;
}

// FUNCTION: ALIEN 0x4463a0
ANGLE SPRITE::DirectionTo(const SPRITE* p_other, int* p_dist) const
{
	float dy = p_other->m_y - m_y;
	float dx = p_other->m_x - m_x;
	return ANGLE(dx, dy, p_dist);
}

// FUNCTION: ALIEN 0x4463f0
int SPRITE::GetFireDamage()
{
	int total = 0;
	for (SPRITE* s = this; s; s = s->m_child) {
		VID* vid = s->m_vid;
		if (vid->m_weaponVid && vid->m_unk0x40) {
			total += vid->m_weaponVid->GetFireDamage();
		}
	}
	return total;
}

// FUNCTION: ALIEN 0x446430
void SPRITE::CreateChildAndPlaySFX(int p_ani)
{
	if (m_vid->m_aniChildVid[p_ani]) {
		int ani = m_ani;
		m_ani = p_ani;
		CreateChild();
		m_ani = ani;
	}
	int sfx = m_vid->m_aniSfx[p_ani];
	if (sfx) {
		PlaySFX(sfx);
	}
}

// FUNCTION: ALIEN 0x446480
STRING SPRITE::GetTextItems()
{
	STRING result;
	if (m_exData && m_exData->m_list.m_n) {
		for (int i = 0; i < m_exData->m_list.m_n; ++i) {
			// STRING: ALIEN 0x4845ac
			result += Printf("\1%i", m_exData->m_list.m_data[i]);
		}
		// STRING: ALIEN 0x48412c
		result += "\2";
	}
	return result;
}

static __forceinline void ListIntInsert(LIST_INT* p_list, int p_value)
{
	int max = p_list->m_max;
	if (p_list->m_n >= max) {
		int newMax = 2 * max + 4;
		if (newMax > max) {
			int* oldData = p_list->m_data;
			p_list->m_data = new int[newMax];
			if (!p_list->m_data) {
				MYERROR::LogExit(::Error, "!!!ERROR!!!::LIST: Not enough memory %i", newMax);
			}
			if (oldData) {
				for (int i = 0; i < p_list->m_max; ++i) {
					p_list->m_data[i] = oldData[i];
				}
				delete[] oldData;
			}
			p_list->m_max = newMax;
		}
	}
	p_list->m_data[p_list->m_n++] = p_value;
}

// FUNCTION: ALIEN 0x446570
void SPRITE::SetTextItems(const STRING& p_text)
{
	if (!p_text.m_str || !*p_text.m_str) {
		return;
	}

	if (!m_exData) {
		m_exData = new EX_SPRITE_DATA(this);
	}

	// Parse SOH-prefixed decimal items from immutable source text.
	const char* itemText = strchr(p_text.m_str, '\1');
	while (itemText && itemText[1]) {
		++itemText;
		int item = 0;
		if (sscanf(itemText, "%i", &item) == 1) {
			ListIntInsert(&m_exData->m_list, item);
		}
		itemText = strchr(itemText, '\1');
	}
}

// FUNCTION: ALIEN 0x4474b0
ANGLE SPRITE::DirectionTo(const SPRITE* p_other) const
{
	float dy = p_other->m_y - m_y;
	float dx = p_other->m_x - m_x;
	return ANGLE(dx, dy);
}

// FUNCTION: ALIEN 0x44e1b0
int SPRITE::HaveFightLink()
{
	return m_child && m_child->m_vid == m_vid->m_unk0x5c;
}
