#ifndef EX_SPRITE_DATA_H
#define EX_SPRITE_DATA_H

#include "util/decomp.h"
#include "util/string.h"

class SPRITE;
class STREAM;

// VTABLE: ALIEN 0x47a858

class LIST_INT {
public:
	LIST_INT() : m_data(0)
	{
		m_n = 0;
		m_max = 0;
	}
	virtual ~LIST_INT()
	{
		delete[] m_data;
		m_data = 0;
		m_n = 0;
	}

	int Location(int* p_value);
	int DeleteNumber(int p_idx);

	void Read(STREAM* p_stream);
	void Expand(int p_max);

	int m_n;     // 0x04
	int m_max;   // 0x08
	int* m_data; // 0x0c
};

// SYNTHETIC: ALIEN 0x446b30
// LIST_INT::`scalar deleting destructor'

// VTABLE: ALIEN 0x47a8e8

class LIST_SHORT {
public:
	LIST_SHORT() : m_data(0)
	{
		m_n = 0;
		m_max = 0;
	}
	virtual ~LIST_SHORT()
	{
		delete[] m_data;
		m_data = 0;
		m_n = 0;
	}

	void Read(STREAM* p_stream);
	void Expand(int p_max);

	int m_n;       // 0x04
	int m_max;     // 0x08
	short* m_data; // 0x0c
};

// SYNTHETIC: ALIEN 0x447fa0
// LIST_SHORT::`scalar deleting destructor'

class EX_SPRITE_DATA {
public:
	EX_SPRITE_DATA(SPRITE* p_sprite);

	float m_x;               // 0x00
	float m_y;               // 0x04
	float m_z;               // 0x08
	unsigned int m_coorTime; // 0x0c
	int m_unk0x10;           // 0x10

	unsigned int m_unk0x14; // 0x14
	unsigned int m_time;    // 0x18
	float m_unk0x1c;        // 0x1c
	float m_unk0x20;        // 0x20
	int m_unk0x24;          // 0x24
	int m_unk0x28;          // 0x28
	LIST_INT m_list;        // 0x2c

	STRING m_spriteName;
};

#endif
