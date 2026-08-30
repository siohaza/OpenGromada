#ifndef VID_HARDWARE_Z_H
#define VID_HARDWARE_Z_H

#include "video/vid_software.h"

// VTABLE: ALIEN 0x47a460

class VID_HARDWARE_Z : public VID_SOFTWARE {
public:
	VID_HARDWARE_Z() {}
	VID_HARDWARE_Z(STREAM* p_stream) : VID_SOFTWARE(p_stream) {}

	VID* CreateMirror();
	int Draw(SPRITE* p_sprite);
	void SetLayer();

	int SetGamma(const GAMMA& p_gamma, unsigned int p_flags);

private:
	int DrawFrame(int p_frame, float p_x, float p_y, float p_z, float p_shiftX, float p_shiftY, const GAMMA& p_gamma);

	friend struct VID_HARDWARE_Z_TEST_ACCESS;
};

#endif
