#define DECOMP_INLINE_STRING_CHARP_CTOR
#define DECOMP_INLINE_STRING_CHARP_NONNULL
#define DECOMP_INLINE_STRING_DTOR
#include "sprite/stext.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "misc.h"

#include "game/map.h"
#include "util/profile.h"
#include "util/stream.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "video/vid.h"

// FUNCTION: ALIEN 0x40ed40
STEXT::STEXT(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent)
	: FRAME(p_vid, p_x, p_y, p_z, p_dir, p_parent)
{
	m_len = 0;
	m_flag = 0;
	m_cols = 0;
	m_rows = 1;
}

// FUNCTION: ALIEN 0x40eda0
void* STEXT::ScalarDeletingDestructor(unsigned int p_flags)
{
	STEXT* result = this;
	this->~STEXT();
	if (p_flags & 1)
		operator delete(result);
	return result;
}

// FUNCTION: ALIEN 0x448ba0
VID* STEXT::Draw()
{
	VID* vid = m_vid;
	if (vid && vid != EmptyVid && (int) vid->m_noDir > 126) {
		int saveCadr = m_noCadr;
		float saveX = m_x;
		float saveY = m_y;
		VID* font = vid;
		if ((m_flag & 0x70) == 0x60) {
			char* value;
			m_text = *(STRING*) Map->GetVariableStr(&value,
				STRING(m_textId.m_str, STRING::INLINE_CHARP_NONNULL));
			if (value != STRING::EMPTY)
				operator delete(value);
			CalcTextProperty();
		}
		if (strcmp(m_text.m_str, empty_str)) {
			if (m_flag & 1)
				m_x = m_x - (float) (m_cols - 1) * m_vid->m_footprintWidth * 0.5f;
			else if (m_flag & 2)
				m_x = m_x
					- ((float) (m_cols - 1) * m_vid->m_footprintWidth + m_vid->m_unk0x384);
			else
				m_x = m_vid->m_unk0x384 + m_x;
			if (m_flag & 8)
				m_y = m_y - (float) (m_rows - 1) * m_vid->m_footprintHeight * 0.5f;
			else if (m_flag & 4)
				m_y = m_y
					- ((float) (m_rows - 1) * m_vid->m_footprintHeight + m_vid->m_unk0x388);
			else
				m_y = m_vid->m_unk0x388 + m_y;
			float lineX = m_x - m_vid->m_footprintWidth;
			int i = 0;
			while (m_text.m_str[i]) {
				if (i >= m_len)
					break;
				char c = m_text.m_str[i];
				if (c == 10) {
					m_y = font->m_footprintHeight + m_y;
					m_x = lineX;
				}
				else if (c == 13) {
					m_x = lineX;
				}
				else if (c == 9) {
					m_x = font->m_footprintWidth * 7.0f + m_x;
				}
				else if (c == '<' && !strncmp(m_text.m_str,
							 // STRING: ALIEN 0x4847e4
							 "<Font=", strlen("<Font="))) {
					char* after;
					char* str = *m_text.After(&after, "<Font=");
					int idx;
					if (str[1] != 'x')
						idx = atoi(str);
					else {
						int n;
						sscanf(str,
							"%i", &n);
						idx = n;
					}
					if (idx >= 0 && idx < Map->m_noVid && Map->m_vids[idx])
						font = Map->m_vids[idx];
					else
						font = EmptyVid;
					if (after != STRING::EMPTY)
						operator delete(after);
					m_x = m_x - font->m_footprintWidth;
					char* before;
					i += strlen(*m_text.Before(&before,
						">"));
					if (before != STRING::EMPTY)
						operator delete(before);
				}
				else {
					int next;
					if (m_text.m_str[i] == 27 && i + 1 < m_len
						&& (next = (unsigned char) m_text.m_str[i + 1]) >= 0
						&& next < Map->m_noVid && Map->m_vids[next]) {
						int idx = m_text.m_str[++i];
						if (idx >= 0 && idx < Map->m_noVid && Map->m_vids[idx])
							font = Map->m_vids[idx];
						else
							font = EmptyVid;
						m_x = m_x - font->m_footprintWidth;
					}
					else if ((unsigned char) m_text.m_str[i] >= 0x20) {
						m_noCadr = (unsigned char) m_text.m_str[i];
						font->Draw(this);
					}
				}
				++i;
				m_x = font->m_footprintWidth + m_x;
			}
			m_x = saveX;
			m_y = saveY;
			m_noCadr = saveCadr;
		}
	}
	else {
		COLOR white;
		white.m_value = 0xffffffff;
		((GRAPH_CORE*) Graph)->PutsXY(m_x, m_y, (char*) (const char*) m_text, white);
	}
	if (0)
		return 0;
}

