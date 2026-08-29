#ifndef UNIT_H
#define UNIT_H

#include "game/terrain.h"

// VTABLE: ALIEN 0x47a8c8

class UNIT : public TERRAIN {
public:
	UNIT(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent);

	int m_unk0x78; // 0x78
	int m_unk0x7c; // 0x7c
	int m_ammo; // 0x80
	int m_unk0x84; // 0x84
	int m_turn; // 0x88
	int m_unk0x8c; // 0x8c

	decomp_intptr Action(int p_action, int p_a, int p_b, int p_c);
	void MoveTact();
	void DrawSecondaryInfo();
	~UNIT();
	virtual void* ScalarDeletingDestructor(unsigned int p_flags);

	int AddAmmoTick(int p_ammo);
};

DECOMP_SIZE_ASSERT(UNIT, 0x90)

#endif
