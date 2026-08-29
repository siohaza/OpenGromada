#ifndef SOUND_H
#define SOUND_H

#include "util/decomp.h"

#include <stdio.h>
#include "audio/music.h"
#include "audio/sfx.h"
#include "audio/sfxbuffer.h"
#include "util/string.h"

class RESOURCE;

class SFX_CHANNEL {
public:
	SFX_CHANNEL()
	{
		m_sfx = -1;
	}

	void Clear()
	{
		m_sfx = -1;
	}

	int m_sfx; // 0x00
	undefined m_unk0x04[0x4]; // 0x04
	int m_pan; // 0x08
	int m_vol; // 0x0c
};

DECOMP_SIZE_ASSERT(SFX_CHANNEL, 0x10)

class SOUND {
public:
	SOUND(HWND p_hwnd, RESOURCE* p_res, int p_enabled);
	~SOUND();

	unsigned int m_unk0x00; // 0x00
	int m_noSfx; // 0x04
	SFX* m_sfx; // 0x08
	SFX_CHANNEL m_playing[32]; // 0x0c
	HWND m_hwnd; // 0x20c
	IDirectSound8* m_directSound; // 0x210
	SFXBUFFER m_channel[16]; // 0x214
	int m_unk0x3d4; // 0x3d4
	int m_unk0x3d8; // 0x3d8
	undefined m_unk0x3dc[0xc]; // 0x3dc
	int m_disabled; // 0x3e8
	int m_auxDevice; // 0x3ec
	int m_waveDevice; // 0x3f0
	unsigned int m_auxVolume; // 0x3f4
	unsigned int m_waveVolume; // 0x3f8
	unsigned int m_auxResumeVolume; // 0x3fc
	undefined m_unk0x400[0x4]; // 0x400
	int m_volume; // 0x404
	int m_musicVolume; // 0x408
	int m_fade; // 0x40c
	int m_loop; // 0x410
	MUSIC* m_music; // 0x414
	STRING m_musicName; // 0x418

	bool ValidateSFX(int p_sfx) const
	{
		if (!m_sfx)
			return true;
		if (p_sfx < 0 || p_sfx > m_noSfx)
			return false;
		if (!m_sfx[p_sfx].m_buffers[0])
			return false;
		return true;
	}

	int IsLooped(int p_sfx) const
	{
		if (!m_sfx)
			return 0;
		if (!ValidateSFX(p_sfx))
			return 0;
		return m_sfx[p_sfx].m_unk0x20 == 0;
	}

	void InitDS();
	void LoadSFX(RESOURCE* p_res);
	int StartSFX(int p_sfx);
	int GetNoPlayed();
	void ReleaseDS();
	int ReloadSFX();
	int DisableSound();
	int DisableMusic();
	void VolumeMusic(int p_volume);
	int PauseMusic();
	int ResumeMusic();
	int StopMusic();
	IDirectSoundBuffer* CreateOggBuffer(STRING* p_name, struct OggVorbis_File* p_vf, FILE** p_file, int p_size);
	IDirectSoundBuffer* CreateWavBuffer(STRING* p_name, class RESOURCE* p_res, int p_flag);
	int PlayFile(STRING p_file, int p_loop);
	int FadeAndPlayFile(const STRING& p_file, int p_loop);

	void PlaySFX(int p_sfx, int p_pan, int p_volume);
	void PlaySFXFromCoor(int p_sfx, float p_x, float p_y);
	int StopSFX(int p_sfxId);
	int StopAllSFX();
	int PauseSFX();
	int ResumeSFX();

	void Tact();
	int Disable();
	int Pause();
	int Resume();
	int VolumeSound(int p_volume);
};

extern SOUND* Sound;

#endif