// FUNCTION: ALIEN 0x448ff0
decomp_intptr STEXT::Action(int p_action, int p_a, int p_b, int p_c)
{

	switch (p_action) {
	case 94: // ACT_GET_BEHAVE
		return m_flag;
	case 95: // ACT_SET_BEHAVE
		m_flag = p_a;
		return 0;
	case 120: { // ACT_SET_TEXT

		m_textId = p_a ? *(STRING*) p_a : STRING(empty_str);
		unsigned int behave = m_flag & 0x70;
		if (behave == 0x10) {
			m_text = Strings->GetString(
				STRING(
					// STRING: ALIEN 0x4847ec
					"menu"),
				m_textId, m_textId);
		} else if (behave == 0x20) {
			m_text.LoadFile(m_textId);
		} else if (behave) {
			char* value;
			m_text = *(STRING*) Map->GetVariableStr(&value,
				STRING(m_textId.m_str, STRING::INLINE_CHARP_NONNULL));
			if (value != STRING::EMPTY)
				operator delete(value);
			behave = m_flag & 0x70;
			if (behave == 0x40) {
				m_text.LoadFile(m_text);
			} else if (behave == 0x50) {
				m_text = Strings->GetString(
					STRING("menu"),
					m_text, m_text);
			}
		} else {
			m_text = m_textId;
		}
		CalcTextProperty();
		return 0;
	}
	case 124: // ACT_GET_TEXT
		return (decomp_intptr) &m_text;
	case 121: // ACT_GET_TEXT_DESC
		return (decomp_intptr) &m_textId;
	case 123: // ACT_SET_FILE

		m_text.LoadFile(*(STRING*) p_a);
		m_len = strlen(m_text.m_str);
		return 0;
	case 122: // ACT_SET_TEXT_COUNT
		m_len = p_a;
		return 0;
	case 80: { // ACT_SAVE
		STREAM* stream = (STREAM*) p_a;
		FRAME::Action(p_action, p_a, p_b, p_c);
		stream->Write(&m_flag, 4);
		stream->Write(m_textId.m_str, strlen(m_textId.m_str) + 1);
		return 0;
	}
	case 81: { // ACT_RESTORE
		STREAM* stream = (STREAM*) p_a;
		FRAME::Action(p_action, p_a, p_b, p_c);
		stream->Read(&m_flag, 4);
		m_textId.Read_res(stream);
		Action(120, (decomp_intptr) &m_textId, 0, 0);
		return 0;
	}
	case 130: { // ACT_NEXT_COMMAND
		short sawNewline = 0;
		if (strcmp(m_text.m_str, empty_str) && !m_unk0x50) {
			short total = strlen(m_text.m_str);
			if (m_len < total) {
				for (;;) {
					int pos = m_len;
					m_len = pos + 1;
					if (!isspace(m_text.m_str[pos]))
						break;
					if (m_len >= total)
						break;
					if (m_text.m_str[m_len] == 10)
						sawNewline = 1;
				}
				if (sawNewline) {
					if (m_len < total)
						--m_len;
					m_unk0x50 = 150;
					ChangeAnimation(2); // ANI_GO
					return 0;
				}
				ChangeAnimation(1); // ANI_STOP_MOVE
				SPRITE::m_flag &= ~0x200;
				return 0;
			}
			if (m_ani)
				ChangeAnimation(0); // ANI_STAND
		}
		return 0;
	}
	}
	return FRAME::Action(p_action, p_a, p_b, p_c);
}

// STUB: ALIEN 0x4495d0
int STEXT::CalcTextProperty()
{
	char* text;
	int lineStart = 0;
	m_rows = 1;
	m_cols = 0;
	char c;
	int i = 0;
	if ((c = *(text = m_text.m_str)) != 0) {
		do {
			if (text[i] == '\n') {
				if (i - lineStart > m_cols)
					m_cols = i - lineStart;
				lineStart = i + 1;
				++m_rows;
			}
			c = text[i + 1];
			++i;
		} while (c);
	}
	int cols = m_cols;
	m_len = i;
	if (!cols)
		m_cols = i;
	return i;
}
