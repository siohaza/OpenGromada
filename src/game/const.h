#ifndef CONST_H
#define CONST_H

#include "util/decomp.h"

#include <cstddef>

class RESOURCE;

class CONSTS {
public:
	CONSTS(RESOURCE* p_res);

	float m_unk0x00;        // 0x00
	float m_unk0x04;        // 0x04
	float m_unk0x08;        // 0x08
	float m_unk0x0c;        // 0x0c
	int m_repairByRepairHp; // 0x10
	int m_addAmmo;          // 0x14
	float m_unk0x18;        // 0x18
	float m_unk0x1c;        // 0x1c
	int m_unk0x20;          // 0x20
	int m_unk0x24;          // 0x24
	int m_debugMode;        // 0x28
	int m_unk0x2c;          // 0x2c
	int m_unk0x30;          // 0x30

	unsigned int m_unk0x34; // 0x34

	int m_depoAddHp;          // 0x38
	int m_buildingAddHp;      // 0x3c
	undefined m_unk0x40[0x4]; // 0x40
	int m_repairDockTime;     // 0x44
	undefined m_unk0x48[0x4]; // 0x48
	int m_balloonAddAmmo;     // 0x4c

	unsigned int m_unk0x50;   // 0x50
	unsigned int m_unk0x54;   // 0x54
	unsigned int m_unk0x58;   // 0x58
	undefined m_unk0x5c[0x4]; // 0x5c
	float m_minMoveSpeed;     // 0x60
	int m_unk0x64;            // 0x64
};

static_assert(sizeof(CONSTS) == 0x68);
static_assert(offsetof(CONSTS, m_debugMode) == 0x28);
static_assert(offsetof(CONSTS, m_minMoveSpeed) == 0x60);

extern CONSTS* Const;

#endif
