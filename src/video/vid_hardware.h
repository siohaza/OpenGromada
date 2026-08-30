#ifndef VID_HARDWARE_H
#define VID_HARDWARE_H

#include "video/vid.h"

class TEXTURE;

class STREAM;
class TERRAIN_COVERAGE;

struct VID_TEXCOOR {
	undefined4 m_unk0x00; // 0x00
	int m_texture;        // 0x04
	int m_x;              // 0x08
	int m_y;              // 0x0c
	int m_w;              // 0x10
	int m_h;              // 0x14
	int m_offsetX;        // 0x18
	int m_offsetY;        // 0x1c
	int m_next;           // 0x20

	int Intersection(int p_x, int p_y, int p_w, int p_h) const;
};

typedef VID_TEXCOOR VID_CHILD;

// VTABLE: ALIEN 0x47a438

class VID_HARDWARE : public VID {
public:
	VID_HARDWARE()
	{
		m_unk0x484 = 0;
		m_unk0x488 = 0;
		m_unk0x48c = 0;
		m_terrainCoverage = 0;
	}
	VID_HARDWARE(VID_HARDWARE& p_other);
	VID_HARDWARE(int p_idx, int p_w, int p_h);
	virtual void* ScalarDeletingDestructor(unsigned int p_flags);
	~VID_HARDWARE();
	VID* CreateMirror();
	void SetLayer();
	int Draw(SPRITE* p_sprite); // vtable+0x0c
	void Load(RESOURCE* p_res); // vtable+0x18

	VID_TEXCOOR* m_unk0x484;   // 0x484
	unsigned short m_unk0x488; // 0x488
	TEXTURE** m_unk0x48c;      // 0x48c

	TERRAIN_COVERAGE* m_terrainCoverage;

	void DrawVidToVid(const SPRITE* p_sprite);
	TERRAIN_COVERAGE* TakeTerrainCoverage();
};

#endif
