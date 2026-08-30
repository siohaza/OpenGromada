#include "audio/mixer.h"

#include "audio/music.h"
#include "audio/sfxbuffer.h"

#include <SDL3/SDL.h>
#include <math.h>

static SDL_AudioStream* s_stream;
static int s_paused;

const int MIXER_QUEUE_MS = 80;

const int MIXER_CHUNK_FRAMES = 512;

const int MIXER_FRAME_BYTES = MIXER_CHANNELS * (int) sizeof(float);

SOUND_SAMPLE::SOUND_SAMPLE(short* p_pcm, int p_frames, int p_channels, int p_rate)
{
	m_pcm = p_pcm;
	m_frames = p_frames;
	m_channels = p_channels;
	m_rate = p_rate;
	m_refs = 1;
}

SOUND_SAMPLE::~SOUND_SAMPLE()
{
	delete[] m_pcm;
}

void SOUND_SAMPLE::Release()
{
	if (--m_refs <= 0) {
		delete this;
	}
}

int SOUND_SAMPLE::Mix(float* p_out, int p_frames, double* p_pos, int p_loop, float p_left, float p_right) const
{
	if (m_frames <= 0) {
		return 0;
	}

	const double step = (double) m_rate / MIXER_RATE;
	const double scale = 1.0 / 32768.0;
	double pos = *p_pos;
	float* out = p_out;

	for (int i = 0; i < p_frames; ++i) {
		if (pos >= m_frames) {
			if (!p_loop) {
				*p_pos = pos;
				return 0;
			}
			do {
				pos -= m_frames;
			} while (pos >= m_frames);
		}

		int index = (int) pos;
		double frac = pos - index;
		int next = index + 1;
		if (next >= m_frames) {
			next = p_loop ? 0 : index;
		}

		const short* a = m_pcm + index * m_channels;
		const short* b = m_pcm + next * m_channels;
		double left = a[0] + (b[0] - a[0]) * frac;
		double right = m_channels > 1 ? a[1] + (b[1] - a[1]) * frac : left;

		*out++ += (float) (left * scale) * p_left;
		*out++ += (float) (right * scale) * p_right;
		pos += step;
	}

	*p_pos = pos;
	return 1;
}

float Mixer_Gain(int p_centibels)
{
	if (p_centibels >= 0) {
		return 1.0f;
	}

	if (p_centibels <= -10000) {
		return 0.0f;
	}
	return powf(10.0f, (float) p_centibels / 2000.0f);
}

int Mixer_Open()
{
	if (s_stream) {
		return 0;
	}
	if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
		return 1;
	}

	SDL_AudioSpec spec;
	spec.format = SDL_AUDIO_F32;
	spec.channels = MIXER_CHANNELS;
	spec.freq = MIXER_RATE;

	s_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, 0, 0);
	if (!s_stream) {
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return 1;
	}

	s_paused = 0;
	SDL_ResumeAudioStreamDevice(s_stream);
	return 0;
}

void Mixer_Close()
{
	if (!s_stream) {
		return;
	}
	SDL_DestroyAudioStream(s_stream);
	s_stream = 0;
	s_paused = 0;
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void Mixer_Pause()
{
	if (!s_stream || s_paused) {
		return;
	}
	s_paused = 1;
	SDL_PauseAudioStreamDevice(s_stream);
	SDL_ClearAudioStream(s_stream);
}

void Mixer_Resume()
{
	if (!s_stream || !s_paused) {
		return;
	}
	s_paused = 0;
	SDL_ResumeAudioStreamDevice(s_stream);
}

void Mixer_Pump(SFXBUFFER* p_channels, int p_noChannels, MUSIC* p_music)
{
	if (!s_stream || s_paused) {
		return;
	}

	int queued = SDL_GetAudioStreamQueued(s_stream);
	if (queued < 0) {
		queued = 0;
	}
	int frames = MIXER_RATE * MIXER_QUEUE_MS / 1000 - queued / MIXER_FRAME_BYTES;

	while (frames > 0) {
		int chunk = frames < MIXER_CHUNK_FRAMES ? frames : MIXER_CHUNK_FRAMES;
		float mix[MIXER_CHUNK_FRAMES * MIXER_CHANNELS];
		SDL_memset(mix, 0, (size_t) chunk * MIXER_FRAME_BYTES);

		if (p_music) {
			p_music->Mix(mix, chunk);
		}
		for (int i = 0; i < p_noChannels; ++i) {
			p_channels[i].Mix(mix, chunk);
		}

		for (int i = 0; i < chunk * MIXER_CHANNELS; ++i) {
			if (mix[i] > 1.0f) {
				mix[i] = 1.0f;
			}
			else if (mix[i] < -1.0f) {
				mix[i] = -1.0f;
			}
		}

		SDL_PutAudioStreamData(s_stream, mix, chunk * MIXER_FRAME_BYTES);
		frames -= chunk;
	}
}
