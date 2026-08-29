#ifndef PICTURE_FLIC_H
#define PICTURE_FLIC_H

#include "gfx/picture_base.h"

// VTABLE: ALIEN 0x47a754
class PICTURE_FLIC : public PICTURE_BASE {
public:
	virtual int NextFrame();
	virtual int Load(const STRING& p_name);

	unsigned short m_type; // 0x42c
};

DECOMP_SIZE_ASSERT(PICTURE_FLIC, 0x430)

#endif
