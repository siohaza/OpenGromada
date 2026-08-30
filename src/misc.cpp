#include "misc.h"

#include "util/game_random.h"

#include <stdlib.h>

// FUNCTION: ALIEN 0x406130
int Sqrt(int p_value)
{
	int result = 0;
	int bit = 0x40000000;
	do {
		if (p_value >= bit + result) {
			p_value -= bit + result;
			result = (result >> 1) | bit;
		}
		else {
			result >>= 1;
		}
		bit >>= 2;
	} while (bit);
	return result;
}

// FUNCTION: ALIEN 0x43cd80
double sqr(double p_value)
{
	return p_value * p_value;
}

// FUNCTION: ALIEN 0x4455b0
int Random(int p_max)
{
	return GameRand() % (p_max + 1);
}
