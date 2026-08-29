#define DECOMP_INLINE_STRING_DTOR
#define DECOMP_INLINE_SFX_DTOR
#include "audio/sound.h"

#include <string.h>

#include <stdlib.h>

#include "audio/music_dshow.h"
#include "audio/music_ogg.h"
#include "game/gametime.h"
#include "util/myerror.h"
#include "util/resource.h"

// GLOBAL: ALIEN 0x491750
SOUND* Sound;

// FUNCTION: ALIEN 0x41cba0
SOUND::SOUND(HWND p_hwnd, RESOURCE* p_res, int p_enabled)
{
	m_loop = 0;
	m_noSfx = 0;
	m_sfx = 0;
	m_directSound = 0;
	m_music = 0;
	m_fade = 0;
	m_unk0x3d8 = -1;
	m_disabled = 1;
	m_unk0x00 ^= (m_unk0x00 ^ (p_enabled != 0)) & 1;
	m_waveDevice = -1;
	m_auxDevice = -1;
	m_musicVolume = -1;
	m_volume = 100;
	m_hwnd = p_hwnd;
	InitDS();
	LoadSFX(p_res);
}

// FUNCTION: ALIEN 0x41cc90
SOUND::~SOUND()
{
	delete m_music;
	m_music = 0;
	Disable();
	delete[] m_sfx;
	m_sfx = 0;
	m_noSfx = 0;
}

// FUNCTION: ALIEN 0x41d020
int SOUND::GetNoPlayed()
{
	if (m_disabled)
		return 0;
	int result = 0;
	undefined4* p = &m_channel[15].m_unk0x08;
	int v3 = 16;
	do {
		if (*p)
			++result;
		p -= 7;
		--v3;
	} while (v3);
	return result;
}

// FUNCTION: ALIEN 0x41d080
int SOUND::Disable()
{
	DisableMusic();
	return DisableSound();
}

// GLOBAL: ALIEN 0x491754
static unsigned int g_fadeStartTime;

// FUNCTION: ALIEN 0x41d770
void SOUND::Tact()
{
	if (m_music) {
		m_music->Tact();
		if (m_fade < 0) {
			if (!g_fadeStartTime || RealCurrentTime - g_fadeStartTime > 1000)
				g_fadeStartTime = RealCurrentTime;
			int vol;
			if (m_musicVolume >= 0)
				vol = 32 * (m_musicVolume - 100);
			else
				vol = 0;
			m_music->SetVolume(vol + m_fade);
			m_fade += (int) (RealCurrentTime - g_fadeStartTime) / -2;
			if (m_fade < -3000) {
				g_fadeStartTime = m_fade = 0;
				if (strcmp(m_musicName.m_str, empty_str))
					PlayFile(m_musicName, m_loop);
				else
					StopMusic();
			}
		}
		else if (!(rand() % 16) && !m_music->IsPlaying()) {
			if (strcmp(m_musicName.m_str, empty_str))
				PlayFile(m_musicName, m_loop);
			else
				StopMusic();
		}
	}

	if (m_disabled)
	{
		return;
	}

	int pending = 0;
	SFX_CHANNEL* q = m_playing;
	int n = 32;
	do {
		if (q->m_sfx >= 0) {
			int tries = 0;
			int k = 0;
			SFXBUFFER* ch = m_channel;
			while (k < 16) {
					if (ch->m_unk0x0c == q->m_sfx && ch->m_unk0x08) {
						if (IsLooped(q->m_sfx) && abs(ch->m_volume - q->m_vol) < 100
							&& abs(ch->m_pan - q->m_pan) < 100) {
							q->Clear();
						m_channel[k].Play(q->m_pan, q->m_vol);
						break;
					}
						++tries;
						if (tries > m_sfx[q->m_sfx].MaxVoices()) {
							q->m_sfx = -1;
						break;
					}
				}
				++k;
				++ch;
			}
			if (q->m_sfx >= 0)
				++pending;
		}
		++q;
		--n;
	} while (n);
	int freeVoices = 0;
	SFXBUFFER* ch = m_channel;
	int k = 16;
	do {
		if (!ch->IsPlaying())
			++freeVoices;
		++ch;
		--k;
	} while (k);
	if (pending > freeVoices) {
		do {
			int found = 0;
			if (m_playing[0].m_sfx < 0)
				m_playing[0].m_vol = 0;
			int best = 0;
			int i = 0;
			do {
				if (m_playing[i].m_sfx >= 0) {
					if (m_sfx[m_playing[i].m_sfx].m_unk0x20 != 100
						&& m_playing[i].m_vol <= m_playing[best].m_vol) {
						found = 1;
						best = i;
					}
				}
				++i;
			} while (i < 32);
			if (!found)
				break;
			m_playing[best].m_sfx = -1;
			--pending;
		} while (pending > freeVoices);
	}

	SFXBUFFER* steal = m_channel;
	int v = 0;
	while (pending > freeVoices) {
		if (v >= 16)
			break;
		if (steal->m_unk0x08 && m_sfx[steal->m_unk0x0c].m_unk0x20 != 100) {
			steal->m_unk0x08 = 0;
			if (steal->m_buffer) {
				steal->m_buffer->Stop();
			}
			++freeVoices;
		}
		++v;
		++steal;
	}

	int result;
	q = m_playing;
	n = 32;
		do {
			result = q->m_sfx;
			if (result >= 0) {
				result = StartSFX(q->m_sfx);
				if (result >= 0) {
					result = m_channel[result].Play(q->m_pan, q->m_vol);
				}
				q->m_sfx = -1;
		}
		++q;
		--n;
	} while (n);
}

