#ifndef MIXER_H
#define MIXER_H

class MUSIC;
class SFXBUFFER;

const int MIXER_RATE = 44100;
const int MIXER_CHANNELS = 2;

class SOUND_SAMPLE {
public:
	SOUND_SAMPLE(short* p_pcm, int p_frames, int p_channels, int p_rate);
	~SOUND_SAMPLE();

	void AddRef() { ++m_refs; }
	void Release();

	int Mix(float* p_out, int p_frames, double* p_pos, int p_loop, float p_left, float p_right) const;

	short* m_pcm;
	int m_frames;
	int m_channels;
	int m_rate;
	int m_refs;
};

int Mixer_Open();
void Mixer_Close();

void Mixer_Pause();
void Mixer_Resume();

void Mixer_Pump(SFXBUFFER* p_channels, int p_noChannels, MUSIC* p_music);

float Mixer_Gain(int p_centibels);

#endif
