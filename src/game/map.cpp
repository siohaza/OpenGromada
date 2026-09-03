#include "game/map.h"

#include "audio/sound.h"
#include "game/const.h"
#include "game/data_version.h"
#include "game/game_descriptor.h"
#include "game/zs1_commands.h"
#include "game/engine.h"
#include "game/filedata.h"
#include "game/gametime.h"
#include "game/player_arcade.h"
#include "game/region.h"
#include "game/settings.h"
#include "game/terrain_camera.h"
#include "game/train_info.h"
#include "game/viewport_math.h"
#include "gfx/gamma.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "gfx/picture_font.h"
#include "gfx/picture_makevid.h"
#include "platform/gamepad.h"
#include "platform/paths.h"
#include "platform/portable_config.h"
#include "platform/render.h"
#include "platform/store.h"
#include "platform/timing.h"
#include "sprite/ex_sprite_data.h"
#include "sprite/r_map.h"
#include "ui/mouse.h"
#include "util/crc32.h"
#include "util/game_random.h"
#include "util/myerror.h"
#include "util/packed.h"
#include "util/profile.h"
#include "util/registry.h"
#include "util/string.h"
#include "video/vid.h"

#include <string>
#include "video/vid_exdata.h"
#include "video/vid_font.h"
#include "video/vid_hardware.h"
#include "video/vid_hardware_z.h"
#include "video/vid_light.h"
#include "video/vid_software16.h"
#include "world/hash_map.h"

#include <SDL3/SDL.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FILE* FOpen(char** p_name, const char* p_mode);

// Script file handles are one-based indices.
enum {
	SCRIPT_FILE_MAX = 16
};
static FILE* s_scriptFiles[SCRIPT_FILE_MAX];

static int ScriptFileOpen(FILE* p_file)
{
	if (!p_file) {
		return 0;
	}
	for (int i = 0; i < SCRIPT_FILE_MAX; ++i) {
		if (!s_scriptFiles[i]) {
			s_scriptFiles[i] = p_file;
			return i + 1;
		}
	}
	fclose(p_file);
	return 0;
}

static FILE* ScriptFile(int p_handle)
{
	if (p_handle < 1 || p_handle > SCRIPT_FILE_MAX) {
		return 0;
	}
	return s_scriptFiles[p_handle - 1];
}

static void ScriptFileClose(int p_handle)
{
	FILE* file = ScriptFile(p_handle);
	if (!file) {
		return;
	}
	fclose(file);
	s_scriptFiles[p_handle - 1] = 0;
}

static void SynchronizeGrantedWeaponHud(MAP* p_map, SPRITE* p_sprite, decomp_intptr p_item)
{
	if (!p_map || p_sprite != p_map->Flagman(0) || p_item < 260 || p_item > 269) {
		return;
	}

	const unsigned int slot = (unsigned int) (p_item - 260);
	VID* weaponVid = p_map->Vid(710);
	VID* ammoVid = p_map->Vid(745);
	SPRITE* weapon = 0;
	for (int i = 0; i < p_map->m_menu.m_n; ++i) {
		SPRITE* root = p_map->m_menu.m_data[i];
		if (root && root->m_vid == weaponVid && weaponVid->RealDirection(ANGLE(root->m_dir)) == slot) {
			weapon = root;
			break;
		}
	}
	if (!weapon) {
		return;
	}

	// Pair VID745 by its fixed eight-pixel row offset.
	const float scale = weapon->UIDrawScale();
	const float targetY = weapon->m_y - weapon->m_z + 8.0f * scale;
	float bestDistance = FLT_MAX;
	SPRITE* ammo = 0;
	for (int i = 0; i < p_map->m_menu.m_n; ++i) {
		SPRITE* root = p_map->m_menu.m_data[i];
		if (!root || root->m_vid != ammoVid || root->UIAnchorX() != weapon->UIAnchorX() ||
			root->UIAnchorY() != weapon->UIAnchorY() || fabsf(root->UIDrawScale() - scale) > 0.001f) {
			continue;
		}
		float distance = fabsf(root->m_y - root->m_z - targetY);
		if (distance < bestDistance) {
			bestDistance = distance;
			ammo = root;
		}
	}
	weapon->Action(98, 0, 0, 0);
	if (ammo && bestDistance < 18.0f * scale) {
		ammo->Action(98, 0, 0, 0);
	}
}

static SPRITE* FindStateBarAmmoRoot(MAP* p_map, int p_nvid, int p_x, int p_y, int p_z)
{
	if (!p_map || p_nvid != 4) {
		return 0;
	}

	VID* ammoVid = p_map->Vid(745);
	if (ammoVid == EmptyVid) {
		return 0;
	}
	for (int i = 0; i < p_map->m_menu.m_n; ++i) {
		SPRITE* root = p_map->m_menu.m_data[i];
		if (!root || root->m_vid != ammoVid || !root->HasUIScriptLayout()) {
			continue;
		}
		int expectedX = (int) ((float) (int) root->ScriptX() - p_map->m_shiftX) + 61;
		int expectedY = (int) ((float) ((int) root->ScriptY() + 3) - p_map->m_shiftY);
		int expectedZ = (int) root->m_z + 1;
		if (p_x == expectedX && p_y == expectedY && p_z == expectedZ) {
			return root;
		}
	}
	return 0;
}

// GLOBAL: ALIEN 0x490600
unsigned int PrevRealCurrentTime;

// GLOBAL: ALIEN 0x47f794
char CurrentSav[] = "current.sav";

// GLOBAL: ALIEN 0x49072c
static unsigned int oldabsCurrentTime;
// GLOBAL: ALIEN 0x490730
static unsigned int oldCurrentTime;

// GLOBAL: ALIEN 0x490738
MAP* Map;

// GLOBAL: ALIEN 0x49073c
static unsigned int prev_second_time;

// FUNCTION: ALIEN 0x405a90
VID* MAP::Vid(int p_idx) const
{
	if (!ValidVidIndex(p_idx) || p_idx >= m_noVid) {
		return EmptyVid;
	}
	VID* result = m_vids[p_idx];
	if (!result) {
		return EmptyVid;
	}
	return result;
}

// FUNCTION: ALIEN 0x40bc20
void MAP::SetShiftCoor(float p_x, float p_y, int p_flag)
{
	float w = Graph->m_width;
	float x = p_x - w * 0.5f;
	float h = Graph->m_height;
	float y = p_y - h * 0.5f;
	if (!(p_flag & 0x10000000)) {
		float xMin = Graph->m_viewXMin;
		xMin = m_shiftX1 - xMin;
		float yMin = Graph->m_viewYMin;
		yMin = m_shiftY1 - yMin;
		float xMax = Graph->m_viewXMax;
		xMax = m_shiftX2 - xMax;
		float yMax = Graph->m_viewYMax;
		yMax = m_shiftY2 - yMax;
		x = VIEWPORT_MATH::ClampCameraAxis(x, xMin, xMax);
		y = VIEWPORT_MATH::ClampCameraAxis(y, yMin, yMax);
	}
	if (m_terrainCamera) {
		float focusX = p_x;
		float focusY = p_y;
		MAN* flagman = Flagman((int) m_curArmy);
		if (flagman) {
			focusX = flagman->m_x;
			focusY = flagman->m_y - flagman->m_z;
		}
		float safeX;
		float safeY;
		if (m_terrainCamera->Project(x, y, focusX, focusY, 640, 480, &safeX, &safeY) ||
			m_terrainCamera->Project(x, y, focusX, focusY, (int) w, (int) h, &safeX, &safeY) ||
			m_terrainCamera->ProjectSafe(x, y, &safeX, &safeY)) {
			x = safeX;
			y = safeY;
		}
	}
	if (m_shiftX != x || y != m_shiftY) {
		if (p_flag == 2) {
			((GRAPH_CORE*) Graph)->Effect(2, (int) p_x, (int) p_y, 0);
		}
		else {
			float dx = x - m_shiftX;
			m_shiftX = x;
			float dy = y - m_shiftY;
			m_shiftY = y;
			MENU* menu = &m_menu;
			int i = 0;
			if (i < menu->m_n) {
				do {
					SPRITE* spr = (SPRITE*) menu->m_data[i];
					spr->ChangeCoor(spr->X() + dx, spr->Y() + dy, spr->Z());
					++i;
				} while (i < menu->m_n);
			}
			Mouse->ChangeCoor(Mouse->X() + dx, Mouse->Y() + dy, Mouse->Z());
			m_input.m_worldX = m_input.m_worldX + dx;
			m_input.m_worldY = m_input.m_worldY + dy;
		}
	}
}

// GLOBAL: ALIEN 0x490620
static float g_shiftSpeedX;

// GLOBAL: ALIEN 0x490624
static float g_shiftSpeedY;

// FUNCTION: ALIEN 0x40bed0
void MAP::ControlShiftCoor()
{

	if (m_shiftFlag & 0x21) {
		if (m_input.m_x <= 5.0f && (m_shiftFlag & 1)) {
			if (-Const->m_unk0x00 < g_shiftSpeedX) {
				g_shiftSpeedX -= 0.04f;
			}
		}
		else if (Graph->m_width - 5.0f <= m_input.m_x && (m_shiftFlag & 1)) {
			if (g_shiftSpeedX < Const->m_unk0x00) {
				float v = g_shiftSpeedX;
				g_shiftSpeedX = v + 0.04f;
			}
		}
		else if ((m_input.m_button & 0x80) && (m_shiftFlag & 0x20)) {
			if (-Const->m_unk0x00 < g_shiftSpeedX) {
				g_shiftSpeedX -= 0.04f;
			}
		}
		else if ((m_input.m_button & 0x100) && (m_shiftFlag & 0x20)) {
			if (g_shiftSpeedX < Const->m_unk0x00) {
				float v = g_shiftSpeedX;
				g_shiftSpeedX = v + 0.04f;
			}
		}
		else {
			g_shiftSpeedX = 0.0f;
		}
		if (m_input.m_y <= 5.0f && (m_shiftFlag & 1)) {
			if (-Const->m_unk0x04 < g_shiftSpeedY) {
				g_shiftSpeedY -= 0.03f;
			}
		}
		else if (Graph->m_height - 5.0f <= m_input.m_y && (m_shiftFlag & 1)) {
			if (g_shiftSpeedY < Const->m_unk0x04) {
				float v = g_shiftSpeedY;
				g_shiftSpeedY = v + 0.03f;
			}
		}
		else if ((m_input.m_button & 0x400) && (m_shiftFlag & 0x20)) {
			if (-Const->m_unk0x04 < g_shiftSpeedY) {
				g_shiftSpeedY -= 0.03f;
			}
		}
		else if ((m_input.m_button & 0x200) && (m_shiftFlag & 0x20)) {
			if (g_shiftSpeedY < Const->m_unk0x04) {
				float v = g_shiftSpeedY;
				g_shiftSpeedY = v + 0.03f;
			}
		}
		else {
			g_shiftSpeedY = 0.0f;
		}
	}
	else {
		g_shiftSpeedY = 0.0f;
		g_shiftSpeedX = 0.0f;
	}

	if (Flagman(m_curArmy) && (m_shiftFlag & 4) && g_shiftSpeedX == 0.0f && g_shiftSpeedY == 0.0f) {
		g_shiftSpeedX = (Flagman(m_curArmy)->m_x - Map->m_shiftX - 0.5f * Graph->m_width) * 0.001f;
		MAN* man = Flagman(m_curArmy);
		g_shiftSpeedY = (man->m_y - man->m_z - Map->m_shiftY - 0.5f * Graph->m_height) * 0.001f;
	}
	else if (Flagman(m_curArmy) && (m_shiftFlag & 8) && g_shiftSpeedX == 0.0f && g_shiftSpeedY == 0.0f) {
		float cx = VIEWPORT_MATH::ClampDirectionalAimAxis(m_input.m_x, Graph->m_width, 640.0f);
		float cy = VIEWPORT_MATH::ClampDirectionalAimAxis(m_input.m_y, Graph->m_height, 480.0f);
		float midX = (Flagman(m_curArmy)->m_x - Map->m_shiftX + cx) * 0.5f;
		MAN* man = Flagman(m_curArmy);
		float midY = (man->m_y - man->m_z - Map->m_shiftY + cy) * 0.5f;
		g_shiftSpeedX = (0.5f * Graph->m_width - midX) * -0.004f;
		g_shiftSpeedY = (0.5f * Graph->m_height - midY) * -0.004f;
	}
	else if (Flagman(m_curArmy) && (m_shiftFlag & 0x10) && g_shiftSpeedX == 0.0f && g_shiftSpeedY == 0.0f) {
		MAN* a = Flagman(m_curArmy);
		MAN* b = Flagman(m_curArmy);
		SetShiftCoor(b->m_x - Map->m_shiftX + m_shiftX, a->m_y - a->m_z - Map->m_shiftY + m_shiftY, 0);
		return;
	}

	SetShiftCoor(
		(int) (g_shiftSpeedX * (float) (CurrentTime - PrevCurrentTime)) + 0.5f * Graph->m_width + m_shiftX,
		(int) (g_shiftSpeedY * (float) (CurrentTime - PrevCurrentTime)) + 0.5f * Graph->m_height + m_shiftY,
		0
	);
}

// FUNCTION: ALIEN 0x40e600
void MAP::SetFlagman(int p_army, SPRITE* p_sprite) const
{
	m_player[p_army & 3]->SetFlagman(p_sprite);
}

// FUNCTION: ALIEN 0x40e620
MAN* MAP::Flagman(int p_idx) const
{
	return (MAN*) (SPRITE*) m_player[p_idx & 3]->m_flagman;
}

void MAP::GetAudioListener(float* p_x, float* p_y) const
{
	MAN* listener = m_player[m_curArmy & 3] ? (MAN*) (SPRITE*) m_player[m_curArmy & 3]->m_flagman : 0;
	if (listener) {
		*p_x = listener->m_x;
		*p_y = listener->m_y - listener->m_z;
	}
	else {
		*p_x = m_shiftX + Graph->m_width * 0.5f;
		*p_y = m_shiftY + Graph->m_height * 0.5f;
	}
}

// FUNCTION: ALIEN 0x40e640
SPRITE* MAP::SpriteUnderCursor() const
{
	return m_player[m_curArmy & 3]->m_underCursor;
}

