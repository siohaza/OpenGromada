#include "sprite/frame.h"

#include "game/map.h"
#include "sprite/list_sprite.h"

// FUNCTION: ALIEN 0x448a60
FRAME::FRAME(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent)
	: SPRITE(p_vid, (float) Map->AbsX(p_x), (float) Map->AbsY(p_y), p_z, p_dir, p_parent)
{
	Map->m_menu.Insert(this);
}

// FUNCTION: ALIEN 0x448ad0
void* FRAME::ScalarDeletingDestructor(unsigned int p_flags)
{
	FRAME* result = this;
	this->~FRAME();
	if (p_flags & 1)
		operator delete(result);
	return result;
}

// FUNCTION: ALIEN 0x448af0
FRAME::~FRAME()
{
	MENU* menu = &Map->m_menu;
	if (menu->m_underCursor == this)
		menu->m_underCursor = 0;
	menu->Delete(this);
	Map->DeletePointerToSprite(this);
}

// FUNCTION: ALIEN 0x448b30
decomp_intptr FRAME::Action(int p_action, int p_a, int p_b, int p_c)
{
	if (p_action >= 0 && (p_action <= 5 || p_action == 0x82)) {
		if (p_action > 5) {
			int ani = m_ani;
			if (ani < 0xf) {
				if (ani == 4) {
					ChangeAnimation(2);
					m_flag |= 0x200;
				}
				else if (ani == 5) {
					ChangeAnimation(3);
					m_flag |= 0x200;
				}
				if (m_ani == 0xe)
					ChangeAnimation(0);
			}
		}
		return 0;
	}
	return SPRITE::Action(p_action, p_a, p_b, p_c);
}
