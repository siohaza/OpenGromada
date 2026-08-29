#ifndef FRAME_H
#define FRAME_H

#include "sprite/sprite.h"

// VTABLE: ALIEN 0x47a92c

class FRAME : public SPRITE {
public:
	FRAME(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent);
	~FRAME();
	virtual void* ScalarDeletingDestructor(unsigned int p_flags);

	decomp_intptr Action(int p_action, int p_a, int p_b, int p_c);
};

DECOMP_SIZE_ASSERT(FRAME, 0x70)

#endif
