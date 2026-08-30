#ifndef MUSIC_OGG_H
#define MUSIC_OGG_H

#include "audio/music.h"
#include "util/decomp.h"

#define OV_EXCLUDE_STATIC_CALLBACKS
#include "audio/minivorbis.h"

const int MUSIC_OGG_SAMPLES = 8192;

// VTABLE: ALIEN 0x47a640
class MUSIC_OGG : public MUSIC {
public:
	MUSIC_OGG(STRING* p_name);
	~MUSIC_OGG();
	void Play();
	void Tact();
	void Stop();
	void Pause();
	void Resume();
	int IsPlaying();
	void SetVolume(int p_volume);
	void Mix(float* p_out, int p_frames);

	int m_unk0x0c;         // 0x0c
	int m_state;           // 0x10
	OggVorbis_File m_file; // 0x18
	FILE* m_fileHandle;    // 0x2e8

	int m_running;
	int m_volume;
	int m_channels;
	int m_rate;
	short m_pcm[MUSIC_OGG_SAMPLES];
	int m_frames;
	double m_pos;

private:
	void Fill();
};

// SYNTHETIC: ALIEN 0x41c3c0
// MUSIC_OGG::`scalar deleting destructor'

#endif
