#include "game/player_game.h"

#include "audio/sound.h"
#include "game/const.h"
#include "game/depo.h"
#include "game/engine.h"
#include "game/input_as.h"
#include "game/map.h"
#include "game/rts_minimap.h"
#include "game/train_info.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "sprite/ex_sprite_data.h"
#include "sprite/r_map.h"
#include "ui/mouse.h"
#include "ui/ui_scaling.h"
#include "video/vid.h"
#include "video/vid_exdata.h"
#include "world/hash_map.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>






namespace
{
float HudScale()
{
	return Graph ? UI_SCALING::NormalizeDrawScale(Graph->m_uiScale * Graph->m_uiPresentationScale) : 1.0f;
}
int Class(const SPRITE* s)
{
	return s && s->m_vid ? s->m_vid->m_sprClass : -1;
}
int Army(const SPRITE* s)
{
	return s ? (s->m_flag >> 11) & 3 : -1;
}
void Assign(PTR_SPRITE& pointer, SPRITE* sprite)
{


	if (pointer.m_ptr == sprite) {
		return;
	}
	if (sprite) {
		++sprite->m_noRef;
	}
	pointer = sprite;
}
void Frame(SPRITE* s, int n)
{
	if (s && s->m_vid->m_noDir) {
		s->ChangeDirection(ANGLE((n * 256) / s->m_vid->m_noDir));
	}
}
void Text(SPRITE* s, const char* text)
{
	if (s) {
		STRING str(text);
		s->Action(120, reinterpret_cast<decomp_intptr>(&str), 0, 0);
	}
}
void Feedback(int sfx)
{
	if (Sound) {
		Sound->PlaySFX(sfx, 0, 0);
	}
}
int Icon(VID* v)
{
	int icon = 0;
	if (v && v->m_exData) {
		std::memcpy(&icon, v->m_exData->m_unk0x34, sizeof(icon));
	}
	return icon;
}
float Approx(float x, float y)
{
	x = std::fabs(x);
	y = std::fabs(y);
	return x > y ? x + y * .5f : y + x * .5f;
}
bool CaptureTarget(SPRITE* s)
{
	if (Class(s) != 24 && Class(s) != 3) {
		return false;
	}
	if (Army(s) == 2) {
		return true;
	}
	if (s->m_vid->m_idx != 162 && s->m_vid->m_idx != 165) {
		return false;
	}
	if (Hash) {
		for (int i = 0; i < Hash->m_list.m_n; ++i) {
			SPRITE* candidate = Hash->m_list.m_data[i];
			if (candidate && (candidate->m_vid->m_idx == 110 || candidate->m_vid->m_idx == 115) &&
				Approx(candidate->m_x - s->m_x, candidate->m_y - s->m_y) < 150.0f) {
				return false;
			}
		}
	}
	return true;
}
void SaveQueueTick(DEPO* d)
{
	if (d->m_unk0x47c > 0 && d->m_unk0x47c <= d->m_queueLen) {
		d->m_buildTicks[d->m_unk0x47c - 1] = d->m_unk0x50;
	}
}
void RestartQueue(DEPO* d)
{
	d->m_unk0x47c = 0;
	if (d->m_queueLen) {
		d->BuildNextUnit();
	}
	else {
		d->SetCommand(0, nullptr);
		d->ChangeAnimation(0);
	}
}

void RemoveQueue(DEPO* d, int index)
{
	if (index < 0 || index >= d->m_queueLen || d->m_queueLen > 100) {
		return;
	}
	SaveQueueTick(d);
	Map->m_player[Army(d)]->m_money += Map->GetVid(static_cast<unsigned short>(d->m_queue[index]))->GetBuildTime();
	for (int i = index; i + 1 < d->m_queueLen; ++i) {
		d->m_queue[i] = d->m_queue[i + 1];
		d->m_buildTicks[i] = d->m_buildTicks[i + 1];
		d->m_unk0x2ec[i] = d->m_unk0x2ec[i + 1];
	}
	--d->m_queueLen;
	RestartQueue(d);
}
void ReplaceQueue(DEPO* d, int index, int vid)
{
	if (index < 0 || index >= d->m_queueLen || index >= 100) {
		return;
	}
	PLAYER* owner = Map->m_player[Army(d)];
	owner->m_money += Map->GetVid(static_cast<unsigned short>(d->m_queue[index]))->GetBuildTime();
	owner->m_money -= Map->GetVid(vid)->GetBuildTime();
	d->m_queue[index] = static_cast<short>(vid);
	d->m_buildTicks[index] = Const->m_unk0x24 * Map->GetVid(vid)->GetBuildTime();
	if (d->m_unk0x47c != 1) {
		RestartQueue(d);
	}
}
void ToggleQueue(DEPO* d, int index)
{
	if (index < 0 || index >= d->m_queueLen || index >= 100) {
		return;
	}
	if (d->m_unk0x2ec[index]) {
		d->m_unk0x2ec[index] = 0;
		if (d->m_unk0x47c && index >= d->m_unk0x47c - 1) {
			return;
		}
		SaveQueueTick(d);
		RestartQueue(d);
	}
	else {
		d->m_unk0x2ec[index] = 1;
		if (index == d->m_unk0x47c - 1) {
			SaveQueueTick(d);
			RestartQueue(d);
		}
	}
}
void SwapQueue(DEPO* d, int a, int b)
{
	if (a < 0 || b < 0 || a >= d->m_queueLen || b >= d->m_queueLen || a >= 100 || b >= 100 || a == b) {
		return;
	}
	bool active = a == d->m_unk0x47c - 1 || b == d->m_unk0x47c - 1;
	if (active) {
		SaveQueueTick(d);
	}
	std::swap(d->m_queue[a], d->m_queue[b]);
	std::swap(d->m_buildTicks[a], d->m_buildTicks[b]);
	std::swap(d->m_unk0x2ec[a], d->m_unk0x2ec[b]);
	if (active) {
		RestartQueue(d);
	}
}
void BreakAt(ENGINE* engine, ENGINE* target)
{


	if (!target || target == engine || !engine->InTrain(target)) {
		return;
	}
	ENGINE* front = engine->FirstEngine();
	ENGINE* back = engine->LastEngine();
	bool cut = false;
	for (ENGINE* e = engine->m_prevEngine; e; e = e->m_prevEngine) {
		if (e == target) {
			target->m_nextEngine->m_prevEngine = nullptr;
			target->m_nextEngine = nullptr;
			cut = true;
			break;
		}
	}
	if (!cut) {
		for (ENGINE* e = engine->m_nextEngine; e; e = e->m_nextEngine) {
			if (e == target) {
				target->m_prevEngine->m_nextEngine = nullptr;
				target->m_prevEngine = nullptr;
				cut = true;
				break;
			}
		}
	}
	if (!cut) {
		return;
	}
	engine->PlaySFX(15);
	if (back != front) {
		back->Stop();
		if (back->m_speed == 0.0f) {
			back->ReverseTrain();
			ENGINE* first = back->FirstEngine();
			first->m_speed = first->m_unk0x90 & 1 ? -.01f : .01f;
		}
		if (front->m_speed == 0.0f) {
			ENGINE* first = front->FirstEngine();
			first->m_speed = first->m_unk0x90 & 1 ? -.01f : .01f;
		}
		else {
			front->ReCalcMoveParameters();
		}
	}
}
}

