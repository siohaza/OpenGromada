#define DECOMP_INLINE_STRING_DTOR
#define DECOMP_INLINE_SPRITE_RELEASE
#define DECOMP_INLINE_SPRITE_LIST_CTOR
#include "game/engine.h"

#include "gfx/graph.h"
#include "gfx/graph_core.h"

#include "game/const.h"
#include "game/gametime.h"
#include "game/map.h"
#include "game/man.h"
#include "game/train_info.h"
#include "sprite/r_pos.h"
#include "util/polar.h"
#include "game/terrain.h"
#include "game/unit.h"
#include "sprite/r_dot.h"
#include "sprite/r_map.h"
#include "sprite/sprite.h"
#include "game/player_arcade.h"
#include "game/rail.h"
#include "world/hash_map.h"
#include "util/myerror.h"
#include "video/vid.h"
#include "video/vid_exdata.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>

// GLOBAL: ALIEN 0x5da590
int ENGINE::NoStepForNotFound;

// GLOBAL: ALIEN 0x5da594
int ENGINE::NoStep;

// GLOBAL: ALIEN 0x5da58c
int ENGINE::globaldeleting;

// GLOBAL: ALIEN 0x5da578
SPRITE_LIST ENGINE::PathDots;

// FUNCTION: ALIEN 0x43a910
ENGINE* ENGINE::NextEngine()
{
	return m_nextEngine;
}

// FUNCTION: ALIEN 0x43a920
void ENGINE::SetCommandToTrain(int p_cmd, int p_x, int p_y)
{
	SetCommandToTrain(p_cmd, 0, RailMap.GetNearestDot_xy(p_x, p_y), 0);
}

// FUNCTION: ALIEN 0x44e8b0
ENGINE::ENGINE(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent)
	: UNIT(p_vid, p_x, p_y, p_z, p_dir, p_parent)
{
	m_unk0x90 &= 0xfffffffe;
	m_unk0xb4 = 0;
	m_unk0xd8 = 1;
	m_unk0xe8 = 0;
	m_unk0xe4 = 0;
	m_unk0xe0 = 0;
	m_unk0xec = 0;
	m_unk0xf0 = 0;
	m_unk0xdc = 0;
	m_unk0xf4 = -1;
	m_noPathLinks = 0;
	m_commandPathDot = 0;
	m_secondaryCommandDot = 0;
	m_unk0xb0 = 0.0f;
	m_unk0xac = 0;
	m_prevEngine = 0;
	m_nextEngine = 0;
	m_commandDot = 0;
	m_commandOwner = 0;
	if (!(Map->m_flag & 0x20)) {
		SetRDot();
		SPRITE* child = m_child;
		if (child && child->m_vid == m_vid->m_unk0x5c)
			Child()->ChangeDirection(m_dir);
	}
}

// FUNCTION: ALIEN 0x44e9d0
void* ENGINE::ScalarDeletingDestructor(unsigned int p_flags)
{
	ENGINE* result = this;
	this->~ENGINE();
	if (p_flags & 1)
		operator delete(result);
	return result;
}

// FUNCTION: ALIEN 0x44ec20
void ENGINE::MoveTact()
{
	MoveEngineTact();
	if ((CurrentTime & 0xffffc000) > PrevCurrentTime)
		m_flag &= ~2;
	if ((CurrentTime & 0xfffffc00) > PrevCurrentTime && !m_prevEngine) {
		TRAIN_INFO info(this);
		if (info.m_unk0x3c <= 10)
			Map->ScriptRun(EvFunctionNumber[1], this, 0, 0);
		if (info.m_unk0x20 < info.m_unk0x24 / 2)
			Map->ScriptRun(EvFunctionNumber[3], this, 0, 0);
	}
	if ((CurrentTime & 0xfffffc00) > PrevCurrentTime) {
		int idx = m_vid->m_idx;
		if (idx == 85)
			RepairTact();
		else if (idx == 45)
			AddAmmoTact();
		else if (idx == 35)
			m_unk0xd8 = 0;
	}
}

// FUNCTION: ALIEN 0x44ed10
void ENGINE::DeletePointerToSprite(SPRITE* p_sprite)
{
	if (m_commandOwner == p_sprite)
		SetCommandToTrain(0, 0, 0, 0);
	if (p_sprite->m_vid->m_sprClass == 0x15) {
		if (m_goal != p_sprite)
			goto done;
		if ((m_flag & 0x7c) == 0x68) {
			ENGINE* e = (ENGINE*) p_sprite;
			SPRITE* next = (SPRITE*) e->m_nextEngine;
			if (next || (next = (SPRITE*) e->m_prevEngine) != 0)
				SetCommandToTrain(26, next, 0, 0);
		}
	}
	if (m_goal == p_sprite)
		SetCommandToTrain(0, 0, 0, 0);
done:
	((SPRITE*) this)->SPRITE::DeletePointerToSprite(p_sprite);
}

static inline void EngineDebugIndices(int& p_best_result, int& p_goal_result,
	SPRITE* p_best, SPRITE* p_goal, int p_zero)
{
	p_best_result = p_best ? p_best->m_vid->m_idx : p_zero;
	p_goal_result = p_goal ? p_goal->m_vid->m_idx : p_zero;
}

// STUB: ALIEN 0x44ed90
void ENGINE::DrawSecondaryInfo()
{
	GRAPH_CORE* core = (GRAPH_CORE*) Graph;
	float y = core->m_viewYMin;
	float viewXMin = core->m_viewXMin;
	int bestIdx;
	int goalIdx;
	EngineDebugIndices(bestIdx, goalIdx, m_unk0x6c, m_goal, 0);
	int noRef = m_ani;
	int command = (m_flag >> 2) & 0x1f;
	int ani = m_noRef;
	GRAPH_CORE::PrintfXY(core, viewXMin + 22.0f, y,
		// STRING: ALIEN 0x484a1c
		"Ref=%-3i cmd=%1i ani=%-2i ammo=%-3i hp=%-3i AT=%i goal=%-3i best=%-3i speed=%-3i timer=%i",
		ani, command, noRef, (int) Action(92, 0, 0, 0), m_unk0x54, m_unk0x04,
		goalIdx, bestIdx, (int) (m_speed * 1000.0f), m_unk0x50);
	y = y + 12.0f;
	GRAPH_CORE::PrintfXY(core, (viewXMin = core->m_viewXMin) + 22.0f, y,
		// STRING: ALIEN 0x4849d8
		"IS_PBJMF(%i%i%i%i%i) Accel=%i MaxSpeed=%i NoStep=%i NoStepNotF=%i",
		m_unk0xb4, m_unk0xd8, m_flag & 1, m_unk0xec, (m_flag >> 7) & 1,
		m_unk0xac, (int) (m_unk0xb0 * 1000.0f), NoStep, NoStepForNotFound);
	SPRITE* child = m_child;
	if (child && child->m_vid == m_vid->m_linkVid) {
		y = y + 12.0f;
		float childViewXMin;
		int childBest = child->m_unk0x6c ? child->m_unk0x6c->m_vid->m_idx : 0;
		int childGoal = child->m_goal ? child->m_goal->m_vid->m_idx : 0;
		int childNoRef = child->m_ani;
		int childCommand = (child->m_flag >> 2) & 0x1f;
		int childAni = child->m_noRef;
		childViewXMin = Graph->m_viewXMin;
		GRAPH_CORE::PrintfXY(Graph, childViewXMin + 22.0f, y,
			// STRING: ALIEN 0x484988
			"Ref=%-3i cmd=%1i ani=%-2i ammo=%-3i hp=%-3i AT=%i goal=%-3i best=%-3i timer=%i",
			childAni, childCommand, childNoRef,
			(int) child->Action(92, 0, 0, 0), child->m_unk0x54, child->m_unk0x04, childGoal,
			childBest, child->m_unk0x50);
	}
	if (m_actions.m_n) {
		y = y + 12.0f;
		STRING text;
		text += Printf("%i - ", m_actions.m_n);
		for (int i = 0; i < m_actions.m_n; ++i) {
			ACT* act = &m_actions.m_data[i];
			text += Printf("%i(%i,%i,%i) ", m_actions.m_data[i].m_cmd,
				m_actions.m_data[i].m_a, m_actions.m_data[i].m_b,
				m_actions.m_data[i].m_c);
		}
		COLOR white;
		white.m_value = 0xffffffff;
		Graph->PutsXY(Graph->GetViewXMin() + 30.0f, y, text, white);
	}
	DrawGoalLine();
	if (m_commandOwner)
		Graph->PutBigPixel(m_commandOwner->ScreenX(), m_commandOwner->ScreenY(),
			GRAPH_CORE::RED);
}

// FUNCTION: ALIEN 0x44f0f0
void ENGINE::DrawGoalLine()
{
	if (m_secondaryCommandDot)
		Graph->Line(ScreenX() + 2.0f, ScreenY() + 2.0f,
			m_secondaryCommandDot->GetScreenX() + 2.0f,
			m_secondaryCommandDot->GetScreenY() + 2.0f, GRAPH_CORE::WHITE);
	if (m_commandPathDot)
		Graph->Line(ScreenX() + 2.0f, ScreenY() + 2.0f,
			m_commandPathDot->GetScreenX() + 2.0f,
			m_commandPathDot->GetScreenY() + 2.0f, GRAPH_CORE::WHITE);
	if (m_goal)
		Graph->Line(ScreenX(), ScreenY(), Goal()->ScreenX(), Goal()->ScreenY(),
			GRAPH_CORE::GREEN);
	if (m_child && m_child->m_goal)
		Graph->Line(Child()->ScreenX(), Child()->ScreenY(),
			Child()->Goal()->ScreenX(), Child()->Goal()->ScreenY(),
			GRAPH_CORE::RED);
	if (m_commandDot)
		Graph->Line(ScreenX(), ScreenY(), m_commandDot->GetScreenX(),
			m_commandDot->GetScreenY(), GRAPH_CORE::BLUE);
	GRAPH_CORE* core = (GRAPH_CORE*) Graph;
	if (core->m_locked) {
		core->m_backBuffer->UnlockRect();
		core->m_locked = 0;
	}
}

