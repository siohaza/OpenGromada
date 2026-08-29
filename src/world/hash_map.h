#ifndef HASH_MAP_H
#define HASH_MAP_H

#include "sprite/list_sprite.h"
#include "util/decomp.h"

class SPRITE;
class VID;

class HASH_MAP {
public:
	HASH_MAP(float p_w, float p_h, VID** p_vids, int p_noVid);
	~HASH_MAP();

	int m_x0; // 0x00
	int m_y0; // 0x04
	int m_x1; // 0x08
	int m_y1; // 0x0c
	int m_curX; // 0x10
	int m_curIdx; // 0x14
	int m_iter; // 0x18
	int m_shift; // 0x1c
	int m_maxX; // 0x20
	int m_maxY; // 0x24
	float m_cellW; // 0x28
	float m_cellH; // 0x2c
	SPRITE_LIST* m_cells; // 0x30
	SPRITE_LIST m_list; // 0x34

	void Insert(SPRITE* p_sprite);
	int Delete(SPRITE* p_sprite);
	void ChangeCoor(SPRITE* p_sprite, float p_x, float p_y);
	int CanPlace(VID* p_vid, float p_x, float p_y, float p_z);
	int AskLine(VID* p_vid, float p_x, float p_y, float p_z, float* p_lx, float* p_ly, float* p_lz);
	SPRITE* FirstInBox(float p_left, float p_top, float p_right, float p_bot);
	SPRITE* NextInBox();
};

DECOMP_SIZE_ASSERT(HASH_MAP, 0x44)

extern HASH_MAP* Hash;

#endif
