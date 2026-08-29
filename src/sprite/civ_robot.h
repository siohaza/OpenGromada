#ifndef CIV_ROBOT_H
#define CIV_ROBOT_H

#include "sprite/creature.h"
#include "util/decomp.h"
#include "util/myerror.h"
#include "video/vid.h"

class VID;

// VTABLE: ALIEN 0x47a994
class CIV_ROBOT : public CREATURE {
public:
	CIV_ROBOT(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent);
	virtual void* ScalarDeletingDestructor(unsigned int p_flags);

	static int RobotBuildingVids[16];

	int m_state; // 0x98
	PTR_SPRITE m_target; // 0x9c
	int m_unk0xa0; // 0xa0
	int m_unk0xa4; // 0xa4

	void MoveTact();
	decomp_intptr Action(int p_action, int p_a, int p_b, int p_c);
	int IsRobotBuilding(const SPRITE* p_sprite);
	SPRITE* FindRobotBuilding();
	void PathIsBlocked();
	void RotateHead(ANGLE p_dir);
	void DeletePointerToSprite(SPRITE* p_sprite);
	void ChangeAnimation(int p_ani);
};

#endif
