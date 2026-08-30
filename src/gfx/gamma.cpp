#include "gfx/gamma.h"

// FUNCTION: ALIEN 0x414f80
GAMMA* GAMMA::Add(GAMMA p_a, GAMMA p_b)
{
	unsigned char* a = (unsigned char*) &p_a;
	const unsigned char* b = (const unsigned char*) &p_b;
	for (int i = 0; i < 8; ++i) {
		unsigned int sum = (unsigned int) a[i] + (unsigned int) b[i];
		a[i] = (unsigned char) (sum > 255 ? 255 : sum);
	}
	*this = p_a;
	return this;
}

// FUNCTION: ALIEN 0x414fb0
GAMMA::GAMMA(const GAMMA& p_other)
{
	m_a = p_other.m_a;
	m_b = p_other.m_b;
}

// FUNCTION: ALIEN 0x431b60
COLOR::COLOR(int p_a, int p_r, int p_g, int p_b)
{
	if (p_a < 0) {
		p_a = 0;
	}
	else if (p_a > 255) {
		p_a = 255;
	}
	if (p_r < 0) {
		p_r = 0;
	}
	else if (p_r > 255) {
		p_r = 255;
	}
	if (p_g < 0) {
		p_g = 0;
	}
	else if (p_g > 255) {
		p_g = 255;
	}
	if (p_b < 0) {
		p_b = 0;
	}
	else if (p_b > 255) {
		p_b = 255;
	}
	m_value = ((((p_a << 8) | p_r) << 8 | p_g) << 8) | p_b;
}

// FUNCTION: ALIEN 0x431be0
GAMMA::GAMMA(COLOR p_neg, COLOR p_pos)
{
	m_a = ~p_neg.m_value;
	m_b = p_pos.m_value;
}

// FUNCTION: ALIEN 0x439a60
GAMMA::GAMMA(GAMMA_CREATE, unsigned int p_packed)
{
	m_a = 0;
	m_b = 0;
	if ((p_packed & 0x80) != 0) {
		m_b = 2 * (~p_packed & 0x7f);
	}
	else {
		m_a = 2 * (p_packed & 0x7f);
	}
	if ((p_packed & 0x8000) != 0) {
		m_b |= 2 * (~p_packed & 0x7f80);
	}
	else {
		m_a |= 2 * (p_packed & 0x7f80);
	}
	if ((p_packed & 0x800000) != 0) {
		m_b |= 2 * (~p_packed & 0x7f8000);
	}
	else {
		m_a |= 2 * (p_packed & 0x7f8000);
	}
	if ((p_packed & 0x80000000) != 0) {
		m_b |= 2 * (~p_packed & 0xff800000);
	}
	else {
		m_a |= 2 * (p_packed & 0xff800000);
	}
}

static char CountByteGamma(int p_a, int p_b, int p_time);

// FUNCTION: ALIEN 0x43a950
int CountGamma(int p_g1, int p_g2, int p_time)
{
	int result = CountByteGamma(p_g1 & 0xff, p_g2 & 0xff, p_time) & 0xff;
	result |= (CountByteGamma((p_g1 >> 8) & 0xff, (p_g2 >> 8) & 0xff, p_time) & 0xff) << 8;
	return result | ((CountByteGamma((p_g1 >> 16) & 0xff, (p_g2 >> 16) & 0xff, p_time) & 0xff) << 16);
}

// FUNCTION: ALIEN 0x43a9c0
static char CountByteGamma(int p_a, int p_b, int p_time)
{
	if (p_a >= 128) {
		p_a -= 254;
	}
	if (p_b >= 128) {
		p_b -= 254;
	}
	char result = (char) p_a;
	result += p_time * (p_b - p_a) / 255;
	return result;
}

// FUNCTION: ALIEN 0x4432e0
GAMMA* GAMMA::SetRed(int p_red)
{
	if (p_red < -255) {
		p_red = -255;
	}
	else if (p_red > 255) {
		p_red = 255;
	}
	m_a &= 0xff00ffff;
	m_b &= 0xff00ffff;
	if (p_red < 0) {
		m_a |= -p_red << 16;
	}
	else {
		m_b |= p_red << 16;
	}
	return this;
}

// FUNCTION: ALIEN 0x443340
GAMMA* GAMMA::SetGreen(int p_green)
{
	if (p_green < -255) {
		p_green = -255;
	}
	else if (p_green > 255) {
		p_green = 255;
	}
	m_a &= 0xffff00ff;
	m_b &= 0xffff00ff;
	if (p_green < 0) {
		m_a |= -p_green << 8;
	}
	else {
		m_b |= p_green << 8;
	}
	return this;
}

// FUNCTION: ALIEN 0x4433a0
GAMMA* GAMMA::SetBlue(int p_blue)
{
	if (p_blue < -255) {
		p_blue = -255;
	}
	else if (p_blue > 255) {
		p_blue = 255;
	}
	m_a &= 0xffffff00;
	m_b &= 0xffffff00;
	if (p_blue < 0) {
		m_a |= -p_blue;
	}
	else {
		m_b |= p_blue;
	}
	return this;
}
