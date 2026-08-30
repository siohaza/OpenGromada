
#include "ui/mouse.h"

#include "game/map.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "platform/cursor.h"
#include "platform/render.h"
#include "util/myerror.h"
#include "video/vid.h"
#include "world/hash_map.h"

#include <string.h>

// GLOBAL: ALIEN 0x4845b4
static const char* g_cursorNames[36] = {
	// STRING: ALIEN 0x4847b8
	"arrow",
	// STRING: ALIEN 0x4847b0
	"noammo",
	// STRING: ALIEN 0x4847a8
	"move",
	// STRING: ALIEN 0x4847a0
	"clash",
	// STRING: ALIEN 0x484798
	"repair",
	// STRING: ALIEN 0x484790
	"attack",
	// STRING: ALIEN 0x484784
	"farattack",
	// STRING: ALIEN 0x48477c
	"select",
	// STRING: ALIEN 0x484774
	"nomove",
	// STRING: ALIEN 0x48476c
	"cycle",
	"link",
	// STRING: ALIEN 0x484764
	"unlink",
	// STRING: ALIEN 0x484758
	"cantmove",
	// STRING: ALIEN 0x484750
	"patrol",
	// STRING: ALIEN 0x484748
	"delete",
	// STRING: ALIEN 0x484740
	"capture",
	// STRING: ALIEN 0x484738
	"mine",
	// STRING: ALIEN 0x484730
	"unmine",
	// STRING: ALIEN 0x484724
	"small-arrow",
	// STRING: ALIEN 0x484714
	"small-noammo",
	// STRING: ALIEN 0x484708
	"small-move",
	// STRING: ALIEN 0x4846fc
	"small-taran",
	// STRING: ALIEN 0x4846ec
	"small-repair",
	// STRING: ALIEN 0x4846dc
	"small-attack",
	// STRING: ALIEN 0x4846cc
	"small-farattack",
	// STRING: ALIEN 0x4846bc
	"small-select",
	// STRING: ALIEN 0x4846ac
	"small-nomove",
	"cycle",
	// STRING: ALIEN 0x4846a0
	"small-link",
	// STRING: ALIEN 0x484690
	"small-unlink",
	// STRING: ALIEN 0x484680
	"small-cantmove",
	// STRING: ALIEN 0x484670
	"small-patrol",
	"delete",
	// STRING: ALIEN 0x484660
	"small-capture",
	// STRING: ALIEN 0x484654
	"small-mine",
	// STRING: ALIEN 0x484644
	"small-unmine"
};

// GLOBAL: ALIEN 0x5da550
MOUSE* Mouse;

// FUNCTION: ALIEN 0x446c90
MOUSE::MOUSE(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent)
	: SPRITE(p_vid, p_x, p_y, p_z, p_dir, p_parent)
{
	m_hardware = 0;
	void** cursors = m_cursors;
	for (int i = 0; i < 36; ++i) {
		cursors[i] = 0;
	}
	m_unk0x70 = 1;
	if (m_vid != EmptyVid) {
		Remove();
	}
	else if (m_child) {
		m_child->Remove();
	}
	if (m_vid == EmptyVid) {
		++m_noRef;
	}
}

// FUNCTION: ALIEN 0x446d20
void MOUSE::Draw()
{
	if (m_unk0x70) {
		return;
	}
	if (m_hardware) {
		m_vid->Draw(this);
	}
}

// FUNCTION: ALIEN 0x446d40
void* MOUSE::ScalarDeletingDestructor(unsigned int p_flags)
{
	MOUSE* result = this;
	this->~MOUSE();
	if (p_flags & 1) {
		operator delete(result);
	}
	return result;
}

// FUNCTION: ALIEN 0x446d60
MOUSE::~MOUSE()
{
	MYERROR::Log(
		::Error,
		// STRING: ALIEN 0x4847c0
		"Mouse  release"
	);
}

