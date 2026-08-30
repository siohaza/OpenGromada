#ifndef ANGLE_H
#define ANGLE_H

#include "util/decomp.h"

class ANGLE {
public:
	unsigned char m_dir; // 0x00

	static float SinTable[256];
	static float CosTable[256];
	static float SinTable2[256];
	static float CosTable2[256];

	ANGLE() {}
	ANGLE(unsigned char p_dir);
	// FUNCTION: ALIEN 0x405ee0
	ANGLE(const ANGLE& p_other) { m_dir = p_other.m_dir; }
	ANGLE(float p_x, float p_y);

	ANGLE(float p_x, float p_y, int* p_dist);
	float Sin() const;
	float Cos() const;
	ANGLE& operator=(const ANGLE& p_other);
	ANGLE operator+(const ANGLE& p_other);
	ANGLE operator-(const ANGLE& p_other);
	ANGLE Invert() const;
};

static_assert(sizeof(ANGLE) == 1, "ANGLE is serialized as one direction byte");

inline void AngleAssign(ANGLE* p_dst, const ANGLE& p_src)
{
	if (p_dst != &p_src) {
		p_dst->m_dir = p_src.m_dir;
	}
}

#endif
