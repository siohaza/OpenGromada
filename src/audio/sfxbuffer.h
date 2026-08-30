#ifndef SFXBUFFER_H
#define SFXBUFFER_H

#include "audio/mixer.h"
#include "util/decomp.h"

// VTABLE: ALIEN 0x47a6a0

class SFXBUFFER {
public:
	SFXBUFFER()
	{
		m_sample = 0;
		m_unk0x08 = 0;
		m_unk0x0c = -1;
		m_volume = 0;
		m_pan = 0;
		m_unk0x18 = 0;
		m_pos = 0;
		m_loop = 0;
		m_running = 0;
	}
	virtual ~SFXBUFFER(); // vtable+0x00

	SOUND_SAMPLE* m_sample; // 0x04
	undefined4 m_unk0x08;   // 0x08
	int m_unk0x0c;          // 0x0c
	int m_volume;           // 0x10
	int m_pan;              // 0x14
	undefined4 m_unk0x18;   // 0x18

	double m_pos;
	int m_loop;
	int m_running;

	int Play(int p_pan, int p_volume);
	void Release();
	int Resume();
	int IsPlaying();
	void SetSample(int p_id, SOUND_SAMPLE* p_sample);
	void Mix(float* p_out, int p_frames);
};

// SYNTHETIC: ALIEN 0x41cc70
// SFXBUFFER::`scalar deleting destructor'

#endif
