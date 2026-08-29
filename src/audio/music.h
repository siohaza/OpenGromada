#ifndef MUSIC_H
#define MUSIC_H

#include "util/decomp.h"
#include "util/string.h"

// VTABLE: ALIEN 0x47a660

class MUSIC {
public:
	MUSIC(const STRING& p_name)
		: m_name(p_name)
	{
	}

	MUSIC(const STRING& p_name, STRING::CALL_COPY_NONNULL_TAG)
		: m_name(p_name.m_str, STRING::CALL_COPY_NONNULL)
	{
	}
	// SYNTHETIC: ALIEN 0x41c480
	// MUSIC::`scalar deleting destructor'
	virtual ~MUSIC() {} // vtable+0x00
	virtual int IsPlaying() { return 0; } // vtable+0x04
	// FUNCTION: ALIEN 0x41f0f0
	virtual void Tact() {} // vtable+0x08
	virtual void Pause() {} // vtable+0x0c
	virtual void Resume() {} // vtable+0x10
	virtual void Stop() {} // vtable+0x14
	virtual void Play() {} // vtable+0x18
	virtual void SetVolume(int p_volume); // vtable+0x1c

	STRING m_name; // 0x04
};

#endif