// STUB: ALIEN 0x44f320
int ENGINE::Action(int p_cmd, int p_a, int p_b, int p_c)
{
	switch (p_cmd & 0xff) {
	case 86: { // ACT_REPAIR
		VID* vid = m_vid;
		if (vid->m_idx != 82)
			return (int) UNIT::Action(p_cmd, p_a, p_b, p_c);
		if (m_ammo / 64
			>= ((vid->m_linkVid && vid->m_linkVid->m_aniChildVid[8]
					&& vid->m_linkVid->m_weapon)
					? vid->m_linkVid->m_exData->m_maxAmmo
					: vid->m_exData->m_maxAmmo))
			return (int) UNIT::Action(p_cmd, p_a, p_b, p_c);
		int army = (m_flag >> 11) & 3;
		int engines = vid->m_entitiesNumber[army];
		VID* shellVid = (Map->m_noVid > 70 && Map->m_vids[70])
			? Map->m_vids[70] : EmptyVid;
		int shells = shellVid->m_entitiesNumber[army];
		int carried = 0;
		HASH_MAP* h = Hash;
		if (h->m_list.m_n) {
			int i = h->m_list.m_n - 1;
			SPRITE* s = (SPRITE*) h->m_list.m_data[i];
			while (s) {
				if (s->m_vid->m_idx == 82 && !((m_flag ^ s->m_flag) & 0x1800))
					carried += ((ENGINE*) s)->m_ammo / 64;
				if (i > Hash->m_list.m_n)
					i = Hash->m_list.m_n;
				if (--i < 0)
					break;
				s = (SPRITE*) Hash->m_list.m_data[i];
			}
		}
		if (carried + shells
			< ((vid->m_linkVid && vid->m_linkVid->m_aniChildVid[8]
					&& vid->m_linkVid->m_weapon)
					? vid->m_linkVid->m_exData->m_maxAmmo
					: vid->m_exData->m_maxAmmo)
				* engines)
			return (int) UNIT::Action(p_cmd, p_a, p_b, p_c);
		return (int) TERRAIN::Action(p_cmd, p_a, p_b, p_c);
	}
	case 132: // ACT_UNDO_REMOVE
		SPRITE::Action(p_cmd, p_a, p_b, p_c);
		ClearDotBusy();
		if (m_prevEngine)
			m_prevEngine->m_nextEngine = 0;
		if (!m_nextEngine)
			return 0;
		m_nextEngine->m_prevEngine = 0;
		return 0;
	case 133: // ACT_UNDO_INSERT
		SPRITE::Action(p_cmd, p_a, p_b, p_c);
		SetDotBusy();
		if (m_prevEngine)
			m_prevEngine->m_nextEngine = this;
		if (!m_nextEngine)
			return 0;
		m_nextEngine->m_prevEngine = this;
		return 0;
	case 97: // ACT_SET_ARMY
		Map->m_player[(m_flag >> 11) & 3]->DeletePointerToSprite(this);
		ChangeArmy(p_a);
		if (m_vid->m_exData->m_unk0x10 == 0.0f)
			return 0;
		Map->m_player[(m_flag >> 11) & 3]->AddPointerToSprite(this);
		return 0;
	case 90: // ACT_GET_GOAL
		return (int) FirstEngine()->m_goal;
	case 74: // ACT_RESTORE_COMMAND
		SetCommandToTrain((p_cmd >> 8) & 0xff, (SPRITE*) p_a, (R_DOT*) p_b, (R_DOT*) p_c);
		if (p_a)
			((SPRITE*) p_a)->Release();
		return 0;
	case 70: // ACT_BACKUP_COMMAND
		m_actions.Insert(ACT((((m_flag >> 2) & 0x1f) << 8) + 74, (int) m_goal,
			(int) m_commandDot, (int) m_secondaryCommandDot));
		if (m_goal)
			++m_goal->m_noRef;
		return 0;
	case 130: { // ACT_NEXT_COMMAND
		if (m_ani >= 15)
			return 0;
		int accel = m_unk0xac;
		int ani = m_ani;
		if (accel > 0) {
			if (ani != 3)
				ChangeAnimation(3); // ANI_START_MOVE
		} else if (accel < 0) {
			if (ani != 1)
				ChangeAnimation(1); // ANI_STOP_MOVE
		} else if (m_speed != 0.0f) {
			if (ani != 2)
				ChangeAnimation(2); // ANI_GO
		} else if (ani != 0) {
			ChangeAnimation(0); // ANI_STAND
		}
		VID* vid = m_vid;
		int armed = (vid->m_aniChildVid[8] && vid->m_weapon)
			|| (m_child && m_child->m_vid == vid->m_linkVid
				&& m_child->m_vid->m_aniChildVid[8] && m_child->m_vid->m_weapon);
		if (armed)
			ActNextCommandFighter();
		if ((m_flag & 0x7c) != 0x60 || m_vid->m_idx != 85 || !m_unk0xec
			|| m_speed != 0.0f)
			return 0;
		if (!m_unk0xf0) {
			m_unk0xf0 = CurrentTime;
		}
		else {
			if ((int) (CurrentTime - m_unk0xf0) > Const->m_repairDockTime) {
				if (m_ammo / 64 > 0) {
					R_DOT* dot = m_commandDot;
					Map->CreateSprite(Map->Vid(595), (float) dot->m_x,
						(float) dot->m_y, (float) dot->m_z, ANGLE(0), this);
					SPRITE* crate = Map->CreateSprite(Map->Vid(86), GetX(), GetY(),
						GetZ(), Direction(), this);
					if (crate) {
						crate->Action(95, 0, 0, 0);
						Action(93, -1, 0, 0);
					}
				}
				SetCommandToTrain(0, 0, 0, 0);
			}
		}
		Map->CreateSprite(Map->Vid(588), GetX(), GetY(), GetZ(), ANGLE(0), this);
		return 0;
	}
	case 85: { // ACT_DAMAGE
		if (p_a > 0) {
			ENGINE* first = FirstEngine();
			if (!(first->m_flag & 2) && p_b
				&& ((m_flag ^ ((SPRITE*) p_b)->m_flag) & 0x1800)) {
				Map->ScriptRun(EvFunctionNumber[10], this, (SPRITE*) p_b, 0);
				first->m_flag |= 2;
			}
		}
		SPRITE* child = m_child;
		if (child && child->m_vid == m_vid->m_linkVid
			&& child->m_vid->m_maxHp[(child->m_flag >> 11) & 3])
			return (int) child->Action(p_cmd, p_a, p_b, p_c);
		return (int) SPRITE::Action(p_cmd, p_a, p_b, p_c);
	}
	case 80: { // ACT_SAVE
		UNIT::Action(p_cmd, p_a, p_b, p_c);
		STREAM* stream = (STREAM*) p_a;
		((R_POS*) &m_curDotRef)->Write(stream);
		((R_POS*) &m_lastDotRef)->Write(stream);
		stream->Write(&m_prevEngine, 4);
		stream->Write(&m_nextEngine, 4);
		return 0;
	}
	case 81: // ACT_RESTORE
	case 200: {
		UNIT::Action(p_cmd, p_a, p_b, p_c);
		if (p_b >= 6) {
			STREAM* stream = (STREAM*) p_a;
			((R_POS*) &m_curDotRef)->Read(stream);
			((R_POS*) &m_lastDotRef)->Read(stream);
			m_prevEngine = (ENGINE*) Map->ReadPointer(stream);
			m_nextEngine = (ENGINE*) Map->ReadPointer(stream);
			SetDotBusy();
			ANGLE linkDir = m_curDotRef.m_dot
				? m_curDotRef.m_dot->m_links[m_curDotRef.m_link].m_dir : ANGLE(0);
			unsigned char d1 = (unsigned char) (m_dir - linkDir.m_dir);
			unsigned char d2 = (unsigned char) (linkDir.m_dir - m_dir);
			unsigned char delta = d1 < d2 ? d1 : d2;
			if (delta > 0x7f)
				m_unk0x90 |= 1;
		} else {
			SetRDot();
		}
		if (m_vid->m_exData->m_unk0x10 == 0.0f)
			return 0;
		Map->m_player[(m_flag >> 11) & 3]->AddPointerToSprite(this);
		return 0;
	}
	case 37: { // ACT_COOR_ATTACK
		int idx = m_vid->m_idx;
		if (idx == 85) {
			SetCommandToTrain(24, 0, RailMap.GetNearestDot_xy(p_a, p_b), 0);
			return 0;
		}
		if (idx == 97 && !m_nextEngine && !m_prevEngine) {
			SetCommandToTrain(27, 0, RailMap.GetNearestDot_xy(p_a, p_b), 0);
			return 0;
		}

		float z = Map->GetGroundZ_ff((float) p_a, (float) p_b) + 19.0f;
		int y = p_b + (int) z - 19;
		SPRITE* marker = new SPRITE(EmptyVid, (float) p_a, (float) y, z, ANGLE(0), 0);
		SetCommandToTrain(29, marker, 0, 0);
		return 0;
	}
	case 33: // ACT_MOVE
		Move((float) p_a, (float) p_b, 0.0f, 0, 0);
		return 0;
	case 32: // ACT_ATTACK
		if (!p_a)
			return 0;
		if (m_vid->m_idx == 97 && !m_nextEngine && !m_prevEngine)
			SetCommandToTrain(27, (SPRITE*) p_a, 0, 0);
		else
			SetCommandToTrain(28, (SPRITE*) p_a, 0, 0);
		return 0;
	case 150: // ACT_LINK_ENGINE
	case 152: // ACT_FORCELINK_ENGINE
		if (!p_a || ((SPRITE*) p_a)->m_vid->m_sprClass != 21
			|| InTrain((SPRITE*) p_a))
			return 0;
		SetCommandToTrain(26, (SPRITE*) p_a, 0, 0);
		return 0;
	case 151: { // ACT_CLASH_ENGINE
		if (!p_a || ((SPRITE*) p_a)->m_vid->m_sprClass != 21
			|| InTrain((SPRITE*) p_a))
			return 0;
		VID* vid = m_vid;
		if (vid->m_idx == 35 && m_child && m_child->m_vid == vid->m_linkVid) {
			m_child->SetCommand(8, (SPRITE*) p_a);
			return 0;
		}
		SetCommandToTrain(27, (SPRITE*) p_a, 0, 0);
		return 0;
	}
	case 153: { // ACT_TRAIN_BEHAVE
		ENGINE* e = FirstEngine();
		if (!e)
			return 0;
		for (; e; e = e->m_nextEngine) {
			if (p_a == 1) {
				e->m_unk0x8c = 0;
				continue;
			}
			int behave = 1;
			VID* vid = e->m_vid;
			VID* link = vid->m_linkVid;
			int maxAmmo = (link && link->m_aniChildVid[8] && link->m_weapon)
				? link->m_exData->m_maxAmmo : vid->m_exData->m_maxAmmo;
			if (maxAmmo <= 5 && (p_a == 2 || p_a == 4))
				behave = 0;
			if (p_a == 4 || p_a == 5)
				behave |= 2;
			if (p_a == 3 || p_a == 5)
				behave |= 0x10;
			e->m_unk0x8c = behave;
		}
		return 0;
	}
	case 39: // ACT_STOP
		if (p_a)
			ForceStop();
		SetCommandToTrain(0, 0, 0, 0);
		return 0;
	case 159: // ACT_IS_TRAIN
		return m_vid->m_exData->m_unk0x10 - m_vid->m_exData->m_unk0x0c > 1.0f;
	case 154: // ACT_FIRST_ENGINE
		return (int) FirstEngine();
	case 155: // ACT_LAST_ENGINE
		return (int) LastEngine();
	case 156: // ACT_NEXT_ENGINE
		return (int) m_nextEngine;
	case 157: // ACT_IS_FIRST
		return m_prevEngine == 0;
	case 158: // ACT_IN_TRAIN
		return InTrain((SPRITE*) p_a);
	}
	return (int) UNIT::Action(p_cmd, p_a, p_b, p_c);
}

