#include "audio/sound.h"

#include <stdio.h>

// FUNCTION: ALIEN 0x41d050
int SOUND::DisableSound()
{
	int result = m_disabled;
	if (!result) {
		ReleaseDS();
		result = m_waveDevice;
		if (result >= 0)
			result = waveOutSetVolume((HWAVEOUT) result, m_waveVolume);
	}
	return result;
}

// FUNCTION: ALIEN 0x41d0a0
int SOUND::DisableMusic()
{
	delete m_music;
	m_music = 0;
	mciSendStringA("stop FWMUSIC", 0, 0, (HWND) 0);
	mciSendStringA("close FWMUSIC", 0, 0, (HWND) 0);
	int result = m_auxDevice;
	if (result >= 0)
		result = auxSetVolume(result, m_auxVolume);
	m_fade = 0;
	m_unk0x3d4 = -1;
	return result;
}

// FUNCTION: ALIEN 0x41db80
int SOUND::PauseMusic()
{
	if (m_music)
		m_music->Pause();
	if (m_auxDevice >= 0)
		auxSetVolume(m_auxDevice, m_auxVolume);
	// STRING: ALIEN 0x482e5c
	return mciSendStringA("stop FWMUSIC", 0, 0, m_hwnd) != 0;
}

// FUNCTION: ALIEN 0x41dbd0
int SOUND::ResumeMusic()
{
	char buf[0x100];
	if (m_music)
		m_music->Resume();
	if (m_auxDevice >= 0)
		auxSetVolume(m_auxDevice, m_auxResumeVolume);
	if (m_unk0x3d8 >= 0) {
		// STRING: ALIEN 0x482eec
		sprintf(buf, "play FWMUSIC to %i notify", m_unk0x3d4 + 1);
		if (mciSendStringA(buf, 0, 0, m_hwnd))
			return 1;
	}
	// STRING: ALIEN 0x482ed8
	else if (mciSendStringA("play FWMUSIC notify", 0, 0, m_hwnd))
		return 1;
	return 0;
}

// FUNCTION: ALIEN 0x41dc70
int SOUND::StopMusic()
{
	if (m_music)
		m_music->Stop();
	m_musicName = empty_str;
	m_loop = 0;
	m_unk0x3d8 = -1;
	mciSendStringA("stop FWMUSIC", 0, 0, (HWND) 0);
	// STRING: ALIEN 0x482e4c
	return mciSendStringA("close FWMUSIC", 0, 0, (HWND) 0) != 0;
}
