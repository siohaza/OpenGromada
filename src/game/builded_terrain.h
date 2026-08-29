#ifndef BUILDED_TERRAIN_H
#define BUILDED_TERRAIN_H

#include "sprite/sprite.h"

class VID;

// VTABLE: ALIEN 0x47a3d0
class BUILDED_TERRAIN : public SPRITE {
public:
	BUILDED_TERRAIN(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent);

	VID* Draw();
};

DECOMP_SIZE_ASSERT(BUILDED_TERRAIN, 0x70)

#endif