// STUB: ALIEN 0x450000
void ENGINE::SetCommandToTrain(int p_cmd, SPRITE* p_goal, R_DOT* p_dot, R_DOT* p_secondaryDot)
{
	int forcedStop = 0;
	ENGINE* engine;

	if (p_cmd == 30) {
		p_cmd = 0;
		forcedStop = 1;
	}
	if (!p_goal && !p_dot && p_cmd != 29) {
		p_cmd = 0;
	}
	else if (p_cmd == 24) {
		engine = FirstEngine();
		while (engine && (engine->m_vid->m_idx != 85 || engine->m_ammo / 64 <= 0))
			engine = engine->m_nextEngine;
		if (!engine) {
			p_cmd = 0;
			p_goal = 0;
			p_dot = 0;
		}
	}
	R_DOT* secondaryDot = 0;
	R_DOT* pathDot = 0;

	if (p_cmd == 25) {
		if (!p_secondaryDot)
			secondaryDot = RailMap.GetNearestDot_xyr((int) ((SPRITE*) this)->X(),
				(int) ((SPRITE*) this)->Y(), (int) ((SPRITE*) this)->Z());
		else
			secondaryDot = p_secondaryDot;
		pathDot = p_dot;
	}

	for (engine = FirstEngine(); engine; engine = engine->m_nextEngine) {
		if (engine->m_commandOwner) {
			engine->m_commandOwner->ReleaseRef();
			engine->m_commandOwner = 0;
		}

		if ((p_cmd == 28 || p_cmd == 29) && m_vid->m_idx != 45 &&
			m_vid->m_exData->m_unk0x10 == 0.0f) {
			engine->m_commandOwner = (SPRITE*) this;
			((SPRITE*) this)->m_noRef++;
		}

		((SPRITE*) engine)->SetCommandWithoutLink(p_cmd, p_goal);
		engine->m_unk0xb0 = 0.0f;
		engine->m_commandPathDot = pathDot;
		engine->m_commandDot = p_dot;
		engine->m_secondaryCommandDot = secondaryDot;
		engine->m_unk0xf0 = 0;
		engine->m_unk0xec = 0;

		SPRITE* child = engine->m_child;
		VID* childVid;
		if (child && (childVid = child->m_vid) == engine->m_vid->m_unk0x5c && childVid->m_weaponVid &&
			childVid->m_unk0x40 && !forcedStop) {
			if ((m_vid->m_idx != 45 && m_vid->m_exData->m_unk0x10 == 0.0f && engine != this) ||
				!((SPRITE*) this)->CanAttackThisSprite(p_goal)) {
				engine->m_child->SetCommandWithoutLink(0, 0);
			}
			else if (p_cmd == 28) {
				engine->m_child->SetCommandWithoutLink(3, p_goal);
			}
			else if (p_cmd == 29) {
				int weaponType;
				if (engine->m_child && engine->m_child->m_vid == engine->m_vid->m_unk0x5c &&
					engine->m_child->m_vid->m_weaponVid && engine->m_child->m_vid->m_unk0x40)
					weaponType = engine->m_child->m_vid->m_exData->m_unk0x00;
				else
					weaponType = engine->m_vid->m_exData->m_unk0x00;
				if (weaponType == 8 && p_goal) {
					SPRITE* target = new SPRITE(EmptyVid, p_goal->GetX(), p_goal->GetY() + 70.0f,
						p_goal->GetZ() + 70.0f, ANGLE(0), 0);
					engine->m_child->SetCommandWithoutLink(4, target);
				}
				else {
					engine->m_child->SetCommandWithoutLink(4, p_goal);
				}
			}
			else {
				engine->m_child->SetCommandWithoutLink(0, 0);
			}
		}
	}

	if (p_cmd == 23 || p_cmd == 26 || p_cmd == 27 || p_cmd == 25) {
		((SPRITE*) FirstEngine())->StartMove();
		return;
	}
	if (p_cmd == 24) {
		if (m_curDotRef.m_dot != m_commandDot) {
			((SPRITE*) FirstEngine())->StartMove();
			return;
		}
		FirstEngine()->Stop();
		for (engine = FirstEngine(); engine; engine = engine->m_nextEngine) {
			if (engine->m_vid->m_idx == 85 && m_ammo / 64)
				engine->m_unk0xec = 1;
		}
		return;
	}
	if (p_cmd == 0)
		FirstEngine()->Stop();
}

// FUNCTION: ALIEN 0x4503f0
void ENGINE::AddAmmoTact()
{
	ENGINE* best = 0;
	int bestNeed = 0;
	ENGINE* p = FirstEngine();
	if (p) {
		do {
			if (!((p->m_flag ^ m_flag) & 0x1800)) {
				int need = p->NeedAddAmmo();
				if (need > bestNeed && p->m_vid->m_idx != 85) {
					bestNeed = need;
					best = p;
				}
			}
			p = p->m_nextEngine;
		} while (p);
		if (best) {
			((UNIT*) best)->AddAmmoTick(Const->m_addAmmo);
			if (m_ani != 6)
				((SPRITE*) this)->ChangeAnimation(6);
			return;
		}
	}
	if (m_ani == 6)
		((SPRITE*) this)->ChangeAnimation(0);
}

// FUNCTION: ALIEN 0x450470
int ENGINE::RepairByRepair(ENGINE* p_engine)
{
	if (!p_engine)
		return 0;
	int notDamaged = p_engine->Action(85, -Const->m_repairByRepairHp, 0, 0) == 0;
	if (!notDamaged) {
		VID* vid = p_engine->m_vid;
		if (vid->m_unk0x5c) {
			SPRITE* child = p_engine->m_child;
			if (!child || child->m_vid != vid->m_unk0x5c) {
				if (p_engine->m_repairProgress < 0)
					p_engine->m_repairProgress = 0;
				p_engine->m_repairProgress += Const->m_repairByRepairHp;
				int total = p_engine->m_repairProgress;
				int army = (p_engine->m_flag >> 11) & 3;
				if (total >= vid->m_unk0x5c->m_maxHp[army]) {
					((TERRAIN*) p_engine)->Repair(1);
					m_repairProgress = -1;
				}
			}
		}
	}
	return notDamaged;
}

// FUNCTION: ALIEN 0x450510
void ENGINE::RepairTact()
{
	ENGINE* best = 0;
	int bestNeed = 0;
	ENGINE* p = FirstEngine();
	if (p) {
		do {
			if (!((p->m_flag ^ m_flag) & 0x1800)) {
				int need = p->NeedRepairByRepair();
				if (need > bestNeed) {
					bestNeed = need;
					best = p;
				}
			}
			p = p->m_nextEngine;
		} while (p);
		if (best) {
			RepairByRepair(best);
			if (m_ani != 6)
				((SPRITE*) this)->ChangeAnimation(6);
			return;
		}
	}
	if (m_ani == 6)
		((SPRITE*) this)->ChangeAnimation(0);
}

// FUNCTION: ALIEN 0x450580
ENGINE* ENGINE::GetTrain()
{
	ENGINE* p = FirstEngine();
	if (!p)
		return 0;
	while (1) {
		if (p->m_vid->m_exData->m_unk0x10 - p->m_vid->m_exData->m_unk0x0c > 1.0f)
			return p;
		p = p->m_nextEngine;
		if (!p)
			return 0;
	}
}

// FUNCTION: ALIEN 0x4505c0
int ENGINE::HaveArmy(int p_army) const
{
	const ENGINE* p;
	for (p = this; p; p = p->m_nextEngine)
		if (((p->m_flag >> 11) & 3) == p_army)
			return 1;
	for (p = m_prevEngine; p; p = p->m_prevEngine)
		if (((p->m_flag >> 11) & 3) == p_army)
			return 1;
	return 0;
}

// FUNCTION: ALIEN 0x450620
void ENGINE::ActNextCommandFighter()
{
	VID* linkedVid;
	if (HaveFightLink()) {
		SPRITE* linked = m_child;
		linkedVid = linked->m_vid;
		if (linkedVid->m_weaponVid && linkedVid->m_weaponIdx) {
			SPRITE* linkedGoal = linked->m_goal;
			if (linkedGoal && !((m_flag ^ linkedGoal->m_flag) & 0x1800)) {
				int cmd = linked->m_flag & 0x7c;
				if (cmd == 12 || cmd == 16)
					linked->SetCommand(0, 0);
			}
		}
	}
	if (m_vid->m_idx != 35
		|| (HaveFightLink() && (linkedVid = m_child->m_vid)->m_weaponVid && linkedVid->m_weaponIdx
			&& m_child->Action(92, 0, 0, 0) >= linkedVid->m_exData->m_maxAmmo)) {
		if (!(m_flag & 1)) {
			int cmd = m_flag & 0x7c;
			if (cmd == 112 || cmd == 116) {
				SPRITE* owner = m_commandOwner;
				if (!owner || owner == this) {
					SPRITE* goal = m_goal;
					if (goal != m_child->m_goal
						&& (goal->m_vid == EmptyVid || CanAttackThisSprite(m_goal))) {
						SPRITE* target = m_goal;
						float dx = (float) fabs(target->m_x - m_x);
						float dy = (float) fabs(target->m_y - m_y);
						float dist;
						if (dx > dy)
							dist = dx + dy * 0.5f;
						else
							dist = dx * 0.5f + dy;
						SPRITE* child = m_child;
						float range;
						if (child && child->m_vid == m_vid->m_unk0x5c && child->m_vid->m_weaponVid
							&& child->m_vid->m_weapon
							&& m_vid->m_exData->m_unk0x18 == 0.0f)
							range = child->m_vid->m_exData->m_unk0x18;
						else
							range = m_vid->m_exData->m_unk0x18;
						if (dist < range)
							child->SetCommandWithoutLink(4, target);
					}
				}
			}
			else {
				SPRITE* child = m_child;
				if (child) {
					VID* childVid = child->m_vid;
					if (childVid == m_vid->m_unk0x5c && childVid->m_weaponVid
						&& childVid->m_weapon) {
						SPRITE* childGoal = child->m_goal;
						if (childGoal) {
							float dx = (float) fabs(childGoal->m_x - m_x);
							float dy = (float) fabs(childGoal->m_y - m_y);
							float dist;
							if (dx > dy)
								dist = dx + dy * 0.5f;
							else
								dist = dx * 0.5f + dy;
							if (dist > childVid->m_exData->m_unk0x14)
								child->SetCommandWithoutLink(0, 0);
						}
					}
				}
			}
			unsigned int dt = m_vid->m_aniDuration[m_ani];
			int state = AttackTact(CurrentTime - PrevCurrentTime > dt ? CurrentTime - PrevCurrentTime : dt);
			m_unk0x04 = state;
			if (state == 1 || state == 2 || state == 5 || state == 6) {
				if ((m_unk0x8c & 1) && (m_child->m_unk0x50 || rand() % 4 == 0)) {
					SPRITE* enemy = SeekEnemy();
					if (enemy)
						m_child->SetCommand(5, enemy);
				}
				else if (m_unk0x6c) {
					m_child->SetCommand(5, m_unk0x6c);
				}
			}
		}
	}
}

// STUB: ALIEN 0x4508e0
int ENGINE::NeedRepairByRepair()
{
	int army = (m_flag >> 11) & 3;
	int maxHp = m_vid->m_maxHp[army];
	int hp = m_unk0x54;
	if (hp < maxHp) {
		VID* linkVid = m_vid->m_unk0x5c;
		if (linkVid && (!m_child || m_child->m_vid != linkVid))
			return 100 - 100 * hp
				/ (maxHp + linkVid->m_maxHp[(m_flag >> 11) & 3]);
		return 100 - 100 * hp / maxHp;
	}
	VID* linkVid = m_vid->m_unk0x5c;
	if (linkVid && (!m_child || m_child->m_vid != linkVid)) {
		army = m_vid->m_idx == 35;
		return --army & 50;
	}
	SPRITE* child = m_child;
	if (child && child->m_vid == linkVid && child->m_vid->m_sprClass != 9) {
		int childArmy = (child->m_flag >> 11) & 3;
		if (child->m_unk0x54 < child->m_vid->m_maxHp[childArmy])
			return 100 - 100 * (child->m_unk0x54 + hp)
				/ (maxHp + child->m_vid->m_maxHp[childArmy]);
	}
	return 0;
}

inline int VID::GetMaxAmmo()
{
	VID* lv = m_unk0x5c;
	if (lv && lv->m_weaponVid && lv->m_unk0x40)
		return lv->m_exData->m_maxAmmo;
	return m_exData->m_maxAmmo;
}

// FUNCTION: ALIEN 0x4509e0
int ENGINE::NeedAddAmmo()
{
	return m_vid->GetMaxAmmo() ? 100 - 100 * (m_ammo / 64) / m_vid->GetMaxAmmo() : 0;
}

// FUNCTION: ALIEN 0x450a70
void ENGINE::Stop()
{
	ENGINE* p = this;
	for (;;) {
		if (p->InTrain(Map->Flagman(Map->m_curArmy)))
			PathDots.DeleteAll();
		if (!p->m_prevEngine)
			break;
		p = p->FirstEngine();
	}
	unsigned int flag = p->m_flag;
	if ((flag & 0x7c) == 0x64) {
		if (!(flag & 0x80))
			return;
		p->m_commandDot = p->m_secondaryCommandDot;
		R_DOT* commandPathDot = p->m_commandPathDot;
		p->m_flag = flag | 0x80;
		ENGINE* e = p->m_nextEngine;
		p->m_secondaryCommandDot = commandPathDot;
		p->m_commandPathDot = p->m_commandDot;
		p->m_unk0xb0 = 0.0f;
		for (p->m_unk0xac = 0; e; e = e->m_nextEngine) {
			e->m_commandDot = p->m_commandDot;
			e->m_secondaryCommandDot = p->m_secondaryCommandDot;
			e->m_commandPathDot = p->m_commandPathDot;
			e->m_flag |= 0x80;
			e->m_unk0xb0 = 0;
			e->m_unk0xac = 0;
		}
	}
	else {
		for (ENGINE* e = p; e; e = e->m_nextEngine) {
			e->m_flag &= ~0x80u;
			e->m_unk0xb0 = 0;
			if (p->m_speed != 0.0f) {
				float speed = p->m_speed;
				e->m_unk0xac = -((abs((int) (speed * 1000.0f)) + 10) / 2);
			}
		}
	}
}

