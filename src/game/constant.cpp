#include "game/constant.h"

#include "util/myerror.h"
#include "util/resource.h"

// FUNCTION: ALIEN 0x42df40
CONSTANT::CONSTANT(RESOURCE* p_res)
{
	RESOURCE* res = p_res;
	int ignored;
	if (res->GoBegin(0x54534e43)) {
		MYERROR::Log(::Error,
			// STRING: ALIEN 0x483ab8
			"!!!ERROR!!! CNST Load Constant section not found");
		return;
	}
	res->Read(&m_walkSpeed, 4);
	res->Read(&m_runSpeed, 4);
	res->Read(&m_acceleration, 4);
	res->Read(&m_deceleration, 4);
	res->Read(&m_unk0x10, 4);
	res->Read(&m_unk0x14, 4);
	res->Read(&m_unk0x18, 4);
	res->Read(&m_unk0x1c, 4);
	res->Read(&m_unk0x20, 4);
	res->Read(&m_unk0x24, 4);
	res->Read(&ignored, 4);
	res->Read(&m_unk0x2c, 4);
	res->Read(&m_unk0x30, 4);
	res->Read(&m_unk0x34, 4);
	res->Read(&m_unk0x38, 4);
	res->Read(&m_unk0x3c, 4);
	res->Read(&m_unk0x40, 4);
	res->Read(&m_unk0x44, 4);
	res->Read(&m_unk0x48, 4);
	res->Read(&m_unk0x4c, 4);
	res->Read(&m_unk0x50, 4);
	res->Read(&m_unk0x54, 4);
	res->Read(&m_unk0x58, 4);
	res->Read(&m_unk0x5c, 4);
	res->Read(&m_unk0x60, 4);
	res->Read(&m_unk0x64, 4);
	float walk = m_walkSpeed;
	m_walkSpeed = walk * 0.001f;
	float run = m_runSpeed;
	*(volatile float*) &m_runSpeed = run * 0.001f;
	float acc = m_acceleration;
	m_acceleration = acc * 0.000001f;
	float dec = m_deceleration;
	m_deceleration = dec * 0.000001f;
	float v1c = m_unk0x1c;
	m_unk0x1c = v1c * 0.001f;
	float v18 = m_unk0x18;
	m_unk0x18 = v18 * 0.001f;
	float v60 = m_unk0x60;
	m_unk0x60 = v60 * 0.001f;
}