PLAYER_GAME::PLAYER_GAME(int control, int army) : PLAYER(control, army), m_message(14, 449, 20.0f, 5.0f, 3, 15000)
{
	RefreshUILayout();
}
PLAYER_GAME::~PLAYER_GAME()
{
	Release();
}
unsigned int PLAYER_GAME::SetCleverAttack(int on)
{




	m_controlFlags = (m_controlFlags & ~2u) | (on ? 2u : 0u);
	return m_controlFlags;
}
void PLAYER_GAME::PutMessage(const STRING& msg, float x, float y)
{
	m_message.Put(msg, x, y);
}
void PLAYER_GAME::Release()
{
	m_releasing = true;
	StateBarOff();
	m_message.Release();
	m_selection.DeleteAll();
	for (auto& slot : m_slots) {
		slot = nullptr;
	}
	PLAYER::Release();
	m_releasing = false;
}
void PLAYER_GAME::DeletePointerToSprite(SPRITE* s)
{
	m_message.DeletePointerToSprite(s);
	m_selection.Delete(s);
	bool refresh = m_flagman == s;
	if (refresh) {
		m_flagman = nullptr;
	}
	for (auto& slot : m_slots) {
		if (slot == s) {
			slot = nullptr;
			refresh = true;
		}
	}
	PLAYER::DeletePointerToSprite(s);
	if (refresh && !m_releasing) {
		SetHudMode(m_flagman ? m_hudMode : 0);
	}
}
void PLAYER_GAME::SetFlagman(SPRITE* s)
{
	if (m_flagman != s) {
		PLAYER::SetFlagman(s);


		ENGINE::PathDots.DeleteAll();
	}
	SetHudMode(s ? (Class(s) == 24 ? 2 : 1) : 0);
}
void PLAYER_GAME::AddPointerToSprite(SPRITE* s)
{
	if (!s || (Class(s) != 21 && Class(s) != 24)) {
		return;
	}
	int i = Class(s) == 24 && !m_slots[0] ? 0 : 1;
	for (; i < 10; ++i) {
		if (!m_slots[i]) {
			Assign(m_slots[i], s);
			static_cast<UNIT*>(s)->m_unk0x84 = i;
			if (!m_hudMode) {
				SetHudMode(0);
			}
			break;
		}
	}
}
void PLAYER_GAME::AssignSlot(SPRITE* s, unsigned int slot)
{
	if (slot > 9 || Class(s) != 21) {
		return;
	}
	if (s->m_vid->m_exData->m_unk0x10 == 0.0f) {
		ENGINE* e = static_cast<ENGINE*>(s)->FirstEngine();
		while (e && e->m_vid->m_exData->m_unk0x10 - e->m_vid->m_exData->m_unk0x0c <= 1.0f) {
			e = e->NextEngine();
		}
		s = e;
	}
	if (!s) {
		return;
	}
	PTR_SPRITE retained;
	Assign(retained, s);
	for (int i = 0; i < 10; ++i) {
		if (m_slots[i] == s) {
			Assign(m_slots[i], m_slots[slot].m_ptr);
			if (m_slots[i]) {
				static_cast<UNIT*>(m_slots[i].m_ptr)->m_unk0x84 = i;
			}
			break;
		}
	}
	Assign(m_slots[slot], s);
	static_cast<UNIT*>(s)->m_unk0x84 = slot;
	retained = nullptr;
	if (!m_hudMode) {
		SetHudMode(0);
	}
}
bool PLAYER_GAME::CanSelect(SPRITE* s) const
{
	return s && (Army(s) == 0 || (m_controlFlags & 1)) && (Class(s) == 21 || s->m_vid->m_idx == 102);
}
SPRITE* PLAYER_GAME::Hud(int index) const
{
	return index >= 0 && index < m_stateBar.m_n ? m_stateBar.m_data[index] : nullptr;
}
SPRITE* PLAYER_GAME::AddHud(int vid, float x, float y, float z)
{
	if (!Map->VidExists(vid)) {
		Map->m_logic.RuntimeError("Steamland HUD requires missing VID", vid);
		return nullptr;
	}
	SPRITE* s = Map->CreateSprite(Map->GetVid(vid), x, y + z, z, ANGLE(0), nullptr);
	if (!s) {
		Map->m_logic.RuntimeError("Steamland HUD could not create VID", vid);
		return nullptr;
	}
	m_stateBar.Insert(s);
	if (Map->m_curArmy != m_army) {
		for (SPRITE* child = s; child; child = child->m_child) {
			child->m_flag |= 0x10000;
		}
	}
	return s;
}
void PLAYER_GAME::RefreshUILayout()
{
	const float scale = HudScale();
	m_message.m_z = 20.0f * scale;
	m_message.m_y = 5.0f * scale;
	if (m_stateBar.m_n) {
		int mode = m_hudMode;
		StateBarOff();
		StateBarOn();
		SetHudMode(mode);
	}
}
void PLAYER_GAME::StateBarOff()
{
	if (!Map) {
		return;
	}
	for (int v : {14, 16, 17, 18, 449}) {
		if (Map->VidExists(v)) {
			Map->GetVid(v)->SetPropHide(1);
		}
	}
	Map->m_shiftX1 = Map->m_shiftY1 = 0;
	Map->m_shiftX2 = Map->m_w;
	Map->m_shiftY2 = Map->m_h;
	bool releasing = m_releasing;
	m_releasing = true;
	m_stateBar.DeleteAll();
	m_releasing = releasing;
	m_hudMode = 6;
}
void PLAYER_GAME::StateBarOn()
{
	if (!Map || !Graph) {
		return;
	}
	for (int v : {14, 16, 17, 18, 449}) {
		if (Map->VidExists(v)) {
			Map->GetVid(v)->SetPropHide(0);
		}
	}
	if (!m_stateBar.m_n) {
		float s = HudScale(), x = Graph->m_width * .5f - 225.0f * s;
		float y = Graph->m_viewYMax - (Map->GetVid(450)->m_messageLineHeight / 2 + 2) * s;
		for (int i = 1; i <= 10; ++i) {
			AddHud(4, x + (41 * i - 14) * s, y - 13 * s, 2005);
		}
		for (int i = 1; i <= 10; ++i) {
			AddHud(8, x + (41 * i - 17) * s, y - 58 * s, 2005);
		}
		AddHud(456, Graph->m_width * .5f, y, 2004);
		AddHud(498, x - 49 * s, y + 8 * s, 2003);
		AddHud(499, x - 11 * s, y + 3 * s, 2003);
		for (int i = 1; i <= 10; ++i) {
			AddHud(455, x + 41 * i * s, y, 2003);
		}
		for (int i = 1; i <= 10; ++i) {
			AddHud(463, x + (41 * i + 3) * s, y + 22 * s, 2002);
		}
		for (int i = 1; i <= 10; ++i) {
			AddHud(457, x + (41 * i + 3) * s, y + 21 * s, 2001);
		}
		for (int i = 1; i <= 10; ++i) {
			AddHud(458, x + (41 * i + 3) * s, y + 24 * s, 2001);
		}
		for (int i = 1; i <= 10; ++i) {
			AddHud(459, x + (41 * i - 17) * s, y + 22 * s, 2001);
		}
		VID* mini = Map->GetVid(464);
		VID* bar = Map->GetVid(450);
		VID* money = Map->GetVid(469);
		AddHud(
			464,
			Graph->m_viewXMax - (mini->m_unk0x2f6 / 2) * s,
			Graph->m_viewYMax - (mini->m_messageLineHeight / 2) * s,
			2001
		);
		AddHud(450, Graph->m_width * .5f, Graph->m_viewYMax - (bar->m_messageLineHeight / 2) * s, 2000);
		AddHud(
			469,
			Graph->m_viewXMax + (1 - money->m_footprintWidth * .5f) * s,
			Graph->m_viewYMin + money->m_footprintHeight * .5f * s,
			2000
		);
		AddHud(2, Graph->m_viewXMax - 95 * s, Graph->m_viewYMin + 4 * s, 2001);
		AddHud(2, Graph->m_viewXMax - 43 * s, Graph->m_viewYMin + 4 * s, 2001);
		AddHud(
			502,
			Graph->m_viewXMax + (-29 - money->m_footprintWidth * .5f) * s,
			Graph->m_viewYMin + money->m_footprintHeight * .5f * s,
			2000
		);
		AddHud(
			503,
			Graph->m_viewXMax + (31 - money->m_footprintWidth * .5f) * s,
			Graph->m_viewYMin + money->m_footprintHeight * .5f * s,
			2000
		);
		if (m_stateBar.m_n != 80) {
			StateBarOff();
			return;
		}
		Frame(Hud(22), 1);
		Hud(22)->ChangeArmy(2);
	}
	SetHudMode(Class(m_flagman) == 21 ? 1 : 0);
	Map->m_shiftX1 = Map->m_shiftY1 = 0;
	Map->m_shiftX2 = Map->m_w;
	Map->m_shiftY2 = Map->m_h + (Map->GetVid(450)->m_messageLineHeight - 2) * HudScale();
}

