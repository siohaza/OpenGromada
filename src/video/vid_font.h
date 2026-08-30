#ifndef VID_FONT_H
#define VID_FONT_H

#include "video/vid.h"

class DEBUG_FONT;
class RESOURCE;
class STREAM;

// VTABLE: ALIEN 0x47a4b8

class VID_FONT : public VID {
public:
	VID_FONT() { m_font = 0; }
	VID_FONT(VID_FONT& p_other);
	virtual void* ScalarDeletingDestructor(unsigned int p_flags);
	~VID_FONT();
	VID* CreateMirror();
	void Load(RESOURCE* p_res); // vtable+0x18
	int Draw(SPRITE* p_sprite);
	void SetLayer();

	DEBUG_FONT* m_font; // 0x484
};

#endif
