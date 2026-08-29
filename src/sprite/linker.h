#ifndef LINKER_H
#define LINKER_H

#include "util/decomp.h"
#include "sprite/sprite.h"

class VID;

// VTABLE: ALIEN 0x47a390
class LINKER : public SPRITE {
public:
	LINKER(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent);
	~LINKER() { BreakLink(); }
	virtual void* ScalarDeletingDestructor(unsigned int p_flags);

	float m_dx; // 0x70
	float m_dy; // 0x74
	float m_dz; // 0x78
	ANGLE m_ddir; // 0x7c
	SPRITE* m_owner; // 0x80

	void LinkRotate(ANGLE p_dir);
};

DECOMP_SIZE_ASSERT(LINKER, 0x84)

#endif
