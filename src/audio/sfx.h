#ifndef SFX_H
#define SFX_H

#include <dxsdk/dsound.h>
#include "util/decomp.h"
#include "util/string.h"

class SFX {
public:
	SFX();
#ifdef DECOMP_INLINE_SFX_DTOR
	~SFX()
	{
		Release();
	}
#else
	~SFX();
#endif

#ifdef DECOMP_INLINE_SFX_DTOR
	STRING m_names[8]; // 0x00
#else
	char* m_names[8]; // 0x00
#endif

	unsigned char m_unk0x20; // 0x20
	undefined m_unk0x21[3]; // 0x21
	IDirectSoundBuffer* m_buffers[8]; // 0x24
	int m_unk0x44; // 0x44

	int MaxVoices() const
	{
		if (m_unk0x20 == 100)
			return 999999;
		if (!m_unk0x20)
			return 1;
		return m_unk0x20 / 10;
	}

	void Load(STRING* p_names, int p_flag, class SOUND* p_sound);
	IDirectSoundBuffer* Play(IDirectSound* p_ds);
	int Release();
};

DECOMP_SIZE_ASSERT(SFX, 0x48)

#endif