// FUNCTION: ALIEN 0x40e660
int MAP::DemoTact()
{
	int result = 1;
	if (m_flag & 0x200) {
		RESOURCE& demo = m_resource;
		int time;
		demo.Read(&time, 4);
		if (time != -1 && !m_input.m_key && !(m_input.m_button & 5)) {
			m_input.Load(&demo);
			if (time - oldCurrentTime > 0x47) {
				oldCurrentTime = time - 71;
			}
			if (time - oldCurrentTime > Platform_Ticks() - oldabsCurrentTime) {
				// The original spun here. Sleeping gives the same pacing
				// without burning a core through demo playback.
				while (time - oldCurrentTime >= Platform_Ticks() - oldabsCurrentTime) {
					Platform_Sleep(1);
				}
			}
			else {
				result = CurrentTime - time <= 0x14;
			}
			CurrentTime = time;
			oldCurrentTime = time;
			oldabsCurrentTime = Platform_Ticks();
		}
		else {
			STRING next;
			demo.GoNext(0x4F4D4544);
			int size = m_resource.m_resSize;
			FILE* f = demo.m_file;
			demo.m_state = 2;
			fseek(f, size, 1);
			next.Read_res(&demo);
			if (!strcmp(next.m_str, empty_str)) {
				m_quit = 1;
			}
			else {
				demo.Close();
				Mouse->Enable();
				m_flag = (m_flag & 0xFFFFFDBF) | 0x40;
				m_scriptName = next;
			}
			return 0;
		}
	}
	if (m_flag & 0x100) {
		m_resource.Write(&CurrentTime, 4);
		m_input.Save(&m_resource);
	}
	return result;
}

// STUB: ALIEN 0x40f210
int MAP::StartTact()
{
	unsigned int delta;
	do {
		delta = Platform_Ticks() - RealCurrentTime;
	} while (!delta);
	PrevRealCurrentTime = RealCurrentTime;
	RealCurrentTime = Platform_Ticks();
	PrevCurrentTime = CurrentTime;
	if (delta > 0x47) {
		delta = 71;
	}
	CurrentTime += (int) (delta * m_speed);
	++m_fpsCnt;

	if (RealCurrentTime - prev_second_time >= 1000) {
		prev_second_time = RealCurrentTime;
		m_fps = m_fpsCnt;
		m_fpsCnt = 0;
	}
	m_input.Tact();

	if (!(CurrentTime & 3)) {
		LIST_SPRITE* layer = m_layers;
		int layers = 17;
		do {
			int n = layer->m_n;
			for (int hole = 0; hole < n; ++hole) {
				if (!layer->m_data[hole]) {
					int gap = 1;
					for (int i = hole + 1; i < layer->m_n; ++i) {
						if (!layer->m_data[i]) {
							++gap;
						}
						else {
							layer->m_data[i - gap] = layer->m_data[i];
						}
					}
					int left = layer->m_n - gap;
					if (left <= 0) {
						SPRITE** data = layer->m_data;
						layer->m_max = 0;
						layer->m_n = 0;
						if (data) {
							operator delete(data);
						}
						layer->m_data = 0;
					}
					else if (left < layer->m_n) {
						layer->m_n = left;
					}
					break;
				}
			}
			++layer;
			--layers;
		} while (layers);
	}

	if (m_flag & 0x40) {
		m_flag &= 0xffffffbf;
		Load(m_scriptName);
	}

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		SDL_ConvertEventToRenderCoordinates(Platform_RenderRenderer(), &event);
		ProcessEvent(event);
	}
	m_input.ApplyGamepad(delta);

	{
		static const char* schedule = 0;
		static int scheduleRead;
		if (!scheduleRead) {
			scheduleRead = 1;
			schedule = SDL_getenv("ALIEN_AUTOKEYS");
		}
		if (schedule) {
			static unsigned int startTime;
			if (!startTime) {
				startTime = Platform_Ticks();
			}
			static int fired[32];
			unsigned int elapsed = Platform_Ticks() - startTime;
			const char* p = schedule;
			for (int i = 0; i < 32 && p && *p; ++i) {
				int when = atoi(p);
				const char* colon = strchr(p, ':');
				if (!colon) {
					break;
				}
				int key = atoi(colon + 1);
				if (!fired[i] && (int) elapsed >= when) {
					fired[i] = 1;
					m_input.m_key = key;
					MYERROR::Log(::Error, "AUTOKEY: %i at %ums", key, elapsed);
				}
				const char* comma = strchr(colon, ',');
				p = comma ? comma + 1 : 0;
			}
		}
	}

	Platform_StorePump();
	return m_quit;
}

// FUNCTION: ALIEN 0x40f430
int MAP::ScriptRun(int p_fn, const SPRITE* p_a, const SPRITE* p_b, int p_c)
{
	if (m_flag & 0x80000) {
		return 0;
	}
	return m_logic.CallFunction(p_fn, p_a, p_b, p_c);
}

// FUNCTION: ALIEN 0x40f9a0
SPRITE* MAP::NextSprite(int p_layer, int* p_iter) const
{
	int i = *p_iter - 1;
	*p_iter = i;
	if (i >= 0) {
		SPRITE** const* data = &m_layers[p_layer].m_data;
		while (i >= 0) {
			if ((*data)[i]) {
				return (*data)[*p_iter];
			}
			*p_iter = --i;
		}
	}
	return 0;
}

VID* MAP::ReadVid(STREAM* p_stream) const
{
	int idx = -1;
	if (p_stream->Read(&idx, sizeof(idx))) {
		return 0;
	}
	if (!ValidVidIndex(idx) || idx >= m_noVid) {
		return 0;
	}
	return m_vids[idx];
}

void MAP::WriteVid(STREAM* p_stream, const VID* p_vid) const
{
	const int noVid = -1;
	p_stream->Write(p_vid ? &p_vid->m_idx : &noVid, 4);
}

// FUNCTION: ALIEN 0x40f9e0
int MAP::PauseOn()
{
	int result = 0x10;
	if (!(m_flag & result)) {
		PauseOldClock = CurrentTime;
	}
	m_flag |= result;
	return result;
}

// FUNCTION: ALIEN 0x40fa90
int MAP::RemoveSpriteFromLayer(SPRITE* p_sprite)
{
	int layer = p_sprite->m_vid->m_layer;
	int idx = m_layers[layer].m_n - 1;
	while (idx >= 0) {
		if (m_layers[layer].m_data[idx] == p_sprite) {
			m_layers[layer].m_data[idx] = 0;
			return idx;
		}
		--idx;
	}
	return idx;
}

// FUNCTION: ALIEN 0x40fad0
void MAP::InsertSpriteToLayer(SPRITE* p_sprite)
{
	m_layers[p_sprite->m_vid->m_layer].Insert(p_sprite);
	int refs = --p_sprite->m_noRef;
	if (refs > 0) {
		return;
	}
	if (refs < 0) {
		MYERROR::Error(
			::Error,
			"SPRITE %i",
			4,
			// STRING: ALIEN 0x48290c
			"noRef at Release",
			refs,
			p_sprite->m_vid ? p_sprite->m_vid->m_idx : -1
		);
		return;
	}
	if (p_sprite) {
		p_sprite->ScalarDeletingDestructor(1);
	}
}

enum {
	U_TERRAIN = 0x1,
	U_OBJECT = 0x2,
	U_UNIT = 0x4,
	U_AVIA = 0x8,
	U_MENU = 0x10,
	U_RAILWAY = 0x20,
	U_REGION = 0x40,
	U_CANNON = 0x200,
	U_SPRITE = 0x400,
};

inline static int IsSpriteCorrectForGetSprite(const SPRITE* p_sprite, int p_query);

// FUNCTION: ALIEN 0x40fbb0
SPRITE* MAP::GetSprite(int p_type, float p_x, float p_y, SPRITE* p_prev)
{
	bool menuQuery = false;
	int queryVid = VidFromQueryFor(p_type);
	if (IsVidQueryFor(p_type) && VidExist(queryVid)) {
		int spriteClass = m_vids[queryVid]->m_sprClass;
		menuQuery = spriteClass == 10 || spriteClass == 19;
	}

	if (menuQuery) {
		VID* requested = m_vids[queryVid];
		if (requested->m_entitiesNumber[0] + requested->m_entitiesNumber[1] + requested->m_entitiesNumber[2] +
				requested->m_entitiesNumber[3] ==
			0) {
			return 0;
		}
		int type = requested->m_unk0x0c;
		int army = p_type & 0xf0000;
		if (!army) {
			army = 0xf0000;
		}
		float minimum = p_prev ? SPRITE::NearDistanceTo(p_x - p_prev->m_x, p_y - p_prev->m_y) : -1.0f;
		float best = FLT_MAX;
		SPRITE* result = 0;
		for (int i = m_menu.m_n - 1; i >= 0; --i) {
			SPRITE* candidate = m_menu.m_data[i];
			if (!candidate || candidate->m_parent || !(type & candidate->m_vid->m_unk0x0c) ||
				!((0x10000u << ((candidate->m_flag >> 11) & 3)) & army) ||
				!IsSpriteCorrectForGetSprite(candidate, p_type)) {
				continue;
			}
			VID* vid = candidate->m_vid;
			if (!UI_SCALING::HitTestCentered(
					candidate->m_x,
					candidate->m_y,
					vid->m_unk0x384,
					vid->m_unk0x388,
					candidate->UIDrawScale(),
					p_x,
					p_y
				)) {
				continue;
			}
			float distance = SPRITE::NearDistanceTo(p_x - candidate->m_x, p_y - candidate->m_y);
			if (distance > minimum && distance < best) {
				best = distance;
				result = candidate;
			}
		}
		return result;
	}

	SPRITE* result = FindNearestSprite(p_type, p_x, p_y, 300.0f, p_prev);
	if (!result) {
		return 0;
	}
	VID* vid = result->m_vid;
	return UI_SCALING::HitTestCentered(
			   result->m_x,
			   result->m_y,
			   vid->m_unk0x384,
			   vid->m_unk0x388,
			   result->UIDrawScale(),
			   p_x,
			   p_y
		   )
			   ? result
			   : 0;
}

inline static int RegionIsInsideScr(const REGION* p_region, float p_x, float p_y)
{
	float x = p_region->m_x;
	float halfW = p_region->m_w * 0.5f;
	if (x - halfW <= p_x && p_x <= halfW + x) {
		float top = p_region->m_y - p_region->m_z;
		float halfH = p_region->m_h * 0.5f;
		if (top - p_region->m_vid->m_unk0x24 - halfH < p_y && p_y < halfH + top) {
			return 1;
		}
	}
	return 0;
}

inline static int SpriteIsInsideScr(const SPRITE* p_sprite, float p_x, float p_y)
{
	return p_sprite->IsInside(p_x, p_y);
}

inline static int IsSpriteCorrectForGetSprite(const SPRITE* p_sprite, int p_query)
{
	if ((p_query & 0x80000000) && 0 != (p_sprite->m_flag & 0x7c)) {
		return 0;
	}
	if (!MAP::IsVidQueryFor(p_query) && (p_query & 0x1000) &&
		(int) p_sprite->m_vid->m_sprClass != (p_query & 0x7ff)) {
		return 0;
	}
	if (MAP::IsVidQueryFor(p_query) && p_sprite->m_vid->m_idx != MAP::VidFromQueryFor(p_query)) {
		return 0;
	}
	return 1;
}

inline static SPRITE* LastHashSprite(LIST_SPRITE& p_list, int* p_iter)
{
	unsigned int n = p_list.m_n;
	if (n) {
		*p_iter = n - 1;
		return p_list.m_data[*p_iter];
	}
	return 0;
}

inline static SPRITE* LastMenuSprite(MENU& p_menu, int* p_iter)
{
	unsigned int n = p_menu.m_n;
	if (n) {
		*p_iter = n - 1;
		return p_menu.m_data[*p_iter];
	}
	return 0;
}

inline static SPRITE* NextMenuSprite(MENU& p_menu, int* p_iter)
{
	if (*p_iter > p_menu.m_n) {
		*p_iter = p_menu.m_n;
	}
	if (--*p_iter >= 0) {
		return p_menu.m_data[*p_iter];
	}
	return 0;
}

