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
#ifdef DECOMP_ANGLE_CTOR_OUT_OF_LINE
	ANGLE(unsigned char p_dir);
#else
	// FUNCTION: ALIEN 0x405a70
	ANGLE(unsigned char p_dir) { m_dir = p_dir; }
#endif
	// FUNCTION: ALIEN 0x405ee0
	ANGLE(const ANGLE& p_other) { m_dir = p_other.m_dir; }
	ANGLE(float p_x, float p_y);

	ANGLE(float p_x, float p_y, int* p_dist);
	float Sin() const;
	float Cos() const;
#ifdef DECOMP_ANGLE_ASSIGN_INLINE

	ANGLE& operator=(const ANGLE& p_other)
	{
		if (this != &p_other)
			m_dir = p_other.m_dir;
		return *this;
	}
#else
	ANGLE& operator=(const ANGLE& p_other);
#endif
	ANGLE operator+(const ANGLE& p_other);
	ANGLE operator-(const ANGLE& p_other);
	ANGLE Invert() const;
};

DECOMP_SIZE_ASSERT(ANGLE, 0x1)

inline void AngleAssign(ANGLE* p_dst, const ANGLE& p_src)
{
	if (p_dst != &p_src)
		p_dst->m_dir = p_src.m_dir;
}

#endif
