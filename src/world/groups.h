#ifndef GROUPS_H
#define GROUPS_H

#include "util/decomp.h"
#include "world/group.h"

class RESOURCE;
class SPRITE;

class GROUPS {
public:
	GROUP m_head; // 0x00

	GROUPS() : m_head(0, 0) {}

	void Load(RESOURCE* p_res);
	int Save(RESOURCE* p_res);
	void DeletePointerToSprite(SPRITE* p_sprite);

	GROUP* First() { return m_head.m_next != &m_head ? m_head.m_next : 0; }

	GROUP* Next(GROUP* p_group);
	void DrawNumber() const;
};

#endif
