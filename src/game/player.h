#ifndef PLAYER_H
#define PLAYER_H

#include "util/decomp.h"
#include "game/man.h"
#include "sprite/list_sprite.h"
#include "util/stream.h"
#include "util/string.h"

class SPRITE;
class INPUT_AS;

// VTABLE: ALIEN 0x47a544

class PLAYER {
public:
	PLAYER(int p_control, int p_army);
	virtual ~PLAYER(); // vtable+0x00
	virtual void DeletePointerToSprite(SPRITE* p_sprite); // vtable+0x04
	virtual int Save(STREAM* p_stream) const; // vtable+0x08
	virtual void Load(STREAM* p_stream); // vtable+0x0c
	virtual void Release(); // vtable+0x10
	virtual void SetFlagman(SPRITE* p_sprite); // vtable+0x14

	virtual void Control(INPUT_AS* p_input) {} // vtable+0x18

	virtual void StateBarOn() {} // vtable+0x1c
	virtual void StateBarOff() {} // vtable+0x20
	virtual void PutMessage(const STRING& p_msg, float p_x, float p_y); // vtable+0x24

	virtual void AddPointerToSprite(SPRITE* p_sprite) {} // vtable+0x28
	virtual STRING GetMouseTipsString() const; // vtable+0x2c

	int m_money; // 0x04
	undefined4 m_control; // 0x08
	undefined4 m_army; // 0x0c
	PTR_SPRITE m_flagman; // 0x10
	SPRITE_LIST m_stateBar; // 0x14
	SPRITE* m_underCursor; // 0x24

	unsigned int GetMoney();
	unsigned int SetMoney(unsigned int p_money);
	MAN* Flagman();
};

// SYNTHETIC: ALIEN 0x4131c0
// PLAYER::`scalar deleting destructor'

#endif