// FUNCTION: ALIEN 0x41dd80
int SOUND::Pause()
{
	PauseMusic();
	return PauseSFX();
}

// FUNCTION: ALIEN 0x41dda0
int SOUND::Resume()
{
	ResumeMusic();
	return ResumeSFX();
}

// FUNCTION: ALIEN 0x41ddc0
int SOUND::VolumeSound(int p_volume)
{
	if (p_volume < 0)
		p_volume = 0;
	else if (p_volume > 100)
		p_volume = 100;
	return m_volume = p_volume;
}

// FUNCTION: ALIEN 0x41ddf0
void SOUND::VolumeMusic(int p_volume)
{
	int vol = p_volume;
	if (vol < 0) {
		vol = 0;
		if (m_music)
			DisableMusic();
	}
	else {
		if (vol > 100)
			vol = 100;
		else if (!vol) {
			if (m_music)
				DisableMusic();
		}
		if (vol && !m_music) {
			int scaled = (0xffff * vol) / 100;
			int aux = m_auxDevice;
			m_auxResumeVolume = scaled | (scaled << 16);
			if (aux >= 0)
				auxSetVolume(aux, scaled | (scaled << 16));
		}
	}
	m_musicVolume = vol;
	if (!m_music && vol)
		PlayFile(m_musicName, m_loop);
	if (m_music)
		m_music->SetVolume(32 * (vol - 100));
}

// FUNCTION: ALIEN 0x41dec0
int SOUND::FadeAndPlayFile(const STRING& p_file, int p_loop)
{
	if (!m_music)
		return PlayFile(p_file, p_loop);
	m_fade = -1;
	m_musicName = p_file;
	m_loop = p_loop;
	return 0;
}

// FUNCTION: ALIEN 0x41df20
int SOUND::PlayFile(STRING p_file, int p_loop)
{
	if (!strcmp(p_file.m_str, empty_str))
		return 1;
	m_loop = p_loop;
	m_fade = 0;
	m_musicName = p_loop ? p_file : STRING(empty_str, STRING::INLINE_CHARP);
	if (m_music && !strcmp(p_file.m_str, m_music->m_name.m_str)) {
	} else {
		if (m_music)
			delete m_music;
		m_music = 0;
		if (!m_musicVolume)
			return 0;
		if (strstr(p_file.m_str,
				".ogg")
			|| strstr(p_file.m_str,
				".OGG")
			|| strstr(p_file.m_str,
				".Ogg"))
			m_music = new MUSIC_OGG(&p_file);
		else
			m_music = new MUSIC_DSHOW(p_file);
	}
	if (m_musicVolume > 0)
		m_music->SetVolume(32 * (m_musicVolume - 100));
	m_music->Play();
	return 0;
}

