#ifndef ACT_H
#define ACT_H

#include "util/decomp.h"

class ACT {
public:
	ACT() {}

	ACT(int p_cmd, int p_a, int p_b, int p_c)
	{
		m_cmd = p_cmd;
		m_a = p_a;
		m_b = p_b;
		m_c = p_c;
	}

	ACT(const ACT& p_other)
	{
		m_cmd = p_other.m_cmd;
		m_a = p_other.m_a;
		m_b = p_other.m_b;
		m_c = p_other.m_c;
	}
	int operator==(const ACT& p_other) const
	{
		return m_cmd == p_other.m_cmd && m_a == p_other.m_a &&
			m_b == p_other.m_b && m_c == p_other.m_c;
	}

	union {
		int m_cmd; // 0x00
		unsigned char m_cmdByte;
	};
	int m_a; // 0x04
	int m_b; // 0x08
	int m_c; // 0x0c
};

DECOMP_SIZE_ASSERT(ACT, 0x10)

// VTABLE: ALIEN 0x47a87c

class LIST_ACT {
public:
	LIST_ACT()
		: m_data(0)
	{
		m_n = 0;
		m_max = 0;
	}
	virtual ~LIST_ACT()
	{
		if (m_data)
			operator delete(m_data);
		m_data = 0;
		m_n = 0;
	}

	int m_n; // 0x04
	int m_max; // 0x08
	ACT* m_data; // 0x0c

	void Release()
	{
		ACT* data = m_data;
		m_max = 0;
		m_n = 0;
		if (data)
			operator delete(data);
		m_data = 0;
	}

	void Expand(int p_max);
#ifdef DECOMP_INLINE_LIST_ACT_SET_NUMBER

	void SetNumber(int p_n)
	{
		m_n = p_n;
		if (p_n > m_max)
			Expand(p_n);
	}
#endif
	void Insert(ACT p_act);
	void InsertFirst(ACT p_act);
	void InsertBefore(int p_idx, ACT p_act);
	int Location(const ACT& p_act) const
	{
		int i = m_n;
		if (i) {
			ACT* item = m_data + i;
			do {
				--item;
				--i;
				if (*item == p_act)
					return i;
			} while (i);
		}
		return -1;
	}
};

DECOMP_SIZE_ASSERT(LIST_ACT, 0x10)

// SYNTHETIC: ALIEN 0x446b70
// LIST_ACT::`scalar deleting destructor'

#endif
