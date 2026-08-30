#include "logic/relation.h"

#include "util/myerror.h"

inline static void InsertRelationSprite(LIST_SPRITE* p_list, SPRITE* p_sprite)
{
	int max = p_list->m_max;
	if (p_list->m_n >= max) {
		int newMax = 2 * max + 4;
		if (newMax > max) {
			SPRITE** oldData = p_list->m_data;
			p_list->m_data = (SPRITE**) operator new(sizeof(SPRITE*) * newMax);
			if (!p_list->m_data) {
				MYERROR::LogExit(::Error, "!!!ERROR!!!::LIST: Not enough memory %i", newMax);
			}
			if (oldData) {
				for (int i = 0; i < p_list->m_max; ++i) {
					p_list->m_data[i] = oldData[i];
				}
				operator delete(oldData);
			}
			p_list->m_max = newMax;
		}
	}
	p_list->m_data[p_list->m_n++] = p_sprite;
}

// FUNCTION: ALIEN 0x412b10
void* RELATION::Decode(const void* p_first) const
{
	int n = m_first.m_n;
	if (n == 0) {
		return 0;
	}
	SPRITE** p = m_first.m_data + n;
	while (1) {
		SPRITE* v = *--p;
		--n;
		if (v == p_first) {
			break;
		}
		if (n == 0) {
			return 0;
		}
	}
	if (n >= 0) {
		return m_second.m_data[n];
	}
	return 0;
}

// FUNCTION: ALIEN 0x412b50
void RELATION::Insert(void* p_first, void* p_second)
{
	if (p_first) {
		InsertRelationSprite(&m_first, (SPRITE*) p_first);
		InsertRelationSprite(&m_second, (SPRITE*) p_second);
	}
}

// FUNCTION: ALIEN 0x412c60
void RELATION::Release()
{
	SPRITE** d1 = m_first.m_data;
	m_first.m_max = 0;
	m_first.m_n = 0;
	if (d1) {
		operator delete(d1);
	}
	m_first.m_data = 0;
	SPRITE** d2 = m_second.m_data;
	m_second.m_max = 0;
	m_second.m_n = 0;
	if (d2) {
		operator delete(d2);
	}
	m_second.m_data = 0;
}
