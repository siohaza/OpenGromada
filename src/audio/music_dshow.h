#ifndef MUSIC_DSHOW_H
#define MUSIC_DSHOW_H

#include "audio/music.h"
#include "util/decomp.h"

struct IGraphBuilder;
struct IMediaControl;
struct IMediaSeeking;

// VTABLE: ALIEN 0x47a680

class MUSIC_DSHOW : public MUSIC {
public:
	IGraphBuilder* m_graphBuilder; // 0x08
	IMediaControl* m_mediaControl; // 0x0c
	IMediaSeeking* m_mediaSeeking; // 0x10

	MUSIC_DSHOW(const STRING& p_name);
	~MUSIC_DSHOW();
	void Play();
	int IsPlaying();
	void Stop();
	void Pause();
	void Resume();
	void SetVolume(int p_volume);
};

DECOMP_SIZE_ASSERT(MUSIC_DSHOW, 0x14)

// SYNTHETIC: ALIEN 0x41c960
// MUSIC_DSHOW::`scalar deleting destructor'

#endif