bool PLAYER_GAME::SetIcon(int index, VID* vid, SPRITE* sprite)
{
	if (!vid || !vid->m_idx || !Hud(index)) {
		return false;
	}
	VID* link = vid->m_linkVid;
	if (link && (!sprite || (sprite->m_child && sprite->m_child->m_vid == link)) && Icon(link)) {
		Frame(Hud(index), Icon(link));
	}
	else {
		Frame(Hud(index), Icon(vid));
	}
	return true;
}
bool PLAYER_GAME::CanBuild(VID* vid) const
{
	if (!vid || vid->m_idx <= 0) {
		return false;
	}
	int price = vid->GetBuildTime();
	if (Class(m_flagman) == 24 && m_queueIndex >= 0) {
		DEPO* depo = static_cast<DEPO*>(m_flagman.m_ptr);
		if (m_queueIndex < depo->m_queueLen) {
			price -= Map->GetVid(static_cast<unsigned short>(depo->m_queue[m_queueIndex]))->GetBuildTime();
		}
	}
	if (!m_army && m_money < price) {
		return false;
	}
	int cap = vid->m_unk0x394[0], armyCap = vid->m_unk0x394[m_army + 1];
	if (cap < 0 && armyCap < 0) {
		return true;
	}
	int queued = 0;
	if (Hash) {
		for (int i = 0; i < Hash->m_list.m_n; ++i) {
			SPRITE* sprite = Hash->m_list.m_data[i];
			if (Class(sprite) == 24 && Army(sprite) == 0) {
				DEPO* depo = static_cast<DEPO*>(sprite);
				for (int j = 0; j < std::min(depo->m_queueLen, 10); ++j) {
					if (depo->m_queue[j] == vid->m_idx) {
						++queued;
					}
				}
			}
		}
	}
	int total =
		vid->m_entitiesNumber[0] + vid->m_entitiesNumber[1] + vid->m_entitiesNumber[2] + vid->m_entitiesNumber[3];


	return (armyCap < 0 || queued + vid->m_entitiesNumber[m_army] < cap) && (cap < 0 || queued + total < cap);
}

