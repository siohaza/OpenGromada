#ifndef PLAYER_ARCADE_H
#define PLAYER_ARCADE_H

#include "game/message.h"
#include "game/player.h"

class SPRITE;
class INPUT_AS;

class STRING;

// VTABLE: ALIEN 0x47a828

class PLAYER_ARCADE : public PLAYER {
private:
	// Script opcode 246 toggles bit 1 of this word.  The recovered Win32
	// expression accidentally addressed the MESSAGE vptr at offset 0x28;
	// keeping the script state separate is required now that vptrs are 64-bit.
	unsigned int m_cleverAttackFlags;

public:
	PLAYER_ARCADE(int p_control, int p_army);

	MESSAGE m_msg;

	void Control(INPUT_AS* p_input);
	void RefreshUILayout();
	unsigned int SetCleverAttack(int p_on);
	virtual void PutMessage(const STRING& p_msg, float p_x, float p_y); // vtable+0x24
	void DeletePointerToSprite(SPRITE* p_sprite);
};

// SYNTHETIC: ALIEN 0x43e950
// PLAYER_ARCADE::`scalar deleting destructor'

#endif
