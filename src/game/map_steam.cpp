#define DECOMP_INLINE_MAP_NEXTSPRITE
#define DECOMP_INLINE_MAP_NEXTSPRITE_CURSOR
#define DECOMP_INLINE_MAP_FIRSTSPRITE
#define DECOMP_INLINE_STRING_CHARP_CTOR
#define DECOMP_INLINE_STRING_COPY_CTOR
#define DECOMP_INLINE_STRING_DTOR
#define DECOMP_INLINE_LIST_SPRITE_ITERATE

#define DECOMP_GAMMA_DEFAULT_CTOR_ZERO
#include "game/map_steam.h"

#include "game/building.h"
#include "game/depo.h"
#include "audio/sound.h"
#include "game/const.h"
#include "game/engine.h"
#include "game/rail.h"
#include "game/terrain.h"
#include "sprite/balloon.h"
#include "sprite/civ_robot.h"
#include "sprite/creature.h"
#include "game/region.h"
#include "game/gametime.h"
#include "gfx/gamma.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "sprite/plane.h"
#include "world/hash_map.h"

// FUNCTION: ALIEN 0x405060
MAP_STEAM::MAP_STEAM(HINSTANCE p_instance, HINSTANCE p_prevInstance, STRING& p_argv,
	int p_showCmd, SETTINGS* p_settings)
	: MAP(p_instance, p_prevInstance, p_argv, p_showCmd, p_settings)
{
	if (m_flag & 4) {
		m_unk0x22c0_d &= 0xfffffff0;
		Load(m_scriptName);
	}
}

// FUNCTION: ALIEN 0x405130
MAP_STEAM::~MAP_STEAM()
{
	m_mousetips.Clear();
	Release();
}

// FUNCTION: ALIEN 0x405160
void MAP_STEAM::DeletePointerToSprite(SPRITE* p_sprite)
{
	m_mousetips.DeletePointerToSprite(p_sprite);
	MAP::DeletePointerToSprite(p_sprite);
}

// GLOBAL: ALIEN 0x4905f4
static unsigned int s_bannerTime;

