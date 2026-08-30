#include "world/group.h"

#include "game/map.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "sprite/list_sprite.h"
#include "sprite/sprite.h"

// FUNCTION: ALIEN 0x43ab90
GROUP::GROUP(GROUP* p_prev, SPRITE* p_sprite)
{
	if (p_prev) {
		m_next = p_prev->m_next;
		p_prev->m_next = this;
	}
	else {
		m_next = this;
	}
	if (p_sprite) {
		Insert(p_sprite);
	}
}

// FUNCTION: ALIEN 0x43abd0
void* GROUP::ScalarDeletingDestructor(unsigned int p_flags)
{
	GROUP* result = this;
	this->~GROUP();
	if (p_flags & 1) {
		operator delete(result);
	}
	return result;
}

// FUNCTION: ALIEN 0x43abf0
GROUP::~GROUP()
{
	GROUP* next = m_next;
	GROUP* prev = m_next;
	while (prev->m_next != this) {
		prev = prev->m_next;
	}
	prev->m_next = next;
}

// FUNCTION: ALIEN 0x43ac40
void GROUP::DrawNumber(int p_n)
{
	for (int i = 0; i < m_n; ++i) {
		SPRITE* sprite = (SPRITE*) ((LIST_SPRITE*) this)->m_data[i];
		GRAPH_CORE::PrintfXY(
			(GRAPH_CORE*) Graph,
			sprite->m_x - Map->m_shiftX,
			sprite->m_y - sprite->m_z - Map->m_shiftY,
			"%i",
			p_n
		);
	}
}

// FUNCTION: ALIEN 0x43aca0
void GROUP::Insert(SPRITE* p_sprite)
{
	float x;
	if (m_n) {
		x = p_sprite->m_x;
		m_unk0x10 = (x + m_unk0x10) * 0.5f;
		m_unk0x14 = (p_sprite->m_y + m_unk0x14) * 0.5f;
	}
	else {
		x = p_sprite->m_x;
		m_unk0x10 = x;
		x = p_sprite->m_y;
		m_unk0x14 = x;
	}
	((LIST_SPRITE*) this)->Insert(p_sprite);
}
