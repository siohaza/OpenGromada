#ifndef TERRAIN_H
#define TERRAIN_H

#include "sprite/sprite.h"

// VTABLE: ALIEN 0x47a9bc

class TERRAIN : public SPRITE {
public:
	TERRAIN(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent);

	decomp_intptr Action(int p_action, int p_a, int p_b, int p_c);
	int Repair(int p_full);
	int AskCell(float p_x, float p_y);

	int Action();

	int m_repairProgress; // 0x70
	int m_unk0x74; // 0x74

	void AddHpPerSecond(int p_hp);
};

DECOMP_SIZE_ASSERT(TERRAIN, 0x78)

#endif
