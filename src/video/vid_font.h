#ifndef VID_FONT_H
#define VID_FONT_H

#include "video/vid.h"

class CD3DFont;
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
	void RestoreFont();
	void ReleaseFont();
	void Load(RESOURCE* p_res); // vtable+0x18
	int Draw(SPRITE* p_sprite);
	void SetLayer();

	CD3DFont* m_font; // 0x484
};

DECOMP_SIZE_ASSERT(VID_FONT, 0x488)

#endif
