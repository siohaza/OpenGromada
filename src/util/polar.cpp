#include "util/polar.h"

#include <math.h>

// GLOBAL: ALIEN 0x4813f0
static int g_cordicTable[8] = {64, 37, 19, 10, 5, 2, 1};

// FUNCTION: ALIEN 0x406000
ANGLE Decart2Polar(int p_x, int p_y, int* p_dist)
{
	int x = p_x;
	int y, angle;
	if (p_x < 0) {
		x = -p_x;
		angle = 384;
		y = -p_y;
	}
	else {
		y = p_y;
		angle = 128;
	}
	for (int shift = 0; shift < 7; ++shift) {
		int xs;
		int ys;
		if (y >= 0) {
			xs = x;
			ys = y;
			x += ys >> shift;
			y -= xs >> shift;
			angle += g_cordicTable[shift];
		}
		else {
			xs = x;
			ys = y;
			x -= ys >> shift;
			y += xs >> shift;
			angle -= g_cordicTable[shift];
		}
	}
	if (p_dist)
		*p_dist = (636750 * x) >> 20;
	return ANGLE((char) (angle >> 1));
}

// FUNCTION: ALIEN 0x406080
ANGLE Decart2Polar_f(float p_x, float p_y)
{
	if (p_y == 0.0f) {
		if (p_x == 0.0f)
			return ANGLE(0);
		if (p_x > 0.0f)
			return ANGLE(64);
		return ANGLE(-64);
	}

	float angle = (float) atan(-(p_x / p_y)) * 40.743668f;
	if (p_y < 0.0f) {
		if (p_x < 0.0f)
			angle += 256.0f;
	}
	else {
		angle += 128.0f;
	}
	return ANGLE((char) (int) angle);
}
