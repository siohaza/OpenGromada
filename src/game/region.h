#ifndef REGION_H
#define REGION_H

#include "util/decomp.h"
#include "gfx/color.h"
#include "sprite/sprite.h"

class VID;

// VTABLE: ALIEN 0x47a94c
class REGION : public SPRITE {
public:
	REGION(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent);
	~REGION();
	virtual void* ScalarDeletingDestructor(unsigned int p_flags);

	int m_unk0x70; // 0x70
	int m_unk0x74; // 0x74
	void* m_fogTable; // 0x78
	int m_flag; // 0x7c
	int m_fogZ2; // 0x80
	int m_fogZ1; // 0x84
	COLOR m_fogColor; // 0x88
	int m_unk0x8c; // 0x8c
	float m_w; // 0x90
	float m_h; // 0x94
	VID* m_unk0x98; // 0x98
	int m_unk0x9c; // 0x9c
	VID* m_vidFrom[12]; // 0xa0

	static VID* ConvertVid(VID* p_vid, float p_x, float p_y, float p_z);

	int IsInsideXY(float p_x, float p_y) const
	{
		float x = m_x;
		float halfW = m_w * 0.5f;
		if (x - halfW <= p_x && p_x <= halfW + x) {
			float y = m_y;
			float halfH = m_h * 0.5f;
			if (y - halfH <= p_y && p_y <= halfH + y)
				return 1;
		}
		return 0;
	}
	VID* Draw();
	decomp_intptr Action(int p_action, int p_a, int p_b, int p_c);
	void SetFogParameters(int p_z1, int p_z2, COLOR p_color);
	void DrawSecondaryInfo();

	float X1Scr() const;
	float X2Scr() const;
	float Y1Scr() const;
	float Y2Scr() const;
};

DECOMP_SIZE_ASSERT(REGION, 0xd0)

#endif