// FUNCTION: ALIEN 0x450bb0
int ENGINE::TrainWeaponRange()
{
	if (!m_goal || ((m_flag & 0x7c) != 0x70 && (m_flag & 0x7c) != 0x74))
		return 0;
	float range = 10000.0f;
	ENGINE* engine = (ENGINE*) m_commandOwner;
	if (engine) {
		if (m_commandOwner->m_child && m_commandOwner->m_child->m_vid == m_commandOwner->m_vid->m_unk0x5c &&
			m_commandOwner->m_child->m_vid->m_weaponVid && m_commandOwner->m_child->m_vid->m_unk0x40 &&
			engine->m_ammo / 64 > 0) {
			if (engine->m_vid->m_idx == 35) {
				if (engine->m_vid->m_exData->m_unk0x18 < range)
					range = engine->m_vid->m_exData->m_unk0x18;
			}
			else if (engine->m_child->m_vid->m_exData->m_unk0x18 < range) {
				range = engine->m_child->m_vid->m_exData->m_unk0x18;
			}
		}
	}
	else {
		for (engine = FirstEngine(); engine; engine = engine->m_nextEngine) {
			if (engine->m_child && engine->m_child->m_vid == engine->m_vid->m_unk0x5c &&
				engine->m_child->m_vid->m_weaponVid && engine->m_child->m_vid->m_unk0x40 &&
				engine->m_ammo / 64 > 0) {
				if (engine->m_vid->m_idx == 35) {
					if (engine->m_vid->m_exData->m_unk0x18 < range)
						range = engine->m_vid->m_exData->m_unk0x18;
				}
				else if (engine->m_child->m_vid->m_exData->m_unk0x18 < range) {
					range = engine->m_child->m_vid->m_exData->m_unk0x18;
				}
			}
		}
	}
	return (int) (range == 10000.0f ? 0.0f : range);
}

// FUNCTION: ALIEN 0x450d30
void ENGINE::ForceStop()
{
	ENGINE* p = this;
	while (p->m_prevEngine)
		p = p->FirstEngine();
	if ((float) fabs(p->m_speed) > Const->m_minMoveSpeed) {
		for (ENGINE* q = p; q; q = q->m_nextEngine)
			q->Action(85, 2, 0, 0);
	}
	p->m_unk0xb0 = 0;
	p->m_speed = 0;
}

// FUNCTION: ALIEN 0x450d90
void ENGINE::Move(float p_x, float p_y, float p_z, int p_a5, int p_a6)
{
	if (p_a6)
		SetCommandToTrain(23, 0, RailMap.GetNearestDot_xy((int) p_x, (int) p_y - (int) p_z), 0);
	else
		SetCommandToTrain(23, 0, RailMap.GetNearestDot_xyr((int) p_x, (int) p_y, (int) p_z), 0);
}

// FUNCTION: ALIEN 0x450e20
void ENGINE::ReCalcMoveParameters()
{
	ENGINE* front = this;
	while (front->m_prevEngine)
		front = front->FirstEngine();

	TRAIN_INFO info(front);
	if (front->m_unk0xb0 == 0.0f && info.m_accelTime != 0.0f
		&& (int) (info.m_speedInc / info.m_accelTime * 8.0f) > 7) {
		unsigned int flag = front->m_flag;
		if ((flag & 0x80) && !globaldeleting) {
			R_DOT* goal = front->m_commandDot;
			SPRITE* target = goal ? 0 : front->m_goal;
			int nostep = ((R_POS*) &front->m_curDotRef)->NoStepToTarget(goal, target,
				(flag >> 2) & 0x1f, front);
			MAN* flagman = Map->Flagman(Map->m_curArmy);
			if (front->InTrain(flagman)
				&& !(Map->Flagman(Map->m_curArmy)->m_flag & 0x1800))
				front->CreatePathDots(front->m_curDotRef.LinkedDot());
			if (nostep < 0)
				front->m_unk0xb0 = -0.001f;
			else
				front->m_unk0xb0 = 0.001f;
		}
	}

	if (front->m_unk0xb0 < 0.0f) {
		front->m_unk0xb0 = -info.m_unk0x18 * 0.001f;
	}
	else if (front->m_unk0xb0 > 0.0f) {
		front->m_unk0xb0 = info.m_unk0x18 * 0.001f;
		int speed = (info.m_accelTime != 0.0f)
			? (int) (info.m_speedInc / info.m_accelTime * 8.0f) : 0;
		front->m_unk0xac = speed;
		if (!speed)
			front->m_flag &= ~0x80u;
	}
}

inline ANGLE::ANGLE(float p_x, float p_y)
{
	AngleAssign(this, Decart2Polar_f(p_x, p_y));
}

// FUNCTION: ALIEN 0x450fe0
void ENGINE::CalcCoor()
{
	float fx = (float) (((m_curDotRef.m_pos
		* (m_curDotRef.LinkedDot()->m_x - m_curDotRef.m_dot->m_x)) << 8)
		/ m_curDotRef.LinkDist()) + m_curDotRef.m_dot->m_x * 256.0f;
	float fy = (float) (((m_curDotRef.m_pos
		* (m_curDotRef.LinkedDot()->m_y - m_curDotRef.m_dot->m_y)) << 8)
		/ m_curDotRef.LinkDist()) + m_curDotRef.m_dot->m_y * 256.0f;
	float fz = (float) (((m_curDotRef.m_pos
		* (m_curDotRef.LinkedDot()->m_z - m_curDotRef.m_dot->m_z)) << 8)
		/ m_curDotRef.LinkDist()) + m_curDotRef.m_dot->m_z * 256.0f;

	if (m_lastDotRef.m_link >= m_lastDotRef.m_dot->m_noLinks) {
		if (::Error) {
			int idx = m_vid ? m_vid->m_idx : -1;
			MYERROR::Error(::Error,
				// STRING: ALIEN 0x484a9c
				"ENGINE %i", 10,
				// STRING: ALIEN 0x484aa8
				"\xf0\xe5\xeb\xfc\xf1\xfb \xed\xe5\xef\xf0\xe0\xe2\xe8\xeb\xfc\xed\xfb\xe5 link>nolink",
				0, idx);
		}
		m_lastDotRef.m_link = m_lastDotRef.m_dot->m_noLinks - 1;
	}

	R_DOT* rearLinked;
	if ((rearLinked = m_lastDotRef.LinkedDot()) != 0) {
		float bx = (float) (((m_lastDotRef.m_pos * (rearLinked->m_x - m_lastDotRef.m_dot->m_x)) << 8)
			/ m_lastDotRef.LinkDist()) + m_lastDotRef.m_dot->m_x * 256.0f;
		float by = (float) (((m_lastDotRef.m_pos * (rearLinked->m_y - m_lastDotRef.m_dot->m_y)) << 8)
			/ m_lastDotRef.LinkDist()) + m_lastDotRef.m_dot->m_y * 256.0f;
		float bz = (float) (((m_lastDotRef.m_pos * (rearLinked->m_z - m_lastDotRef.m_dot->m_z)) << 8)
			/ m_lastDotRef.LinkDist()) + m_lastDotRef.m_dot->m_z * 256.0f;

		ChangeCoor((bx + fx) * 0.001953125f, (by + fy) * 0.001953125f,
			(bz + fz) * 0.001953125f);

		if (m_unk0x90 & 1) {
			float dy = (by - fy + 128.0f) * 3.0f;
			ChangeDirection(ANGLE((bx - fx + 128.0f) * 0.00390625f, dy * 0.001953125f));
			return;
		}
		else {
			float dy = (fy - by + 128.0f) * 3.0f;
			ChangeDirection(ANGLE((fx - bx + 128.0f) * 0.00390625f, dy * 0.001953125f));
			return;
		}
	}

	if (::Error) {
		int idx = m_vid ? m_vid->m_idx : -1;
		MYERROR::Error(::Error, "ENGINE %i", 10,
			// STRING: ALIEN 0x484a78
			"\xf0\xe5\xeb\xfc\xf1\xfb \xed\xe5\xef\xf0\xe0\xe2\xe8\xeb\xfc\xed\xfb\xe5 \xed\xe5\xf2 tail.Dot2()",
			0, idx);
	}
}

// FUNCTION: ALIEN 0x4513c0
void ENGINE::BreakTrain(float p_x, float p_y)
{
	ENGINE* front = FirstEngine();
	ENGINE* back = LastEngine();
	PlaySFX(15);

	ENGINE* prev = m_prevEngine;
	int breakPrev = 0;
	if (prev) {
		ENGINE* next = m_nextEngine;
		if (next) {
			float px = (float) fabs(p_x - prev->m_x);
			float py = (float) fabs(p_y - prev->m_y);
			float dp = (px > py) ? px + py * 0.5f : px * 0.5f + py;
			float nx = (float) fabs(p_x - next->m_x);
			float ny = (float) fabs(p_y - next->m_y);
			float dn = (nx > ny) ? nx + ny * 0.5f : nx * 0.5f + ny;
			breakPrev = dp < dn;
		}
	}

	ENGINE* next = m_nextEngine;
	if (!next || breakPrev) {
		if (prev) {
			prev->m_nextEngine = 0;
			m_prevEngine = 0;
		}
	}
	else {
		next->m_prevEngine = 0;
		m_nextEngine = 0;
	}

	if (back != front) {
		back->Stop();
		if (back->m_speed == 0.0f) {
			back->ReverseTrain();
			ENGINE* bf = back->FirstEngine();
			bf->m_speed = (bf->m_unk0x90 & 1) ? -0.0099999998f : 0.0099999998f;
		}
		if (front->m_speed != 0.0f) {
			front->ReCalcMoveParameters();
		}
		else {
			ENGINE* ff = front->FirstEngine();
			ff->m_speed = (ff->m_unk0x90 & 1) ? -0.0099999998f : 0.0099999998f;
		}
	}
}

static inline float DistanceTo(const R_DOT* p_dot, float p_x, float p_y, float p_z)
{
	float dz = p_dot->m_z - p_z;
	float dy = p_dot->m_y - p_y;
	float dx = p_dot->m_x - p_x;
	return (float) sqrt(dx * dx + dy * dy + dz * dz);
}

static inline ANGLE DirOf(const R_DOT_REF* p_ref)
{
	return p_ref->m_dot ? p_ref->m_dot->m_links[p_ref->m_link].m_dir : ANGLE(0);
}

