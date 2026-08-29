#ifndef GAMMA_H
#define GAMMA_H

#include "util/decomp.h"
#include "gfx/color.h"

class GAMMA {
public:
	enum GAMMA_CREATE { DECODE };

	enum RAW_COPY_TAG { RAW_COPY };

	int m_a; // 0x00
	int m_b; // 0x04

	GAMMA()
	{
#ifdef DECOMP_GAMMA_DEFAULT_CTOR_ZERO
		m_a = 0;
		m_b = 0;
#endif
	}
	GAMMA(const GAMMA& p_other);
	GAMMA(RAW_COPY_TAG, const GAMMA& p_other)
	{
		m_a = p_other.m_a;
		m_b = p_other.m_b;
	}

	GAMMA(RAW_COPY_TAG, int p_a, int p_b)
	{
		m_a = p_a;
		m_b = p_b;
	}

	GAMMA(COLOR p_neg, COLOR p_pos);
	GAMMA(GAMMA_CREATE p_tag, unsigned int p_packed);

	GAMMA(int p_a, int p_r, int p_g, int p_b)
	{
		if (p_a < -255)
			p_a = -255;
		else if (p_a > 255)
			p_a = 255;
		unsigned int neg = 0;
		unsigned int pos = 0;
		if (p_a >= 0)
			pos = p_a << 24;
		else
			neg = -p_a << 24;
		if (p_r < -255)
			p_r = -255;
		else if (p_r > 255)
			p_r = 255;
		neg &= 0xff00ffff;
		pos &= 0xff00ffff;
		if (p_r >= 0)
			pos |= p_r << 16;
		else
			neg |= -p_r << 16;
		if (p_g < -255)
			p_g = -255;
		else if (p_g > 255)
			p_g = 255;
		neg &= 0xffff00ff;
		pos &= 0xffff00ff;
		if (p_g >= 0)
			pos |= p_g << 8;
		else
			neg |= -p_g << 8;
		if (p_b < -255)
			p_b = -255;
		else if (p_b > 255)
			p_b = 255;
		neg &= 0xffffff00;
		pos &= 0xffffff00;
		m_a = neg;
		m_b = pos;
		if (p_b >= 0)
			m_b = pos | p_b;
		else
			m_a = -p_b | neg;
	}

	GAMMA* Add(GAMMA p_a, GAMMA p_b);
	GAMMA* SetRed(int p_red);
	GAMMA* SetGreen(int p_green);
	GAMMA* SetBlue(int p_blue);
};

DECOMP_SIZE_ASSERT(GAMMA, 0x8)

#ifndef DECOMP_GAMMA_DECODE_OUT_OF_LINE

inline GAMMA::GAMMA(GAMMA_CREATE, unsigned int p_packed)
{
	m_a = 0;
	m_b = 0;
	if ((p_packed & 0x80) != 0)
		m_b = 2 * (~p_packed & 0x7f);
	else
		m_a = 2 * (p_packed & 0x7f);
	if ((p_packed & 0x8000) != 0)
		m_b |= 2 * (~p_packed & 0x7f80);
	else
		m_a |= 2 * (p_packed & 0x7f80);
	if ((p_packed & 0x800000) != 0)
		m_b |= 2 * (~p_packed & 0x7f8000);
	else
		m_a |= 2 * (p_packed & 0x7f8000);
	if ((p_packed & 0x80000000) != 0)
		m_b |= 2 * (~p_packed & 0xff800000);
	else
		m_a |= 2 * (p_packed & 0xff800000);
}
#endif

int CountGamma(int p_g1, int p_g2, int p_time);

#endif
