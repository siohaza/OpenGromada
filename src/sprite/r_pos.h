#ifndef R_POS_H
#define R_POS_H

#include "util/angle.h"
#include "util/decomp.h"

class R_DOT;
class STREAM;
class SPRITE;
class ENGINE;

class R_POS {
public:
	R_POS() : m_dot(0), m_pos(0), m_unk0x08(0), m_link(0) {}

	R_DOT* m_dot;  // 0x00
	int m_pos;     // 0x04
	int m_unk0x08; // 0x08
	int m_link;    // 0x0c

	R_DOT* LinkedDot() const;
	int LinkDist() const;
	int LinkBackLink() const;
	ANGLE LinkedDotAngle() const;
	ANGLE GetAngle() const;
	R_DOT* GetLinkedDot();
	int Write(STREAM* p_stream) const;
	int Read(STREAM* p_stream);
	int NoStepToTarget(R_DOT* p_goal, SPRITE* p_target, int p_command, ENGINE* p_eng);
	int DoStep(R_DOT* p_goal, SPRITE* p_target, ENGINE* p_engine);
};

#endif
