#ifndef PICTURE_FONT_H
#define PICTURE_FONT_H

#include "gfx/picture_makevid.h"

// VTABLE: ALIEN 0x47a51c

class PICTURE_FONT : public PICTURE_MAKEVID {
public:
	virtual int NextFrame();
	virtual int Rewind();
	virtual int Load(STRING p_name, STRING p_alpha, STRING p_z);

	PICTURE_MAKEVID m_source; // 0x430
};

// SYNTHETIC: ALIEN 0x4128f0
// PICTURE_FONT::`scalar deleting destructor'

#endif