void PLAYER_GAME::SetHudMode(int mode)
{
	if (m_releasing) {
		return;
	}
	if (!m_stateBar.m_n && mode == 2) {
		mode = 18;
		StateBarOn();
	}
	if ((m_hudMode & 16) && mode != 2 && mode != 5) {
		StateBarOff();
	}
	if (m_stateBar.m_n < 80) {
		return;
	}
	if (m_hudMode != mode) {
		Feedback(109);
	}
	Hud(22)->ChangeAnimation((mode & ~16) ? 0 : 6);
	m_hudMode = mode | (m_hudMode & 16);
	Hud(20)->ChangeCoor(Hud(20)->m_x - 1000, Hud(20)->m_y, Hud(20)->m_z);
	for (int i = 0; i < m_stateBar.m_n; ++i) {
		if (i < 20 || i >= 33) {
			Frame(Hud(i), Hud(i)->m_vid->m_noDir - 1);
		}
	}
	for (int i = 0; i < 10; ++i) {
		Frame(Hud(23 + i), 0);
		Hud(23 + i)->ChangeArmy(2);
		Text(Hud(10 + i), "");
	}
	bool releasing = m_releasing;
	m_releasing = true;
	while (m_stateBar.m_n > 80) {
		m_stateBar.DeleteSpriteNumber(m_stateBar.m_n - 1);
	}
	m_releasing = releasing;
	mode &= ~16;
	if (mode == 0) {
		for (int i = 0; i < 10; ++i) {
			if (m_slots[i] && SetIcon(23 + i, m_slots[i]->m_vid, m_slots[i])) {
				Hud(23 + i)->ChangeArmy(0);
			}
		}
		return;
	}
	if (mode != 2 && mode != 5) {
		return;
	}
	if (Class(m_flagman) != 24) {
		SetHudMode(0);
		return;
	}
	DEPO* depo = static_cast<DEPO*>(m_flagman.m_ptr);
	float scale = HudScale(), x = Hud(23)->m_x - Map->m_shiftX;
	float y = Hud(23)->m_y - Hud(23)->m_z - Map->m_shiftY - 41 * scale;
	static constexpr int buildVids[] = {5, 10, 20, 25, 80, 85, 45, 30, 35, 82, 97, 90, 75, 62};
	for (int i = 0; i < 10; ++i) {
		SPRITE* button = AddHud(455, x + 41 * i * scale, y, 2003);
		if (!button) {
			return;
		}
		button->ChangeArmy(1);
		if (mode == 5) {
			if (i < 5) {
				Frame(button, 3 + i);
				button->ChangeArmy(3);
			}
		}
		else {
			VID* vid =
				Map->GetVid(depo->m_exData && i < depo->m_exData->m_list.m_n ? depo->m_exData->m_list.m_data[i] : 0);
			if (SetIcon(80 + i, vid)) {
				button->ChangeArmy(CanBuild(vid) ? 3 : 2);
				char cost[32];
				std::snprintf(cost, sizeof(cost), "%d", vid->GetBuildTime());
				Text(Hud(10 + i), cost);
			}
		}
	}
	if (mode == 2) {
		for (int i = 0; i < 10; ++i) {
			SPRITE* label = AddHud(8, x + (41 * i + 18) * scale, y + 10 * scale, 2004);
			if (!label) {
				return;
			}
			label->Action(95, 2, 0, 0);
			int vid = depo->m_exData && i < depo->m_exData->m_list.m_n ? depo->m_exData->m_list.m_data[i] : 0;
			for (int k = 0; k < 14; ++k) {
				if (buildVids[k] == vid) {
					char key[8];
					if (k < 12) {
						std::snprintf(key, sizeof(key), "F%d", k + 1);
					}
					else {
						std::snprintf(key, sizeof(key), "%c", k == 12 ? '[' : ']');
					}
					Text(label, key);
					break;
				}
			}
		}
	}
	AddHud(497, Graph->m_width * .5f, y, 2002);
}