// FUNCTION: ALIEN 0x451540
int ENGINE::ForceLink(ENGINE* p_other)
{
	if (!p_other || InTrain(p_other))
		return 1;

	int reverse;
	if (!p_other->m_prevEngine && !p_other->m_nextEngine) {

		reverse = DistanceTo(p_other->m_curDotRef.LinkedDot(), X(), Y(), Z())
			< DistanceTo(p_other->m_lastDotRef.LinkedDot(), X(), Y(), Z());
	}
	else {
		ENGINE* first = p_other->FirstEngine();
		float fy = first->m_y - m_y;
		float fx = first->m_x - m_x;
		ENGINE* last = p_other->LastEngine();
		float ly = last->m_y - m_y;
		float lx = last->m_x - m_x;
		reverse = (float) sqrt(fx * fx + fy * fy) < (float) sqrt(lx * lx + ly * ly);
	}
	if (reverse)
		p_other->ReverseTrain();

	ENGINE* tail = p_other->LastEngine();
	tail->m_nextEngine = this;
	m_prevEngine = tail;
	R_DOT_REF* tailRef = &tail->m_lastDotRef;
	R_DOT_REF ref;
	ref.m_dot = tailRef->m_dot->m_links[tailRef->m_link].m_dot;
	ref.m_pos = tailRef->m_dot->m_links[tailRef->m_link].m_dist - tailRef->m_pos;
	ref.m_unk0x08 = 0;
	ref.m_link = tailRef->m_dot->m_links[tailRef->m_link].m_backLink;
	m_curDotRef = ref;

	m_lastDotRef.m_dot = m_curDotRef.m_dot->m_links[m_curDotRef.m_dot->GetLink_dir(
		((const R_POS*) tailRef)->GetAngle())].m_dot;

	m_lastDotRef.m_link = m_lastDotRef.m_dot->GetLink_dir(DirOf(tailRef));

	unsigned char curDir = DirOf(&m_curDotRef).m_dir;
	unsigned char d1 = (unsigned char) (m_dir - curDir);
	unsigned char d2 = (unsigned char) (curDir - m_dir);
	unsigned char delta = (d1 < d2) ? d1 : d2;
	if (delta > 0x7f)
		m_unk0x90 |= 1;

	PullTail(&m_curDotRef);
	CalcCoor();
	return 0;
}

// FUNCTION: ALIEN 0x451810
ENGINE* ENGINE::FirstEngine()
{
	ENGINE* result = this;
	while (result->m_prevEngine)
		result = result->m_prevEngine;
	return result;
}

// FUNCTION: ALIEN 0x451830
ENGINE* ENGINE::LastEngine()
{
	ENGINE* result = this;
	while (result->m_nextEngine)
		result = result->m_nextEngine;
	return result;
}

static inline float SinOf(unsigned char p_dir) { return ANGLE::SinTable[p_dir]; }
static inline float CosOf(unsigned char p_dir) { return ANGLE::CosTable[p_dir]; }

static inline void StepRefToNearerDot(R_DOT_REF* p_ref, const SPRITE* p_sprite,
	float p_z, float p_y, float p_x)
{
	float dz = (float) p_ref->m_dot->m_z - p_sprite->m_z;
	float dy = (float) p_ref->m_dot->m_y - p_sprite->m_y;
	float dx = (float) p_ref->m_dot->m_x - p_sprite->m_x;
	R_DOT* linked = p_ref->LinkedDot();
	float lz = (float) linked->m_z - p_z;
	float ly = (float) linked->m_y - p_y;
	float lx = (float) linked->m_x - p_x;
	if (sqrt((double) (dx * dx + dy * dy + dz * dz))
		> sqrt((double) (lx * lx + ly * ly + lz * lz))) {
		R_DOT* linkDot = p_ref->m_dot;
		int linkIdx = p_ref->m_link;
		p_ref->m_pos = linkDot->m_links[linkIdx].m_dist - p_ref->m_pos;
		p_ref->m_dot = linkDot->m_links[linkIdx].m_dot;
		p_ref->m_link = linkDot->m_links[linkIdx].m_backLink;
	}
}

// STUB: ALIEN 0x451850
void ENGINE::SetRDot()
{
	float halfLen = m_vid->m_exData->m_unk0x08 * 0.5f;
	ANGLE dir = m_dir;
	R_DOT* dot = RailMap.GetNearestDot_xyr((int) X(), (int) Y(), (int) Z());
	if (!dot)
		return;
	if (!dot->m_noLinks)
		return;
	int i;
	for (i = dot->m_noLinks - 1; i >= 0; --i) {
		unsigned char linkDir = dot->m_links[i].m_dir.m_dir;
		unsigned char d1 = (unsigned char) (dir.m_dir - linkDir);
		unsigned char d2 = (unsigned char) (linkDir - dir.m_dir);
		if ((d1 < d2 ? d1 : d2) > 0x6c
			|| (d2 = d1 < d2 ? d1 : d2) < 0x14)
			break;
	}
	if (i < 0)
		AngleAssign(&dir, dot->m_links[0].m_dir);
	else
		dir.m_dir = m_dir;
	dot->SetNearestPos((int) (SinOf(dir.m_dir) * halfLen + m_x),
		(int) (Y() - CosOf(dir.m_dir) * halfLen), (int) m_z,
		(R_POS*) &m_curDotRef);
	StepRefToNearerDot(&m_curDotRef, this, m_z, m_y, m_x);
	dot->SetNearestPos(
		(int) (SinOf((unsigned char) (dir.m_dir + 0x80)) * halfLen + m_x),
		(int) (Y() - CosOf((unsigned char) (dir.m_dir + 0x80)) * halfLen),
		(int) m_z, (R_POS*) &m_lastDotRef);
	StepRefToNearerDot(&m_lastDotRef, this, m_z, m_y, m_x);
	PullTail(&m_curDotRef);
	CalcCoor();
	ForceLink(GetIntersecting());
	SetDotBusy();
}

// FUNCTION: ALIEN 0x451b80
int ENGINE::InTrain(const SPRITE* p_sprite) const
{
	if (!p_sprite)
		return 0;
	const ENGINE* p;
	for (p = this; p; p = p->m_nextEngine)
		if (p == (const ENGINE*) p_sprite)
			return 1;
	for (p = m_prevEngine; p; p = p->m_prevEngine)
		if (p == (const ENGINE*) p_sprite)
			return 1;
	return 0;
}

// FUNCTION: ALIEN 0x451bd0
void ENGINE::ReverseTrain()
{
	ENGINE* first = FirstEngine();
	ENGINE* engine = first;
	if (engine) {
		do {
			ENGINE* next = engine->m_nextEngine;
			engine->m_nextEngine = engine->m_prevEngine;
			R_DOT_REF cur = engine->m_curDotRef;
			engine->m_curDotRef = engine->m_lastDotRef;
			engine->m_lastDotRef = cur;
			engine->m_prevEngine = next;
			engine->m_unk0x90 ^= (engine->m_unk0x90 ^ ~engine->m_unk0x90) & 1;
			if (!next) {
				if (engine != first) {
					engine->m_flag ^= (engine->m_flag ^ first->m_flag) & 0x80;
					engine->m_speed = first->m_speed;
					engine->m_unk0xb0 = -first->m_unk0xb0;
					engine->m_unk0xac = first->m_unk0xac;
					memcpy(engine->m_pathLinks, first->m_pathLinks, first->m_noPathLinks);
					first->m_noPathLinks = 0;
				}
				else {
					engine->m_unk0xb0 = -first->m_unk0xb0;
				}
			}
			engine = next;
		} while (engine);
	}
}

// FUNCTION: ALIEN 0x451d00
ENGINE* ENGINE::GetIntersecting()
{
	ENGINE* e = m_curDotRef.m_dot->m_busyEngine;
	if (e && IsTouch(e, 0))
		return m_curDotRef.m_dot->m_busyEngine;
	if (m_curDotRef.LinkedDot()->m_busyEngine) {
		if (IsTouch(m_curDotRef.LinkedDot()->m_busyEngine, 0))
			return m_curDotRef.LinkedDot()->m_busyEngine;
	}
	e = m_lastDotRef.m_dot->m_busyEngine;
	if (e && IsTouch(e, 0))
		return m_lastDotRef.m_dot->m_busyEngine;
	if (m_lastDotRef.LinkedDot()->m_busyEngine) {
		if (IsTouch(m_lastDotRef.LinkedDot()->m_busyEngine, 0))
			return m_lastDotRef.LinkedDot()->m_busyEngine;
	}
	return 0;
}

// FUNCTION: ALIEN 0x451e60
ENGINE* ENGINE::GetBadIntersecting()
{
	if (m_curDotRef.m_dot) {
		R_DOT_LINK* cross = m_curDotRef.m_dot->m_links[m_curDotRef.m_link].m_crossLink;
		if (cross) {
			ENGINE* e = cross->m_dot->m_busyEngine;
			if (e && !e->InTrain(this))
				return cross->m_dot->m_busyEngine;
			R_DOT_LINK* back = &cross->m_dot->m_links[cross->m_backLink];
			ENGINE* other = back->m_dot->m_busyEngine;
			if (other) {
				if (!other->InTrain(this))
					return back->m_dot->m_busyEngine;
			}
		}
	}
	if (m_curDotRef.LinkedDot()) {
		int back = m_curDotRef.m_dot->m_links[m_curDotRef.m_link].m_backLink;
		if (m_curDotRef.LinkedDot()
				->m_links[m_curDotRef.m_dot->m_links[m_curDotRef.m_link].m_backLink]
				.m_crossLink) {
			R_DOT_LINK* cross = m_curDotRef.LinkedDot()
				->m_links[m_curDotRef.m_dot->m_links[m_curDotRef.m_link].m_backLink]
				.m_crossLink;
			ENGINE* e = cross->m_dot->m_busyEngine;
			if (e && !e->InTrain(this))
				return cross->m_dot->m_busyEngine;
			R_DOT_LINK* backLink = &cross->m_dot->m_links[cross->m_backLink];
			ENGINE* other = backLink->m_dot->m_busyEngine;
			if (other && !other->InTrain(this))
				return backLink->m_dot->m_busyEngine;
		}
	}
	return 0;
}

// FUNCTION: ALIEN 0x451fa0
ENGINE* ENGINE::GetForwardIntersecting(int* p_touch)
{
	ENGINE* here = m_curDotRef.m_dot->m_busyEngine;
	if (here) {
		int touch = IsTouch(here, 1);
		*p_touch = touch;
		if (touch)
			return m_curDotRef.m_dot->m_busyEngine;
	}
	if (m_curDotRef.LinkedDot()->m_busyEngine) {
		int touch = IsTouch(m_curDotRef.LinkedDot()->m_busyEngine, 1);
		*p_touch = touch;
		if (touch)
			return m_curDotRef.LinkedDot()->m_busyEngine;
	}
	*p_touch = 0;
	return 0;
}

// FUNCTION: ALIEN 0x452070
int ENGINE::IsTouch(const ENGINE* p_engine, int p_flag)
{
	if (!p_engine || InTrain(p_engine))
		return 0;
	if (m_curDotRef.m_dot) {
		if (p_engine->m_curDotRef.m_dot && m_curDotRef.LinkedDot() == p_engine->m_curDotRef.m_dot) {
			if (m_curDotRef.m_dot == p_engine->m_curDotRef.LinkedDot()) {
				if (p_flag) {
					if (m_curDotRef.m_pos > m_curDotRef.LinkDist() - p_engine->m_curDotRef.m_pos)
						return 1;
				}
				else if (m_curDotRef.m_pos >= m_curDotRef.LinkDist() - p_engine->m_curDotRef.m_pos)
					return 1;
			}
		}
		if (p_engine->m_lastDotRef.m_dot && m_curDotRef.LinkedDot() == p_engine->m_lastDotRef.m_dot) {
			if (m_curDotRef.m_dot == p_engine->m_lastDotRef.LinkedDot()) {
				if (p_flag) {
					if (m_curDotRef.m_pos > m_curDotRef.LinkDist() - p_engine->m_lastDotRef.m_pos)
						return 2;
				}
				else if (m_curDotRef.m_pos >= m_curDotRef.LinkDist() - p_engine->m_lastDotRef.m_pos)
					return 2;
			}
		}
	}
	if (m_lastDotRef.m_dot) {
		if (p_engine->m_curDotRef.m_dot && m_lastDotRef.LinkedDot() == p_engine->m_curDotRef.m_dot) {
			if (m_lastDotRef.m_dot == p_engine->m_curDotRef.LinkedDot()) {
				if (p_flag) {
					if (m_lastDotRef.m_pos > m_lastDotRef.LinkDist() - p_engine->m_curDotRef.m_pos)
						return 3;
				}
				else if (m_lastDotRef.m_pos >= m_lastDotRef.LinkDist() - p_engine->m_curDotRef.m_pos)
					return 3;
			}
		}
		if (p_engine->m_lastDotRef.m_dot && m_lastDotRef.LinkedDot() == p_engine->m_lastDotRef.m_dot) {
			if (m_lastDotRef.m_dot == p_engine->m_lastDotRef.LinkedDot()) {
				if (p_flag) {
					if (m_lastDotRef.m_pos > m_lastDotRef.LinkDist() - p_engine->m_lastDotRef.m_pos)
						return 4;
				}
				else if (m_lastDotRef.m_pos >= m_lastDotRef.LinkDist() - p_engine->m_lastDotRef.m_pos)
					return 4;
			}
		}
	}
	if (!p_flag) {
		if (m_curDotRef.m_dot) {
			if (m_curDotRef.m_dot == p_engine->m_curDotRef.m_dot)
				return 100;
			if (m_curDotRef.m_dot == p_engine->m_lastDotRef.m_dot)
				return 200;
		}
		if (m_lastDotRef.m_dot) {
			if (m_lastDotRef.m_dot == p_engine->m_curDotRef.m_dot)
				return 300;
			if (m_lastDotRef.m_dot == p_engine->m_lastDotRef.m_dot)
				return 400;
		}
	}
	else {
		if (m_curDotRef.m_dot) {
			if (m_curDotRef.m_dot == p_engine->m_curDotRef.m_dot) {
				if (m_curDotRef.m_link == p_engine->m_curDotRef.m_link)
					return 2;
				return 1;
			}
			if (m_curDotRef.m_dot == p_engine->m_lastDotRef.m_dot)
				return 200;
		}
		if (!m_lastDotRef.m_dot)
			return 0;
		if (m_lastDotRef.m_dot == p_engine->m_curDotRef.m_dot)
			return 300;
		if (m_lastDotRef.m_dot == p_engine->m_lastDotRef.m_dot)
			return 400;
	}
	return 0;
}

