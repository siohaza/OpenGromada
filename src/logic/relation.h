#ifndef RELATION_H
#define RELATION_H

#include "sprite/list_sprite.h"
#include "util/decomp.h"

class RELATION {
public:
	LIST_SPRITE m_first;  // 0x00
	LIST_SPRITE m_second; // 0x10

	void* Decode(const void* p_first) const;
	void Insert(void* p_first, void* p_second);
	void Release();
};

#endif