// FUNCTION: ALIEN 0x41e1d0
IDirectSoundBuffer* SOUND::CreateOggBuffer(STRING* p_name, OggVorbis_File* p_vf, FILE** p_file, int p_size)
{
	if (!m_directSound) {
		*p_file = 0;
		return 0;
	}
	FILE* file;
	if (*p_name->m_str)
		file = fopen(p_name->m_str, "rb");
	else
		file = 0;
	*p_file = file;
	if (!file) {
		MYERROR::Error(::Error, "SOUND", 7, p_name->m_str, 0);
		return 0;
	}
	if (ov_open(file, p_vf, 0, 0) < 0) {
		MYERROR::Error(::Error, "SOUND", 4, p_name->m_str, 0);
		fclose(*p_file);
		*p_file = 0;
		return 0;
	}
	vorbis_info* info = ov_info(p_vf, -1);
	WAVEFORMATEX wfx;
	wfx.wFormatTag = 1;
	wfx.nChannels = info->channels;
	wfx.nSamplesPerSec = info->rate;
	wfx.nAvgBytesPerSec = 2 * info->rate * info->channels;
	wfx.nBlockAlign = 2 * info->channels;
	DSBUFFERDESC desc;
	memset(&desc, 0, sizeof(desc));
	int size = p_size;
	wfx.wBitsPerSample = 16;
	wfx.cbSize = 18;
	desc.dwSize = 36;
	desc.dwFlags = 0x10082;
	if (!p_size)
		size = (int) (2 * info->channels * ov_pcm_total(p_vf, -1));
	desc.lpwfxFormat = &wfx;
	desc.dwBufferBytes = size;
	IDirectSoundBuffer* buffer;
	int result = m_directSound->CreateSoundBuffer(&desc, &buffer, 0);
	if (result < 0) {
		MYERROR::Error(::Error, "SOUND", 3,
					   // STRING: ALIEN 0x482f20
					   "SoundBuffer for ogg", result);
		return 0;
	}
	return buffer;
}

// FUNCTION: ALIEN 0x41e370
IDirectSoundBuffer* SOUND::CreateWavBuffer(STRING* p_name, RESOURCE* p_res, int p_flag)
{
	if (!m_directSound)
		return 0;
	STRING* name = p_name;
	RESOURCE* res = p_res;
	if (res->OpenForRead(*name, 0x45564157))
		return 0;
	if (res->GoBegin(0x20746d66)) {
		MYERROR::Log(::Error,
			// STRING: ALIEN 0x482f84
			"!!!ERROR!!!SFX:'%s' 'fmt ' not found", name->m_str);
		return 0;
	}
	if ((unsigned int) res->m_resSize < 14) {
		MYERROR::Log(::Error,
			// STRING: ALIEN 0x482f5c
			"!!!ERROR!!!SFX:'%s' incorrect size %i", name->m_str, res->m_resSize);
		return 0;
	}
	WAVEFORMATEX wfx;
	res->Read(&wfx, 18);
	if (res->GoNext(0x61746164) && res->GoBegin(0x61746164)) {
		MYERROR::Log(::Error,
			// STRING: ALIEN 0x482f34
			"!!!ERROR!!!SFX:'%s' 'data' not found", name->m_str);
		return 0;
	}
	DSBUFFERDESC desc;
	memset(&desc, 0, sizeof(desc));
	desc.dwSize = 36;
	desc.dwFlags = 194;
	desc.dwBufferBytes = res->m_resSize;
	desc.lpwfxFormat = &wfx;
	IDirectSoundBuffer* buffer;
	int result = m_directSound->CreateSoundBuffer(&desc, &buffer, 0);
	if (result < 0) {
		MYERROR::Error(::Error, "SOUND", 3,
			"SoundBuffer", result);
		return 0;
	}
	return buffer;
}