// STUB: ALIEN 0x405190
int MAP_STEAM::Tact()
{
	if (StartTact())
		return 1;
	if (Const->m_mapName) {

		switch (m_input.m_key) {
		case '~':
			ReloadVid();
			break;
		case 'O':
			m_flag ^= (m_flag ^ ~m_flag) & 0x8000;
			m_flag ^= (m_flag ^ ~m_flag) & 0x800;
			break;
		case 'P':
			m_unk0x22c0_d ^= (m_unk0x22c0_d ^ ~m_unk0x22c0_d) & 1;
			break;
		case 'I':
			m_flag ^= (m_flag ^ ~m_flag) & 0x1000;
			break;
		case 'H':
			m_unk0x22c0_d ^= (m_unk0x22c0_d ^ ~m_unk0x22c0_d) & 4;
			break;
		case 'R':
			m_flag ^= (m_flag ^ ~m_flag) & 0x2000;
			break;
		case 'G':
			m_unk0x22c0_d ^= (m_unk0x22c0_d ^ ~m_unk0x22c0_d) & 8;
			break;
		}
		if ((!(m_flag & 8) && !(m_unk0x22c0_d & 2)) || (((GRAPH_CORE*) Graph)->m_flags & 1)
			|| ((m_unk0x22c0_d & 1) && m_input.m_key != 'p')) {
			CurrentTime = PrevCurrentTime;
			WaitMessage();
			return 0;
		}
	}
	else if ((!(m_flag & 8) && !(m_unk0x22c0_d & 2)) || (((GRAPH_CORE*) Graph)->m_flags & 1)) {
		CurrentTime = PrevCurrentTime;
		WaitMessage();
		return 0;
	}

	int draw = ((m_flag & 8) || !(((GRAPH_CORE*) Graph)->m_flags & 0x80)) && !Graph->m_movie.m_graph;
	if (!draw || !((GRAPH_CORE*) Graph)->PreTact()) {
		++m_noTact;
		int demo = DemoTact();
		if (demo == 999999) {
			((GRAPH_CORE*) Graph)->PostTact(1);
			return 0;
		}
		int active = demo && draw;

		((PLANE*) this)->PLANE::CheckFlightProperties();
		m_menu.Control(&m_input);
		ScriptRun(-1, 0, 0, 0);
		if (active && Graph->m_movie.m_graph) {
			active = 0;
			draw = 0;
		}
		if (Flagman(m_curArmy) && !(Flagman(m_curArmy)->m_flag & 0x1800) && Const->m_unk0x50) {
			{
				GAMMA gamma(GAMMA::DECODE, Const->m_unk0x50);
				SPRITE* sprite = Flagman(m_curArmy);
				sprite->SetGamma(gamma);
			}
			MAN* flagman = Flagman(m_curArmy);
			SPRITE* child = flagman->m_child;
			if (child) {
				if (child->m_vid == flagman->m_vid->m_unk0x5c) {
					GAMMA gamma(GAMMA::DECODE, Const->m_unk0x50);
					MAN* sprite = Flagman(m_curArmy);
					sprite->m_child->SetGamma(gamma);
				}
			}
		}
		if (SpriteUnderCursor() && (m_flag & 0x100000) && (Const->m_unk0x58 || Const->m_unk0x54)) {
			GAMMA gamma(GAMMA::DECODE,
				(SpriteUnderCursor()->m_flag & 0x1800) == 0x800 ? Const->m_unk0x54 : Const->m_unk0x58);
			SpriteUnderCursor()->SetGamma(gamma);
			SPRITE* cursor = SpriteUnderCursor();
			SPRITE* child = cursor->m_child;
			if (child) {
				if (child->m_vid == cursor->m_vid->m_unk0x5c) {
					SPRITE* sprite = SpriteUnderCursor()->m_child;
					sprite->SetGamma(gamma);
				}
			}
		}
		Graph->Tact(active);
		if (Flagman(m_curArmy)) {
			if (!(Flagman(m_curArmy)->m_flag & 0x1800)) {
				if (Const->m_unk0x50) {
					{
						GAMMA gamma;
						SPRITE* sprite = Flagman(m_curArmy);
						sprite->SetGamma(gamma);
					}
					MAN* flagman = Flagman(m_curArmy);
					SPRITE* child = flagman->m_child;
					if (child) {
						if (child->m_vid == flagman->m_vid->m_unk0x5c) {
							GAMMA gamma;
							MAN* sprite = Flagman(m_curArmy);
							sprite->m_child->SetGamma(gamma);
						}
					}
				}
			}
		}
		if (SpriteUnderCursor() && (Const->m_unk0x58 || Const->m_unk0x54)) {
			SpriteUnderCursor()->SetGamma(GAMMA());
			SPRITE* cursor = SpriteUnderCursor();
			SPRITE* child = cursor->m_child;
			if (child) {
				if (child->m_vid == cursor->m_vid->m_unk0x5c) {
					GAMMA gamma;
					SPRITE* sprite = SpriteUnderCursor()->m_child;
					sprite->SetGamma(gamma);
				}
			}
		}
		if (m_flag & 0x10) {
			for (int i = m_menu.m_n - 1; i >= 0; --i)
				((SPRITE*) m_menu.m_data[i])->Tact();
		}
		else {
			for (int layer = 0; layer < 16; ++layer) {
				int iter = m_layers[layer].m_n;
				SPRITE* sprite;
				for (sprite = FirstSprite(layer, &iter); sprite; sprite = NextSprite(layer, &iter))
					sprite->Tact();
			}
		}
		m_mousetips.Tact(&m_input);
		ControlShiftCoor();
		if (!(m_flag & 0x10) && (m_flag & 0x80)) {
			if ((m_flag & 0x40000) && RealCurrentTime - s_bannerTime > 2000) {
				SPRITE* banner = Map->CreateSprite(Map->Vid(2),
					((GRAPH_CORE*) Graph)->GetWidth() * 0.5f, 32.0f, 0.0f, ANGLE(0), 0);
				STRING text(
					// STRING: ALIEN 0x47f7a0
					"Presentation version. Not for sale!");
				s_bannerTime = RealCurrentTime;
				banner->Action(95, 1, 0, 0);
				banner->Action(120, (int) &text, 0, 0);
				banner->Action(40, 1000, 0, 0);
				banner->m_actions.InsertFirst(ACT(15, 0, 0, 0));
			}
			for (int i = 0; i < 4; ++i)
				m_player[i]->Control(&m_input);
		}
		if (active)
			DrawSecondaryInfo();
		if (draw)
			((GRAPH_CORE*) Graph)->PostTact(1);
		Sound->Tact();
	}
	return 0;
}