// STUB: ALIEN 0x40fc40
SPRITE* MAP::GetSpriteScr(int p_type, float p_scrX, float p_scrY)
{
	SPRITE* result;
	int army = p_type & 0xf0000;
	if (!army) {
		army = 0xf0000;
	}

	int type;
	int iter = 0;
	if (IsVidQueryFor(p_type)) {
		int idx = VidFromQueryFor(p_type);
		VID* vid = VidExist(idx) ? m_vids[idx] : EmptyVid;
		if (vid->m_entitiesNumber[0] + vid->m_entitiesNumber[1] + vid->m_entitiesNumber[2] + vid->m_entitiesNumber[3] ==
			0) {
			return 0;
		}
		VID* v = VidExist(idx) ? m_vids[idx] : EmptyVid;
		if (v->m_flag & 0x40) {
			p_type |= 0x8000;
		}
		type = (VidExist(idx) ? m_vids[idx] : EmptyVid)->m_unk0x0c;
	}
	else {
		type = (p_type >> 20) & 0x67f;
		if (!type) {
			type = 0x67f;
		}
	}
	result = 0;

	if (p_type & 0x8000) { // GETSPRITE_HASH
		for (SPRITE* sprite = Hash->FirstInBox(
				 p_scrX - 300.0f,
				 p_scrY - 300.0f,
				 p_scrX + 300.0f,
				 GetGroundZ_ff(p_scrX, p_scrY) + p_scrY + 300.0f
			 );
			 sprite;
			 sprite = Hash->NextInBox()) {
			if (!sprite->m_parent && (type & sprite->m_vid->m_unk0x0c) &&
				((0x10000u << ((sprite->m_flag >> 11) & 3)) & army) && IsSpriteCorrectForGetSprite(sprite, p_type) &&
				SpriteIsInsideScr(sprite, p_scrX, p_scrY)) {
				if (!result || sprite->m_vid->m_footprintWidth < result->m_vid->m_footprintWidth ||
					sprite->m_vid->m_footprintHeight < result->m_vid->m_footprintHeight) {
					result = sprite;
				}
			}
		}
		return result;
	}

	if ((type & (U_UNIT | U_AVIA)) &&
		!(type & (U_TERRAIN | U_OBJECT | U_MENU | U_RAILWAY | U_REGION | U_CANNON | U_SPRITE))) {
		for (SPRITE* sprite = (SPRITE*) Hash->m_list.LastIterate(&iter); sprite;
			 sprite = (SPRITE*) Hash->m_list.NextIterate(&iter)) {
			if ((type & sprite->m_vid->m_unk0x0c) && ((0x10000u << ((sprite->m_flag >> 11) & 3)) & army) &&
				IsSpriteCorrectForGetSprite(sprite, p_type) && SpriteIsInsideScr(sprite, p_scrX, p_scrY)) {
				if (!result || sprite->m_vid->m_footprintWidth < result->m_vid->m_footprintWidth ||
					sprite->m_vid->m_footprintHeight < result->m_vid->m_footprintHeight) {
					result = sprite;
				}
			}
		}
		return result;
	}

	if (IsVidQueryFor(p_type) && ((int) GetVid(VidFromQueryFor(p_type))->m_sprClass == 10 ||
								  (Game_IsZS1() && (int) GetVid(VidFromQueryFor(p_type))->m_sprClass == 19))) {
		for (SPRITE* sprite = LastMenuSprite(m_menu, &iter); sprite; sprite = NextMenuSprite(m_menu, &iter)) {
			if (!sprite->m_parent && (type & sprite->m_vid->m_unk0x0c) &&
				((0x10000u << ((sprite->m_flag >> 11) & 3)) & army) && IsSpriteCorrectForGetSprite(sprite, p_type) &&
				SpriteIsInsideScr(sprite, p_scrX, p_scrY)) {
				if (!result || sprite->m_vid->m_footprintWidth < result->m_vid->m_footprintWidth ||
					sprite->m_vid->m_footprintHeight < result->m_vid->m_footprintHeight) {
					result = sprite;
				}
			}
		}
		return result;
	}

	if (type & U_REGION) {
		int startLayer = IsVidQueryFor(p_type) ? GetVid(VidFromQueryFor(p_type))->m_layer : 0;
		int endLayer = IsVidQueryFor(p_type) ? GetVid(VidFromQueryFor(p_type))->m_layer + 1 : LayerWalkCount();
		for (int layer = startLayer; layer < endLayer; ++layer) {
			iter = m_layers[layer].m_n;
			SPRITE* sprite = NextSprite(layer, &iter);
			while (sprite) {
				if (!sprite->m_parent && (type & sprite->m_vid->m_unk0x0c) &&
					((0x10000u << ((sprite->m_flag >> 11) & 3)) & army) &&
					IsSpriteCorrectForGetSprite(sprite, p_type) &&
					((int) sprite->m_vid->m_sprClass == 23 ? RegionIsInsideScr((const REGION*) sprite, p_scrX, p_scrY)
														   : SpriteIsInsideScr(sprite, p_scrX, p_scrY))) {
					if (!result || sprite->m_vid->m_footprintWidth < result->m_vid->m_footprintWidth ||
						sprite->m_vid->m_footprintHeight < result->m_vid->m_footprintHeight) {
						result = sprite;
					}
				}
				sprite = 0;
				for (--iter; iter >= 0; --iter) {
					if (m_layers[layer].m_data[iter]) {
						sprite = m_layers[layer].m_data[iter];
						break;
					}
				}
			}
		}
	}
	else {
		int startLayer = IsVidQueryFor(p_type) ? GetVid(VidFromQueryFor(p_type))->m_layer : 0;
		int endLayer = IsVidQueryFor(p_type) ? GetVid(VidFromQueryFor(p_type))->m_layer + 1 : LayerWalkCount();
		for (int layer = startLayer; layer < endLayer; ++layer) {
			iter = m_layers[layer].m_n;
			SPRITE* sprite = NextSprite(layer, &iter);
			while (sprite) {
				if (!sprite->m_parent && (type & sprite->m_vid->m_unk0x0c) &&
					((0x10000u << ((sprite->m_flag >> 11) & 3)) & army) &&
					IsSpriteCorrectForGetSprite(sprite, p_type) && SpriteIsInsideScr(sprite, p_scrX, p_scrY)) {
					if (!result || sprite->m_vid->m_footprintWidth < result->m_vid->m_footprintWidth ||
						sprite->m_vid->m_footprintHeight < result->m_vid->m_footprintHeight) {
						result = sprite;
					}
				}
				sprite = 0;
				for (--iter; iter >= 0; --iter) {
					if (m_layers[layer].m_data[iter]) {
						sprite = m_layers[layer].m_data[iter];
						break;
					}
				}
			}
		}
	}
	return result;
}

// FUNCTION: ALIEN 0x410640
int MAP::VidExist(int p_idx) const
{
	return ValidVidIndex(p_idx) && p_idx < m_noVid && m_vids[p_idx];
}

inline static float NearDistanceInline(float p_x, float p_y)
{
	float x = (float) fabs(p_x);
	float y = (float) fabs(p_y);
	if (x > y) {
		return x + y * 0.5f;
	}
	return x * 0.5f + y;
}

// STUB: ALIEN 0x410670
SPRITE* MAP::FindNearestSprite(int p_type, float p_x, float p_y, float p_radius, SPRITE* p_prev)
{
	float best = p_radius;
	SPRITE* result;
	int army = p_type & 0xf0000;
	int type;
	int iter;
	float minDistance = p_prev ? SPRITE::NearDistanceTo(p_x - p_prev->m_x, p_y - p_prev->m_y) : -1.0;
	if (!army) {
		army = 0xf0000;
	}

	if (IsVidQueryFor(p_type)) {
		int idx = VidFromQueryFor(p_type);
		VID* vid = GetVid(idx);
		if (vid->m_entitiesNumber[0] + vid->m_entitiesNumber[1] + vid->m_entitiesNumber[2] + vid->m_entitiesNumber[3] ==
			0) {
			return 0;
		}
		if (GetVid(idx)->m_flag & 0x40) {
			p_type |= 0x8000;
		}
		type = GetVid(idx)->m_unk0x0c;
	}
	else {
		type = (p_type >> 20) & 0x67f;
		if (!type) {
			type = 0x67f;
		}
	}
	result = 0;

	if (p_type & 0x8000) { // GETSPRITE_HASH
		for (SPRITE* sprite = Hash->FirstInBox(p_x - p_radius, p_y - p_radius, p_x + p_radius, p_y + p_radius); sprite;
			 sprite = Hash->NextInBox()) {
			if (!sprite->m_parent && (type & sprite->m_vid->m_unk0x0c) &&
				((0x10000u << ((sprite->m_flag >> 11) & 3)) & army) && IsSpriteCorrectForGetSprite(sprite, p_type)) {
				float distance = SPRITE::NearDistanceTo(p_x - sprite->m_x, p_y - sprite->m_y);
				float comparedDistance = distance;
				if (comparedDistance < best && distance > minDistance) {
					best = distance;
					result = sprite;
				}
			}
		}
		return result;
	}

	if ((type & (U_UNIT | U_AVIA)) &&
		!(type & (U_TERRAIN | U_OBJECT | U_MENU | U_RAILWAY | U_REGION | U_CANNON | U_SPRITE))) {
		for (SPRITE* sprite = LastHashSprite(Hash->m_list, &iter); sprite;
			 sprite = (SPRITE*) Hash->m_list.NextIterate(&iter)) {
			if (!sprite->m_parent && (type & sprite->m_vid->m_unk0x0c) &&
				((0x10000u << ((sprite->m_flag >> 11) & 3)) & army) && IsSpriteCorrectForGetSprite(sprite, p_type)) {
				float distance = NearDistanceInline(p_x - sprite->m_x, p_y - sprite->m_y);
				float comparedDistance = distance;
				if (comparedDistance < best && distance > minDistance) {
					best = distance;
					result = sprite;
				}
			}
		}
		return result;
	}

	if (IsVidQueryFor(p_type) && ((int) GetVid(VidFromQueryFor(p_type))->m_sprClass == 10
							   || (int) GetVid(VidFromQueryFor(p_type))->m_sprClass == 19)) {
		for (SPRITE* sprite = LastMenuSprite(m_menu, &iter); sprite; sprite = NextMenuSprite(m_menu, &iter)) {
			if (!sprite->m_parent && (type & sprite->m_vid->m_unk0x0c) &&
				((0x10000u << ((sprite->m_flag >> 11) & 3)) & army) && IsSpriteCorrectForGetSprite(sprite, p_type)) {
				float distance = NearDistanceInline(p_x - sprite->m_x, p_y - sprite->m_y);
				float comparedDistance = distance;
				if (comparedDistance < best && distance > minDistance) {
					best = distance;
					result = sprite;
				}
			}
		}
		return result;
	}

	int startLayer = IsVidQueryFor(p_type) ? GetVid(VidFromQueryFor(p_type))->m_layer : 0;
	int endLayer = IsVidQueryFor(p_type) ? GetVid(VidFromQueryFor(p_type))->m_layer + 1 : LayerWalkCount();
	for (int layer = startLayer; layer < endLayer; ++layer) {
		iter = m_layers[layer].m_n;
		SPRITE* sprite = NextSprite(layer, &iter);
		while (sprite) {
			if (!sprite->m_parent && (type & sprite->m_vid->m_unk0x0c) &&
				((0x10000u << ((sprite->m_flag >> 11) & 3)) & army) && IsSpriteCorrectForGetSprite(sprite, p_type)) {
				float distance = NearDistanceInline(p_x - sprite->m_x, p_y - sprite->m_y);
				float comparedDistance = distance;
				if (comparedDistance < best && distance > minDistance) {
					best = distance;
					result = sprite;
				}
			}
			sprite = 0;
			for (--iter; iter >= 0; --iter) {
				if (m_layers[layer].m_data[iter]) {
					sprite = m_layers[layer].m_data[iter];
					break;
				}
			}
		}
	}
	return result;
}

// FUNCTION: ALIEN 0x410d70
int MAP::ResetGroundZ()
{
	if (m_groundz) {
		operator delete(m_groundz);
	}
	if (m_tempGroundz) {
		operator delete(m_tempGroundz);
	}
	int w = (int) (m_w + 7.0f);
	int h = (int) (m_h + 7.0f);
	m_groundW = w / 8;
	m_groundH = h / 8;
	m_groundz = (short*) operator new(2 * m_groundW * m_groundH);
	m_tempGroundz = (short*) operator new(2 * m_groundW * m_groundH);
	memset(m_groundz, 0, 2 * m_groundW * m_groundH);
	memset(m_tempGroundz, 0, 2 * m_groundW * m_groundH);
	return 0;
}

// STUB: ALIEN 0x410f20
float MAP::GetGroundZScr(float p_x, float p_y)
{
	float x;
	if (p_x < 0.0f) {
		x = 0.0f;
	}
	else {
		x = (p_x >= m_w ? m_w - 1.0f : p_x) * 0.125f;
	}
	float y;
	if (p_y < 0.0f) {
		y = 0.0f;
	}
	else {
		y = (p_y >= m_h ? m_h - 1.0f : p_y) * 0.125f;
	}
	int row = (int) y;
	int probe;
	if (row + 32 >= m_groundH) {
		probe = m_groundH - 1;
	}
	else {
		probe = row + 32;
	}
	int idx = probe;
	probe = m_groundW * idx + (int) x;
	while (idx >= row) {
		if (m_groundz[probe] >= 8 * idx - 8 * row || m_tempGroundz[probe] >= 8 * idx - 8 * row) {
			return (float) (8 * (idx - row));
		}
		--idx;
		probe -= m_groundW;
	}
	return 0.0f;
}

// FUNCTION: ALIEN 0x411510
void MAP::LoadVid(RESOURCE* p_res)
{
	int idx = 0;
	unsigned int start = Platform_Ticks();
	if (!m_weapon) {
		LoadWeapon(p_res);
		int noWeapon = m_noWeapon;
		if (noWeapon) {
			MYERROR::Log(
				::Error,
				// STRING: ALIEN 0x4829c0
				"LoadWeapon::No=%-5i             sizeof(WEAPON)=%-4i",
				noWeapon,
				GameDesc->m_weapRecordBytes
			);
		}
	}
	if (p_res->GoBegin(0x204a424f)) {
		if (::Error) {
			MYERROR::Error(
				::Error,
				// STRING: ALIEN 0x4824d8
				"MAP",
				11,
				// STRING: ALIEN 0x4829b4
				"load 'VID'",
				0
			);
		}
		return;
	}
	int hadVids = m_noVid != 0;
	do {
		idx = -1;
		int readError = p_res->m_subSize < (int) sizeof(idx) ? 1 : p_res->Read(&idx, sizeof(idx));
		if (readError || !ValidVidIndex(idx)) {
			if (::Error) {
				MYERROR::Error(
					::Error,
					"MAP",
					4,
					// STRING: ALIEN 0x4829a4
					"nvid > MAX_VID",
					idx
				);
			}
			continue;
		}
		VID* old = m_vids[idx];
		if (old) {
			old->ScalarDeletingDestructor(1);
			m_vids[idx] = 0;
			if (::Error) {
				MYERROR::Error(
					::Error,
					"MAP",
					5,
					// STRING: ALIEN 0x48298c
					"this VID already loaded",
					idx
				);
			}
		}
		m_vids[idx] = CreateVid(p_res, idx);
		if (m_vids[idx]) {
			if (idx >= m_noVid) {
				m_noVid = idx + 1;
			}
			if (hadVids) {
				m_vids[idx]->m_pixelFlag16 |= 0x200;
			}
			VID* vid = m_vids[idx];
			int weapon = vid->m_weaponIdx;
			if (weapon < m_noWeapon) {
				vid->m_exData = (VID_EXDATA*) m_weapon + weapon;
			}
			else {
				vid->Error(
					10,
					// STRING: ALIEN 0x482978
					"nWeapon > noWeapon",
					vid->m_weaponIdx
				);
				m_vids[idx]->m_exData = (VID_EXDATA*) m_weapon;
			}
			Graph->DrawLoadBar(m_vids[0]);
		}
	} while (!p_res->GoNextSub(0x204a424f));
	int maxX = 0;
	int maxY = 0;
	idx = 0;
	if (m_noVid > 0) {
		do {
			if (m_vids[idx]) {
				m_vids[idx]->SetChildAndLink();
				VID* vid = m_vids[idx];
				if (vid->m_unk0x2f6 > maxX) {
					maxX = vid->m_unk0x2f6;
				}
				if (vid->m_messageLineHeight > maxY) {
					maxY = vid->m_messageLineHeight;
				}
			}
			++idx;
		} while (idx < m_noVid);
	}
	MYERROR::Log(
		::Error,
		// STRING: ALIEN 0x482920
		"LoadVid::No   =%-15i   sizeof(VID)   =%-5i    load time     =%ims   MaxSizeX,Y=%i,%i",
		m_noVid,
		1156,
		Platform_Ticks() - start,
		maxX,
		maxY
	);
}

