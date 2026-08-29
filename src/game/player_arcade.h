#ifndef PLAYER_ARCADE_H
#define PLAYER_ARCADE_H

#include "game/message.h"
#include "game/player.h"

class SPRITE;
class INPUT_AS;

class PLAYER_MSG {
public:
	virtual void vf00() = 0;
	virtual int vf04(SPRITE* p_sprite) = 0;
};

class STRING;

// VTABLE: ALIEN 0x47a828

class PLAYER_ARCADE : public PLAYER {
public:
	PLAYER_ARCADE(int p_control, int p_army);

	MESSAGE m_msg; // 0x28

	void Control(INPUT_AS* p_input);
	unsigned int SetCleverAttack(int p_on);
	virtual void PutMessage(const STRING& p_msg, float p_x, float p_y); // vtable+0x24
	void DeletePointerToSprite(SPRITE* p_sprite);
};

DECOMP_SIZE_ASSERT(PLAYER_ARCADE, 0x32c)

// SYNTHETIC: ALIEN 0x43e950
// PLAYER_ARCADE::`scalar deleting destructor'

#endif