// FUNCTION: ALIEN 0x405ac0
void MAP_STEAM::Release()
{
	m_flag &= 0xfffffffd;
	ENGINE::PathDots.DeleteAll();
	m_mousetips.Clear();
	MAP::Release();
}

// FUNCTION: ALIEN 0x405af0
void MAP_STEAM::DrawSecondaryInfo()
{
	MAP::DrawSecondaryInfo();
	if (m_unk0x22c0[0] & 4) {
		SPRITE* s = (SPRITE*) Hash->m_list.LastIterate(&Hash->m_iter);
		while (s) {
			if (!s->m_parent)
				GRAPH_CORE::PrintfXY((GRAPH_CORE*) Graph, s->m_x - Map->m_shiftX,
									 s->m_y - s->m_z - Map->m_shiftY, "%i", s->m_unk0x54);
			HASH_MAP* h = Hash;
			if (h->m_iter > h->m_list.m_n)
				h->m_iter = h->m_list.m_n;
			int idx = h->m_iter - 1;
			h->m_iter = idx;
			if (idx < 0)
				break;
			s = (SPRITE*) h->m_list.m_data[idx];
		}
	}
	if (m_unk0x22c0[0] & 8) {
		SPRITE* s = (SPRITE*) Hash->m_list.LastIterate(&Hash->m_iter);
		while (s) {
			s->DrawGoalLine();
			HASH_MAP* h = Hash;
			if (h->m_iter > h->m_list.m_n)
				h->m_iter = h->m_list.m_n;
			int idx = h->m_iter - 1;
			h->m_iter = idx;
			if (idx < 0)
				break;
			s = (SPRITE*) h->m_list.m_data[idx];
		}
	}
}

// FUNCTION: ALIEN 0x405bd0
SPRITE* MAP_STEAM::CreateSprite(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir,
	SPRITE* p_parent)
{
	if (!p_vid)
		return 0;
	VID* vid = p_vid;
	if (vid->m_unk0x47c & 0x10)
		vid = REGION::ConvertVid(vid, p_x, p_y, p_z);
	int limit = vid->m_unk0x394[0];
	if (limit >= 0
		&& (int) (vid->m_entitiesNumber[0] + vid->m_entitiesNumber[1] + vid->m_entitiesNumber[2]
			   + vid->m_entitiesNumber[3]) >= limit)
		return 0;
	SPRITE* sprite;
	switch (vid->m_sprClass) {
	case 0:
	case 1:
		sprite = new TERRAIN(vid, p_x, p_y, p_z, p_dir, p_parent);
		break;
	case 22:
		sprite = new RAIL(vid, p_x, p_y, p_z, p_dir, p_parent);
		break;
	case 21:
		sprite = new ENGINE(vid, p_x, p_y, p_z, p_dir, p_parent);
		break;
	case 26:
		sprite = new BALLOON(vid, p_x, p_y, p_z, p_dir, p_parent);
		break;
	case 3:
		sprite = new BUILDING(vid, p_x, p_y, p_z, p_dir, p_parent);
		break;
	case 24:
		sprite = new DEPO(vid, p_x, p_y, p_z, p_dir, p_parent);
		break;
	case 20:
		sprite = new CIV_ROBOT(vid, p_x, p_y, p_z, p_dir, p_parent);
		break;
	case 25:
		sprite = new CREATURE(vid, p_x, p_y, p_z, p_dir, p_parent);
		break;
	default:
		return MAP::CreateSprite(vid, p_x, p_y, p_z, p_dir, p_parent);
	}
	if (!(m_flag & 0x20) && sprite) {
		int script = sprite->m_vid->m_unk0x408[14];
		if (script >= 0)
			ScriptRun(script, sprite, 0, 0);
	}
	return sprite;
}
