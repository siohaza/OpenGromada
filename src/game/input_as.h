#ifndef INPUT_AS_H
#define INPUT_AS_H

union SDL_Event;

#include "util/decomp.h"
#include "util/stream.h"
#include "util/string.h"

class INPUT_AS {
public:
	undefined4 m_button;  // 0x00
	undefined4 m_wheel;   // 0x04
	float m_worldX;       // 0x08
	float m_worldY;       // 0x0c
	float m_x;            // 0x10
	float m_y;            // 0x14
	undefined4 m_key;     // 0x18
	undefined4 m_unk0x1c; // 0x1c

	static int firstKey1;
	static int firstKey2;
	static int secondKey1;
	static int secondKey2;
	static int prevKey1;
	static int nextKey1;
	static int firstClearForButtonUp;
	static int secondClearForButtonUp;

	INPUT_AS();
	int ProcessEvent(const union SDL_Event& p_event);
	int Save(STREAM* p_stream) const;
	int Load(STREAM* p_stream);
	static int GetKeyByName(STRING p_name);
	void Tact();
	void ClearLClick();
	void ClearRClick();
};

extern int g_keyScrollLeft;
extern int g_keyScrollLeft2;
extern int g_keyScrollRight;
extern int g_keyScrollRight2;
extern int g_keyScrollUp;
extern int g_keyScrollUp2;
extern int g_keyScrollDown;
extern int g_keyScrollDown2;
extern int g_relativeControl;

extern int g_windowPosX;
extern int g_windowPosY;

static_assert(sizeof(INPUT_AS) == 0x20, "INPUT_AS demo records are serialized as 32 raw bytes");

#endif
