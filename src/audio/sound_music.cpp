#include "audio/sound.h"

#include <stdio.h>

// FUNCTION: ALIEN 0x41d050
int SOUND::DisableSound()
{
	int result = m_disabled;
	if (!result) {
		ReleaseDS();
	}
	return result;
}

// FUNCTION: ALIEN 0x41d0a0
int SOUND::DisableMusic()
{
	delete m_music;
	m_music = 0;
	m_fade = 0;
	return 0;
}

// FUNCTION: ALIEN 0x41db80
int SOUND::PauseMusic()
{
	if (m_music) {
		m_music->Pause();
	}
	return 0;
}

// FUNCTION: ALIEN 0x41dbd0
int SOUND::ResumeMusic()
{
	if (m_music) {
		m_music->Resume();
	}
	return 0;
}

// FUNCTION: ALIEN 0x41dc70
int SOUND::StopMusic()
{
	if (m_music) {
		m_music->Stop();
	}
	m_musicName = empty_str;
	m_loop = 0;
	return 0;
}
