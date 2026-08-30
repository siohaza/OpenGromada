#ifndef CANNON_H
#define CANNON_H

#include "sprite/sprite.h"

// VTABLE: ALIEN 0x47a8f4

class CANNON : public SPRITE {
public:
	CANNON(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent);

	void MoveTact();
	decomp_intptr Action(int p_action, decomp_intptr p_a, decomp_intptr p_b, decomp_intptr p_c);
	void DeletePointerToSprite(SPRITE* p_sprite);

	int m_unk0x70; // 0x70
};

#endif
