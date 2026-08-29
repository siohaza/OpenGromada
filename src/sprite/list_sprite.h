#ifndef LIST_SPRITE_H
#define LIST_SPRITE_H

#include "util/decomp.h"

class SPRITE;

#if defined(DECOMP_INLINE_SPRITE_LIST_CTOR) && !defined(DECOMP_INLINE_LIST_SPRITE_SPECIAL_MEMBERS)
#define DECOMP_INLINE_LIST_SPRITE_SPECIAL_MEMBERS
#define DECOMP_UNDEF_INLINE_LIST_SPRITE_SPECIAL_MEMBERS
#endif

// VTABLE: ALIEN 0x47a344

class LIST_SPRITE {
public:

#ifdef DECOMP_INLINE_LIST_SPRITE_SPECIAL_MEMBERS
	// FUNCTION: ALIEN 0x412e10
	LIST_SPRITE()
		: m_data(0)
	{
		m_n = 0;
		m_max = 0;
	}
	// FUNCTION: ALIEN 0x412e30
	~LIST_SPRITE()
	{
		if (m_data)
			operator delete(m_data);
		m_data = 0;
		m_n = 0;
	}
#else
	LIST_SPRITE();
	~LIST_SPRITE();
#endif
	virtual void* ScalarDeletingDestructor(unsigned int p_flags);

	int m_n; // 0x04
	int m_max; // 0x08
	SPRITE** m_data; // 0x0c

#ifdef DECOMP_INLINE_LIST_SPRITE_ITERATE
	void* LastIterate(int* p_idx)
	{
		unsigned int n = m_n;
		if (n) {
			unsigned int last = n - 1;
			*p_idx = last;
			return m_data[last];
		}
		return 0;
	}
	void* NextIterate(int* p_idx)
	{
		if (*p_idx > m_n)
			*p_idx = m_n;
		int idx = *p_idx - 1;
		*p_idx = idx;
		if (idx >= 0)
			return m_data[idx];
		return 0;
	}
#else
	void* LastIterate(int* p_idx);
	void* NextIterate(int* p_idx);
#endif
	void Insert(SPRITE* p_sprite);
	int Delete(SPRITE* p_sprite);

	int DeleteNumber(int p_idx)
	{
		if (p_idx < 0 || p_idx >= m_n)
			return 1;
		--m_n;
		m_data[p_idx] = m_data[m_n];
		return 0;
	}
	int DeleteSpriteNumber(int p_idx);
	void DeleteAll();
};

DECOMP_SIZE_ASSERT(LIST_SPRITE, 0x10)

// VTABLE: ALIEN 0x47a340

class SPRITE_LIST : public LIST_SPRITE {
public:
#ifdef DECOMP_INLINE_SPRITE_LIST_CTOR
	SPRITE_LIST() {}
#else
	SPRITE_LIST();
#endif
	virtual void* ScalarDeletingDestructor(unsigned int p_flags);

	void Release();
};

DECOMP_SIZE_ASSERT(SPRITE_LIST, 0x10)

#ifdef DECOMP_UNDEF_INLINE_LIST_SPRITE_SPECIAL_MEMBERS
#undef DECOMP_INLINE_LIST_SPRITE_SPECIAL_MEMBERS
#undef DECOMP_UNDEF_INLINE_LIST_SPRITE_SPECIAL_MEMBERS
#endif

#endif
