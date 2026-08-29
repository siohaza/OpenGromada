#ifndef SFXBUFFER_H
#define SFXBUFFER_H

#include "util/decomp.h"
#include <dxsdk/dsound.h>

// VTABLE: ALIEN 0x47a6a0

class SFXBUFFER {
public:
	SFXBUFFER()
	{
		m_buffer = 0;
		m_unk0x08 = 0;
		m_unk0x0c = -1;
	}
	virtual ~SFXBUFFER(); // vtable+0x00

	IDirectSoundBuffer* m_buffer; // 0x04
	undefined4 m_unk0x08; // 0x08
	int m_unk0x0c; // 0x0c
	int m_volume; // 0x10
	int m_pan; // 0x14
	undefined4 m_unk0x18; // 0x18

	int Play(int p_pan, int p_volume);
	int Release();
	int Resume();
	int IsPlaying();
	char* SetBuffer(int p_id, IDirectSoundBuffer* p_buffer);
};

DECOMP_SIZE_ASSERT(SFXBUFFER, 0x1c)

// SYNTHETIC: ALIEN 0x41cc70
// SFXBUFFER::`scalar deleting destructor'

#endif
