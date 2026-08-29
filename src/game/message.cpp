#define DECOMP_INLINE_STRING_COPY_LIFETIME
#define DECOMP_ANGLE_CTOR_OUT_OF_LINE
#include "game/message.h"

#include <string.h>

#include "audio/sound.h"
#include "game/gametime.h"
#include "game/map.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "ui/menu.h"
#include "sprite/sprite.h"
#include "util/angle.h"
#include "util/string.h"
#include "video/vid.h"

// FUNCTION: ALIEN 0x42e120
MESSAGE::MESSAGE(int p_messageVid, int p_markerVid, float p_z, float p_y, int p_n,
				 unsigned long p_tactDelay)
	: m_n(p_n)
	, m_tactDelay(p_tactDelay)
	, m_messageVid(p_messageVid)
	, m_markerVid(p_markerVid)
	, m_z(p_z)
	, m_y(p_y)
{
	m_lineSpacing = -1;
	int i = 0;
	if (i < m_n) {
		SPRITE** p = m_data;
		do {
			p[45] = 0;
			p[0] = 0;
			++i;
			++p;
		} while (i < m_n);
	}
}

// FUNCTION: ALIEN 0x42e230
void MESSAGE::Release()
{
	int i = 0;
	if (m_n > 0) {
		do {
			SPRITE* target = m_data2[i];
			if (target)
				target->ScalarDeletingDestructor(1);
			m_data2[i] = 0;
			SPRITE* owner = m_data[i];
			if (owner)
				owner->ScalarDeletingDestructor(1);
			m_data[i] = 0;
			++i;
		} while (i < m_n);
	}
}

// FUNCTION: ALIEN 0x42e280
void MESSAGE::DeletePointerToSprite(SPRITE* p_sprite)
{
	int i = 0;
	if (m_n > 0) {
		SPRITE** p = m_data;
		do {
			if (p[45] == p_sprite)
				p[45] = 0;
			if (p[0] == p_sprite)
				p[0] = 0;
			++i;
			++p;
		} while (i < m_n);
	}
}

// FUNCTION: ALIEN 0x42e2c0
void MESSAGE::Shift()
{
	int idx = m_messageVid;
	VID* vid;
	if (idx < 0 || idx >= Map->m_noVid || (vid = Map->m_vids[idx]) == 0)
		vid = EmptyVid;
	int height = *(short*) &vid->m_unk0x2f2[6] + 1;
	for (int i = 0; i < m_stack.m_n; ++i) {
		MESSAGE_STACK* d = &m_stack.m_data[i];
		if (CurrentTime - PrevCurrentTime >= d->m_delay) {
			Put(d->m_text, d->m_x, d->m_y);
			m_stack.DeleteNumberS(i);
			--i;
		}
		else
			d->m_delay = d->m_delay - CurrentTime + PrevCurrentTime;
	}
	if (CurrentTime - m_lastTact > m_tactDelay) {
		Tact();
		m_lastTact = CurrentTime;
	}
	if ((Map->m_menu.m_state & 1) && Map->m_menu.NVidUnderCursor() == m_markerVid) {
		int row = (int) (Map->m_menu.m_underCursor->m_y - Map->m_menu.m_underCursor->m_z
						 - Map->m_shiftY - Graph->m_viewYMin)
			/ height;
		if (m_targetX[row] != -999999.0f || m_targetY[row] != -999999.0f)
			Map->SetShiftCoor(m_targetX[row], m_targetY[row], 2);
	}
}

// FUNCTION: ALIEN 0x42e4a0
void MESSAGE::Put(const STRING& p_msg, float p_x, float p_y)
{
	int idx = m_messageVid;
	VID* vid;
	if (idx < 0 || idx >= Map->m_noVid || (vid = Map->m_vids[idx]) == 0)
		vid = EmptyVid;
	int height = *(short*) &vid->m_unk0x2f2[6] + 1;
	if (strcmp(p_msg.m_str, empty_str)) {
		int n = m_n;
		m_lastTact = CurrentTime;
		if (m_data2[n - 1])
			Tact();
		if (!m_data2[m_n - 1])
			m_data2[m_n - 1] = Map->CreateSprite(Map->Vid(m_messageVid), m_z,
				((GRAPH_CORE*) Graph)->GetViewYMin() + height * (m_n - 1) + m_y + 2000.0f,
				2000.0f, ANGLE(0), 0);
		SPRITE* spr = m_data2[m_n - 1];
		if (spr)
			spr->Action(120, (decomp_intptr) &p_msg, 0, 0);
		m_targetX[m_n - 1] = p_x;
		m_targetY[m_n - 1] = p_y;
		if (p_x != -1.0f || p_y != -1.0f)
			m_data[m_n - 1] = Map->CreateSprite(Map->Vid(m_markerVid), m_z - 10.0f,
				((GRAPH_CORE*) Graph)->GetViewYMin() + height * (m_n - 1) + height / 2 + m_y + 2000.0f,
				2000.0f, ANGLE(0), 0);
		Sound->PlaySFX(106, 0, 0);
	}
}

// FUNCTION: ALIEN 0x42e6a0
void MESSAGE::Tact()
{
	int idx = m_messageVid;
	VID* vid;
	if (idx < 0 || idx >= Map->m_noVid || (vid = Map->m_vids[idx]) == 0)
		vid = EmptyVid;
	int height = *(short*) &vid->m_unk0x2f2[6] + 1;
	if (m_data2[0])
		m_data2[0]->ScalarDeletingDestructor(1);
	m_data2[0] = 0;
	if (m_data[0])
		m_data[0]->ScalarDeletingDestructor(1);
	m_data[0] = 0;
	int i = 1;
	if (i < m_n) {
		SPRITE** p = &m_data[1];
		do {
			SPRITE* spr = *p;
			if (spr) {
				spr->ChangeCoor(spr->m_x, (height * m_lineSpacing) + spr->m_y, spr->m_z);
				p[-1] = *p;
				((float*) (p - 1))[90] = ((float*) (p - 1))[91];
				((float*) (p - 1))[135] = ((float*) (p - 1))[136];
				*p = 0;
			}
			++i;
			++p;
		} while (i < m_n);
	}
	i = 1;
	if (i < m_n) {
		SPRITE** p = &m_data2[1];
		do {
			SPRITE* spr = *p;
			if (spr) {
				spr->ChangeCoor(spr->m_x, (height * m_lineSpacing) + spr->m_y, spr->m_z);
				p[-1] = *p;
				*p = 0;
			}
			++i;
			++p;
		} while (i < m_n);
	}
}

// FUNCTION: ALIEN 0x42e830
void LIST_MESSAGE_STACK::Release()
{
	m_max = 0;
	m_n = 0;
	if (m_data)
		delete[] m_data;
	m_data = 0;
}
