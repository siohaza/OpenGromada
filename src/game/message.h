#ifndef MESSAGE_H
#define MESSAGE_H

#include "util/decomp.h"
#include "util/string.h"

class SPRITE;
class STRING;

class MESSAGE_STACK {
public:
	STRING m_text; // 0x00
	float m_x; // 0x04
	float m_y; // 0x08
	unsigned int m_delay; // 0x0c
};

DECOMP_SIZE_ASSERT(MESSAGE_STACK, 0x10)

// VTABLE: ALIEN 0x47a784

class LIST_MESSAGE_STACK {
public:

	LIST_MESSAGE_STACK()
		: m_data(0)
	{
		m_n = 0;
		m_max = 0;
	}
	virtual ~LIST_MESSAGE_STACK()
	{
		if (m_data)
			delete[] m_data;
		m_data = 0;
		m_n = 0;
	}

	void Release();

	void DeleteNumberS(int p_idx)
	{
		if (p_idx >= 0 && p_idx < m_n) {
			m_n = m_n - 1;
			for (; p_idx < m_n; ++p_idx)
				m_data[p_idx] = m_data[p_idx + 1];
		}
		if (!m_n)
			Release();
	}

	int m_n; // 0x04
	int m_max; // 0x08
	MESSAGE_STACK* m_data; // 0x0c
};

DECOMP_SIZE_ASSERT(LIST_MESSAGE_STACK, 0x10)

// SYNTHETIC: ALIEN 0x42e7c0
// LIST_MESSAGE_STACK::`scalar deleting destructor'

// SYNTHETIC: ALIEN 0x42e1a0
// MESSAGE::`scalar deleting destructor'

// VTABLE: ALIEN 0x47a77c

class MESSAGE {
public:
	MESSAGE(int p_messageVid, int p_markerVid, float p_z, float p_y, int p_n,
			unsigned long p_tactDelay);
	virtual ~MESSAGE() { Release(); }
	virtual void DeletePointerToSprite(SPRITE* p_sprite);

	int m_n; // 0x04
	unsigned int m_tactDelay; // 0x08
	int m_messageVid; // 0x0c
	int m_markerVid; // 0x10
	float m_z; // 0x14
	float m_y; // 0x18
	int m_lineSpacing; // 0x1c
	unsigned int m_lastTact; // 0x20
	SPRITE* m_data[45]; // 0x24
	SPRITE* m_data2[45]; // 0xd8
	float m_targetX[45]; // 0x18c
	float m_targetY[45]; // 0x240
	LIST_MESSAGE_STACK m_stack; // 0x2f4

	void Release();
	void Shift();
	void Put(const STRING& p_msg, float p_x, float p_y);
	void Tact();
};

DECOMP_SIZE_ASSERT(MESSAGE, 0x304)

#endif