// FUNCTION: ALIEN 0x411790
void MAP::ExchangeVid(VID* p_vid1, VID* p_vid2)
{
	VID* v1 = p_vid1;
	if (p_vid1 && p_vid2 && p_vid1 != p_vid2 && ValidVidIndex(p_vid1->m_idx) && ValidVidIndex(p_vid2->m_idx) &&
		p_vid1->m_idx < m_noVid && p_vid2->m_idx < m_noVid) {
		int i = 0;
		if (m_noVid > 0) {
			VID** vids = m_vids;
			do {
				VID* v = *vids;
				if (v) {
					VID* link = v->m_linkVid;
					if (link == v1) {
						v->m_linkVid = p_vid2;
					}
					else if (link == p_vid2) {
						v->m_linkVid = v1;
					}
					for (int child = 0; child < 17; ++child) {
						VID* c = v->m_aniChildVid[child];
						if (c == v1) {
							v->m_aniChildVid[child] = p_vid2;
						}
						else if (c == p_vid2) {
							v->m_aniChildVid[child] = v1;
						}
					}
				}
				++i;
				++vids;
			} while (i < m_noVid);
		}
		m_vids[v1->m_idx] = p_vid2;
		m_vids[p_vid2->m_idx] = v1;
		VID* mirror = v1->m_mirror;
		v1->m_mirror = p_vid2->m_mirror;
		p_vid2->m_mirror = mirror;
		int idx = v1->m_idx;
		v1->m_idx = p_vid2->m_idx;
		p_vid2->m_idx = idx;
		for (int n = 0; n < 20; ++n) {
			int t = v1->m_unk0x408[n];
			v1->m_unk0x408[n] = p_vid2->m_unk0x408[n];
			p_vid2->m_unk0x408[n] = t;
		}
	}
}

// FUNCTION: ALIEN 0x411880
int MAP::LoadWeapon(RESOURCE* p_res)
{
	if (m_weapon) {
		operator delete(m_weapon);
	}
	m_weapon = 0;
	int count = p_res->GetNoSubRes(0x50414557);
	if (count <= 0) {
		return p_res->Load(0x50414557, &m_weapon, sizeof(VID_EXDATA));
	}
	m_noWeapon = count;
	VID_EXDATA* weapons = (VID_EXDATA*) operator new(sizeof(VID_EXDATA) * count);
	memset(weapons, 0, sizeof(VID_EXDATA) * count);
	m_weapon = weapons;
	p_res->GoBegin(0x50414557);

	if (p_res->m_subSize != GameDesc->m_weapRecordBytes) {
		MYERROR::LogExit(
			::Error,
			"LoadWeapon: WEAP record is %i bytes, expected %i for %s data",
			p_res->m_subSize,
			GameDesc->m_weapRecordBytes,
			GameDesc->m_className
		);
	}

	for (int i = 0; i < count; ++i) {
		VID_EXDATA* w = &weapons[i];
		if (Game_IsZS1()) {
			p_res->Read(&w->m_unk0x00, 4);
			p_res->Read(&w->m_unk0x04, 4);
			p_res->Read(&w->m_unk0x08, 4);
			p_res->Read(&w->m_unk0x0c, 4);
			p_res->Read(&w->m_unk0x10, 4);
			p_res->Read(&w->m_detectPeriod, 4);
			p_res->Read(&w->m_unk0x14, 4);
			p_res->Read(&w->m_unk0x18, 4);
			p_res->Read(&w->m_unk0x1c, 4);
			p_res->Read(&w->m_fireInVolley, 4);
			p_res->Read(&w->m_maxAmmo, 4);
			p_res->Read(&w->m_unk0x20, 4);
			p_res->Read(&w->m_reloadTimeInVolley, 4);
			p_res->Read(&w->m_buildTime, 4);
			p_res->Read(&w->m_army, 4);
			p_res->Read(&w->m_defaultBehavior, 4);
			p_res->Read(w->m_unk0x34, 4);
			FILE* file = p_res->m_file;
			p_res->m_state = 2;
			fseek(file, 16, SEEK_CUR);
			p_res->Read(&w->m_unk0x38, 4);
			p_res->Read(&w->m_unk0x3c, 4);
			p_res->Read(&w->m_unk0x40, 4);
			p_res->Read(w->m_unk0x44, 0x220);
		}
		else {
			p_res->Read(w, 0x264);
		}
		p_res->GoNextSub(0x50414557);
	}

	for (int i = 0; i < count; ++i) {
		for (int j = 0; j < 8; ++j) {
			unsigned char* speed = (unsigned char*) &weapons[i].m_speed[j];
			if (PackedRead<int>(speed) != 0x497423f0) {
				PackedWrite<float>(speed, PackedRead<float>(speed) * 0.001f);
			}
			unsigned char* zSpeed = (unsigned char*) &weapons[i].m_zSpeed[j];
			if (PackedRead<int>(zSpeed) != 0x497423f0) {
				PackedWrite<float>(zSpeed, PackedRead<float>(zSpeed) * 0.001f);
			}
		}
	}
	return count;
}

// FUNCTION: ALIEN 0x411920
void MAP::ReloadVid()
{
	RESOURCE res;
	int i;
	for (i = 0; i < m_noVid; ++i) {
		VID* vid = m_vids[i];
		if (vid && vid->m_mirror != vid) {
			ExchangeVid(vid->m_mirror, vid);
		}
	}

	if (res.OpenForRead(m_resName, 0x41544144)) {
		if (::Error) {
			MYERROR::Error(
				::Error,
				"MAP",
				7,
				// STRING: ALIEN 0x4823a0
				"resource file",
				0
			);
		}
		return;
	}
	LoadWeapon(&res);
	EmptyVid->m_exData = (VID_EXDATA*) m_weapon;
	if (res.GoBegin(0x204a424f)) {
		if (::Error) {
			MYERROR::Error(::Error, "MAP", 11, "load 'VID'", 0);
		}
		return;
	}
	do {
		i = -1;
		int readError = res.m_subSize < (int) sizeof(i) ? 1 : res.Read(&i, sizeof(i));
		if (readError || !ValidVidIndex(i)) {
			if (::Error) {
				MYERROR::Error(::Error, "MAP", 4, "nvid > MAX_VID", i);
			}
			continue;
		}
		if (m_vids[i]) {
			VID* mirror = m_vids[i]->m_mirror;
			int mirrorIdx = mirror ? mirror->m_idx : -1;
			if (!ValidVidIndex(mirrorIdx) || mirrorIdx >= m_noVid || !m_vids[mirrorIdx]) {
				if (::Error) {
					MYERROR::Error(::Error, "MAP", 4, "nvid > MAX_VID", mirrorIdx);
				}
				continue;
			}
			i = mirrorIdx;
			((STRING*) &m_vids[i]->m_name)->Read_res(&res);
			m_vids[i]->LoadParameters(&res);
			VID* vid = m_vids[i];
			int weapon = vid->m_weaponIdx;
			if (weapon < m_noWeapon) {
				vid->m_exData = (VID_EXDATA*) m_weapon + weapon;
			}
			else {
				vid->m_exData = (VID_EXDATA*) m_weapon;
			}
		}
	} while (!res.GoNextSub(0x204a424f));

	for (i = 0; i < m_noVid; ++i) {
		VID* vid = m_vids[i];
		if (vid && !(vid->m_pixelFlag16 & 0x200)) {
			vid->SetChildAndLink();
		}
	}
	res.Close();
}

inline float MapWorldDim(float p_dim)
{
	return p_dim;
}

// FUNCTION: ALIEN 0x412a40
void MAP::CreateEmptyHardwareGround()
{
	if (m_noVid < 1025) {
		m_noVid = 1025;
	}
	VID* old = m_vids[1024];
	if (old) {
		old->ScalarDeletingDestructor(1);
	}
	VID_HARDWARE* vid = new VID_HARDWARE(1024, (int) MapWorldDim(m_w), (int) MapWorldDim(m_h));
	m_vids[1024] = vid;
	vid->m_exData = (VID_EXDATA*) m_weapon;
	CreateSprite(m_vids[1024], m_w * 0.5f, m_h * 0.5f, 0.0f, ANGLE((unsigned char) 0), 0);
	MYERROR::Log(
		::Error,
		// STRING: ALIEN 0x482a70
		"Create Empty Hardware Ground"
	);
}

// FUNCTION: ALIEN 0x41f0e0
int MAP::Tact()
{
	return 0;
}


static unsigned int EncodeGammaPacked(const GAMMA& p_gamma)
{
	static const unsigned int masks[4] = {0x7f, 0x7f80, 0x7f8000, 0xff800000};
	static const unsigned int signs[4] = {0x80, 0x8000, 0x800000, 0x80000000};
	unsigned int packed = 0;
	for (int i = 0; i < 4; ++i) {
		unsigned int add = ((unsigned int) p_gamma.m_a >> 1) & masks[i];
		unsigned int sub = ((unsigned int) p_gamma.m_b >> 1) & masks[i];
		if (sub) {
			packed |= (~sub & masks[i]) | signs[i];
		}
		else {
			packed |= add;
		}
	}
	return packed;
}

bool MAP::IsVidQueryFor(int p_query)
{
	if (p_query & GETSPRITE_VID) {
		return true;
	}
	if (Game_IsZS1()) {
		return (p_query & 0xfff) != 0 && !(p_query & 0x1000);
	}
	return IsVidQuery(p_query);
}

int MAP::VidFromQueryFor(int p_query)
{
	if (p_query & GETSPRITE_VID) {
		return p_query & GETSPRITE_VID_INDEX;
	}
	if (Game_IsZS1()) {
		return IsVidQueryFor(p_query) ? p_query & 0xfff : -1;
	}
	return VidFromQuery(p_query);
}

int MAP::LayerCount()
{
	return Game_IsZS1() ? 21 : 17;
}

int MAP::LayerWalkCount()
{
	return Game_IsZS1() ? 20 : 16;
}

// FUNCTION: ALIEN 0x4335d0
float MAP::AbsX(float p_x)
{
	return p_x + m_shiftX;
}

// FUNCTION: ALIEN 0x4335e0
float MAP::AbsY(float p_y)
{
	return p_y + m_shiftY;
}

// FUNCTION: ALIEN 0x434700
float MAP::ScrX(float p_x)
{
	return p_x - m_shiftX;
}

// FUNCTION: ALIEN 0x434710
float MAP::ScrY(float p_y)
{
	return p_y - m_shiftY;
}

// GLOBAL: ALIEN 0x483fc4
char* RegistrationInfo = empty_str;

// FUNCTION: ALIEN 0x435660
void __stdcall GetRegistrationInformation(char* p_info)
{
	RegistrationInfo = p_info;
}

// FUNCTION: ALIEN 0x435670
VID** ScriptExecFunc(int p_cmd)
{
	return Map->ExecFunc(p_cmd);
}

static __forceinline int InlineLogicStackInt(LOGICSTACK* p_value)
{
	return p_value->Int();
}

