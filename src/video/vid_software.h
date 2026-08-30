#ifndef VID_SOFTWARE_H
#define VID_SOFTWARE_H

#include "video/vid.h"

class STREAM;

// VTABLE: ALIEN 0x47a5a8

class VID_SOFTWARE : public VID {
public:
	VID_SOFTWARE();
	virtual void* ScalarDeletingDestructor(unsigned int p_flags);
	~VID_SOFTWARE();
	VID_SOFTWARE(STREAM* p_stream) throw();

	int* m_unk0x484;  // 0x484
	int m_unk0x488;   // 0x488
	void* m_unk0x48c; // 0x48c

	int Draw(SPRITE* p_sprite);
	int SetGamma(const GAMMA& p_gamma, unsigned int p_idx);
	void DrawShadow(SPRITE* p_sprite);
	void Load(RESOURCE* p_res);
	virtual int PaletteSize();                                                      // vtable+0x28
	virtual void SetGammaToPalette(unsigned char* p_palette, const GAMMA& p_gamma); // vtable+0x2c
	int HaveShadow();
	VID* CreateMirror();
	void SetLayer();
};

#endif
