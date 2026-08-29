#ifndef PLANE_H
#define PLANE_H

#include "game/unit.h"

// VTABLE: ALIEN 0x47aa1c

class PLANE : public UNIT {
public:
	PLANE(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent);

	decomp_intptr Action(int p_action, int p_a, int p_b, int p_c);

	virtual int PlaneNextCommand(int p_a, int p_b); // vtable+0x20
	virtual void ZSpeedInitialization(); // vtable+0x24
	virtual void CheckFlightProperties(); // vtable+0x28
	virtual int WayBlocked(); // vtable+0x2c
	virtual int FlightIfWayBlocked(); // vtable+0x30
	virtual int FlightToTarget(); // vtable+0x34
	virtual int FlightToTargetAdditionalActions(); // vtable+0x38
	virtual int FreeFlight(); // vtable+0x3c
};

DECOMP_SIZE_ASSERT(PLANE, 0x90)

#endif
