#define DECOMP_INLINE_STRING_DTOR
#include "ui/mousetips.h"

#include <string.h>

#include "game/const.h"
#include "game/gametime.h"
#include "game/input_as.h"
#include "game/map.h"
#include "gfx/graph.h"
#include "sprite/sprite.h"
#include "util/string.h"
#include "video/vid.h"

// GLOBAL: ALIEN 0x5da538
static unsigned int MouseLastMove;

// GLOBAL: ALIEN 0x5da53c
static float OldX;

// GLOBAL: ALIEN 0x5da540
static float OldY;

// STUB: ALIEN 0x43da80
void MOUSETIPS::Tact(INPUT_AS* p_input)
{

	// GLOBAL: ALIEN 0x5da544
	static STRING OldString;

	if (OldX != p_input->m_x || OldY != p_input->m_y) {
		MouseLastMove = RealCurrentTime;
		OldX = p_input->m_x;
		OldY = p_input->m_y;
	}
	if (RealCurrentTime - MouseLastMove > Const->m_unk0x34 && !(p_input->m_button & 1)
		&& !p_input->m_key && !(Map->m_menu.m_state & 1) && (Map->m_flag & 0x100000)) {
		if (!m_sprite) {
			VID* vid = Map->GetVid(6);
			OldString = Map->GetMouseTipsString();
			if (vid != EmptyVid && strcmp(OldString.m_str, empty_str)) {
				float x = p_input->m_x + 5.0f;
				float y = p_input->m_y - vid->m_footprintHeight + 3000.0f - 10.0f;
				float half = vid->m_footprintHeight * 0.5f;
				if (half + y - 3000.0f <= (double) Graph->m_viewYMin)
					y = half + p_input->m_y + 3010.0f;
				if ((int) (strlen(OldString.m_str) + 2) * vid->m_footprintWidth + x >=
					(double) Graph->m_viewXMax)
					x = (double) Graph->m_viewXMax -
						(int) (strlen(OldString.m_str) + 2) * vid->m_footprintWidth;
				m_sprite = Map->CreateSprite(vid, x, y, 3000.0f, ANGLE(0), 0);
				if (m_sprite) {
					STRING tip = "{" + OldString + "}";
					m_sprite->Action(120, (int) &tip, 0, 0);
				}
			}
			return;
		}
		if (RealCurrentTime - MouseLastMove <= Const->m_unk0x34 + 500)
			return;
		MouseLastMove += 500;
		if (strcmp(Map->GetMouseTipsString().m_str, OldString.m_str) != 0)
			Clear();
		return;
	}
	Clear();
}
// FUNCTION: ALIEN 0x43dde0
void MOUSETIPS::Clear()
{
	SPRITE* sprite = m_sprite;
	if (sprite)
		((DELETABLE*) sprite)->vf00(1);
	m_sprite = 0;
}

// FUNCTION: ALIEN 0x43de00
SPRITE* MOUSETIPS::DeletePointerToSprite(SPRITE* p_sprite)
{
	SPRITE* result = m_sprite;
	if (result == p_sprite)
		m_sprite = 0;
	return result;
}
