#ifndef PICTURE_Z_H
#define PICTURE_Z_H

#include "gfx/picture_base.h"

// VTABLE: ALIEN 0x47a740

class PICTURE_Z : public PICTURE_BASE {
public:
	virtual int NextFrame();
	virtual int Load(const STRING& p_name);
};

#endif