void PLAYER_GAME::UpdateHud()
{
	m_message.Shift();
	if (m_stateBar.m_n < 80 || Map->m_curArmy != m_army) {
		return;
	}
	DrawRtsMinimap(Hud(73));
	SPRITE* mini = Hud(73);
	float scale = HudScale();
	float mx = mini->m_x - Map->m_shiftX - (mini->m_vid->m_unk0x2f6 / 2 - 8) * scale;
	float my = mini->m_y - mini->m_z - Map->m_shiftY - (mini->m_vid->m_messageLineHeight / 2 - 10) * scale;
	bool inside = Map->m_input.m_x >= mx && Map->m_input.m_y >= my &&
				  Map->m_input.m_x < mx + (mini->m_vid->m_unk0x2f6 - 14) * scale &&
				  Map->m_input.m_y < my + (mini->m_vid->m_messageLineHeight - 13) * scale;
	Map->m_flag = (Map->m_flag & ~0x40000u) | (inside ? 0u : 0x40000u);
	char text[40];
	std::snprintf(text, sizeof(text), "%d", m_money);
	Text(Hud(77), text);
	std::snprintf(text, sizeof(text), "%.1f", Map->m_speed);
	Text(Hud(76), text);
	int mode = m_hudMode & ~16;
	auto highlight = [&](int i) {
		if (Hud(23 + i)) {
			Hud(20)->ChangeCoor(Hud(23 + i)->m_x, Hud(20)->m_y, Hud(20)->m_z);
		}
	};
	auto unit = [&](int i, SPRITE* sprite, int key) {
		if (!sprite) {
			Frame(Hud(23 + i), 0);
			Hud(23 + i)->ChangeArmy(2);
			for (int base : {43, 53, 63}) {
				Frame(Hud(base + i), Hud(base + i)->m_vid->m_noDir - 1);
			}
			return;
		}
		if (m_flagman == sprite) {
			highlight(i);
		}
		SetIcon(23 + i, sprite->m_vid, sprite);
		Hud(23 + i)->ChangeArmy(0);
		Hud(43 + i)->ChangeDirection(ANGLE(static_cast<unsigned char>(-5 - sprite->PercentHp())));
		UNIT* u = static_cast<UNIT*>(sprite);
		VID* ammoVid = sprite->m_vid;
		if (ammoVid->m_linkVid && ammoVid->m_linkVid->m_weaponVid && ammoVid->m_linkVid->m_weapon) {
			ammoVid = ammoVid->m_linkVid;
		}
		int maxAmmo = ammoVid->GetMaxAmmo();
		int ammo = maxAmmo ? static_cast<int>(255LL * (u->m_ammo / 64) / maxAmmo) : 0;
		Hud(53 + i)->ChangeDirection(ANGLE(static_cast<unsigned char>(-5 - ammo)));
		int behavior = Class(sprite) == 24 ? ((~u->m_unk0x8c & 2) | 4) >> 1
										   : ((~u->m_unk0x8c >> (sprite->m_vid->m_exData->m_unk0x10 != 0.0f)) & 1);
		Frame(Hud(63 + i), behavior);
		Frame(Hud(i), key);
	};
	if (mode == 0) {
		for (int i = 0; i < 10; ++i) {
			if (m_slots[i]) {
				unit(i, m_slots[i], 49 + i);
			}
		}
	}
	else if (mode == 1) {
		if (Class(m_flagman) != 21) {
			SetHudMode(0);
			return;
		}
		ENGINE* engine = static_cast<ENGINE*>(m_flagman.m_ptr)->FirstEngine();
		for (int i = 0; i < 10; ++i) {
			unit(i, engine, engine && engine->m_unk0x84 > 0 ? engine->m_unk0x84 + 49 : Hud(i)->m_vid->m_noDir - 1);
			if (engine) {
				engine = engine->NextEngine();
			}
		}
	}
	else if (mode == 2 || mode == 5) {
		if (Class(m_flagman) != 24) {
			SetHudMode(0);
			return;
		}
		DEPO* d = static_cast<DEPO*>(m_flagman.m_ptr);
		for (int i = 0; i < 10; ++i) {
			VID* vid = Map->GetVid(i < d->m_queueLen ? static_cast<unsigned short>(d->m_queue[i]) : 0);
			if (!SetIcon(23 + i, vid)) {
				Frame(Hud(23 + i), 2);
			}
			else {
				Frame(Hud(63 + i), d->m_unk0x2ec[i] ? 3 : 4);
			}
			Hud(23 + i)->ChangeArmy(1);
			int progress = 255;
			if (i < d->m_queueLen && vid->GetBuildTime() > 0 && Const->m_unk0x24 > 0) {
				int ticks = i == d->m_unk0x47c - 1 ? d->m_unk0x50 : d->m_buildTicks[i];
				progress = static_cast<int>(255LL * ticks / vid->GetBuildTime() / Const->m_unk0x24);
			}
			Frame(Hud(33 + i), static_cast<int>(Hud(33 + i)->m_vid->m_noDir) * progress / 256);
		}
		int selected = m_queueIndex >= 0 ? m_queueIndex : d->m_queueLen;
		if (selected < 10) {
			highlight(selected);
		}
		Hud(20)->ChangeArmy(3);
	}
}

