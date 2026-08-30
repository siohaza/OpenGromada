#include "world/groups.h"

#include "game/map.h"
#include "sprite/list_sprite.h"
#include "world/group.h"

// FUNCTION: ALIEN 0x43acf0
int GROUPS::Save(RESOURCE* p_res)
{
	RESOURCE* res = p_res;
	int end;
	for (GROUP* g = m_head.m_next; g != &m_head && g;) {
		if (g->m_n) {
			for (int i = 0; i < g->m_n; ++i) {
				res->Write(&g->m_data[i], 4);
			}
			end = -1;
			res->Write(&end, 4);
		}
		g = g->m_next;
		if (g == &m_head || !g) {
			break;
		}
	}
	end = -1;
	return res->Write(&end, 4);
}

// FUNCTION: ALIEN 0x43ad70
void GROUPS::Load(RESOURCE* p_res)
{
	for (;;) {
		SPRITE* result = Map->ReadPointer((STREAM*) p_res);
		if (result == (SPRITE*) -1) {
			break;
		}
		GROUP* group = new GROUP(&m_head, result);
		for (SPRITE* j = Map->ReadPointer((STREAM*) p_res); j != (SPRITE*) -1; j = Map->ReadPointer((STREAM*) p_res)) {
			group->Insert(j);
		}
	}
}

// FUNCTION: ALIEN 0x43adf0
void GROUPS::DeletePointerToSprite(SPRITE* p_sprite)
{
	GROUP* v3 = m_head.m_next;
	if (v3 != &m_head && v3) {
		do {
			if (((LIST_SPRITE*) v3)->Delete(p_sprite) || v3->m_n) {
				v3 = v3->m_next;
				if (v3 == &m_head) {
					return;
				}
			}
			else {
				GROUP* v4 = v3;
				v3 = v3->m_next == &m_head ? 0 : v3->m_next;
				if (v4) {
					v4->ScalarDeletingDestructor(1);
				}
			}
		} while (v3);
	}
}

inline static GROUP* FirstInline(const GROUPS* p_self)
{
	return p_self->m_head.m_next != &p_self->m_head ? p_self->m_head.m_next : 0;
}

inline static GROUP* NextInline(const GROUPS* p_self, GROUP* p_group)
{
	if (p_group) {
		GROUP* result = p_group->m_next;
		if (result != &p_self->m_head) {
			return result;
		}
	}
	return 0;
}

// FUNCTION: ALIEN 0x43ae50
void GROUPS::DrawNumber() const
{
	GROUP* i = FirstInline(this);
	for (int v3 = 0; i; ++v3) {
		i->DrawNumber(v3);
		i = NextInline(this, i);
	}
}

// FUNCTION: ALIEN 0x43f160
GROUP* GROUPS::Next(GROUP* p_group)
{
	if (p_group) {
		GROUP* result = p_group->m_next;
		if (result != &m_head) {
			return result;
		}
	}
	return 0;
}
