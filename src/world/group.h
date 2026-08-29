#ifndef GROUP_H
#define GROUP_H

#include "sprite/list_sprite.h"
#include "util/decomp.h"

class SPRITE;

// VTABLE: ALIEN 0x47a7e0

class GROUP : public LIST_SPRITE {
public:
	GROUP(GROUP* p_prev, SPRITE* p_sprite);
	~GROUP();
	virtual void* ScalarDeletingDestructor(unsigned int p_flags);

	float m_unk0x10; // 0x10
	float m_unk0x14; // 0x14
	undefined m_unk0x18[0x8]; // 0x18
	GROUP* m_next; // 0x20

	void DrawNumber(int p_n);
	void Insert(SPRITE* p_sprite);
};

DECOMP_SIZE_ASSERT(GROUP, 0x24)

#endif