void PLAYER_GAME::Keyboard(INPUT_AS* input)
{
	SPRITE* selected = m_flagman;
	unsigned int key = input->m_key;
	if ((Map->m_shiftFlag & 4) && (!selected || input->m_x < 5 || input->m_y < 5 || input->m_x > Graph->m_width - 5 ||
								   input->m_y > Graph->m_height - 5)) {
		Map->m_shiftFlag = 33;
	}
	if (Const->m_debugMode && key == 'E') {
		m_controlFlags ^= 1;
	}
	if (key >= 0x3100 && key <= 0x3900 && (input->m_button & 0x1000)) {
		AssignSlot(selected, (key >> 8) - 48);
	}
	if (Class(selected) == 21 && key == ' ') {
		static_cast<ENGINE*>(selected)->SetCommandToTrain(0, nullptr, nullptr, nullptr);
	}
	if (selected && (key == 'A' || key == 'a')) {
		if (Class(selected) == 21) {
			static_cast<UNIT*>(selected)->m_unk0x8c ^= selected->m_vid->m_exData->m_unk0x10 == 0.0f ? 1 : 2;
		}
		else if (Class(selected) == 24) {
			static_cast<UNIT*>(selected)->m_unk0x8c ^= 2;
		}
	}
	if (selected && (key == 'C' || key == 'c')) {
		Map->SetShiftCoor(selected->m_x, selected->m_y - selected->m_z, 2);
	}
	if (key == 'L' || key == 'l') {
		Map->m_shiftFlag = (Map->m_shiftFlag & 4) ? 1 : 4;
	}
	if (Class(selected) == 21 && (key == 'R' || key == 'r')) {

		SPRITE* dock =
			Map->FindNearestSprite(MAP::MakeVidQuery(104) | 0x50000, selected->m_x, selected->m_y, 15000, nullptr);
		if (!dock) {
			dock =
				Map->FindNearestSprite(MAP::MakeVidQuery(105) | 0x50000, selected->m_x, selected->m_y, 15000, nullptr);
		}
		if (dock) {
			static_cast<ENGINE*>(selected)->Move(dock->m_x, dock->m_y, dock->m_z, 0, 0);
		}
	}
	if (Class(selected) == 21 && key == 1) {
		UNIT* u = static_cast<UNIT*>(selected);
		int bit = u->m_vid->m_exData->m_unk0x10 - u->m_vid->m_exData->m_unk0x0c <= 1 ? 1 : 2;
		bool on = !(u->m_unk0x8c & bit);
		for (ENGINE* e = static_cast<ENGINE*>(selected)->FirstEngine(); e; e = e->NextEngine()) {
			bit = e->m_vid->m_exData->m_unk0x10 - e->m_vid->m_exData->m_unk0x0c <= 1 ? 1 : 2;
			e->m_unk0x8c = (e->m_unk0x8c & ~bit) | (on ? bit : 0);
		}
	}
	if (Class(selected) == 21 && (key == 'M' || key == 'm')) {
		ENGINE* mine = static_cast<ENGINE*>(selected)->FirstEngine();
		while (mine && mine->m_vid->m_idx != 85) {
			mine = mine->NextEngine();
		}
		if (mine) {
			ENGINE* tail = static_cast<ENGINE*>(selected)->LastEngine();
			tail->SetCommandToTrain(24, nullptr, tail->m_curDotRef.m_dot, nullptr);
		}
		else {
			Feedback(114);
		}
	}
	int slot = key >= '0' && key <= '9' ? key - '0' : key == 0x2400 ? 0 : -1;
	if (slot >= 0 && m_slots[slot]) {
		if (m_flagman == m_slots[slot]) {
			Map->SetShiftCoor(m_slots[slot]->m_x, m_slots[slot]->m_y - m_slots[slot]->m_z, 2);
		}
		SetFlagman(m_slots[slot]);
		Feedback(109);
	}
}

void PLAYER_GAME::Toolbar(int index, unsigned int key)
{
	int mode = m_hudMode & ~16;
	if (mode == 0 && index > 0 && index <= 10) {
		SetFlagman(m_slots[index - 1]);
		Feedback(109);
		return;
	}
	if (mode == 1) {
		if (!index) {
			SetHudMode(0);
		}
		else if (index > 0 && index <= 10 && Class(m_flagman) == 21) {
			ENGINE* e = static_cast<ENGINE*>(m_flagman.m_ptr)->FirstEngine();
			for (int i = 1; e && i < index; ++i) {
				e = e->NextEngine();
			}
			if (e) {
				if (m_flagman == e) {
					Map->SetShiftCoor(e->m_x, e->m_y - e->m_z, 2);
				}
				SetFlagman(e);
				Feedback(109);
			}
		}
		return;
	}
	if ((mode != 2 && mode != 5) || Class(m_flagman) != 24) {
		return;
	}
	DEPO* d = static_cast<DEPO*>(m_flagman.m_ptr);
	if (!index || index == 100) {
		if (mode == 5) {
			m_queueIndex = -1;
			SetHudMode(2);
		}
		else {
			SetHudMode(0);
		}
		return;
	}
	if (index > 100 && index <= 110) {
		int i = index - 101;
		if (i < d->m_queueLen && d->m_queue[i]) {
			m_queueIndex = i;
			SetHudMode(5);
		}
		return;
	}
	if (mode == 2 && index > 0 && index <= 10 && Hud(79 + index) && Army(Hud(79 + index)) != 2) {
		int vid = d->m_exData && index <= d->m_exData->m_list.m_n ? d->m_exData->m_list.m_data[index - 1] : 0;
		if (m_queueIndex < 0) {
			d->Action(35, vid, 0, 0);
		}
		else {
			ReplaceQueue(d, m_queueIndex, vid);
			SetHudMode(5);
		}
		Feedback(24);
		SetHudMode(2);
		return;
	}
	if (mode != 5) {
		return;
	}
	if (index == 1 || key == 'N' || key == 'n' || key == 0x2e00) {
		RemoveQueue(d, m_queueIndex);
		if (m_queueIndex >= d->m_queueLen) {
			m_queueIndex = d->m_queueLen - 1;
		}
		SetHudMode(m_queueIndex < 0 ? 2 : 5);
	}
	else if (index == 2 || key == 'M' || key == 'm') {
		ToggleQueue(d, m_queueIndex);
		SetHudMode(5);
	}
	else if (index == 3 || key == '<' || key == ',') {
		if (m_queueIndex > 0) {
			SwapQueue(d, m_queueIndex - 1, m_queueIndex);
			--m_queueIndex;
		}
	}
	else if (index == 4 || key == '>' || key == '.') {
		if (m_queueIndex + 1 < d->m_queueLen) {
			SwapQueue(d, m_queueIndex, m_queueIndex + 1);
			++m_queueIndex;
		}
	}
	else if (index == 5 || key == '/') {
		SetHudMode(2);
	}
}

