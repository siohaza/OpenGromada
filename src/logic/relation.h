#ifndef RELATION_H
#define RELATION_H

#include "util/decomp.h"
#include "sprite/list_sprite.h"

class RELATION {
public:
	LIST_SPRITE m_first; // 0x00
	LIST_SPRITE m_second; // 0x10

	void* Decode(int p_first) const;
	void Insert(void* p_first, void* p_second);
	void Release();
};

#endif
