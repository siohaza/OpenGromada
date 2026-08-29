#ifndef MUSIC_OGG_H
#define MUSIC_OGG_H

#include "util/decomp.h"
#include <dxsdk/dsound.h>
#include "audio/music.h"

#define OV_EXCLUDE_STATIC_CALLBACKS
#include "audio/minivorbis.h"

// VTABLE: ALIEN 0x47a640
class MUSIC_OGG : public MUSIC {
public:
	IDirectSoundBuffer* m_buffer; // 0x08
	int m_unk0x0c; // 0x0c
	int m_state; // 0x10
	undefined m_unk0x14[0x4]; // 0x14
	OggVorbis_File m_file; // 0x18
	FILE* m_fileHandle; // 0x2e8
	unsigned int m_writePos; // 0x2ec

	MUSIC_OGG(STRING* p_name);
	~MUSIC_OGG();
	void Play();
	void Tact();
	void Stop();
	void Pause();
	void Resume();
	int IsPlaying();
	void SetVolume(int p_volume);
};

DECOMP_STATIC_ASSERT(sizeof(OggVorbis_File) == 0x2d0)
DECOMP_SIZE_ASSERT(MUSIC_OGG, 0x2f0)

// SYNTHETIC: ALIEN 0x41c3c0
// MUSIC_OGG::`scalar deleting destructor'

#endif
