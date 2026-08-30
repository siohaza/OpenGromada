#ifndef RAIL_H
#define RAIL_H

#include "game/terrain.h"

class R_DOT;

// VTABLE: ALIEN 0x47a970

class RAIL : public TERRAIN {
public:
	RAIL(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent);
	~RAIL();
	virtual void* ScalarDeletingDestructor(unsigned int p_flags);

	R_DOT* m_dot1; // 0x78
	R_DOT* m_dot2; // 0x7c

	decomp_intptr Action(int p_action, decomp_intptr p_dir, decomp_intptr p_a, decomp_intptr p_b);
	void UnBreak(R_DOT* p_dot);
};

#endif
