#ifndef FRAME_H
#define FRAME_H

#include "sprite/sprite.h"

// VTABLE: ALIEN 0x47a92c

class FRAME : public SPRITE {
public:
	FRAME(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent);
	~FRAME();
	virtual void* ScalarDeletingDestructor(unsigned int p_flags);

	decomp_intptr Action(int p_action, decomp_intptr p_a, decomp_intptr p_b, decomp_intptr p_c);
};

#endif
