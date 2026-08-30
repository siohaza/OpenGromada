#ifndef LIST_SPRITE_H
#define LIST_SPRITE_H

#include "util/decomp.h"

class SPRITE;

// VTABLE: ALIEN 0x47a344

class LIST_SPRITE {
public:
	LIST_SPRITE();
	~LIST_SPRITE();
	virtual void* ScalarDeletingDestructor(unsigned int p_flags);

	int m_n;         // 0x04
	int m_max;       // 0x08
	SPRITE** m_data; // 0x0c
	void* LastIterate(int* p_idx);
	void* NextIterate(int* p_idx);
	void Insert(SPRITE* p_sprite);
	void Expand(int p_max);
	int Delete(SPRITE* p_sprite);

	int DeleteNumber(int p_idx)
	{
		if (p_idx < 0 || p_idx >= m_n) {
			return 1;
		}
		--m_n;
		m_data[p_idx] = m_data[m_n];
		return 0;
	}
	int DeleteSpriteNumber(int p_idx);
	void DeleteAll();
};

// VTABLE: ALIEN 0x47a340

class SPRITE_LIST : public LIST_SPRITE {
public:
	SPRITE_LIST();
	virtual void* ScalarDeletingDestructor(unsigned int p_flags);

	void Release();
};

#endif