// FUNCTION: ALIEN 0x452480
void ENGINE::ClearDotBusy()
{
	if (m_curDotRef.m_dot && m_curDotRef.m_dot->m_busyEngine == this)
		m_curDotRef.m_dot->m_busyEngine = 0;
	if (m_lastDotRef.m_dot && m_lastDotRef.m_dot->m_busyEngine == this)
		m_lastDotRef.m_dot->m_busyEngine = 0;
	if (m_curDotRef.LinkedDot() && m_curDotRef.LinkedDot()->m_busyEngine == this)
		m_curDotRef.LinkedDot()->m_busyEngine = 0;
	if (m_lastDotRef.LinkedDot() && m_lastDotRef.LinkedDot()->m_busyEngine == this)
		m_lastDotRef.LinkedDot()->m_busyEngine = 0;
}

// FUNCTION: ALIEN 0x452570
void ENGINE::SetDotBusy()
{
	ClearDotBusy();
	if (m_curDotRef.m_dot)
		m_curDotRef.m_dot->m_busyEngine = this;
	if (m_lastDotRef.m_dot && m_lastDotRef.m_dot != m_curDotRef.m_dot)
		m_lastDotRef.m_dot->m_busyEngine = this;
}

static inline void SetRef(R_DOT_REF* p_ref, R_DOT* p_dot, int p_dist, int p_back, int p_pos)
{
	p_ref->m_dot = p_dot;
	p_ref->m_pos = p_dist - p_pos;
	p_ref->m_unk0x08 = 0;
	p_ref->m_link = p_back;
}

static inline int RefPosOf(const R_DOT_REF* p_ref)
{
	return p_ref->m_pos;
}

// STUB: ALIEN 0x4525b0
void ENGINE::PullTail(R_DOT_REF* p_ref)
{
	int frontPos = RefPosOf(&m_curDotRef);
	if ((float) frontPos > m_vid->m_exData->m_unk0x08) {
		SetRef(&m_lastDotRef, m_curDotRef.m_dot->m_links[m_curDotRef.m_link].m_dot,
			m_curDotRef.m_dot->m_links[m_curDotRef.m_link].m_dist,
			m_curDotRef.m_dot->m_links[m_curDotRef.m_link].m_backLink, m_curDotRef.m_pos);
		int dist = m_curDotRef.m_dot ? m_curDotRef.m_dot->m_links[m_curDotRef.m_link].m_dist : 0;
		m_lastDotRef.m_pos = (int) m_vid->m_exData->m_unk0x08 - frontPos + dist;
	}
	else if (m_lastDotRef.m_dot == m_curDotRef.m_dot) {
		m_lastDotRef.m_pos = (int) m_vid->m_exData->m_unk0x08 - frontPos;
	}
	else {
		R_DOT* refDot = p_ref->m_dot;
		if (refDot == m_curDotRef.m_dot) {
			int idx = m_curDotRef.m_dot->GetLink_idx(m_lastDotRef.m_dot);
			if (idx < 0) {
				if (::Error)
					MYERROR::Error(::Error,
						"ENGINE %i", 10,
						// STRING: ALIEN 0x484ae0
						"rail 1", 0, m_vid ? m_vid->m_idx : -1);
				idx = m_curDotRef.m_dot->GetLink_dir(
					((R_POS*) &m_curDotRef)->GetAngle().m_dir + 0x80);
				m_lastDotRef.m_dot = m_curDotRef.m_dot->m_links[idx].m_dot;
			}
			int dist = m_curDotRef.m_dot->m_links[idx].m_dist;
			if ((float) (m_curDotRef.m_pos + dist) > m_vid->m_exData->m_unk0x08) {
				if (m_curDotRef.m_link == idx) {
					m_lastDotRef.m_pos =
						(int) m_vid->m_exData->m_unk0x08 - m_curDotRef.m_pos + dist;
				}
				else {
					m_lastDotRef.m_link = idx;
					m_lastDotRef.m_dot = m_curDotRef.m_dot;
					m_lastDotRef.m_pos = (int) m_vid->m_exData->m_unk0x08 - m_curDotRef.m_pos;
				}
			}
			else {
				m_lastDotRef.m_pos =
					(int) m_vid->m_exData->m_unk0x08 - dist - m_curDotRef.m_pos;
			}
		}
		else if (refDot == m_lastDotRef.m_dot) {
			if ((float) (frontPos + (refDot ? refDot->m_links[p_ref->m_link].m_dist : 0))
				> m_vid->m_exData->m_unk0x08) {
				R_DOT_LINK* link = &refDot->m_links[p_ref->m_link];
				m_lastDotRef.m_dot = link->m_dot;
				m_lastDotRef.m_pos = link->m_dist - p_ref->m_pos;
				m_lastDotRef.m_unk0x08 = 0;
				m_lastDotRef.m_link = link->m_backLink;
				m_lastDotRef.m_pos = (int) m_vid->m_exData->m_unk0x08 - frontPos;
			}
			else {
				m_lastDotRef.m_pos = (int) m_vid->m_exData->m_unk0x08 - frontPos
					- (refDot ? refDot->m_links[p_ref->m_link].m_dist : 0);
			}
		}
		else {
			if ((float) (frontPos + (refDot ? refDot->m_links[p_ref->m_link].m_dist : 0))
				> m_vid->m_exData->m_unk0x08) {
				MYERROR::Log(::Error,
					// STRING: ALIEN 0x484ad8
					"zmdots5");
				R_DOT_LINK* link = &p_ref->m_dot->m_links[p_ref->m_link];
				m_lastDotRef.m_dot = link->m_dot;
				m_lastDotRef.m_pos = link->m_dist - p_ref->m_pos;
				m_lastDotRef.m_unk0x08 = 0;
				m_lastDotRef.m_link = link->m_backLink;
				m_lastDotRef.m_pos = (int) m_vid->m_exData->m_unk0x08 - m_curDotRef.m_pos;
			}
			else {
				MYERROR::Log(::Error,
					// STRING: ALIEN 0x484ad0
					"zmdots6");
				int idx = p_ref->m_dot->GetLink_idx(m_lastDotRef.m_dot);
				m_lastDotRef.m_link = idx;
				m_lastDotRef.m_dot = p_ref->m_dot;
				int rdist = p_ref->m_dot ? p_ref->m_dot->m_links[p_ref->m_link].m_dist : 0;
				m_lastDotRef.m_pos =
					(int) m_vid->m_exData->m_unk0x08 - rdist - m_curDotRef.m_pos;
				if (idx < 0) {
					if (::Error)
						MYERROR::Error(::Error, "ENGINE %i", 10,
							// STRING: ALIEN 0x484ac8
							"rail 2", 0, m_vid ? m_vid->m_idx : -1);
					m_lastDotRef.m_link = m_lastDotRef.m_dot->GetLink_dir(
						((R_POS*) p_ref)->GetAngle().m_dir + 0x80);
				}
			}
		}
	}
}

static inline SPRITE* GoalOf(SPRITE* p_sprite)
{
	return p_sprite->m_goal;
}

// STUB: ALIEN 0x4529c0
float ENGINE::Clash(ENGINE* p_other, int p_dir)
{
	TRAIN_INFO thisInfo(this);
	TRAIN_INFO otherInfo(p_other);
	float thisSpeed = (float) fabs(m_speed);
	float otherSpeed = (float) fabs(p_other->m_speed);
	float speed = (thisInfo.m_accelTime * thisSpeed + otherInfo.m_accelTime * otherSpeed)
		/ (thisInfo.m_accelTime + otherInfo.m_accelTime);
	if (speed > 0.001f && speed < 0.01f)
		speed = 0.01f;

	float impactSpeed;
	if (p_dir == 2 || p_dir == 3)
		impactSpeed = (float) fabs((double) otherSpeed - thisSpeed);
	else
		impactSpeed = thisSpeed + otherSpeed;

	if (p_dir == 1 || p_dir == 3)
		p_other->ReverseTrain();
	ENGINE* engine = p_other->FirstEngine();
	while (engine) {
		engine->m_speed = (engine->m_unk0x90 & 1) ? -speed : speed;
		engine = engine->m_nextEngine;
	}

	if (impactSpeed > Const->m_minMoveSpeed
		&& (((m_flag & 0x7c) == 0x6c && p_other->InTrain(GoalOf(this)))
			|| ((p_other->m_flag & 0x7c) == 0x6c && InTrain(GoalOf(p_other))))) {
		ChangeAnimation(12);
		PlaySFX(16);
		int damage = (int) (impactSpeed * 1500.0f) / 2;
		int falloff = damage;
		if (p_other->m_vid->m_idx == 97) {
			int army = (p_other->m_flag >> 11) & 3;
			p_other->Action(85, p_other->m_vid->m_maxHp[army] + 10, 0, 0);
		}
		else {
			p_other->Action(85, damage, 0, 0);
		}
		for (engine = p_other->m_prevEngine; engine && falloff >= 2;
			 engine = engine->m_prevEngine) {
			falloff /= 2;
			engine->Action(85, falloff, 0, 0);
		}

		if (m_vid->m_idx == 97) {
			int army = (m_flag >> 11) & 3;
			Action(85, m_vid->m_maxHp[army] + 10, 0, 0);
		}
		else {
			Action(85, damage, 0, 0);
		}
		engine = m_prevEngine;
		if (engine) {
			while (engine) {
				if (damage < 2)
					break;
				damage /= 2;
				engine->Action(85, damage, 0, 0);
				engine = engine->m_prevEngine;
			}
		}
		else {
			engine = m_nextEngine;
			if (engine) {
				while (engine) {
					if (damage < 2)
						break;
					damage /= 2;
					engine->Action(85, damage, 0, 0);
					engine = engine->m_nextEngine;
				}
			}
		}
	}
	else {
		PlaySFX(19);
	}
	return 0.0f;
}

// FUNCTION: ALIEN 0x452c50
int ENGINE::GetTrainLengthInRails()
{
	int n = 0;
	int result = 0;
	ENGINE* p = FirstEngine();
	if (p) {
		do {
			p = p->m_nextEngine;
			++n;
		} while (p);
		result = n;
	}
	return (int) (result * 1.33f + 0.5f);
}

