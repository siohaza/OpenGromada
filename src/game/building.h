#ifndef BUILDING_H
#define BUILDING_H

#include "game/unit.h"

// VTABLE: ALIEN 0x47a9fc

class BUILDING : public UNIT {
public:
	BUILDING(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent);

	VID* m_buildVid; // 0x90
	int m_unk0x94; // 0x94
	int m_unk0x98; // 0x98
	int m_unk0x9c; // 0x9c
	int m_unk0xa0; // 0xa0

	decomp_intptr Action(int p_action, int p_a, int p_b, int p_c);
	void MoveTact();
};

#endif
