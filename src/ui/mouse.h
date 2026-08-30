#ifndef MOUSE_H
#define MOUSE_H

#include "sprite/sprite.h"

// VTABLE: ALIEN 0x47a8a8

class MOUSE : public SPRITE {
public:
	MOUSE(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent);
	~MOUSE();
	virtual void* ScalarDeletingDestructor(unsigned int p_flags);

	int m_unk0x70;       // 0x70
	void* m_cursors[36]; // 0x74
	int m_hardware;      // 0x104

	void Enable();
	void Disable();
	void Draw();
	void ChangeAnimation(int p_ani);
	decomp_intptr Action(int p_action, decomp_intptr p_a, decomp_intptr p_b, decomp_intptr p_c);
	void HardwareOn();
	void HardwareOff();
};

extern MOUSE* Mouse;

#endif
