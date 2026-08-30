#include "game/constant.h"

#include "util/myerror.h"
#include "util/resource.h"

#include <string.h>

// FUNCTION: ALIEN 0x42df40
CONSTS::CONSTS(RESOURCE* p_res)
{
	memset(this, 0, sizeof(*this));
	RESOURCE* res = p_res;
	int ignored;
	if (res->GoBegin(0x54534e43)) {
		MYERROR::Log(
			::Error,
			// STRING: ALIEN 0x483ab8
			"!!!ERROR!!! CNST Load Constant section not found"
		);
		return;
	}
	res->Read(&m_unk0x00, 4);
	res->Read(&m_unk0x04, 4);
	res->Read(&m_unk0x08, 4);
	res->Read(&m_unk0x0c, 4);
	res->Read(&m_repairByRepairHp, 4);
	res->Read(&m_addAmmo, 4);
	res->Read(&m_unk0x18, 4);
	res->Read(&m_unk0x1c, 4);
	res->Read(&m_unk0x20, 4);
	res->Read(&m_unk0x24, 4);
	res->Read(&ignored, 4);
	res->Read(&m_unk0x2c, 4);
	res->Read(&m_unk0x30, 4);
	res->Read(&m_unk0x34, 4);
	res->Read(&m_depoAddHp, 4);
	res->Read(&m_buildingAddHp, 4);
	res->Read(&m_unk0x40, 4);
	res->Read(&m_repairDockTime, 4);
	res->Read(&m_unk0x48, 4);
	res->Read(&m_balloonAddAmmo, 4);
	res->Read(&m_unk0x50, 4);
	res->Read(&m_unk0x54, 4);
	res->Read(&m_unk0x58, 4);
	res->Read(&m_unk0x5c, 4);
	res->Read(&m_minMoveSpeed, 4);
	res->Read(&m_unk0x64, 4);
	float walk = m_unk0x00;
	m_unk0x00 = walk * 0.001f;
	float run = m_unk0x04;
	m_unk0x04 = run * 0.001f;
	float acc = m_unk0x08;
	m_unk0x08 = acc * 0.000001f;
	float dec = m_unk0x0c;
	m_unk0x0c = dec * 0.000001f;
	float v1c = m_unk0x1c;
	m_unk0x1c = v1c * 0.001f;
	float v18 = m_unk0x18;
	m_unk0x18 = v18 * 0.001f;
	float v60 = m_minMoveSpeed;
	m_minMoveSpeed = v60 * 0.001f;
}
