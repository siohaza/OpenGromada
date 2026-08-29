#ifndef PRIMITIVE_H
#define PRIMITIVE_H

#include "sprite/sprite.h"

// VTABLE: ALIEN 0x47a368

class PRIMITIVE : public SPRITE {
public:
	PRIMITIVE(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent);

	void Tact();
	void MoveTact() {}
	void DeletePointerToSprite(SPRITE*) {}
	void DrawSecondaryInfo() {}
};

DECOMP_SIZE_ASSERT(PRIMITIVE, 0x70)

#endif
