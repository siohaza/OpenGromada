#define DECOMP_INLINE_STRING_DTOR
#include "gfx/picture_font.h"

#include "util/string.h"

static inline PICTURE_BASE* FontImplOf(const PICTURE& p_picture)
{
	return p_picture.m_impl;
}

// STUB: ALIEN 0x42cd10
int PICTURE_FONT::Load(STRING p_name, STRING p_alpha, STRING p_z)
{
	Close();
	int result = m_source.Load(p_name, p_alpha,
		STRING(p_z.m_str, STRING::CALL_COPY_NONNULL));
	if (result)
		return result;
	m_color.m_impl->SetSize(m_source.m_color.m_impl->m_width / 16 - 1,
							m_source.m_color.m_impl->m_height / 16 - 1,
							m_source.m_color.m_impl->m_bpp);
	m_color.m_impl->m_noFrames = 256;
	m_unk0x42c = 1;
	PICTURE_BASE* source = FontImplOf(m_source.m_color);
	if (source->m_bpp == 1) {
		int n = 256;
		unsigned int* src = source->m_palette;
		unsigned int* dst = m_color.m_impl->m_palette;
		do {
			*dst++ = *src++;
			--n;
		} while (n);
		m_unk0x42c |= 8;
	}
	Rewind();
	(STRING&) m_color.m_impl->m_name = p_name;
	return 0;
}

// FUNCTION: ALIEN 0x42ce80
int PICTURE_FONT::NextFrame()
{
	PICTURE_MAKEVID::NextFrame();
	int cellX = (m_color.m_impl->m_frame % 16) * (m_color.m_impl->m_width + 1);
	int cellY = (m_color.m_impl->m_frame / 16) * (m_color.m_impl->m_height + 1);
	for (int y = 0; y < m_color.m_impl->m_height; ++y) {
		for (int x = 0; x < m_color.m_impl->m_width; ++x) {
			int data = m_source.m_color.m_impl->GetData(x + cellX, y + cellY);
			m_color.m_impl->PutData(x, y, data);
		}
	}
	if (0)
		return 0;
}

// FUNCTION: ALIEN 0x42cf10
int PICTURE_FONT::Rewind()
{
	PICTURE_MAKEVID::Rewind();
	int result = (int) m_color.m_impl;
	for (int i = 0; i < ((PICTURE_BASE*) result)->m_height; ++i) {
		int x = 0;
		if (((PICTURE_BASE*) result)->m_width > 0) {
			do {
				int data = m_source.m_color.m_impl->GetData(x, i);
				m_color.m_impl->PutData(x, i, data);
				++x;
			} while (x < m_color.m_impl->m_width);
		}
		result = (int) m_color.m_impl;
	}
	return result;
}