// FUNCTION: ALIEN 0x452c90
void ENGINE::MT_SpeedProcessing(float* p_speed)
{
	if (m_unk0xb4 && m_unk0xb0 == 0.0f) {
		*p_speed = 0.035f;
		m_unk0xac = 0;
		return;
	}
	int dt;
	if (m_unk0xb0 > *p_speed) {
		if (!m_unk0xac)
			ReCalcMoveParameters();
		int decel = m_unk0xac;
		if (decel)
			*p_speed = decel * (CurrentTime - PrevCurrentTime) * 0.000001f * 2.0 + *p_speed;
		if (*p_speed < m_unk0xb0)
			return;
	}
	else {
		if (m_unk0xb0 >= *p_speed) {
			m_unk0xac = 0;
			return;
		}
		int accel = (int) ((*p_speed * 1000.0f + 10.0f) * -0.5f);
		m_unk0xac = accel;
		dt = CurrentTime - PrevCurrentTime;
		*p_speed = accel * dt * 0.000001f * 2.0 + *p_speed;
		if (*p_speed > m_unk0xb0)
			return;
	}
	*p_speed = m_unk0xb0;
	m_unk0xac = 0;
}

// FUNCTION: ALIEN 0x452db0
int ENGINE::CanLinkWithEngine(ENGINE* p_other)
{
	if (!p_other)
		return 0;
	if (p_other->InTrain(GoalOf(this)) && (m_flag & 0x7c) == 0x68 ||
		InTrain(GoalOf(p_other)) && (p_other->m_flag & 0x7c) == 0x68)
		return 1;
	return (m_unk0xb4 || p_other->m_unk0xb4) && !((p_other->m_flag ^ m_flag) & 0x1800);
}

// STUB: ALIEN 0x452e30
int ENGINE::MT_IntersectingProcessing(const R_POS& p_pos, float* p_speed)
{
	int result = 0;
	int dir = -1;
	ENGINE* hit = GetForwardIntersecting(&dir);
	if (hit) {
		m_curDotRef = *(const R_DOT_REF*) &p_pos;
		if (CanLinkWithEngine(hit)) {
			if (hit->m_nextEngine || dir == 1 || dir == 4)
				hit->ReverseTrain();
			if (!((hit->m_flag ^ m_flag) & 0x1800)) {
				if ((m_flag & 0x7c) == 0x68 && hit->InTrain(GoalOf(this))) {
					if ((hit->m_flag & 0x7c) != 0x68 || !InTrain(GoalOf(hit)))
						hit->Action(70, 0, 0, 0);
				}
				else {
					Action(70, 0, 0, 0);
				}
			}
			if (hit->m_nextEngine) {
				if (::Error)
					MYERROR::Error(::Error,
						"ENGINE %i", 10,
						// STRING: ALIEN 0x484ae8
						"can't link", 0, m_vid ? m_vid->m_idx : -1);
			}
			else {
				hit->m_nextEngine = this;
				m_prevEngine = hit;
			}
			SetCommandToTrain(0, 0, 0, 0);
			ChangeAnimation(11); // ANI_LOAD
			if ((hit->m_flag ^ m_flag) & 0x1800) {
				for (ENGINE* engine = FirstEngine(); engine; engine = engine->m_nextEngine) {
					if (engine->m_flag & 0x1800)
						engine->DeleteAttackToEngine();
				}
				ENGINE* who = this;
				if ((hit->m_flag & 0x1800) == 0x800)
					who = hit;
				Map->ScriptRun(EvFunctionNumber[21], who, 0, 0);
				return 0;
			}
		}
		else {
			result = 1;
			*p_speed = Clash(hit, dir);
			Map->ScriptRun(EvFunctionNumber[22], this, hit, 0);
		}
	}
	else if (GetBadIntersecting()) {
		result = 1;
		if (*p_speed > 0.2f)
			PlaySFX(148);
		m_curDotRef = *(const R_DOT_REF*) &p_pos;
		*p_speed = 0.0f;
		m_unk0xb0 = -m_unk0xb0;
		return result;
	}
	return result;
}

// STUB: ALIEN 0x453070
void ENGINE::MT_PullCoordinates(R_POS* p_ref, float p_speed, int p_deceleration)
{
	ENGINE* lead = this;
	for (ENGINE* e = this; e; e = e->m_nextEngine) {
		e->ClearDotBusy();
		e->PullTail((R_DOT_REF*) p_ref);
		if (e->m_nextEngine) {
			*(R_DOT_REF*) p_ref = e->m_nextEngine->m_curDotRef;
			R_DOT* lastDot = e->m_lastDotRef.m_dot;
			int lastIdx = e->m_lastDotRef.m_link;
			SetRef(&e->m_nextEngine->m_curDotRef, lastDot->m_links[lastIdx].m_dot,
				lastDot->m_links[lastIdx].m_dist, e->m_lastDotRef.m_dot->m_links[lastIdx].m_backLink,
				e->m_lastDotRef.m_pos);
		}
		e->CalcCoor();
		if ((e->m_unk0xe0 != 0.0f || e->m_unk0xe4 != 0.0f)
			&& (float) fabs(e->m_unk0xe0 - e->X()) > 30.0f)
			MYERROR::Log(::Error,
				// STRING: ALIEN 0x484af4
				"\xf1\xf5\xeb\xe0\xef\xfb\xe2\xe0\xed\xe8\xe5 \xe2\xe0\xe3\xee\xed\xee\xe2 zm-error - kawabanga - begin");
		e->m_unk0xe0 = e->X();
		e->m_unk0xe4 = e->Y();
		e->m_unk0xe8 = e->Z();
		e->SetDotBusy();
		e->m_speed = (e->m_unk0x90 & 1) ? -p_speed : p_speed;

		e->m_unk0xac = p_deceleration;
		lead = this;
	}

	if (p_speed != 0.0f) {
		float z = lead->m_z;
		float x = lead->m_x;
		float y = lead->m_y;
		VID* vid = lead->m_vid;

		for (SPRITE* s = Hash->FirstInBox(x - vid->m_unk0x384, y - vid->m_unk0x388,
				 x + vid->m_unk0x384, y + vid->m_unk0x388);
			 s;
			 s = Hash->NextInBox()) {
			if (s == lead)
				continue;
			VID* myVid = lead->m_vid;
			if (s->m_ani >= 15)
				continue;
			VID* itsVid = s->m_vid;
			if ((float) fabs(s->m_x - x) < itsVid->m_unk0x384 + myVid->m_unk0x384
				&& (float) fabs(s->m_y - y) < itsVid->m_unk0x388 + myVid->m_unk0x388
				&& itsVid->m_unk0x24 + s->m_z >= z && z + myVid->m_unk0x24 >= s->m_z
				&& (itsVid->m_flag & 0x4000))
				s->Action(85, 5, 0, 0);
		}
	}
}

// FUNCTION: ALIEN 0x4532d0
void ENGINE::DeleteAttackToEngine()
{
	SPRITE* sprite = (SPRITE*) Hash->m_list.LastIterate(&Hash->m_iter);
	while (sprite) {
		if (sprite->m_goal == this) {
			if (sprite->m_vid->m_sprClass == 21
				&& ((sprite->m_flag & 0x7c) == 112 || (sprite->m_flag & 0x7c) == 116)) {
				((ENGINE*) sprite)->SetCommandToTrain(0, 0, 0, 0);
			}
			else {
				int cmd = sprite->m_flag & 0x7c;
				if (cmd == 20 || cmd == 12 || cmd == 16)
					sprite->SetCommand(0, 0);
			}
		}
		else {
			SPRITE* child = sprite->m_child;
			if (child && child->m_goal == this) {
				int cmd = child->m_flag & 0x7c;
				if (cmd == 20 || cmd == 12 || cmd == 16 || cmd == 112 || cmd == 116)
					child->SetCommand(0, 0);
			}
		}
		sprite = (SPRITE*) Hash->m_list.NextIterate(&Hash->m_iter);
	}
}

// FUNCTION: ALIEN 0x453390
void ENGINE::CreatePathDots(R_DOT* p_dot)
{
	CreatePathDots(p_dot, &PathDots, 0x25b);
	for (int i = 0; i < PathDots.m_n; ++i) {
		int cls = m_flag & 0x7c;
		if (cls == 0x70 || cls == 0x74)
			((SPRITE*) PathDots.m_data[i])->ChangeArmy(1);
		else if (cls == 0x6c)
			((SPRITE*) PathDots.m_data[i])->ChangeArmy(3);
		else if (cls == 0x68)
			((SPRITE*) PathDots.m_data[i])->ChangeArmy(2);
	}
}

// FUNCTION: ALIEN 0x453410
void ENGINE::CreatePathDots(R_DOT* p_dot, LIST_SPRITE* p_list, int p_vid)
{
	R_DOT* dot = p_dot;
	NoStepForNotFound = R_DOT::NoStepForNotFound;
	NoStep = R_DOT::NoStep;
	p_list->DeleteAll();
	for (int i = 0; i < m_noPathLinks; i++) {
		int link = m_pathLinks[i];
		if (link >= 0 && link < dot->m_noLinks) {
			dot = dot->m_links[link].m_dot;
			p_list->Insert(Map->CreateSprite(Map->Vid(p_vid), dot->m_x, dot->m_y, dot->m_z, ANGLE(0), 0));
		}
	}
}

// FUNCTION: ALIEN 0x4534d0
int ENGINE::IsTailInFindedPath(R_DOT* p_dot)
{
	R_DOT* dot = p_dot;
	R_DOT_REF* r = &LastEngine()->m_lastDotRef;
	R_DOT* tailDot;
	if (r->m_dot)
		tailDot = r->m_dot->m_links[r->m_link].m_dot;
	else
		tailDot = 0;
	int n = m_noPathLinks;
	for (int i = 0; i < n; ++i) {
		int idx = m_pathLinks[i];
		if (idx >= 0 && idx < dot->m_noLinks) {
			dot = dot->m_links[idx].m_dot;
			if (dot == tailDot)
				return 1;
		}
	}
	return 0;
}

// FUNCTION: ALIEN 0x453540
int ENGINE::ReachTheTarget()
{
	int result = 2;
	SPRITE* goal = m_goal;
	if (goal) {
		int flag = m_flag & 0x7c;
		if (flag == 112 || flag == 116) {
			SPRITE* owner = m_commandOwner;
			if (owner) {
				if (owner->m_child) {
					VID* vid = owner->m_child->m_vid;
					if (vid == owner->m_vid->m_unk0x5c && vid->m_weaponVid && vid->m_weapon) {
						float d = NearDistanceTo(goal->m_x - m_commandOwner->m_child->m_x,
							goal->m_y - m_commandOwner->m_child->m_y);
						if (d > m_commandOwner->m_child->m_vid->m_exData->m_unk0x18 - 10.0f)
							return 0;
						return 1;
					}
				}
				VID* vid = owner->m_vid;
				if (vid->m_idx == 35) {
					float dx = (float) fabs(goal->m_x - owner->m_x);
					float dy = (float) fabs(goal->m_y - owner->m_y);
					float d;
					if (dx > dy)
						d = dx + dy * 0.5f;
					else
						d = dx * 0.5f + dy;
					if (d > vid->m_exData->m_unk0x18 - 10.0f)
						return 0;
					return 1;
				}
			}
			else {
				for (ENGINE* engine = FirstEngine(); engine; engine = engine->m_nextEngine) {
					SPRITE* child = engine->m_child;
					if (child) {
						VID* vid = child->m_vid;
						if (vid == engine->m_vid->m_unk0x5c && vid->m_weaponVid && vid->m_weapon) {
							float dx = (float) fabs(m_goal->m_x - child->m_x);
							float dy = (float) fabs(m_goal->m_y - child->m_y);
							float d;
							if (dx > dy)
								d = dx + dy * 0.5f;
							else
								d = dx * 0.5f + dy;
							if (d > vid->m_exData->m_unk0x18 - 10.0f)
								return 0;
							result = 1;
							continue;
						}
					}
					VID* vid = engine->m_vid;
					if (vid->m_idx == 35) {
						float dx = (float) fabs(m_goal->m_x - engine->m_x);
						float dy = (float) fabs(m_goal->m_y - engine->m_y);
						float d;
						if (dx > dy)
							d = dx + dy * 0.5f;
						else
							d = dx * 0.5f + dy;
						if (d > vid->m_exData->m_unk0x18 - 10.0f)
							return 0;
						result = 1;
					}
				}
			}
			return result;
		}
	}
	return 2;
}

