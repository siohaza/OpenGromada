#ifndef VID_LIGHT_H
#define VID_LIGHT_H

#include "video/vid.h"

class STREAM;
class RESOURCE;

// VTABLE: ALIEN 0x47a490

class VID_LIGHT : public VID {
public:

	VID_LIGHT()
	{
		m_unk0x484 = 0;
		m_unk0x488 = 0;
	}
	VID_LIGHT(VID_LIGHT& p_other);
	virtual void* ScalarDeletingDestructor(unsigned int p_flags);
	~VID_LIGHT();
	VID* CreateMirror();
	void SetLayer();
	int Draw(SPRITE* p_sprite);
	void Load(RESOURCE* p_res); // vtable+0x18

	undefined4 m_unk0x484; // 0x484
	undefined4 m_unk0x488; // 0x488
};

DECOMP_SIZE_ASSERT(VID_LIGHT, 0x48c)

#endif
