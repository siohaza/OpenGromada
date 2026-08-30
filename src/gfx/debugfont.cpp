#include "gfx/debugfont.h"

#include "platform/render.h"

DEBUG_FONT::DEBUG_FONT(const char* p_name, int p_height, int p_flags)
{
	(void) p_name;
	m_height = p_height > 0 ? p_height : 8;
	m_flags = p_flags;
}

DEBUG_FONT::~DEBUG_FONT()
{
}

int DEBUG_FONT::DrawDebugText(float p_x, float p_y, unsigned int p_color, const char* p_text, int p_flags)
{
	(void) p_flags;
	if (!p_text || !*p_text) {
		return 0;
	}
	Platform_RenderDebugText(p_x, p_y, p_color, p_text, m_height);
	return 0;
}
