#ifndef DEPO_H
#define DEPO_H

#include "game/unit.h"

// VTABLE: ALIEN 0x47a9dc

class DEPO : public UNIT {
public:
	DEPO(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent);
	~DEPO();
	virtual void* ScalarDeletingDestructor(unsigned int p_flags);

	int m_unk0x90; // 0x90
	short m_queue[100]; // 0x94
	int m_buildTicks[100]; // 0x15c
	int m_unk0x2ec[100]; // 0x2ec
	int m_unk0x47c; // 0x47c
	int m_queueMax; // 0x480
	int m_queueLen; // 0x484

	decomp_intptr Action(int p_action, int p_a, int p_b, int p_c);
	void MoveTact();
	void AddUnitToQueue(int p_vid);
	void BuildNextUnit();
	int ActionBuildUnit(int p_a, int p_b);
};

#endif
