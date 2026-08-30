#ifndef BALLOON_H
#define BALLOON_H

#include "sprite/plane.h"

// VTABLE: ALIEN 0x47aa7c

class BALLOON : public PLANE {
public:
	BALLOON(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent);

	void MoveTact();
	void MoveToNearestBase();
	void ConnectToBase();
	void ZSpeedInitialization();
	void CheckFlightProperties();
	int FlightToTargetAdditionalActions();

	int m_unk0x90;       // 0x90
	char m_landingState; // 0x94

	int IsBalloonMoveFinished();
	int SetCommand(int p_cmd, SPRITE* p_goal);
};

#endif
