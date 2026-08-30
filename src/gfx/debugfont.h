#ifndef DEBUG_FONT_H
#define DEBUG_FONT_H

class DEBUG_FONT {
public:
	DEBUG_FONT(const char* p_name, int p_height, int p_flags);
	~DEBUG_FONT();

	int DrawDebugText(float p_x, float p_y, unsigned int p_color, const char* p_text, int p_flags);

	int m_height;
	int m_flags;
};

#endif
