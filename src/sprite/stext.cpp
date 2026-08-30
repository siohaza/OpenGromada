#include "sprite/stext.h"

#include "game/map.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "misc.h"
#include "ui/text_encoding.h"
#include "util/profile.h"
#include "util/stream.h"
#include "video/vid.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
	if (p_flags & 1) {
		operator delete(result);
	}
	return result;
}

// FUNCTION: ALIEN 0x448ba0
void STEXT::Draw()
{
	VID* vid = m_vid;
	if (vid && vid != EmptyVid && (int) vid->m_noDir > 126) {
		int saveCadr = m_noCadr;
		float saveX = m_x;
		float saveY = m_y;
		VID* font = vid;
		float uiScale = UIDrawScale();
		if ((m_flag & 0x70) == 0x60) {
			char* value;
			m_text = *(STRING*) Map->GetVariableStr(&value, STRING(m_textId.m_str));
			if (value != STRING::EMPTY) {
				operator delete(value);
			}
			CalcTextProperty();
		}
		if (strcmp(m_text.m_str, empty_str)) {
			if (m_flag & 1) {
				m_x = m_x - (float) (m_cols - 1) * m_vid->m_footprintWidth * uiScale * 0.5f;
			}
			else if (m_flag & 2) {
				m_x = m_x - ((float) (m_cols - 1) * m_vid->m_footprintWidth + m_vid->m_unk0x384) * uiScale;
			}
			else {
				m_x = m_vid->m_unk0x384 * uiScale + m_x;
			}
			if (m_flag & 8) {
				m_y = m_y - (float) (m_rows - 1) * m_vid->m_footprintHeight * uiScale * 0.5f;
			}
			else if (m_flag & 4) {
				m_y = m_y - ((float) (m_rows - 1) * m_vid->m_footprintHeight + m_vid->m_unk0x388) * uiScale;
			}
			else {
				m_y = m_vid->m_unk0x388 * uiScale + m_y;
			}
			float lineX = m_x - m_vid->m_footprintWidth * uiScale;
			size_t textSize = strlen(m_text.m_str);
			size_t offset = 0;
			int logical = 0;
			while (offset < textSize && logical < m_len) {
				size_t unitStart = offset;
				TEXT_ENCODING::UNIT unit = TEXT_ENCODING::DecodeUnit(m_text.m_str, textSize, offset);
				unsigned char c = unit.m_glyph;
				offset += unit.m_bytes ? unit.m_bytes : 1;
				++logical;
				if (c == 10) {
					m_y = font->m_footprintHeight * uiScale + m_y;
					m_x = lineX;
				}
				else if (c == 13) {
					m_x = lineX;
				}
				else if (c == 9) {
					m_x = font->m_footprintWidth * uiScale * 7.0f + m_x;
				}
				else if (
					c == '<' && !strncmp(
									m_text.m_str + unitStart,
									// STRING: ALIEN 0x4847e4
									"<Font=",
									strlen("<Font=")
								)
				) {
					const char* value = m_text.m_str + unitStart + strlen("<Font=");
					int idx = (int) strtol(value, 0, 0);
					if (idx >= 0 && idx < Map->m_noVid && Map->m_vids[idx]) {
						font = Map->m_vids[idx];
					}
					else {
						font = EmptyVid;
					}
					m_x = m_x - font->m_footprintWidth * uiScale;
					const char* close = strchr(value, '>');
					if (close) {
						size_t closeOffset = (size_t) (close - m_text.m_str);
						while (offset <= closeOffset) {
							TEXT_ENCODING::UNIT skipped = TEXT_ENCODING::DecodeUnit(m_text.m_str, textSize, offset);
							offset += skipped.m_bytes ? skipped.m_bytes : 1;
							++logical;
						}
					}
				}
				else {
					if (c == 27 && logical < m_len && offset < textSize) {
						TEXT_ENCODING::UNIT next = TEXT_ENCODING::DecodeUnit(m_text.m_str, textSize, offset);
						int idx = next.m_glyph;
						offset += next.m_bytes ? next.m_bytes : 1;
						++logical;
						if (idx >= 0 && idx < Map->m_noVid && Map->m_vids[idx]) {
							font = Map->m_vids[idx];
						}
						else {
							font = EmptyVid;
						}
						m_x = m_x - font->m_footprintWidth * uiScale;
					}
					else if (c >= 0x20) {
						m_noCadr = c;
						font->Draw(this);
					}
				}
				m_x = font->m_footprintWidth * uiScale + m_x;
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
}

// FUNCTION: ALIEN 0x448ff0
decomp_intptr STEXT::Action(int p_action, decomp_intptr p_a, decomp_intptr p_b, decomp_intptr p_c)
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
					"menu"
				),
				m_textId,
				m_textId
			);
		}
		else if (behave == 0x20) {
			m_text.LoadFile(m_textId);
		}
		else if (behave) {
			char* value;
			m_text = *(STRING*) Map->GetVariableStr(&value, STRING(m_textId.m_str));
			if (value != STRING::EMPTY) {
				operator delete(value);
			}
			behave = m_flag & 0x70;
			if (behave == 0x40) {
				m_text.LoadFile(m_text);
			}
			else if (behave == 0x50) {
				m_text = Strings->GetString(STRING("menu"), m_text, m_text);
			}
		}
		else {
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
		m_len = TEXT_ENCODING::LegacyLength(m_text.m_str);
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
			int total = TEXT_ENCODING::LegacyLength(m_text.m_str);
			if (m_len < total) {
				for (;;) {
					int pos = m_len;
					m_len = pos + 1;
					if (!TEXT_ENCODING::IsLegacySpaceAt(m_text.m_str, pos)) {
						break;
					}
					if (m_len >= total) {
						break;
					}
					unsigned char next;
					if (TEXT_ENCODING::LegacyGlyphAt(m_text.m_str, m_len, &next) && next == 10) {
						sawNewline = 1;
					}
				}
				if (sawNewline) {
					if (m_len < total) {
						--m_len;
					}
					m_unk0x50 = 150;
					ChangeAnimation(2); // ANI_GO
					return 0;
				}
				ChangeAnimation(1); // ANI_STOP_MOVE
				SPRITE::m_flag &= ~0x200;
				return 0;
			}
			if (m_ani) {
				ChangeAnimation(0); // ANI_STAND
			}
		}
		return 0;
	}
	}
	return FRAME::Action(p_action, p_a, p_b, p_c);
}

// STUB: ALIEN 0x4495d0
int STEXT::CalcTextProperty()
{
	TEXT_ENCODING::METRICS metrics = TEXT_ENCODING::Measure(m_text.m_str);
	m_rows = metrics.m_rows;
	m_cols = metrics.m_columns;
	m_len = metrics.m_units;
	return m_len;
}
