#ifndef MENU_H
#define MENU_H

#include "sprite/list_sprite.h"
#include "util/decomp.h"

#include <string>
#include <vector>

class SPRITE;
class INPUT_AS;
class STRING;

// VTABLE: ALIEN 0x47a810

class MENU : public LIST_SPRITE {
public:
	MENU();

	int Control(INPUT_AS* p_input);
	unsigned int m_state;  // 0x10
	SPRITE* m_underCursor; // 0x14

	int NVidUnderCursor() const;
	unsigned int NDirUnderCursor() const;
	int Load(const STRING& p_name, int p_opt = 0);
	int DeleteFromFile(const STRING& p_name);
	void DeleteAll();
	void ClearScriptCanvas();
	bool HasScriptCanvas() const;
	int ScriptCanvasWidth() const;
	int ScriptCanvasHeight() const;
	float ScriptScreenX(float p_frameX) const;
	float ScriptScreenY(float p_frameY) const;

private:
	std::vector<std::string> m_scriptCanvasMenus;
};

#endif