// STUB: ALIEN 0x435690
VID** MAP::ExecFunc(int p_cmd)
{
	static int unitType;
	static int unitIterator;
	static int spriteLayer;
	static int spriteIterator;
	static STRING commands;
	switch (p_cmd) {
	case 65: { // script: CreateSprite(nvid, x, y, z, direction, parent)
		SPRITE* parent = (SPRITE*) m_logic.m_stack.PopObject();
		int direction = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Int();
		int z = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Int();
		int y = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Int();
		int x = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Int();
		int nvid = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Int();
		VID* vid = Vid(nvid);
		SPRITE* sprite = 0;
		if (vid == EmptyVid) {
			if (::Error) {
				MYERROR::Log(::Error, "!!!ERROR!!!SCRIPT: Invalid nvid %s %i", "for CreateSprite()", nvid);
			}
			m_logic.PushInt(0);
			return 0;
		}
		bool uiCoordinates = Graph && (vid->m_sprClass == 10 || vid->m_sprClass == 19);
		UI_SCALING::MENU_POINT point = {};
		int uiScale = 0;
		if (uiCoordinates) {
			GRAPH_CORE* graph = (GRAPH_CORE*) Graph;
			uiScale = graph->m_uiScale;
			SPRITE* anchor = FindStateBarAmmoRoot(this, nvid, x, y, z);
			if (anchor) {
				uiScale = anchor->UIScale();
				point = UI_SCALING::TransformAnchoredScriptPoint(
					(float) x,
					(float) y,
					(float) z,
					graph->m_width,
					graph->m_height,
					anchor->UIDrawScale(),
					anchor->UIAnchorX(),
					anchor->UIAnchorY()
				);
			}
			else {
				point = UI_SCALING::TransformScriptPoint(
					(float) x,
					(float) y,
					(float) z,
					graph->m_width,
					graph->m_height,
					graph->m_uiScale * graph->m_uiPresentationScale
				);
			}
		}
		sprite = uiCoordinates ? CreateSprite(vid, point.m_x, point.m_y, point.m_z, ANGLE((char) direction), parent)
							   : CreateSprite(vid, (float) x, (float) y, (float) z, ANGLE((char) direction), parent);
		if (sprite && uiCoordinates) {
			sprite->SetUIScriptLayout(uiScale, point.m_anchorX, point.m_anchorY);
		}
		m_logic.PushObject(sprite);
		return 0;
	}
	case 66: { // script: Flagman(army)
		MAN* flagman = Flagman(((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Int());
		m_logic.PushObject(flagman);
		return 0;
	}
	case 68: { // script: FirstUnit(type)
		unitType = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Int();
		SPRITE* sprite = (SPRITE*) Hash->m_list.LastIterate(&unitIterator);
		while (sprite && !(unitType & sprite->m_vid->m_unk0x0c)) {
			sprite = (SPRITE*) Hash->m_list.NextIterate(&unitIterator);
		}
		m_logic.PushObject(sprite);
		return 0;
	}
	case 69: { // script: NextUnit()
		SPRITE* sprite;
		do {
			sprite = (SPRITE*) Hash->m_list.NextIterate(&unitIterator);
		} while (sprite && !(unitType & sprite->m_vid->m_unk0x0c));
		m_logic.PushObject(sprite);
		return 0;
	}
	case 70: { // script: GetSprite(type, x, y, previous)
		SPRITE* previous = (SPRITE*) m_logic.m_stack.PopObject();
		int y = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Int();
		int x = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Int();
		int type = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Int();
		SPRITE* sprite = GetSprite(type, (float) x, (float) y, previous);
		m_logic.PushObject(sprite);
		return 0;
	}
	case 71: { // script: GetSpriteScr(type, x, y) - find a sprite at a screen position
		int y = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Int();
		int x = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Int();
		int type = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Int();
		m_logic.PushObject(GetSpriteScr(type, (float) x, (float) y));
		return 0;
	}
	case 72: { // script: FindNearestSprite(type, x, y, radius, previous)
		SPRITE* previous = (SPRITE*) m_logic.m_stack.PopObject();
		int radius = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Int();
		int y = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Int();
		int x = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Int();
		int type = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Int();
		SPRITE* sprite = FindNearestSprite(type, (float) x, (float) y, (float) radius, previous);
		m_logic.PushObject(sprite);
		return 0;
	}
	case 74: { // script: FirstInBox(left, top, right, bottom)
		int bottom = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Int();
		int right = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Int();
		int top = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Int();
		int left = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Int();
		SPRITE* sprite = Hash->FirstInBox((float) left, (float) top, (float) right, (float) bottom);
		m_logic.PushObject(sprite);
		return 0;
	}
	case 75: { // script: NextInBox()
		SPRITE* sprite = Hash->NextInBox();
		m_logic.PushObject(sprite);
		return 0;
	}
	case 76: { // script: FirstSprite()
		SPRITE* sprite = 0;
		if (Game_IsZS1()) {
			spriteLayer = -1;
			while (spriteLayer < LayerWalkCount()) {
				++spriteLayer;
				spriteIterator = m_layers[spriteLayer].m_n;
				sprite = NextSprite(spriteLayer, &spriteIterator);
				if (sprite) {
					break;
				}
			}
		}
		else {
			spriteLayer = 0;
			spriteIterator = m_layers[0].m_n;
			sprite = NextSprite(0, &spriteIterator);
		}
		m_logic.PushObject(sprite);
		return 0;
	}
	case 77: { // script: NextSprite()
		SPRITE* sprite = NextSprite(spriteLayer, &spriteIterator);
		while (!sprite && spriteLayer < LayerWalkCount()) {
			++spriteLayer;
			spriteIterator = m_layers[spriteLayer].m_n;
			sprite = NextSprite(spriteLayer, &spriteIterator);
		}
		m_logic.PushObject(sprite);
		return 0;
	}
	case 79: { // script: Action(unit, action, var1, var2, var3)
		decomp_intptr var3 = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Value();
		decomp_intptr var2 = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Value();
		decomp_intptr var1 = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Value();
		int action = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Int();
		SPRITE* sprite = (SPRITE*) m_logic.m_stack.PopObject();
		if (sprite) {
			if (action < 17) {
				sprite->ChangeAnimation(action);
			}
			else {
				if (action == 0x79 || (Game_IsZS1() && (action == 124 || action == 125))) {
					decomp_intptr result = sprite->Action(action, var1, var2, var3);
					if (result) {
						m_logic.PushStr(*reinterpret_cast<const STRING*>(result));
					}
					else {
						m_logic.PushStr(STRING(""));
					}
					return 0;
				}
				if (action == 0x5a || action == 0x9c || action == 0x9b || action == 0x9a || action == 0x65 ||
					action == 0x67 || (action == 0x63 && !Game_IsZS1())) {
					decomp_intptr result = sprite->Action(action, var1, var2, var3);
					m_logic.PushObject(reinterpret_cast<const void*>(result));
					return 0;
				}
				if ((action == 0x21 || action == 0x20 || action == 0x24 || action == 0x22 || action == 0x96 ||
					 action == 0x97) &&
					sprite->m_vid->m_sprClass == 0x15 && (sprite->m_flag & 0x7c) == 0x68) {
					m_logic.PushInt(0);
					return 0;
				}
				decomp_intptr result = sprite->Action(action, var1, var2, var3);
				if (action == 54) {
					SynchronizeGrantedWeaponHud(this, sprite, var1);
				}
				m_logic.PushInt((int) result);
				return 0;
			}
		}
		m_logic.PushInt(0);
		return 0;
	}
	case 80: { // script: SizeTo(unit, x, y)
		int y = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Int();
		int x = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Int();
		SPRITE* sprite = (SPRITE*) m_logic.m_stack.PopObject();
		if (sprite) {
			float dx = (float) x - sprite->m_x;
			float dy = (float) y - sprite->m_y;
			m_logic.PushInt((int) SPRITE::NearDistanceTo(dx, dy));
		}
		else {
			m_logic.PushInt(60000);
		}
		return 0;
	}
	case 82: { // script: AddCommand(unit, action, var1, var2, var3)
		decomp_intptr var3 = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Value();
		decomp_intptr var2 = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Value();
		decomp_intptr var1 = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Value();
		int action = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Int();
		SPRITE* sprite = (SPRITE*) m_logic.m_stack.PopObject();
		if (sprite) {
			sprite->AddActionAfterStop(action, var1, var2, var3);
		}
		return 0;
	}
	case 83: { // script: GetUnitVid(unit)
		SPRITE* sprite = (SPRITE*) m_logic.m_stack.PopObject();
		m_logic.PushInt(sprite ? sprite->m_vid->m_idx : 0);
		return 0;
	}
	case 84: { // script: DeleteUnit(unit)
		SPRITE* sprite = (SPRITE*) m_logic.m_stack.PopObject();
		if (sprite) {
			sprite->ScalarDeletingDestructor(1);
		}
		return 0;
	}
	case 85: { // script: GetX(unit)
		SPRITE* sprite = (SPRITE*) m_logic.m_stack.PopObject();
		PushInt(sprite ? (int) sprite->ScriptX() : 0);
		return 0;
	}
	case 86: { // script: GetY(unit)
		SPRITE* sprite = (SPRITE*) m_logic.m_stack.PopObject();
		PushInt(sprite ? (int) sprite->ScriptY() : 0);
		return 0;
	}
	case 87: { // script: GetZ(unit)
		SPRITE* sprite = (SPRITE*) m_logic.m_stack.PopObject();
		PushInt(sprite ? (int) sprite->m_z : 0);
		return 0;
	}
	case 88: { // script: GetDirection(unit)
		SPRITE* sprite = (SPRITE*) m_logic.m_stack.PopObject();
		PushInt(sprite ? sprite->m_dir : 0);
		return 0;
	}
	case 89: { // script: GetAnimation(unit)
		SPRITE* sprite = (SPRITE*) m_logic.m_stack.PopObject();
		PushInt(sprite ? sprite->m_ani : 0);
		return 0;
	}
	case 90: { // script: DirectionTo(unit, x, y)
		int y = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Int();
		int x = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Int();
		SPRITE* sprite = (SPRITE*) m_logic.m_stack.PopObject();
		int direction = 0;
		if (sprite) {
			direction = ANGLE((float) x - sprite->m_x, (float) y - sprite->m_y).m_dir;
		}
		PushInt(direction);
		return 0;
	}
	case 91: // script: ViewXMin()
	case 92: // script: ViewYMin()
		PushInt(0);
		return 0;
	case 96: { // script: GetCommands(unit)
		SPRITE* sprite = (SPRITE*) m_logic.m_stack.PopObject();
		if (sprite) {
			STRING actions = sprite->GetTextActions();
			STRING items = sprite->GetTextItems();
			STRING commands(items.m_str, actions.m_str);
			m_logic.PushStr(commands);
		}
		else {
			STRING commands(empty_str);
			m_logic.PushStr(commands);
		}
		return 0;
	}
	case 97: { // script: SetCommands(unit, commands)
		LOGICSTACK* value = (LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n;
		commands = *value->String();
		SPRITE* sprite = (SPRITE*) m_logic.m_stack.PopObject();
		if (sprite) {
			sprite->SetTextItems(commands);
			if (strstr(commands.m_str, "\2")) {
				char* after;
				commands.After(&after, "\2");
				commands = after;
				if (after != STRING::EMPTY) {
					operator delete(after);
				}
			}
			sprite->SetTextActions(commands);
		}
		return 0;
	}
	case 98: { // script: SetScript(name) - queue a script/map name and mark it pending
		STRING* name = PopStr();
		m_flag |= 0x40;
		m_scriptName = *name;
		return 0;
	}
	case 99: // script: SaveMap(name) - write the map to the named file
		SaveMap(STRING(PopStr()->m_str));
		return 0;
	case 100: // script: StartDemoRecord(filename) - open the demo stream for writing
		if (m_resource.m_file) {
			return 0;
		}
		m_resource.OpenForWrite(*PopStr(), 0x4f4d4544);
		return 0;
	case 101: { // script: MenuFind(nvid, ndir=999999)
		int ndir = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		int nvid = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Int();
		VID* vid = Vid(nvid);
		if (vid == EmptyVid && ::Error) {
			MYERROR::Log(
				::Error,
				// STRING: ALIEN 0x484130
				"!!!ERROR!!!SCRIPT: Invalid nvid %s %i",
				// STRING: ALIEN 0x48411c
				"for MenuFind",
				nvid
			);
		}
		if (vid != EmptyVid &&
			vid->m_entitiesNumber[0] + vid->m_entitiesNumber[1] + vid->m_entitiesNumber[2] + vid->m_entitiesNumber[3] !=
				0) {
			for (int i = 0; i < m_menu.m_n; ++i) {
				SPRITE* sprite = (SPRITE*) m_menu.m_data[i];
				if (sprite->m_vid == vid &&
					(ndir == 999999 || (unsigned int) ndir == vid->RealDirection(ANGLE(sprite->m_dir)) ||
					 (Game_IsZS1() && ndir >= 999000 && ndir != 999999 &&
					  ndir - 999000 == (int) sprite->m_dir))) {
					m_logic.PushObject(sprite);
					return 0;
				}
			}
		}
		m_logic.PushObject(0);
		return 0;
	}
	case 102: { // script: MenuLoad(filename, opt=1)
		int opt = GameData_IsSteam() ? PopInt() : 0;
		m_menu.Load(*PopStr(), opt);
		return 0;
	}
	case 103: { // script: MenuRelease(filename="")
		STRING name(*PopStr());
		if (!strcmp(name.m_str, empty_str)) {
			m_menu.DeleteAll();
			LeaveFullscreenMenuFrame();
		}
		else {
			m_menu.DeleteFromFile(name);
		}
		return 0;
	}
	case 104: // script: NVidUnderCursor()
		m_logic.PushInt(m_menu.NVidUnderCursor());
		return 0;
	case 105: // script: NDirUnderCursor()
		m_logic.PushInt((int) m_menu.NDirUnderCursor());
		return 0;
	case 106: {
		decomp_intptr var3 = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Value();
		decomp_intptr var2 = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Value();
		decomp_intptr var1 = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Value();
		int action = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		int ndir = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		int nvid = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Int();
		VID* vid = Vid(nvid);
		if (vid == EmptyVid) {
			if (::Error) {
				MYERROR::Log(
					::Error,
					"!!!ERROR!!!SCRIPT: Invalid nvid %s %i",
					// STRING: ALIEN 0x48410c
					"for MenuAction",
					nvid
				);
			}
			return 0;
		}
		for (int i = 0; i < m_menu.m_n; ++i) {
			SPRITE* sprite = (SPRITE*) m_menu.m_data[i];
			if (sprite->m_vid == vid &&
				(ndir == 999999 || (unsigned int) ndir == vid->RealDirection(ANGLE(sprite->m_dir)) ||
				 (Game_IsZS1() && ndir >= 999000 && ndir != 999999 &&
				  ndir - 999000 == (int) sprite->m_dir))) {

				if (action >= 17) {
					sprite->Action(action, var1, var2, var3);
				}
				else {
					sprite->ChangeAnimation(action);
				}
			}
		}
		return 0;
	}
	case 107: { // script: MenuCreate(nvid, ndir, x, y, z) - spawn a menu sprite
		int z = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		int y = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n) + z;
		int x = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		int ndir = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		int nvid = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Int();
		VID* vid = Vid(nvid);
		if (vid == EmptyVid) {
			if (::Error) {
				MYERROR::Log(
					::Error,
					"!!!ERROR!!!SCRIPT: Invalid nvid %s %i",
					// STRING: ALIEN 0x4840fc
					"for MenuCreate",
					nvid
				);
			}
			PushInt(0);
			return 0;
		}
		GRAPH_CORE* graph = (GRAPH_CORE*) Graph;
		UI_SCALING::MENU_POINT point = UI_SCALING::TransformScriptPoint(
			(float) x,
			(float) y,
			(float) z,
			graph->m_width,
			graph->m_height,
			graph->m_uiScale * graph->m_uiPresentationScale
		);
		SPRITE* sprite =
			CreateSprite(vid, point.m_x, point.m_y, point.m_z, ANGLE((char) ((ndir << 8) / (int) vid->m_noDir)), 0);
		if (sprite) {
			sprite->SetUIScriptLayout(graph->m_uiScale, point.m_anchorX, point.m_anchorY);
		}
		PushObject(sprite);
		return 0;
	}
	case 108: // script: SpriteUnderCursor()
		m_logic.PushObject((m_menu.m_state & 1) ? m_menu.m_underCursor : 0);
		return 0;
	case 109: // script: WorldX() - cursor position in world coordinates
		PushInt((int) m_input.m_worldX);
		return 0;
	case 110: // script: WorldY()
		PushInt((int) m_input.m_worldY);
		return 0;
	case 111: // script: GetKey()
		m_logic.PushInt(m_input.m_key);
		return 0;
	case 112: { // script: SetPause(on)
		int on = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		if (on) {
			PauseOn();
			Mouse->ChangeAnimation(0);
		}
		else {
			PauseOff();
		}
		return 0;
	}
	case 113: { // script: SetCursor(type)
		int type = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		if (type == -1) { // CURSOR_OFF
			Mouse->Disable();
		}
		else if (type == 256) { // CURSOR_HARDWARE
			Mouse->HardwareOn();
		}
		else if (type == 257) { // CURSOR_SOFTWARE
			Mouse->HardwareOff();
		}
		else {
			if (!Mouse->m_hardware) {
				Mouse->Enable();
			}
			Mouse->ChangeAnimation(type);
		}
		return 0;
	}
	case 114: { // script: PutMessage(msg, x, y) - show a message for the current army
		int y = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		int x = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		STRING* msg = PopStr();
		m_player[m_curArmy]->PutMessage(*msg, (float) x, (float) y);
		return 0;
	}
	case 115: { // script: GetInputState()
		unsigned int buttons = m_input.m_button;
		int state = (buttons >> 15) & 1;
		state = ((buttons >> 14) & 1) + state * 2;
		state = ((buttons >> 9) & 1) + state * 2;
		state = ((buttons >> 10) & 1) + state * 2;
		state = ((buttons >> 8) & 1) + state * 2;
		state = ((buttons >> 7) & 1) + state * 2;
		state = ((buttons >> 12) & 1) + state * 2;
		state = ((buttons >> 11) & 1) + state * 2;
		state = ((buttons >> 6) & 1) + state * 2;
		state = ((buttons >> 5) & 1) + state * 2;
		state = ((buttons >> 2) & 1) + state * 2;
		state = (buttons & 1) + state * 2;
		m_logic.PushInt(state);
		return 0;
	}
	case 116: { // script: SetShiftCoor(x, y) - set the scroll target position
		int y = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		int x = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		SetShiftCoor((float) x, (float) y, 0);
		return 0;
	}
	case 117: { // script: SetShiftFlag(flag) - scroll/follow mode bits
		unsigned int flag = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		m_shiftFlag = VIEWPORT_MATH::ResolveShiftFlag(flag, Graph->m_width);
		return 0;
	}
	case 118: // script: GetShiftFlag()
		m_logic.PushInt(m_shiftFlag);
		return 0;
	case 119: // script: ScreenX()
		m_logic.PushInt((int) ((GRAPH_CORE*) Graph)->m_width);
		return 0;
	case 120: // script: ScreenY()
		m_logic.PushInt((int) ((GRAPH_CORE*) Graph)->m_height);
		return 0;
	case 121: { // script: SetPlayerControl(flag) | SetSelectUnit(flag)
		int on = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		m_flag = (m_flag & ~0x80u) | (on ? 0x80 : 0);
		return 0;
	}
	case 122: { // script: toggle the current army player's state bar
		int on = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		if (on) {
			m_player[m_curArmy]->StateBarOn();
		}
		else {
			m_player[m_curArmy]->StateBarOff();
		}
		return 0;
	}
	case 123: { // script: GetString(app, key) - read a localized string from the profile
		commands = *PopStr();
		STRING def(empty_str);
		PushStr(Strings->GetString(*PopStr(), commands, def));
		return 0;
	}
	case 124: // script: Quit() - discard the string argument, then close the window
		PopStr();
		m_quit = 1;
		return 0;
	case 125: { // script: ToScreenX(x)
		LOGIC* logic = &m_logic;
		int x = InlineLogicStackInt((LOGICSTACK*) logic->m_stack.m_data + --logic->m_stack.m_n);
		logic->PushInt((int) ((float) x - m_shiftX));
		return 0;
	}
	case 126: { // script: ToScreenY(y, z)
		LOGIC* logic = &m_logic;
		int z = InlineLogicStackInt((LOGICSTACK*) logic->m_stack.m_data + --logic->m_stack.m_n);
		int y = InlineLogicStackInt((LOGICSTACK*) logic->m_stack.m_data + --logic->m_stack.m_n);
		logic->PushInt((int) ((float) y - (float) z - m_shiftY));
		return 0;
	}
	case 127: // script: SpriteUnderCursor (secondary select state)
		m_logic.PushObject((m_menu.m_state & 2) ? m_menu.m_underCursor : 0);
		return 0;
	case 128: { // script: SetKeyBinding(which, key) - bind a control key pair
		int key = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		int which = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		if (which == 0x400) {
			INPUT_AS::firstKey2 = key;
			INPUT_AS::firstKey1 = key;
		}
		else if (which == 0x800) {
			INPUT_AS::secondKey2 = key;
			INPUT_AS::secondKey1 = key;
		}
		return 0;
	}
	case 129: {
		int val1 = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		int val2 = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		Mouse->Action(0x3f, val2, val1, 0);
		return 0;
	}
	case 130: { // script: SetSoundVolume(volume)
		Sound->VolumeSound(InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n));
		return 0;
	}
	case 131: { // script: SetMusicVolume(volume)
		Sound->VolumeMusic(InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n));
		return 0;
	}
	case 132: // script: PlaySFX(sfxId)
		Sound->PlaySFX(InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n), 0, 0);
		return 0;
	case 133:
		Sound->StopSFX(InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n));
		return 0;
	case 134:
		if (Game_IsZS1()) {
			Sound->StopMusicFade(PopInt());
			return 0;
		}
		Sound->StopMusic();
		return 0;
	case 135: { // script: PlaySFXFromCoor(sfx, x, y) - play a sound at a screen position
		int y = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		int x = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		int sfx = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		float listenerX;
		float listenerY;
		GetAudioListener(&listenerX, &listenerY);
		Sound->PlaySFXFromCoor(
			sfx,
			VIEWPORT_MATH::RelativeAudioAxis((float) x, listenerX),
			VIEWPORT_MATH::RelativeAudioAxis((float) y, listenerY)
		);
		return 0;
	}
	case 136: { // script: PlayMusicFile(file, loop=1)
		int fadeMs = Game_IsZS1() ? PopInt() : 0;
		int loop = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		Sound->FadeAndPlayFile(*PopStr(), loop, fadeMs);
		return 0;
	}
	case 137: { // script: Effect(effect, var1, var2, duration)
		int duration = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		int var2 = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		int var1 = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		int effect = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		((GRAPH_CORE*) Graph)->Effect(effect, var1, var2, duration);
		return 0;
	}
	case 138: // script: SetEnvironment(env)
		Graph->SetEnvironment(InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n));
		return 0;
	case 139: // script: reserved - consumes one argument
		InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		return 0;
	case 140: { // script: SetGamma(packed) - split a signed per-byte gamma delta and apply
		unsigned int val = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		int lo = 0;
		int hi = 0;
		if (val & 0x80) {
			hi |= (~val & 0x7f) << 1;
		}
		else {
			lo |= (val & 0x7f) << 1;
		}
		if (val & 0x8000) {
			hi |= (~val & 0x7f80) << 1;
		}
		else {
			lo |= (val & 0x7f80) << 1;
		}
		if (val & 0x800000) {
			hi |= (~val & 0x7f8000) << 1;
		}
		else {
			lo |= (val & 0x7f8000) << 1;
		}
		if (val & 0x80000000) {
			hi |= (~val & 0xff800000) << 1;
		}
		else {
			lo |= (val & 0xff800000) << 1;
		}
		GAMMA g;
		g.m_a = lo;
		g.m_b = hi;
		Graph->SetGamma(g);
		return 0;
	}
	case 141: { // script: SetWind(angle, force)
		int angle = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		int force = PopInt();
		Graph->SetWind(force, ANGLE((unsigned char) angle));
		return 0;
	}
	case 142: // script: PlayMovie(filename)
		Graph->PlayMovie(PopStr()->m_str);
		return 0;
	case 143: // script: IsMoviePlaying()
		// No video files ship with the game, so a movie is never playing.
		m_logic.PushInt(0);
		return 0;
	case 144: // script: StopMovie()
		return 0;
	case 145: { // script: IsMusicPlaying()
		MUSIC* music = Sound->m_music;
		m_logic.PushInt(music && (music->IsPlaying() || Sound->m_loop) ? 1 : 0);
		return 0;
	}
	case 146: { // script: CountGamma(g1, g2, time) - blend two packed gamma pairs
		int time = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		int g2 = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		int g1 = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		m_logic.PushInt(CountGamma(g1, g2, time));
		return 0;
	}
	case 147: { // script: GetGamma() - pack the set gamma (positive dword + negative dword) into one int
		GAMMA& gamma = ((GRAPH_CORE*) Graph)->m_gammaSet;
		unsigned int neg = gamma.m_b;
		unsigned int packed = ((unsigned int) gamma.m_a >> 1) & 0x7f7f7f7f;
		if (neg & 0xff) {
			packed |= ((~neg & 0xfe) | 0x100) >> 1;
		}
		if (neg & 0xff00) {
			packed |= ((~neg & 0xfe00) | 0x10000) >> 1;
		}
		if (neg & 0xff0000) {
			packed |= ((~neg & 0xfe0000) | 0x1000000) >> 1;
		}
		if (neg & 0xff000000) {
			packed |= ((~neg >> 1) & 0x7f000000) | 0x80000000;
		}
		m_logic.PushInt(packed);
		return 0;
	}
	case 148: { // script: GetEffectState(eff)
		int eff = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		m_logic.PushInt(Graph->GetEffectState(eff));
		return 0;
	}
	case 149: // script: GetPrevMapName()
		PushStr(m_prevMap);
		return 0;
	case 150: { // script: GetRegistrationInfo() | SetGraphScreen(sx, sy, fullscreen)
		if (GameData_IsSteam()) {
			int fullscreen = PopInt();
			int sizeY = PopInt();
			int sizeX = PopInt();
			if ((sizeX != -1 || sizeY != -1) && ::Error) {
				MYERROR::Log(::Error, "SetGraphScreen(%i, %i) resolution request ignored", sizeX, sizeY);
			}
			if (fullscreen != -1) {
				Platform_RenderSetFullscreen(fullscreen);
				GRAPH_CORE* graph = (GRAPH_CORE*) Graph;
				if (graph) {
					graph->m_flags = (graph->m_flags & ~0x80u) | (fullscreen ? 0x80 : 0);
				}
				PortableConfig_SetInt("display", "FullScreen", fullscreen != 0);
				PortableConfig_Flush();
			}
			return 0;
		}
		PushStr(STRING(RegistrationInfo));
		return 0;
	}
	case 151: // script: GetMapName()
		PushStr(m_mapName);
		return 0;
	case 152: { // script: Exec(cmdline) - launch an external program via the shell
		commands = *PopStr();
		MYERROR::Log(
			::Error,
			// STRING: ALIEN 0x4840f0
			"Exec '%s'",
			commands.m_str
		);
		STRING params;
		commands.After(
			&params.m_str,
			// STRING: ALIEN 0x47f740
			" "
		);
		STRING file;
		commands.Before(&file.m_str, " ");
		// The script's Exec always names a document or a URL to hand to the
		// desktop, never an executable with arguments, so the parameters are
		// appended rather than passed as a command line.
		STRING target = params.m_str && *params.m_str ? file + " " + params : file;
		if (!strstr(target.m_str, "://")) {
			std::string resolved(Platform_ResolvePath((std::string(Platform_BasePath()) + target.m_str).c_str()));
			if (FILE* doc = fopen(resolved.c_str(), "rb")) {
				fclose(doc);
				SDL_OpenURL(("file://" + resolved).c_str());
				return 0;
			}
		}
		SDL_OpenURL(target.m_str);
		return 0;
	}
	case 153: { // script: CharAt(str, idx) - signed character code at a position
		int idx = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		commands = *PopStr();
		m_logic.PushInt((signed char) commands.m_str[idx]);
		return 0;
	}
	case 154: // script: LogMessage(text)
		// The script's text is data, not a format: a map containing a % would
		// otherwise read arguments that were never passed.
		MYERROR::Log(::Error, "%s", PopStr()->m_str);
		return 0;
	case 155: { // script: Random(n) - uniform value in 0..n
		int n = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		m_logic.PushInt(n > 0 ? GameRand() % (n + 1) : 0);
		return 0;
	}
	case 156: { // script: ChangeZUnit(vidIdx, z) - set the z of every sprite of a vid type
		int z = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		int vidIdx = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Int();
		VID* vid = VidExist(vidIdx) ? m_vids[vidIdx] : EmptyVid;
		if (vid == EmptyVid) {
			if (::Error) {
				MYERROR::Log(
					::Error,
					"!!!ERROR!!!SCRIPT: Invalid nvid %s %i",
					// STRING: ALIEN 0x4840e0
					"for ChangeZUnit",
					vidIdx
				);
			}
			return 0;
		}
		int iter;
		for (SPRITE* sprite = FirstSprite(vid->m_layer, &iter); sprite; sprite = NextSprite(vid->m_layer, &iter)) {
			if (sprite->m_vid == vid) {
				sprite->ChangeCoor(sprite->m_x, sprite->m_y, (float) z);
			}
		}
		return 0;
	}
	case 157: // script: GetTime()
		m_logic.PushInt(CurrentTime);
		return 0;
	case 158: { // script: GroundZ(x, y) - query the ground height at a point
		int y = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		int x = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		PushInt((int) GetGroundZ_ff((float) x, (float) y));
		return 0;
	}
	case 160: { // script: SetFlagman(army, unit)
		SPRITE* unit = (SPRITE*) m_logic.m_stack.PopObject();
		SetFlagman(InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n), unit);
		return 0;
	}
	case 161: { // script: CanPlace(vidIdx, x, y, z) - test placement, push the blocker
		int z = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		int y = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		int x = InlineLogicStackInt((LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n);
		int vidIdx = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Int();
		VID* vid = VidExist(vidIdx) ? m_vids[vidIdx] : EmptyVid;
		if (vid == EmptyVid && ::Error) {
			MYERROR::Log(
				::Error,
				"!!!ERROR!!!SCRIPT: Invalid nvid %s %i",
				// STRING: ALIEN 0x4840d0
				"for CanPlace",
				vidIdx
			);
		}
		PushObject((void*) Hash->CanPlace(vid, (float) x, (float) y, (float) z));
		return 0;
	}
	case 162: {
		int type = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Int();
		if (Game_IsZS1() && type == 245) {
			PushInt(VidExists(PopInt()) ? 1 : 0);
			return 0;
		}
		VID* vid = PopVid(
			// STRING: ALIEN 0x4840c4
			"for GetVid"
		);
		if (vid == EmptyVid) {
			PushInt(0);
			return 0;
		}
		switch (type) {
		case 1:
			PushInt(vid->m_defaultMaxHp);
			return 0;
		case 18:
		case 19:
		case 20:
		case 21:
			if (Game_IsZS1()) {
				PushInt((int) EncodeGammaPacked(vid->m_gamma[type - 18]));
				return 0;
			}
			break;
		case 22:
			if (Game_IsZS1()) {
				PushInt(vid->m_flag);
				return 0;
			}
			break;
		case 23:

			PushInt((int) vid->m_exData->m_unk0x18);
			return 0;
		case 52:
			if (Game_IsZS1()) {
				PushInt(vid->m_mirror ? vid->m_mirror->m_idx : vid->m_idx);
				return 0;
			}
			break;
		case 130:
			if (Game_IsZS1()) {
				PushInt(vid->m_exData->m_unk0x20);
				return 0;
			}
			break;
		case 134:
			if (Game_IsZS1()) {
				PushInt((int) vid->m_blastRadius);
				return 0;
			}
			break;
		case 239:
		case 240:
		case 241:
			if (Game_IsZS1()) {
				float size = type == 239 ? vid->m_footprintWidth : type == 240 ? vid->m_footprintHeight : vid->m_unk0x24;
				PushInt((int) size);
				return 0;
			}
			break;
		case 242:
		case 243:
		case 244:
			if (Game_IsZS1()) {
				float scale = type == 242 ? vid->m_gammaR : type == 243 ? vid->m_gammaG : vid->m_gammaB;
				PushInt((int) (scale * 1000.0f));
				return 0;
			}
			break;
		case 26: // script: VID_AMMO
			PushInt(vid->GetMaxAmmo());
			return 0;
		case 27: // script: VID_NAME
			m_logic.PushStr(STRING(vid->m_name));
			return 0;
		case 28: // script: VID_COUNT
			PushInt(vid->GetEntitiesNumberTotal());
			return 0;
		case 29: // script: VID_KILLEDUNIT
			PushInt(vid->GetDeathsNumberTotal());
			return 0;
		case 30:
		case 31:
		case 32:
		case 33: // script: VID_KILLEDUNIT0..3
			PushInt(vid->GetDeathsNumber(type - 30));
			return 0;
		case 34:
		case 35:
		case 36:
		case 37: // script: VID_COUNT0..3
			PushInt(vid->GetEntitiesNumber(type - 34));
			return 0;
		case 38:
		case 39:
		case 40:
		case 41: // script: VID_MAXHP0..3
			PushInt(vid->GetMaxHp(type - 38));
			return 0;
		case 46: // script: VID_SPRITETYPE
			PushInt(vid->m_unk0x0c);
			return 0;
		case 47: // script: VID_CLASS
			PushInt(vid->m_sprClass);
			return 0;
		case 48: // script: VID_SPEED (999999.0f bit pattern = "immobile" sentinel)
			if (vid->m_unk0x2c == 999999.0f) {
				PushInt(999999);
			}
			else {
				PushInt((int) (vid->m_unk0x2c * 1000.0f));
			}
			return 0;
		case 49: // script: VID_LIFETIME
			PushInt(vid->m_unk0x6c);
			return 0;
		case 50: // script: VID_DETECTRANGE

			PushInt((int) vid->m_exData->m_unk0x14);
			return 0;
		case 51: // script: VID_WEAPONAIM

			PushInt((int) vid->m_exData->m_unk0x1c);
			return 0;
		case 53: // script: VID_NO_DIR
			PushInt(vid->m_noDir);
			return 0;
		case 54: // script: VID_MOVE_MASK
			PushInt(vid->m_unk0x18);
			return 0;
		case 55: // script: VID_BUILDTIME
			PushInt(vid->m_exData->m_buildTime);
			return 0;
		case 56: // script: VID_HIDE
			PushInt(vid->PropHide());
			return 0;
		case 57: // script: VID_NOT_CREATE_AS_CHILD
			PushInt(vid->GetNotCreateAsChild());
			return 0;
		case 58: // script: VID_FRAME_SPEED (high word of the pixel flags)
			PushInt(vid->m_defaultAniPeriod);
			return 0;
		case 59: // script: VID_LINK
			PushInt(vid->m_linkVid ? vid->m_linkVid->m_idx : 0);
			return 0;
		case 124: // script: VID_DAMAGE
			PushInt(vid->m_fireDamage);
			return 0;
		case 125: // script: VID_RECOLORUNIT
			PushInt(vid->GetRecolorsTotal());
			return 0;
		case 126:
		case 127:
		case 128:
		case 129: // script: VID_RECOLORUNIT0..3
			PushInt(vid->GetRecolors(type - 126));
			return 0;
		default:
			if (type >= 60 && type < 77) { // script: VID_CHILD slots: the linked child vid's number
				VID* child = vid->m_aniChildVid[type - 60];
				PushInt(child ? child->m_idx : 0);
				return 0;
			}
			if (type >= 92 && type < 109) { // script: VID_NO_CHILD slots
				PushInt(vid->m_aniFireCount[type - 92]);
				return 0;
			}
			Error(
				14,
				// STRING: ALIEN 0x4840b8
				"GetVid type",
				type
			);
			if (Game_IsZS1()) {
				PushInt(0);
			}
			return 0;
		}
	}
	case 163: {
		int value = PopInt();
		int type = PopInt();
		VID* vid = PopVid(
			// STRING: ALIEN 0x4840ac
			"for SetVid"
		);
		if (vid == EmptyVid) {
			return 0;
		}
		switch (type) {
		case 1:
			vid->m_defaultMaxHp = value;
			return 0;
		case 23:
			if (Game_IsZS1()) {
				vid->m_exData->m_unk0x18 = (float) value;
				return 0;
			}
			break;
		case 125:
			if (Game_IsZS1()) {
				vid->SetReColorForArmy((unsigned int) value);
				return 0;
			}
			break;
		case 130:
			if (Game_IsZS1()) {
				vid->m_exData->m_unk0x20 = value;
				return 0;
			}
			break;
		case 134:
			if (Game_IsZS1()) {
				vid->m_blastRadius = (float) value;
				return 0;
			}
			break;
		case 239:
		case 240:
		case 241:
			if (Game_IsZS1()) {
				float* size = type == 239 ? &vid->m_footprintWidth : type == 240 ? &vid->m_footprintHeight : &vid->m_unk0x24;
				*size = (float) value;
				if (type != 241) {
					vid->m_unk0x384 = vid->m_footprintWidth * 0.5f;
					vid->m_unk0x388 = vid->m_footprintHeight * 0.5f;
				}
				return 0;
			}
			break;
		case 242:
		case 243:
		case 244:
			if (Game_IsZS1()) {
				float* scale = type == 242 ? &vid->m_gammaR : type == 243 ? &vid->m_gammaG : &vid->m_gammaB;
				*scale = (float) value * 0.001f;
				return 0;
			}
			break;
		case 26: { // script: VID_AMMO (the weapon-bearing vid: the link when it has a weapon)
			VID* weapon = (vid->m_linkVid && vid->m_linkVid->HaveWeapon()) ? vid->m_linkVid : vid;
			weapon->m_exData->m_maxAmmo = value;
			return 0;
		}
		case 29: // script: VID_KILLEDUNIT (all armies)
			vid->m_deaths[3] = value;
			vid->m_deaths[2] = value;
			vid->m_deaths[1] = value;
			vid->m_deaths[0] = value;
			return 0;
		case 30:
		case 31:
		case 32:
		case 33: // script: VID_KILLEDUNIT0..3
			vid->m_deaths[type - 30] = value;
			return 0;
		case 38:
		case 39:
		case 40:
		case 41: // script: VID_MAXHP0..3
			vid->SetMaxHp(type - 38, value);
			return 0;
		case 42:
		case 43:
		case 44:
		case 45: // script: VID_HP_COEFF0..3
			vid->SetHpCoeff(type - 42, value);
			return 0;
		case 48: { // script: VID_SPEED - update the vid and every live sprite's movement override
			vid->m_unk0x2c = (value == 999999) ? 999999.0f : value * 0.001f;
			if (Game_IsZS1()) {
				vid->m_randomSpeed = vid->m_unk0x2c;
			}
			int iter;
			for (SPRITE* sprite = FirstSprite(vid->m_layer, &iter); sprite; sprite = NextSprite(vid->m_layer, &iter)) {
				if (sprite->GetVid() == vid && sprite->m_exData) {
					sprite->m_exData->m_unk0x20 = vid->m_unk0x2c;
				}
			}
			return 0;
		}
		case 49: // script: VID_LIFETIME (vid 0 is the reserved empty vid)
			if (vid->m_idx) {
				vid->m_unk0x6c = value;

				vid->m_unk0x478 = 1;
			}
			return 0;
		case 50: // script: VID_DETECTRANGE

			vid->m_exData->m_unk0x14 = (float) value;
			return 0;
		case 51: // script: VID_WEAPONAIM

			vid->m_exData->m_unk0x1c = (float) value;
			return 0;
		case 52: // script: VID_EXCHANGEVID
			if (VidExist(value)) {
				ExchangeVid(vid, Vid(value));
			}
			else {
				Error(
					4,
					// STRING: ALIEN 0x484098
					"SetVid get_image",
					value
				);
			}
			return 0;
		case 54: // script: VID_MOVE_MASK
			vid->m_unk0x18 = value;
			return 0;
		case 55: // script: VID_BUILDTIME
			vid->m_exData->m_buildTime = value;
			return 0;
		case 56: // script: VID_HIDE
			vid->SetPropHide(value);
			return 0;
		case 57: // script: VID_NOT_CREATE_AS_CHILD
			vid->SetNotCreateAsChild(value);
			return 0;
		case 58: { // script: VID_FRAME_SPEED - the flag word and every animation period
			vid->m_defaultAniPeriod = (unsigned short) value;
			for (int i = 0; i < 17; ++i) {
				vid->m_aniDuration[i] = value;
			}
			return 0;
		}
		case 59: // script: VID_LINK
			vid->m_linkVid = Vid(value);
			return 0;
		case 124: // script: VID_DAMAGE
			vid->m_fireDamage = value;
			return 0;
		default:
			if (type >= 60 && type < 77) { // script: VID_CHILD slots (negative number = keep the sign)
				if (!value) {
					vid->m_unk0x20c[type - 60] = 0;
					vid->m_aniChildVid[type - 60] = 0;
				}
				else if (VidExist(abs(value))) {
					vid->m_unk0x20c[type - 60] = value;
					vid->m_aniChildVid[type - 60] = Vid(abs(value));
					if (Vid(abs(value))->IsLight()) {

						vid->m_unk0x478 |= 1;
					}
				}
				else {
					Error(
						4,
						// STRING: ALIEN 0x484088
						"SetVid child",
						value
					);
				}
				return 0;
			}
			if (type >= 18 && type < 22) {
				// script: VID_GAMMA0..3 decodes a packed transform for one army.

				GAMMA gamma(GAMMA::DECODE, (unsigned int) value);
				vid->SetGamma(gamma, type - 18);
				return 0;
			}
			if (type >= 92 && type < 109) { // script: VID_NO_CHILD slots
				vid->m_aniFireCount[type - 92] = value;
				return 0;
			}
			Error(
				14,
				// STRING: ALIEN 0x48407c
				"SetVid type",
				type
			);
			return 0;
		}
	}
	case 164: // script: itoa(value)
		m_logic.PushStr(Int2Str(PopInt()));
		return 0;
	case 165: // script: Sin(angle) -> sin scaled by 1024
		PushInt((int) (ANGLE((char) PopInt()).Sin() * 1024.0f));
		return 0;
	case 166: // script: Cos(angle) -> cos scaled by 1024
		PushInt((int) (ANGLE((char) PopInt()).Cos() * 1024.0f));
		return 0;
	case 167: // script: MapWidth()
		PushInt((int) m_w);
		return 0;
	case 168: // script: MapHeight()
		PushInt((int) m_h);
		return 0;
	case 169: { // script: Genocide(vid) - destroy every sprite of the given vid type
		VID* vid = PopVid(
			// STRING: ALIEN 0x48406c
			"for Genocide"
		);
		if (vid == EmptyVid) {
			return 0;
		}
		int iter;
		for (SPRITE* sprite = FirstSprite(vid->m_layer, &iter); sprite; sprite = NextSprite(vid->m_layer, &iter)) {
			if (sprite->GetVid() == vid) {
				sprite->ScalarDeletingDestructor(1);
			}
		}
		return 0;
	}
	case 170: { // script: ReplaceUnit - swap every old-vid sprite for a new one in place
		VID* newVid = PopVid(
			// STRING: ALIEN 0x484058
			"for Replace Unit 2"
		);
		VID* oldVid = PopVid(
			// STRING: ALIEN 0x484044
			"for Replace Unit 1"
		);
		if (newVid == EmptyVid || oldVid == EmptyVid) {
			return 0;
		}
		int iter;
		for (SPRITE* sprite = FirstSprite(oldVid->m_layer, &iter); sprite;
			 sprite = NextSprite(oldVid->m_layer, &iter)) {
			if (sprite->GetVid() == oldVid) {
				CreateSprite(newVid, sprite->GetX(), sprite->GetY(), sprite->GetZ(), sprite->Direction(), 0);
				sprite->ScalarDeletingDestructor(1);
			}
		}
		return 0;
	}
	case 171: { // script: CRC(str) - push the CRC32 checksum of a string
		commands = *PopStr();
		CRC32 crc((unsigned char*) commands.m_str, (int) commands.Length());
		PushInt((int) crc.m_crc);
		return 0;
	}
	case 172: { // script: Format(fmt, arg) - printf-style with one string or int argument
		if (m_logic.m_stack.IsLastString()) {
			STRING arg(*PopStr());
			PushStr(Printf(PopStr()->m_str, arg.m_str));
		}
		else {
			int arg = PopInt();
			PushStr(Printf(PopStr()->m_str, arg));
		}
		return 0;
	}
	case 173: // script: ReloadVid()
		ReloadVid();
		return 0;
	case 174: { // script: WriteLine(text, stream) - append a string and newline to a file
		commands = *PopStr();
		FILE* stream = ScriptFile(PopInt());
		if (!stream) {
			return 0;
		}
		commands.Write_file(stream);
		fseek(stream, -1, SEEK_CUR);
		fputs("\n", stream);
		return 0;
	}
	case 175: { // script: ReadLine(stream) - read a string, honoring demo record/playback
		FILE* stream = ScriptFile(PopInt());
		if (m_flag & 0x200) {
			commands.Read_res(&m_resource);
		}
		else if (stream) {
			commands.Read_file(stream);
		}
		if (m_flag & 0x100) {
			commands.Write(&m_resource);
		}
		PushStr(commands);
		return 0;
	}
	case 176: { // script: OpenFileRead(name) - open for reading, error if missing
		commands = *PopStr();
		if (m_flag & 0x200) {
			PushInt(0);
			return 0;
		}
		FILE* stream = FOpen(
			&commands.m_str,
			// STRING: ALIEN 0x484040
			"r+t"
		);
		if (!stream) {
			Error(7, commands.m_str, 0);
		}
		PushInt(ScriptFileOpen(stream));
		return 0;
	}
	case 177: // script: CloseFile(stream)
		ScriptFileClose(PopInt());
		return 0;
	case 178: { // script: OpenFileWrite(name)
		commands = *PopStr();
		if (m_flag & 0x200) {
			PushInt(0);
			return 0;
		}
		PushInt(ScriptFileOpen(FOpen(
			&commands.m_str,
			// STRING: ALIEN 0x48403c
			"w+t"
		)));
		return 0;
	}
	case 179: { // script: IsEOF(stream) - true at end of file or for a null stream
		FILE* stream = ScriptFile(PopInt());
		// The original read the MSVC FILE struct's _IOEOF bit directly.
		PushInt(!stream || feof(stream));
		return 0;
	}
	case 182: { // script: GetReg(path, name, def)
		STRING def(*PopStr());
		STRING name(*PopStr());
		STRING path(*PopStr());
		// Keep --red-blood transient.
		if (Settings_ForceRedBlood() && Registry && !strcmp(path.m_str, Registry->m_path.m_str) &&
			!strcmp(name.m_str, "Blood")) {
			m_logic.PushStr(STRING("1"));
			return 0;
		}
		REGISTRY reg;
		reg.m_path = path;
		m_logic.PushStr(reg.GetString(name, def));
		return 0;
	}
	case 183: { // script: SetReg(path, name, value)
		STRING value(*PopStr());
		STRING name(*PopStr());
		STRING path(*PopStr());
		REGISTRY reg;
		reg.m_path = path;
		reg.SetString(name, value);
		return 0;
	}
	case 184: { // script: RegistryDelete(path, name) - delete a value under a registry path
		STRING name(*PopStr());
		STRING path(*PopStr());
		REGISTRY reg;
		reg.m_path = path;
		reg.Delete(name);
		return 0;
	}
	case 185: // script: GetDefaultRegPath()
		m_logic.PushStr(Registry->m_path);
		return 0;
	case 186: { // script: FSaveData(path, value)
		STRING value(*PopStr());
		STRING path(*PopStr());
		FileData_Save(path.m_str, value.m_str);
		return 0;
	}
	case 187: { // script: FLoadData(path, def_value)
		STRING def(*PopStr());
		STRING path(*PopStr());
		PushStr(STRING(FileData_Load(path.m_str, def.m_str)));
		return 0;
	}
	case 188:
		PushInt(FileData_FileExists(PopStr()->m_str));
		return 0;
	case 189: // script: SaveFolder()
		PushStr(STRING(FileData_SaveFolder()));
		return 0;
	case 159: // script: StrLen(text)
	case 205:
		PushInt((int) PopStr()->Length());
		return 0;
	case 206: // script: StrLower(text)
		PushStr(PopStr()->ToLower());
		return 0;
	case 207: // script: StrUpper(text)
		PushStr(PopStr()->ToUpper());
		return 0;
	case 208: { // script: ToBase64(text, key)
		int key = PopInt();
		PushStr(PopStr()->ToBase64(key));
		return 0;
	}
	case 210: // script: StoreSetAchievement(achievement_id)
		Platform_StoreSetAchievement(PopStr()->m_str);
		return 0;
	case 211: // script: StoreGetAchievement(achievement_id)
		PushInt(Platform_StoreGetAchievement(PopStr()->m_str));
		return 0;
	case 213: // script: StoreResetAllStats()
		if (Game_IsZS1()) {
			PushInt(ZS1_CountUnitsInMap(this));
			return 0;
		}
		Platform_StoreResetAllStats();
		return 0;
	case 214: { // script: StoreSetStat(stats_id, value)
		int value = PopInt();
		Platform_StoreSetStat(PopStr()->m_str, value);
		return 0;
	}
	case 215: // script: StoreGetStat(stats_id)
		PushInt(Platform_StoreGetStat(PopStr()->m_str));
		return 0;
	case 216: // script: StoreSaveStatsIfNeed()
		Platform_StoreSaveStatsIfNeeded();
		return 0;
	case 217: { // script: StoreActivateGameOverlayToStore(app_id)
		int appId = PopInt();
		if (appId <= 0) {
			appId = ALIEN_STEAM_APPID;
		}
		char url[64];
		snprintf(url, sizeof(url), "https://store.steampowered.com/app/%i", appId);
		SDL_OpenURL(url);
		return 0;
	}
	case 218: // script: StoreInitLeaderboards(names)
		Platform_StoreInitLeaderboards(PopStr()->m_str);
		return 0;
	case 219: { // script: StoreUpdateLeaderboard(leaderboard_name, new_score)
		int score = PopInt();
		STRING name(*PopStr());
		STRING player(FileData_Load("save://common/PlayerName", "Player"));
		Platform_StoreUpdateLeaderboard(name.m_str, player.m_str, score);
		return 0;
	}
	case 220: { // script: StoreDownloadLeaderboardEntries(leaderboard_name, no_entries, offset)
		int offset = PopInt();
		int count = PopInt();
		PushInt(Platform_StoreDownloadLeaderboardEntries(PopStr()->m_str, count, offset));
		return 0;
	}
	case 221: // script: StoreGetLeaderboardEntriesName(index)
		PushStr(STRING(Platform_StoreLeaderboardEntryName(PopInt())));
		return 0;
	case 222: // script: StoreGetLeaderboardEntriesScore(index)
		PushInt(Platform_StoreLeaderboardEntryScore(PopInt()));
		return 0;
	case 223: // script: GetUpdateLeaderboardRank()
		PushInt(Platform_StoreLastUploadRank());
		return 0;
	case 212: { // script: TrainInfo(engine, query) | StoreClearAchievement(achievement_id)
		if (GameData_IsSteam()) {
			Platform_StoreClearAchievement(PopStr()->m_str);
			return 0;
		}
		int query = PopInt();
		SPRITE* sprite = (SPRITE*) PopObject();
		if (!sprite || !sprite->IsClass(0x15)) {
			PushInt(0);
			return 0;
		}
		ENGINE* engine = (ENGINE*) sprite;
		TRAIN_INFO info(engine);
		switch (query) {
		case 1:
			PushInt(info.m_unk0x18);
			return 0;
		case 2:
			PushInt(info.m_unk0x28);
			return 0;
		case 3: // script: load as a 0..100 percentage
			PushInt(info.m_unk0x20 * 100 / info.m_unk0x24);
			return 0;
		case 4:
			PushInt(info.m_unk0x20);
			return 0;
		case 5:
			PushInt(info.m_unk0x3c);
			return 0;
		case 6:
			PushInt(info.Acceleration());
			return 0;
		case 7:
			PushInt(info.m_unk0x2c);
			return 0;
		case 9: {
			for (ENGINE* e = engine->FirstEngine(); e; e = e->NextEngine()) {
				if (!e->IsCommand(0)) {
					PushInt(0);
				}
			}
			PushInt(1);
			return 0;
		}
		case 10:
			PushInt(info.m_unk0x34);
			return 0;
		case 11:
			PushInt(info.m_unk0x38);
			return 0;
		default:
			PushInt(0);
			return 0;
		}
	}
	case 231: {
		if (Game_IsZS1()) {
			STRING name(*PopStr());
			for (int i = 0; i < m_menu.m_n; ++i) {
				SPRITE* sprite = (SPRITE*) m_menu.m_data[i];
				const char* itemName = sprite->m_exData ? sprite->m_exData->m_spriteName.m_str : "";
				if (!strcmp(itemName, name.m_str)) {
					m_logic.PushObject(sprite);
					return 0;
				}
			}
			m_logic.PushObject(0);
			return 0;
		}
		int d = PopInt();
		int value = PopInt();
		int y = PopInt();
		int x = PopInt();
		RailMap.SetSemaphoreOrMine(x, y, value, d);
		return 0;
	}
	case 232:
		if (!Game_IsZS1()) {
			MYERROR::Log(::Error, "!!!ERROR!!!LOGIC: Unknown extern Function %i", p_cmd);
			return 0;
		}
		m_logic.PushObject(m_menu.m_underCursor);
		return 0;
	case 255: {
		if (!Game_IsZS1()) {
			MYERROR::Log(::Error, "!!!ERROR!!!LOGIC: Unknown extern Function %i", p_cmd);
			return 0;
		}
		STRING str2(*PopStr());
		STRING str1(*PopStr());
		int var2 = PopInt();
		int var1 = PopInt();
		int id = PopInt();
		int outInt = 0;
		const void* outObj = 0;
		STRING outStr;
		switch (ZS1_SendCommand2(this, id, var1, var2, str1.m_str, str2.m_str, &outInt, &outObj, &outStr)) {
		case ZS1_CMD_STR:
			PushStr(outStr);
			break;
		case ZS1_CMD_OBJ:
			m_logic.PushObject(outObj);
			break;
		default:
			PushInt(outInt);
			break;
		}
		return 0;
	}
	case 78: {
		if (!Game_IsZS1()) {
			MYERROR::Log(::Error, "!!!ERROR!!!LOGIC: Unknown extern Function %i", p_cmd);
			return 0;
		}
		decomp_intptr var3 = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Value();
		decomp_intptr var2 = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Value();
		decomp_intptr var1 = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Value();
		int act = ((LOGICSTACK*) m_logic.m_stack.m_data)[--m_logic.m_stack.m_n].Int();
		STRING name(*PopStr());
		for (int layer = 0; layer < LayerCount(); ++layer) {
			for (int i = m_layers[layer].m_n - 1; i >= 0; --i) {
				if (i >= m_layers[layer].m_n) {
					i = m_layers[layer].m_n;
					continue;
				}
				SPRITE* sprite = (SPRITE*) m_layers[layer].m_data[i];
				const char* spriteName = sprite->m_exData ? sprite->m_exData->m_spriteName.m_str : "";
				if (!strcmp(spriteName, name.m_str)) {
					sprite->Action(act, var1, var2, var3);
				}
			}
		}
		return 0;
	}
	case 239: { // script: BreakTrain(engine, x, y) - split a train at a point
		int y = PopInt();
		int x = PopInt();
		ENGINE* engine = (ENGINE*) PopObject();
		if (engine) {
			engine->BreakTrain((float) x, (float) y);
		}
		return 0;
	}
	case 240: // script: FirstTrain(army)
		PushObject(FirstTrain(PopInt()));
		return 0;
	case 241: // script: NextTrain()
		PushObject(NextTrain());
		return 0;
	case 242: { // script: SetCommandToTrain(engine, cmd, param)
		int param = PopInt();
		int cmd = PopInt();
		SPRITE* sprite = (SPRITE*) PopObject();
		if (sprite && sprite->IsClass(0x15)) {
			((ENGINE*) sprite)->SetCommandToTrain(25, cmd, param);
		}
		else {
			MYERROR::Log(
				::Error,
				// STRING: ALIEN 0x48400c
				"\xc1\xee\xf0\xe8\xf1, \xf3 \xf2\xe5\xe1\xff \xe2 PatrolTrain - train \xed\xe5\xe2\xe5\xf0\xed\xfb\xe9 "
				"%p",
				(const void*) sprite
			);
		}
		return 0;
	}
	case 243: { // script: SetPushLine(x1, y1, x2, y2, value) on the rail map
		int value = PopInt();
		int y2 = PopInt();
		int x2 = PopInt();
		int y1 = PopInt();
		int x1 = PopInt();
		RailMap.SetPushLine(x1, y1, x2, y2, value);
		return 0;
	}
	case 244: // script: CursorScreenX()
		PushInt((int) m_input.m_x);
		return 0;
	case 245: // script: CursorScreenY()
		PushInt((int) m_input.m_y);
		return 0;
	case 246: // script: SetCleverAttack(on) for the arcade player
		((PLAYER_ARCADE*) Player(1))->SetCleverAttack(PopInt());
		return 0;
	case 247:
		PopInt();
		return 0;
	case 249: { // script: AddUnitLimit(limit, vid, index) - set a per-vid unit cap
		int index = PopInt();
		VID* vid = PopVid(
			// STRING: ALIEN 0x483ff8
			"for AddUnitLimit"
		);
		int limit = PopInt();
		if (vid == EmptyVid) {
			return 0;
		}
		if (index == 0xff) {
			vid->m_unk0x394[0] = limit;
		}
		else {
			vid->m_unk0x394[index + 1] = limit;
		}
		return 0;
	}
	case 250: { // script: toggle the 0x2 map flag
		int on = PopInt();
		m_flag = (m_flag & ~0x2u) | (on ? 0x2 : 0);
		return 0;
	}
	case 251: { // script: SetMoney(army, money)
		int money = PopInt();
		Player(PopInt())->SetMoney(money);
		return 0;
	}
	case 252: // script: GetMoney(army)
		PushInt((int) Player(PopInt())->GetMoney());
		return 0;
	case 253: // script: reserved - consumes two ints and an object
		PopInt();
		PopInt();
		PopObject();
		return 0;
	case 254: // script: reserved - consumes two objects
		PopObject();
		PopObject();
		return 0;
	default:
		MYERROR::Log(
			::Error,
			// STRING: ALIEN 0x483fc8
			"!!!ERROR!!!LOGIC: Unknown extern Function %i",
			p_cmd
		);
		return 0;
	}
}

