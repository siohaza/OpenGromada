#include "sprite/list_sprite.h"

#include "sprite/sprite.h"
#include "util/myerror.h"

LIST_SPRITE::LIST_SPRITE() : m_n(0), m_max(0), m_data(0)
{
}

LIST_SPRITE::~LIST_SPRITE()
{
	if (m_data) {
		operator delete(m_data);
	}
	m_data = 0;
	m_n = 0;
}

// FUNCTION: ALIEN 0x40b160
SPRITE_LIST::SPRITE_LIST()
{
}

// FUNCTION: ALIEN 0x412e60
void* LIST_SPRITE::LastIterate(int* p_idx)
{
	unsigned int n = m_n;
	if (n) {
		unsigned int last = n - 1;
		*p_idx = last;
		return m_data[last];
	}
	return 0;
}

// FUNCTION: ALIEN 0x412e80
void* LIST_SPRITE::NextIterate(int* p_idx)
{
	if (*p_idx > m_n) {
		*p_idx = m_n;
	}
	if (--*p_idx >= 0) {
		return m_data[*p_idx];
	}
	return 0;
}

// FUNCTION: ALIEN 0x413020
void* LIST_SPRITE::ScalarDeletingDestructor(unsigned int p_flags)
{
	LIST_SPRITE* result = this;
	this->~LIST_SPRITE();
	if (p_flags & 1) {
		operator delete(result);
	}
	return result;
}

// Portable typed growth helper for LIST_SPRITE::Insert.
void LIST_SPRITE::Expand(int p_max)
{
	if (p_max > m_max) {
		SPRITE** oldData = m_data;
		m_data = (SPRITE**) operator new(sizeof(*m_data) * p_max);
		if (!m_data) {
			MYERROR::LogExit(::Error, "!!!ERROR!!!::LIST: Not enough memory %i", p_max);
		}
		if (oldData) {
			for (int i = 0; i < m_n; ++i) {
				m_data[i] = oldData[i];
			}
			operator delete(oldData);
		}
		m_max = p_max;
	}
}

// FUNCTION: ALIEN 0x43de20
void LIST_SPRITE::Insert(SPRITE* p_sprite)
{
	if (p_sprite) {
		++p_sprite->m_noRef;
		if (m_n >= m_max) {
			Expand(2 * m_max + 4);
		}
		m_data[m_n++] = p_sprite;
	}
}

// FUNCTION: ALIEN 0x43deb0
int LIST_SPRITE::Delete(SPRITE* p_sprite)
{
	int n;
	int idx;
	if (!p_sprite || !(idx = n = m_n)) {
		return 1;
	}
	SPRITE** data = m_data;
	SPRITE** p = data + idx;
	SPRITE* sprite;
	while (1) {
		sprite = *--p;
		--idx;
		if (sprite == p_sprite) {
			break;
		}
		if (!idx) {
			return 1;
		}
	}
	if (DeleteNumber(idx)) {
		return 1;
	}
	int refs = --p_sprite->m_noRef;
	if (refs <= 0) {
		if (refs < 0) {
			MYERROR::Error(
				::Error,
				"SPRITE %i",
				4,
				"noRef at Release",
				refs,
				p_sprite->m_vid ? p_sprite->m_vid->m_idx : -1
			);
			return 0;
		}
		if (p_sprite) {
			p_sprite->ScalarDeletingDestructor(1);
		}
	}
	return 0;
}

// FUNCTION: ALIEN 0x43df50
int LIST_SPRITE::DeleteSpriteNumber(int p_idx)
{
	if (p_idx >= 0) {
		int n = m_n;
		if (p_idx < n) {
			SPRITE* sprite = m_data[p_idx];
			--n;
			m_n = n;
			m_data[p_idx] = m_data[n];
			int refs = --sprite->m_noRef;
			if (refs <= 0) {
				if (refs < 0) {
					MYERROR::Error(
						::Error,
						"SPRITE %i",
						4,
						"noRef at Release",
						refs,
						sprite->m_vid ? sprite->m_vid->m_idx : -1
					);
					return 0;
				}
				if (sprite) {
					sprite->ScalarDeletingDestructor(1);
					return 0;
				}
			}
			if (sprite) {
				sprite->ScalarDeletingDestructor(1);
			}
			return 0;
		}
	}
	return 1;
}

// FUNCTION: ALIEN 0x43dfe0
void LIST_SPRITE::DeleteAll()
{
	int result = m_n;
	int i = 0;
	if (result > 0) {
		int offset = 0;
		do {
			for (int j = result - 1; j > i; --j) {
				SPRITE** data = m_data;
				if (data[offset]) {
					SPRITE* sprite = data[j];
					if (data[offset] == sprite) {
						int refs = sprite->m_noRef - 1;
						sprite->m_noRef = refs;
						if (refs <= 0) {
							if (refs < 0) {
								MYERROR::Error(
									::Error,
									"SPRITE %i",
									4,
									"noRef at Release",
									refs,
									sprite->m_vid ? sprite->m_vid->m_idx : -1
								);
							}
							else if (sprite) {
								sprite->ScalarDeletingDestructor(1);
							}
						}
						if (j >= 0) {
							int n = m_n;
							if (j < n) {
								SPRITE** removeData = m_data;
								--n;
								m_n = n;
								removeData[j] = removeData[n];
							}
						}
					}
				}
			}
			result = m_n;
			++i;
			++offset;
		} while (i < result);
	}
	for (int j = m_n - 1; j >= 0; --j) {
		if (m_data[j]) {
			DeleteSpriteNumber(j);
		}
	}
}

// FUNCTION: ALIEN 0x43e0a0
void SPRITE_LIST::Release()
{
	int i = m_n - 1;
	for (; i >= 0; --i) {
		if (m_data[i] && m_data[i]->Release()) {
			DeleteNumber(i);
		}
	}
}
