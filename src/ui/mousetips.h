#ifndef MOUSETIPS_H
#define MOUSETIPS_H

#include "util/decomp.h"

class INPUT_AS;
class SPRITE;

class DELETABLE {
public:
	virtual void* vf00(int p_flag) = 0;
};

// VTABLE: ALIEN 0x47a32c
class MOUSETIPS {
public:

	MOUSETIPS() { m_sprite = 0; }
	virtual ~MOUSETIPS() { Clear(); }

	SPRITE* m_sprite; // 0x04

	void Tact(INPUT_AS* p_input);
	void Clear();
	SPRITE* DeletePointerToSprite(SPRITE* p_sprite);
};

// SYNTHETIC: ALIEN 0x40b110
// MOUSETIPS::`scalar deleting destructor'

#endif