// FUNCTION: ALIEN 0x43a330
int MAP::PopInt()
{
	LOGICSTACK* e = (LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n;
	if (e->m_type & 1) {
		const char* s = e->m_str.m_str;
		if (s[1] != 'x') {
			return atoi(s);
		}
		int v;
		sscanf(s, "%i", &v);
		return v;
	}
	return e->m_num;
}

// FUNCTION: ALIEN 0x43a470
decomp_intptr MAP::PopObject()
{
	LOGICSTACK* top = (LOGICSTACK*) m_logic.m_stack.m_data + m_logic.m_stack.m_n;
	if (top[-1].m_num && !(top[-1].m_type & 0x10)) {
		MYERROR::Error(
			::Error,
			"LOGIC",
			10,
			// STRING: ALIEN 0x48416c
			"this variable is not unit",
			0
		);
	}
	LOGICSTACK* e = (LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n;
	if (e->m_type & 1) {
		const char* s = e->m_str.m_str;
		if (s[1] != 'x') {
			return atoi(s);
		}
		int v;
		// STRING: ALIEN 0x47f7c4
		sscanf(s, "%i", &v);
		return v;
	}
	return e->m_num;
}

// FUNCTION: ALIEN 0x43a510
VID* MAP::PopVid(const char* p_context)
{
	LOGICSTACK* e = (LOGICSTACK*) m_logic.m_stack.m_data + --m_logic.m_stack.m_n;
	int nvid;
	if (e->m_type & 1) {
		const char* s = e->m_str.m_str;
		if (s[1] != 'x') {
			nvid = atoi(s);
		}
		else {
			int v;
			sscanf(s, "%i", &v);
			nvid = v;
		}
	}
	else {
		nvid = e->m_num;
	}
	VID* vid;
	if (nvid < 0 || nvid >= m_noVid || (vid = m_vids[nvid]) == 0) {
		vid = EmptyVid;
	}
	if (vid == EmptyVid && ::Error) {
		MYERROR::Log(::Error, "!!!ERROR!!!SCRIPT: Invalid nvid %s %i", p_context, nvid);
	}
	return vid;
}

// FUNCTION: ALIEN 0x43a5b0
void MAP::PushInt(int p_value)
{
	m_logic.m_stack.Insert(LOGICSTACK(p_value));
}

// FUNCTION: ALIEN 0x43a640
void MAP::PushStr(const STRING& p_value)
{
	m_logic.m_stack.Insert(LOGICSTACK(p_value));
}

// FUNCTION: ALIEN 0x43a720
void MAP::PushObject(const void* p_object)
{
	LOGICSTACK value(p_object);
	m_logic.m_stack.Insert(value);
}

// FUNCTION: ALIEN 0x43a7b0
int MAP::Error(int p_type, const char* p_msg, int p_size)
{
	MYERROR* handler = ::Error;
	int result = 0;
	if (handler) {
		result = MYERROR::Error(handler, "MAP", p_type, p_msg, p_size);
	}
	return result;
}

// FUNCTION: ALIEN 0x43a7e0
PLAYER* MAP::Player(int p_army)
{
	return m_player[p_army & 3];
}

// FUNCTION: ALIEN 0x448fc0
char** MAP::GetVariableStr(char** p_out, STRING p_name)
{
	m_logic.GetVariableStr(p_out, p_name);
	return p_out;
}