static inline void EngineBackLink(const R_DOT_REF* p_ref, int* p_back)
{
	if (p_ref->m_dot)
		*p_back = p_ref->m_dot->m_links[p_ref->m_link].m_backLink;
}

// STUB: ALIEN 0x453750
void ENGINE::MoveEngineTact()
{
	R_POS ref;
	ref = *(R_POS*) &m_curDotRef;

	if (!m_curDotRef.m_dot || !m_lastDotRef.m_dot || m_prevEngine)
		return;

	if (m_vid->m_idx != 85 && m_curDotRef.LinkedDot()
		&& (int) m_curDotRef.LinkedDot()->m_unk0x14 - 4 == ((m_flag >> 11) & 3)
		&& m_curDotRef.m_pos > m_curDotRef.LinkDist() / 2) {
		Stop();
		m_speed = 0.0f;
		ReverseTrain();
		return;
	}

	int sawFlag1 = 0;
	ENGINE* scan = this;
	for (;;) {
		if (scan->m_curDotRef.m_dot
			&& scan->m_curDotRef.m_dot->m_unk0x0c == scan->m_curDotRef.m_link
			&& scan->m_curDotRef.m_dot->m_unk0x10) {
			for (ENGINE* e = this; e; e = e->m_nextEngine)
				e->m_unk0xb4 = 1;
			break;
		}
		if (scan->m_curDotRef.LinkedDot()) {
			int back = 0;
			EngineBackLink(&scan->m_curDotRef, &back);
			if (scan->m_curDotRef.LinkedDot()->m_unk0x0c == back) {
				if (m_speed != 0.0f) {
					if ((m_flag & 0x7c) == 0x5c
						&& (!m_noPathLinks
							|| m_curDotRef.m_dot->m_links[m_pathLinks[0]].m_dot
								== m_curDotRef.LinkedDot()))
						SetCommandToTrain(0, (SPRITE*) 0, (R_DOT*) 0, (R_DOT*) 0);
					else
						m_flag &= ~0x80u;
					m_speed = 0.0f;
					PlaySFX(148);
				}
				if (m_unk0xb0 > 0.0f)
					m_unk0xb0 = -m_unk0xb0;
				ReverseTrain();
				return;
			}
		}
		int f1 = scan->m_flag & 1;
		scan->m_unk0xb4 = 0;
		scan = scan->m_nextEngine;
		sawFlag1 |= f1;
		if (!scan) {
			if (sawFlag1) {
				for (ENGINE* e = this; e; e = e->m_nextEngine) {
					if (e->m_flag & 1) {
						e->m_flag &= ~1u;
						if (e->m_vid->m_exData->m_unk0x10 != 0.0f) {

							Map->m_player[(m_flag >> 11) & 3]->AddPointerToSprite(e);
						}
						if (e->m_unk0xdc) {
							Map->ScriptRun(EvFunctionNumber[4], e, 0, 0);
							e->m_unk0xdc = 0;
						}
					}
				}
			}
			break;
		}
	}

	if (!m_unk0xb4) {
		if (!(m_flag & 0x80) && m_speed == 0.0f) {
			int cmd = m_flag & 0x7c;
			if (cmd == 92 || cmd == 104 || cmd == 108 || cmd == 100)
				StartMove();
			if ((m_flag & 0x7c) == 0x60) {
				ENGINE* e = FirstEngine();
				while (e && (e->m_vid->m_idx != 85 || e->m_unk0xec))
					e = e->m_nextEngine;
				if (e)
					StartMove();
			}
		}
		if (!(m_flag & 0x80)) {
			int cmd = m_flag & 0x7c;
			if ((cmd == 112 || cmd == 116) && !(rand() % 5) && ReachTheTarget() != 1)
				StartMove();
		}
		if (m_unk0xb0 == 0.0f && (m_flag & 0x80)) {
			ReCalcMoveParameters();
			if (m_unk0xb0 == 0.0f && m_speed == 0.0f)
				SetCommandToTrain(0, (SPRITE*) 0, (R_DOT*) 0, (R_DOT*) 0);
		}
	}

	SPRITE* goal = m_goal;
	if (m_commandDot)
		goal = 0;
	float speed = (float) fabs(m_speed);
	MT_SpeedProcessing(&speed);
	if (speed < 0.0f) {
		m_speed = 0.0f;
		ReverseTrain();
		ReCalcMoveParameters();
		return;
	}

	R_DOT* headDot = (R_DOT*) m_curDotRef.m_dot;
	if (*(int*) &headDot->m_unk0x04 && speed > *(float*) ((char*) Const + 24)) {
		if (m_vid->m_idx != 85)
			speed = *(float*) ((char*) Const + 24);
		Map->CreateSprite(Map->Vid(588), GetX(), GetY(), GetZ(), ANGLE((unsigned char) 0),
			(SPRITE*) this);
	}

	int dt = CurrentTime - PrevCurrentTime;
	int delta = (int) (dt * speed * 64000.0f);
	int frac = delta + m_curDotRef.m_unk0x08;
	m_curDotRef.m_unk0x08 = frac;
	if (frac < 0)
		m_curDotRef.m_unk0x08 = 0;
	else if (frac > 0xffff) {
		m_curDotRef.m_pos += frac >> 16;
		m_curDotRef.m_unk0x08 &= 0xffff;
	}

	if (m_curDotRef.m_pos <= m_curDotRef.LinkDist())
		goto intersect;

	if (*(int*) &headDot->m_unk0x04) {
		float by = headDot->m_y;
		float bx = headDot->m_x;
		for (SPRITE* s = Hash->FirstInBox(bx - 100.0f, by - 60.0f, bx + 100.0f, by + 60.0f);
			 s; s = Hash->NextInBox()) {
			if ((int) s->m_vid->m_sprClass == 22)
				((RAIL*) s)->UnBreak(headDot);
		}
		*(int*) &headDot->m_unk0x04 = 0;
	}

	{
		if (m_curDotRef.LinkedDot()->m_noLinks < 2) {
			speed = 0.0f;
			m_curDotRef.m_pos = m_curDotRef.LinkDist();
			int cmd = m_flag & 0x7c;

			if (cmd != 92
				|| (m_noPathLinks
					&& headDot->m_links[(unsigned char) m_pathLinks[0]].m_dot
						!= m_curDotRef.LinkedDot())) {
				Stop();
			}
			else {
				SetCommandToTrain(0, (SPRITE*) 0, (R_DOT*) 0, (R_DOT*) 0);
			}
			goto intersect;
		}
	}

	{
		int stepped = ((R_POS*) &m_curDotRef)->DoStep((R_DOT*) m_commandDot, goal, this);
		MAN* flagman = Map->Flagman(Map->m_curArmy);
		if (InTrain(flagman) && !(Map->Flagman(Map->m_curArmy)->m_flag & 0x1800))
			CreatePathDots((R_DOT*) m_curDotRef.m_dot);

		R_DOT* nowDot = (R_DOT*) m_curDotRef.m_dot;
		if ((m_flag & 0x7c) != 0x6c && nowDot->m_busyEngine)
			((ENGINE*) nowDot->m_busyEngine)->InTrain(m_goal);

		R_DOT* linkedDot = nowDot->m_links[m_curDotRef.m_link].m_dot;
		ENGINE* linkedEngine = linkedDot->m_busyEngine;
		if (!linkedEngine || linkedEngine->InTrain(m_goal)) {
			int cmd = m_flag & 0x7c;
			if ((cmd == 0x6c || stepped) && stepped < 0 && m_unk0xb0 > 0.0f)
				m_unk0xb0 = -m_unk0xb0;
		}

		int commandArrived =
			(m_commandDot && m_curDotRef.LinkedDot() == m_commandDot)
			|| !stepped
			|| m_curDotRef.LinkedDot() == R_DOT::FindedDot;

		if (m_vid->m_idx != 85 && nowDot && nowDot->m_links[m_curDotRef.m_link].m_dot) {
			R_DOT* l = nowDot->m_links[m_curDotRef.m_link].m_dot;
			if ((int) l->m_unk0x14 - 4 == ((m_flag >> 11) & 3)) {
				speed = speed * 0.5f;
				Stop();
			}
		}

		if (commandArrived) {
			int cmd = m_flag & 0x7c;
			if (cmd == 0x60) {
				for (ENGINE* j = this; j; j = j->m_nextEngine) {
					if (j->m_vid->m_idx == 85) {
						j->m_unk0xec = 1;
						Stop();
					}
				}
				if ((m_flag & 0x80))
					SetCommandToTrain(0, (SPRITE*) 0, (R_DOT*) 0, (R_DOT*) 0);
				goto intersect;
			}
			if (cmd == 0x5c) {
				R_DOT* d = (R_DOT*) m_curDotRef.m_dot;
				if (m_vid->m_idx != 85 || !d || !d->m_links[m_curDotRef.m_link].m_dot
					|| (int) d->m_links[m_curDotRef.m_link].m_dot->m_unk0x14 - 4 != ((m_flag >> 11) & 3)) {
					SetCommandToTrain(0, (SPRITE*) 0, (R_DOT*) 0, (R_DOT*) 0);
					Map->ScriptRun(EvFunctionNumber[8], this, 0, 0);
				}
				goto intersect;
			}
			if (cmd == 0x64) {
				Stop();
				StartMove();
				goto intersect;
			}

			if ((cmd == 112 || cmd == 116) && ReachTheTarget() == 1)
				Stop();
		}
		else {
			int cmd = m_flag & 0x7c;
			if (cmd == 112 || cmd == 116) {
				if (ReachTheTarget() == 1)
					Stop();
			}
			else {
				R_DOT* linked = nowDot->m_links[m_curDotRef.m_link].m_dot;
				if (nowDot->m_noLinks < 2 || linked->m_noLinks < 2) {
					if (cmd != 92
						|| (m_noPathLinks
							&& nowDot->m_links[(unsigned char) m_pathLinks[0]].m_dot != linked)) {
						Stop();
					}
					else {
						SetCommandToTrain(0, (SPRITE*) 0, (R_DOT*) 0, (R_DOT*) 0);
					}
				}
				else if (linked->m_busyEngine
					&& !((ENGINE*) linked->m_busyEngine)->InTrain(m_goal)) {
					Stop();
				}
			}
		}
	}

intersect:
	MT_IntersectingProcessing(*(R_POS*) &ref, &speed);
	for (ENGINE* k = FirstEngine(); k; k = k->m_nextEngine)
		k->ClearDotBusy();
	MT_PullCoordinates(&ref, speed, m_unk0xac);
}

// FUNCTION: ALIEN 0x454530
void ENGINE::CheckPrevNextEngine()
{
	if (m_prevEngine && m_prevEngine->m_nextEngine != this) {
		if (::Error)
			MYERROR::Error(::Error, "ENGINE %i", 4,
				// STRING: ALIEN 0x484b48
				"PrevEngine->NextEngine!=this",
				(int) m_prevEngine->m_nextEngine, m_vid ? m_vid->m_idx : -1);
		m_prevEngine->m_nextEngine = this;
	}
	if (m_nextEngine && m_nextEngine->m_prevEngine != this) {
		if (::Error)
			MYERROR::Error(::Error, "ENGINE %i", 4,
				// STRING: ALIEN 0x484b28
				"NextEngine->PrevEngine!=this",
				(int) m_nextEngine->m_prevEngine, m_vid ? m_vid->m_idx : -1);
		m_nextEngine->m_prevEngine = this;
	}
}
