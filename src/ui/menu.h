#ifndef MENU_H
#define MENU_H

#include "sprite/list_sprite.h"
#include "util/decomp.h"

class SPRITE;
class INPUT_AS;
class STRING;

// VTABLE: ALIEN 0x47a810

class MENU : public LIST_SPRITE {
public:
	MENU();

	int Control(INPUT_AS* p_input);
	unsigned int m_state; // 0x10
	SPRITE* m_underCursor; // 0x14

	int NVidUnderCursor() const;
	unsigned int NDirUnderCursor() const;
	int Load(const STRING& p_name);
	int DeleteFromFile(const STRING& p_name);
};

DECOMP_SIZE_ASSERT(MENU, 0x18)

#endif