// FUNCTION: ALIEN 0x446d90
void MOUSE::Enable()
{
	if (!m_hardware) {
		m_hardware = 1;
		Platform_SetCursor(0);
		if (m_unk0x70) {
			for (int i = 0; i < 36; ++i) {
				const char* name = g_cursorNames[i];
				SDL_Cursor** cursor = (SDL_Cursor**) &m_cursors[i];
				if (*cursor) {
					Platform_FreeCursor(*cursor);
					*cursor = 0;
				}
				if (name && *name) {
					// STRING: ALIEN 0x4847d8
					STRING file("cursores\\");
					file += name;
					// STRING: ALIEN 0x4847d0
					file += ".ani";
					*cursor = Platform_LoadCursor(file.m_str);
				}
				if (!*cursor) {
					*cursor = Platform_DefaultCursor();
				}
			}
			Platform_SetCursor((SDL_Cursor*) m_cursors[m_ani]);
		}
	}
}

// FUNCTION: ALIEN 0x446ef0
void MOUSE::Disable()
{
	if (m_hardware) {
		int hw = m_unk0x70;
		m_hardware = 0;
		if (hw) {
			Platform_SetCursor(0);
			for (int i = 0; i < 36; ++i) {
				if (m_cursors[i]) {
					Platform_FreeCursor((SDL_Cursor*) m_cursors[i]);
					m_cursors[i] = 0;
				}
			}
		}
	}
}

// FUNCTION: ALIEN 0x446f40
void MOUSE::HardwareOn()
{
	int result = m_unk0x70;
	if (!result) {
		Disable();
		m_vid = EmptyVid;
		m_unk0x70 = 1;
		Enable();
	}
}

// FUNCTION: ALIEN 0x446f70
void MOUSE::HardwareOff()
{
	if (m_unk0x70) {
		Disable();
		VID* vid;
		if (Map->m_noVid <= 1 || (vid = Map->m_vids[1]) == 0) {
			vid = EmptyVid;
		}
		int ani = m_ani;
		m_vid = vid;
		SPRITE::ChangeAnimation(ani == 0);
		SPRITE::ChangeAnimation(ani);
		m_unk0x70 = 0;
		Enable();
	}
}

// FUNCTION: ALIEN 0x446fd0
void MOUSE::ChangeAnimation(int p_ani)
{
	if (m_unk0x70) {
		if (m_ani != p_ani && m_hardware && p_ani >= 0 && p_ani < 36) {
			Platform_SetCursor((SDL_Cursor*) m_cursors[p_ani]);
		}
		if (p_ani < 17) {
			SPRITE::ChangeAnimation(p_ani);
			return;
		}
		m_ani = p_ani;
	}
	else {
		SPRITE::ChangeAnimation(p_ani);
	}
}

// FUNCTION: ALIEN 0x447030
decomp_intptr MOUSE::Action(int p_action, decomp_intptr p_a, decomp_intptr p_b, decomp_intptr p_c)
{
	switch (p_action) {
	case 0x3d:
		ChangeAnimation(p_a);
		return 0;
	case 0x3f:
		Platform_WarpMouse((float) p_a, (float) p_b);
		return 0;
	case 0x3e: {
		int nvid = p_a;
		if (nvid < 0) {
			return 0;
		}
		if (nvid >= Map->m_noVid) {
			return 0;
		}
		if (!Map->m_vids[nvid]) {
			return 0;
		}
		VID* vid = m_vid;
		if (vid && nvid == vid->m_idx) {
			return 0;
		}
		Insert();
		SPRITE::Action(0x3e, nvid, p_b, p_c);
		MOUSE* child = this;
		if (child) {
			do {
				Hash->Delete(child);
				child = (MOUSE*) child->m_child;
			} while (child);
		}
		Remove();
		return 0;
	}
	default:
		return SPRITE::Action(p_action, p_a, p_b, p_c);
	}
}
