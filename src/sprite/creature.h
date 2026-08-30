#ifndef CREATURE_H
#define CREATURE_H

#include "game/unit.h"

// VTABLE: ALIEN 0x47aa5c

class CREATURE : public UNIT {
public:
	CREATURE(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent);

	decomp_intptr Action(int p_action, decomp_intptr p_a, decomp_intptr p_b, decomp_intptr p_c);
	void MoveTact();
	void DeletePointerToSprite(SPRITE* p_sprite);

	SPRITE* FindRegion(float p_x, float p_y);

	int m_unk0x90;    // 0x90
	SPRITE* m_region; // 0x94
};

#endif