void PLAYER_GAME::ContextOrders(INPUT_AS* input, int base)
{
	if (Map->m_curArmy != m_army || !Mouse) {
		return;
	}
	SPRITE* selected = m_flagman;
	if (Class(selected) != 21) {
		m_selection.DeleteAll();
		Mouse->ChangeAnimation(base);
		return;
	}
	ENGINE* engine = static_cast<ENGINE*>(selected);
	const bool click = (input->m_button & 0x8000) != 0;
	if (click) {
		engine->ResetActionStack();
	}
	TRAIN_INFO info(engine);
	bool movable = info.Acceleration() > 7;
	bool armed = info.m_unk0x28 > 0;
	VID* vid = engine->m_vid;
	float range = info.m_maxWeaponRange;
	if (vid->m_idx != 45 && vid->m_exData->m_unk0x10 == 0.0f) {
		SPRITE* child = engine->m_child;
		bool linkedWeapon =
			child && child->m_vid == vid->m_linkVid && child->m_vid->m_weaponVid && child->m_vid->m_weapon;
		armed = engine->m_ammo / 64 && linkedWeapon;
		range = linkedWeapon && vid->m_exData->m_unk0x18 == 0.0f ? child->m_vid->m_exData->m_unk0x18
																 : vid->m_exData->m_unk0x18;
	}
	SPRITE* target = m_underCursor;
	int cursor = 12;
	auto action = [&](int command) {
		if (click) {
			engine->Action(command, reinterpret_cast<decomp_intptr>(target), 0, 0);
		}
	};
	auto move = [&](int allowedCursor) {
		cursor = movable ? allowedCursor : 12;
		if (movable && click) {
			engine->Move(input->m_worldX, input->m_worldY, 0, 0, 1);
		}
	};
	auto attackCursor = [&](float x, float y, float z) {
		return Approx(Approx(x - engine->m_x, y - engine->m_y), z - engine->m_z) < range ? 5 : movable ? 6 : 12;
	};
	if (input->m_button & 0x800) {
		if (Class(target) == 21 && !engine->InTrain(target) && Army(target) != 0) {
			cursor = movable || vid->m_idx == 35 ? 3 : 12;
			if (cursor == 3) {
				action(151);
			}
		}
		else if (vid->m_idx == 85) {
			cursor = !movable ? 12 : engine->m_ammo / 64 ? 16 : 1;
			if (click && cursor == 16) {
				engine->SetCommandToTrain(
					24,
					nullptr,
					RailMap.GetNearestDot_xy(input->m_worldX, input->m_worldY),
					nullptr
				);
			}
		}
		else if (!armed) {
			cursor = 1;
		}
		else {
			float z = Map->GetGroundZScr(input->m_worldX, input->m_worldY);
			cursor = attackCursor(input->m_worldX, input->m_worldY + z, z);
			if (click) {
				engine->Action(37, static_cast<int>(input->m_worldX), static_cast<int>(input->m_worldY), 0);
			}
		}
	}
	else if (Class(target) == 21 && (engine->m_prevEngine || engine->m_nextEngine) && engine->InTrain(target)) {
		cursor = 11;
		if (click) {
			SPRITE* cut = target;
			if (target == engine) {
				cut = engine->m_prevEngine ? engine->m_prevEngine : engine->m_nextEngine;
				if (engine->m_prevEngine && engine->m_nextEngine &&
					Approx(input->m_worldX - engine->m_prevEngine->m_x, input->m_worldY - engine->m_prevEngine->m_y) >=
						Approx(
							input->m_worldX - engine->m_nextEngine->m_x,
							input->m_worldY - engine->m_nextEngine->m_y
						)) {
					cut = engine->m_nextEngine;
				}
			}
			if (cut) {
				BreakAt(engine, static_cast<ENGINE*>(cut));
			}
		}
	}
	else if (
		Class(target) == 21 && !engine->InTrain(target) &&
		(engine->HaveArmy(Army(engine)) ||
		 (((input->m_button & 0x1000) || (vid->m_exData->m_unk0x10 - vid->m_exData->m_unk0x0c > 1 &&
										  !engine->m_prevEngine && !engine->m_nextEngine)) &&
		  engine->CanLinkWithEngine(static_cast<ENGINE*>(target))))
	) {
		cursor = movable ? 10 : 12;
		if (movable) {
			action(150);
		}
	}
	else if (target && CaptureTarget(target)) {
		move(15);
	}
	else if (
		target && target->m_vid->m_idx == 104 && Army(target) != 1 &&
		(info.m_unk0x3c < 100 || info.m_unk0x20 < info.m_unk0x24)
	) {
		move(4);
	}
	else if (vid->m_idx == 85 && target && target->m_vid->m_idx == 86) {
		cursor = movable ? 17 : 12;
		if (movable && click) {
			engine->Move(target->m_x, target->m_y, target->m_z, 0, 0);
		}
	}
	else if (input->m_button & 0x1000) {
		cursor = movable ? 13 : 12;
		if (movable && click) {
			engine->SetCommandToTrain(25, nullptr, RailMap.GetNearestDot_xy(input->m_worldX, input->m_worldY), nullptr);
		}
	}
	else if (
		target && (Army(target) == 1 || (Army(target) == 2 && Class(target) == 21)) && target->m_vid->m_unk0x0c > 2
	) {
		if (armed) {
			cursor = attackCursor(target->m_x, target->m_y, target->m_z);
			action(32);
		}
		else if (info.m_unk0x28 <= 0 && movable && Class(target) == 21 && target->m_vid->m_idx != 351) {
			cursor = 3;
			action(151);
		}
		else {
			cursor = 1;
		}
	}
	else {
		move(2);
	}
	Mouse->ChangeAnimation(base + cursor);
	if (click) {
		engine->PlaySFX(cursor == 1 ? 114 : cursor == 12 ? 107 : 153);
	}
}

