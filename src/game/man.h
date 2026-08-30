#ifndef MAN_H
#define MAN_H

#include "game/unit.h"

// VTABLE: ALIEN 0x47ab00

class MAN : public UNIT {
public:
	MAN(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent);

	decomp_intptr Action(int p_action, decomp_intptr p_a, decomp_intptr p_b, decomp_intptr p_c);
	void MoveTact();
	int ChangeWeapon(int p_weapon);
	~MAN();
	virtual void* ScalarDeletingDestructor(unsigned int p_flags);

	int m_ammo[10]; // 0x90
};

#endif
