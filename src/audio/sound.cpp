#include "audio/sound.h"

#include "audio/music_ogg.h"
#include "game/gametime.h"
#include "platform/paths.h"
#include "util/game_random.h"
#include "util/myerror.h"

#include <SDL3/SDL.h>
#include <stdlib.h>
#include <string.h>
#include <string>

// GLOBAL: ALIEN 0x491750
SOUND* Sound;

// FUNCTION: ALIEN 0x41cba0
SOUND::SOUND(RESOURCE* p_res, int p_highQuality)
{
	m_loop = 0;
	m_noSfx = 0;
	m_sfx = 0;
	m_music = 0;
	m_fade = 0;
	m_disabled = 1;
	m_unk0x00 = p_highQuality != 0;
	m_musicVolume = -1;
	m_volume = 100;
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
	if (m_disabled) {
		return 0;
	}
	int result = 0;
	for (int i = 0; i < 16; ++i) {
		if (m_channel[i].m_unk0x08) {
			++result;
		}
	}
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
			if (!g_fadeStartTime || (m_fadeMs <= 0 && RealCurrentTime - g_fadeStartTime > 1000)) {
				g_fadeStartTime = RealCurrentTime;
			}
			int vol;
			if (m_musicVolume >= 0) {
				vol = 32 * (m_musicVolume - 100);
			}
			else {
				vol = 0;
			}
			m_music->SetVolume(vol + m_fade);
			if (m_fadeMs > 0) {
				unsigned int elapsed = RealCurrentTime - g_fadeStartTime;
				m_fade = elapsed >= (unsigned int) m_fadeMs ? -3001 : -1 - (int) (3000 * elapsed / (unsigned int) m_fadeMs);
			}
			else {
				m_fade += (int) (RealCurrentTime - g_fadeStartTime) / -2;
			}
			if (m_fade < -3000) {
				g_fadeStartTime = m_fade = 0;
				if (strcmp(m_musicName.m_str, empty_str)) {
					PlayFile(m_musicName, m_loop);
				}
				else {
					StopMusic();
				}
			}
		}
		else if (!(GameRand() % 16) && !m_music->IsPlaying()) {
			if (strcmp(m_musicName.m_str, empty_str)) {
				PlayFile(m_musicName, m_loop);
			}
			else {
				StopMusic();
			}
		}
	}

	if (m_disabled) {
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
					if (IsLooped(q->m_sfx) && abs(ch->m_volume - q->m_vol) < 100 && abs(ch->m_pan - q->m_pan) < 100) {
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
			if (q->m_sfx >= 0) {
				++pending;
			}
		}
		++q;
		--n;
	} while (n);
	int freeVoices = 0;
	SFXBUFFER* ch = m_channel;
	int k = 16;
	do {
		if (!ch->IsPlaying()) {
			++freeVoices;
		}
		++ch;
		--k;
	} while (k);
	if (pending > freeVoices) {
		do {
			int found = 0;
			if (m_playing[0].m_sfx < 0) {
				m_playing[0].m_vol = 0;
			}
			int best = 0;
			int i = 0;
			do {
				if (m_playing[i].m_sfx >= 0) {
					if (m_sfx[m_playing[i].m_sfx].m_unk0x20 != 100 && m_playing[i].m_vol <= m_playing[best].m_vol) {
						found = 1;
						best = i;
					}
				}
				++i;
			} while (i < 32);
			if (!found) {
				break;
			}
			m_playing[best].m_sfx = -1;
			--pending;
		} while (pending > freeVoices);
	}

	SFXBUFFER* steal = m_channel;
	int v = 0;
	while (pending > freeVoices) {
		if (v >= 16) {
			break;
		}
		if (steal->m_unk0x08 && m_sfx[steal->m_unk0x0c].m_unk0x20 != 100) {
			steal->m_unk0x08 = 0;
			steal->m_running = 0;
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

	Mixer_Pump(m_channel, 16, m_music);
}

// FUNCTION: ALIEN 0x41dd80
int SOUND::Pause()
{
	PauseMusic();
	int result = PauseSFX();
	Mixer_Pause();
	return result;
}

// FUNCTION: ALIEN 0x41dda0
int SOUND::Resume()
{
	Mixer_Resume();
	ResumeMusic();
	return ResumeSFX();
}

// FUNCTION: ALIEN 0x41ddc0
int SOUND::VolumeSound(int p_volume)
{
	if (p_volume < 0) {
		p_volume = 0;
	}
	else if (p_volume > 100) {
		p_volume = 100;
	}
	return m_volume = p_volume;
}

// FUNCTION: ALIEN 0x41ddf0
void SOUND::VolumeMusic(int p_volume)
{
	int vol = p_volume;
	if (vol < 0) {
		vol = 0;
		if (m_music) {
			DisableMusic();
		}
	}
	else {
		if (vol > 100) {
			vol = 100;
		}
		else if (!vol) {
			if (m_music) {
				DisableMusic();
			}
		}
	}
	m_musicVolume = vol;
	if (!m_music && vol) {
		PlayFile(m_musicName, m_loop);
	}
	if (m_music) {
		m_music->SetVolume(32 * (vol - 100));
	}
}

// FUNCTION: ALIEN 0x41dec0
int SOUND::FadeAndPlayFile(const STRING& p_file, int p_loop, int p_fadeMs)
{
	if (!m_music) {
		return PlayFile(p_file, p_loop);
	}
	m_fade = -1;
	m_fadeMs = p_fadeMs > 0 ? p_fadeMs : 0;
	if (m_fadeMs > 0) {
		g_fadeStartTime = 0;
	}
	m_musicName = p_file;
	m_loop = p_loop;
	return 0;
}

void SOUND::StopMusicFade(int p_fadeMs)
{
	if (p_fadeMs <= 0 || !m_music) {
		StopMusic();
		return;
	}
	m_fade = -1;
	m_fadeMs = p_fadeMs;
	g_fadeStartTime = 0;
	m_musicName = empty_str;
}

// FUNCTION: ALIEN 0x41df20
int SOUND::PlayFile(STRING p_file, int p_loop)
{
	if (!strcmp(p_file.m_str, empty_str)) {
		return 1;
	}
	m_loop = p_loop;
	m_fade = 0;
	m_fadeMs = 0;
	m_musicName = p_loop ? p_file : STRING(empty_str);
	if (m_music && !strcmp(p_file.m_str, m_music->m_name.m_str)) {
	}
	else {
		if (m_music) {
			delete m_music;
		}
		m_music = 0;
		if (!m_musicVolume) {
			return 0;
		}
		// Match the OGG extension case-insensitively.
		const char* dot = strrchr(p_file.m_str, '.');
		if (dot && _stricmp(dot, ".ogg") == 0) {
			m_music = new MUSIC_OGG(&p_file);
		}
		else {
			MYERROR::Error(::Error, "SOUND '%s'", 5, "music format is not supported", 0, p_file.m_str);
			return 0;
		}
	}
	if (!m_music) {
		return 0;
	}
	if (m_musicVolume > 0) {
		m_music->SetVolume(32 * (m_musicVolume - 100));
	}
	m_music->Play();
	return 0;
}

// The caller retains ownership of the Platform_FOpen handle.
static size_t OggRead(void* p_buf, size_t p_size, size_t p_count, void* p_file)
{
	return fread(p_buf, p_size, p_count, (FILE*) p_file);
}

static int OggSeek(void* p_file, ogg_int64_t p_offset, int p_whence)
{
#ifdef __MINGW32__
	return fseeko64((FILE*) p_file, p_offset, p_whence);
#elif defined(_WIN32)
	return _fseeki64((FILE*) p_file, p_offset, p_whence);
#else
	return fseek((FILE*) p_file, p_offset, p_whence);
#endif
}

static long OggTell(void* p_file)
{
	return ftell((FILE*) p_file);
}

static ov_callbacks g_oggCallbacks = {OggRead, OggSeek, 0, OggTell};

static FILE* OpenWavAsset(const char* p_name)
{
	FILE* file = Platform_FOpen(p_name, "rb");
	if (file) {
		return file;
	}

	// Retry the add-on-era nested sound beside other Wav assets, without a
	// global basename search.
	const char* first = strpbrk(p_name, "/\\");
	const char* slash = strrchr(p_name, '/');
	const char* backslash = strrchr(p_name, '\\');
	const char* last = !slash || (backslash && backslash > slash) ? backslash : slash;
	if (!first || !last || first == last || !last[1]) {
		return 0;
	}

	std::string flattened(p_name, (size_t) (first - p_name + 1));
	flattened += last + 1;
	return Platform_FOpen(flattened.c_str(), "rb");
}

// FUNCTION: ALIEN 0x41e1d0
int SOUND::OpenOgg(STRING* p_name, OggVorbis_File* p_vf, FILE** p_file)
{
	*p_file = 0;
	if (m_disabled) {
		return 1;
	}

	FILE* file = *p_name->m_str ? Platform_FOpen(p_name->m_str, "rb") : 0;
	if (!file) {
		MYERROR::Error(::Error, "SOUND", 7, p_name->m_str, 0);
		return 1;
	}
	if (ov_open_callbacks(file, p_vf, 0, 0, g_oggCallbacks) < 0) {
		MYERROR::Error(::Error, "SOUND", 4, p_name->m_str, 0);
		fclose(file);
		return 1;
	}
	*p_file = file;
	return 0;
}

SOUND_SAMPLE* SOUND::CreateOggSample(STRING* p_name)
{
	OggVorbis_File vf;
	FILE* file;
	if (OpenOgg(p_name, &vf, &file)) {
		return 0;
	}

	SOUND_SAMPLE* sample = 0;
	vorbis_info* info = ov_info(&vf, -1);
	long total = (long) ov_pcm_total(&vf, -1);
	if (info && info->channels > 0 && total > 0) {
		short* pcm = new short[(size_t) total * info->channels];
		long done = 0;
		while (done < total) {
			int bitstream;
			long want = (total - done) * info->channels * (long) sizeof(short);
			if (want > 65536) {
				want = 65536;
			}
			long got = ov_read(&vf, (char*) (pcm + done * info->channels), (int) want, 0, 2, 1, &bitstream);
			if (got < 0) {
				MYERROR::Error(::Error, "SFX", 10, "decode ogg", (int) got);
				break;
			}
			if (!got) {
				break;
			}
			done += got / (info->channels * (long) sizeof(short));
		}
		if (done > 0) {
			sample = new SOUND_SAMPLE(pcm, (int) done, info->channels, info->rate);
		}
		else {
			delete[] pcm;
		}
	}

	ov_clear(&vf);
	fclose(file);
	return sample;
}

// FUNCTION: ALIEN 0x41e370
SOUND_SAMPLE* SOUND::CreateWavSample(STRING* p_name)
{
	if (m_disabled) {
		return 0;
	}

	FILE* file = *p_name->m_str ? OpenWavAsset(p_name->m_str) : 0;
	if (!file) {
		MYERROR::Error(::Error, "SOUND", 7, p_name->m_str, 0);
		return 0;
	}

	long size = compat_filelength(file);
	if (size <= 0 || size > 0x4000000) {
		MYERROR::Log(::Error, "!!!ERROR!!!SFX:'%s' incorrect size %i", p_name->m_str, (int) size);
		fclose(file);
		return 0;
	}
	unsigned char* raw = new unsigned char[size];
	if (!raw || fread(raw, 1, size, file) != (size_t) size) {
		MYERROR::Log(::Error, "!!!ERROR!!!SFX:'%s' incorrect size %i", p_name->m_str, (int) size);
		delete[] raw;
		fclose(file);
		return 0;
	}
	fclose(file);

	SDL_AudioSpec spec;
	unsigned char* wav;
	unsigned int wavSize;
	if (!SDL_LoadWAV_IO(SDL_IOFromConstMem(raw, size), true, &spec, &wav, &wavSize)) {
		MYERROR::Log(::Error, "!!!ERROR!!!SFX:'%s' %s", p_name->m_str, SDL_GetError());
		delete[] raw;
		return 0;
	}
	delete[] raw;

	if (spec.format != SDL_AUDIO_S16) {
		SDL_AudioSpec dst = spec;
		dst.format = SDL_AUDIO_S16;
		unsigned char* converted;
		int convertedSize;
		if (!SDL_ConvertAudioSamples(&spec, wav, (int) wavSize, &dst, &converted, &convertedSize)) {
			MYERROR::Log(::Error, "!!!ERROR!!!SFX:'%s' %s", p_name->m_str, SDL_GetError());
			SDL_free(wav);
			return 0;
		}
		SDL_free(wav);
		wav = converted;
		wavSize = (unsigned int) convertedSize;
		spec = dst;
	}

	int frames = spec.channels > 0 ? (int) (wavSize / (sizeof(short) * spec.channels)) : 0;
	if (frames <= 0) {
		MYERROR::Log(::Error, "!!!ERROR!!!SFX:'%s' incorrect size %i", p_name->m_str, (int) wavSize);
		SDL_free(wav);
		return 0;
	}

	short* pcm = new short[(size_t) frames * spec.channels];
	memcpy(pcm, wav, (size_t) frames * spec.channels * sizeof(short));
	SDL_free(wav);
	return new SOUND_SAMPLE(pcm, frames, spec.channels, spec.freq);
}