void PLAYER_GAME::Control(INPUT_AS* input)
{
	if (!input || !Mouse || !Mouse->m_hardware || (m_control != 1 && m_control != 3)) {
		return;
	}
	int index = -1;
	const int hoverVid = Map->m_menu.NVidUnderCursor();
	SPRITE* hover = Map->m_menu.m_underCursor;
	int mode = m_hudMode & ~16;
	if ((Map->m_menu.m_state & 1) && hoverVid == 455 && hover && hover->m_dir) {
		int first = (mode == 2 || mode == 5) ? 80 : 23;
		if (Hud(first)) {
			int slot = static_cast<int>((hover->m_x - Hud(first)->m_x) / (41 * HudScale()) + .5f);
			if (slot >= 0 && slot < 10) {
				index = slot + 1;
			}
			if ((mode == 2 || mode == 5) && hover->m_z == Hud(23)->m_z &&
				std::fabs(hover->m_y - Hud(23)->m_y) < HudScale()) {
				index = slot + 101;
			}
		}
	}
	if (mode == 2 && Class(m_flagman) == 24) {
		static constexpr int vids[] = {5, 10, 20, 25, 80, 85, 45, 30, 35, 82, 97, 90};
		int buildVid = input->m_key >= 0x7000 && input->m_key <= 0x7b00 ? vids[(input->m_key >> 8) - 0x70]
					   : input->m_key == '['                            ? 75
					   : input->m_key == ']'                            ? 62
																		: 0;
		if (buildVid && m_flagman->m_exData) {
			for (int i = 0; i < m_flagman->m_exData->m_list.m_n; ++i) {
				if (m_flagman->m_exData->m_list.m_data[i] == buildVid) {
					index = i + 1;
					break;
				}
			}
		}
	}
	if (input->m_key == 27 || ((Map->m_menu.m_state & 1) && hoverVid == 499)) {
		index = 0;
	}
	Keyboard(input);
	Toolbar(index, input->m_key);
	if (!m_stateBar.m_n || !hover || hoverVid == 464) {
		float savedX = input->m_worldX, savedY = input->m_worldY;
		if (hoverVid == 464 && Hud(73)) {
			SPRITE* mini = Hud(73);
			float scale = HudScale();
			float x = mini->m_x - Map->m_shiftX - (mini->m_vid->m_unk0x2f6 / 2 - 8) * scale;
			float y = mini->m_y - mini->m_z - Map->m_shiftY - (mini->m_vid->m_messageLineHeight / 2 - 10) * scale;
			float w = (mini->m_vid->m_unk0x2f6 - 14) * scale, h = (mini->m_vid->m_messageLineHeight - 13) * scale;
			if (w > 0 && h > 0) {


				if ((Map->m_menu.m_state & 2) && INPUT_AS::secondKey1 == 2) {
					input->m_button |= 0x8004;
				}
				if ((Map->m_menu.m_state & 1) && INPUT_AS::secondKey1 == 1) {
					input->m_button |= 0x8001;
				}
				input->m_worldX = (input->m_x - x) * Map->m_w / w;
				input->m_worldY = (input->m_y - y) * Map->m_h / h;
				if (Map->m_menu.m_state & 1) {
					Map->SetShiftCoor(input->m_worldX, input->m_worldY, 2);
					if (Map->m_shiftFlag & 4) {
						Map->m_shiftFlag = 33;
					}
				}
			}
		}
		SPRITE* under = Map->GetSpriteScr(0x400000, input->m_worldX, input->m_worldY);
		if (!under) {
			under = Map->GetSpriteScr(10518528, input->m_worldX, input->m_worldY);
		}
		Assign(m_underCursor, under);
		if (m_underCursor && (m_underCursor->m_vid->m_flag & 0x20000) && Army(m_underCursor) == 1) {
			m_underCursor = nullptr;
		}
		ContextOrders(input, hoverVid == 464 ? 18 : 0);
		if ((input->m_button & 0x4000) || ((input->m_button & 0x8000) && !m_flagman)) {
			SPRITE* target = m_underCursor;
			if (target && target != m_flagman && (Army(target) != 1 || target->m_vid->m_idx == 409) &&
				!(Army(target) == 2 && Class(target) == 21)) {
				target->Action(9, 0, 0, 0);
			}
			SetFlagman(CanSelect(target) ? target : nullptr);
		}
		input->m_worldX = savedX;
		input->m_worldY = savedY;
	}
	else {
		m_underCursor = nullptr;
		Mouse->ChangeAnimation(0);
	}
	if (m_underCursor && ((m_underCursor->m_vid->m_unk0x0c & 2) || m_underCursor->m_vid->m_idx == 162)) {
		m_underCursor = nullptr;
	}
	UpdateHud();
}
