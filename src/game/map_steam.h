#ifndef MAP_STEAM_H
#define MAP_STEAM_H

#include "game/map.h"

// VTABLE: ALIEN 0x47a2a4

class MAP_STEAM : public MAP {
public:
	MAP_STEAM(STRING& p_argv, SETTINGS* p_settings);
	virtual ~MAP_STEAM();
	void DeletePointerToSprite(SPRITE* p_sprite);
	int Tact();
	void DrawSecondaryInfo();
	void Release();
	SPRITE* CreateSprite(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent);
};

// SYNTHETIC: ALIEN 0x405110
// MAP_STEAM::`scalar deleting destructor'

#endif
