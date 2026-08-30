#include "audio/sfxbuffer.h"

#include "audio/sfx.h"
#include "audio/sound.h"
#include "game/gametime.h"

// FUNCTION: ALIEN 0x41e4e0
SFXBUFFER::~SFXBUFFER()
{
	if (m_sample) {
		m_sample->Release();
		m_sample = 0;
	}
}

// FUNCTION: ALIEN 0x41e530
void SFXBUFFER::Release()
{
	if (m_sample) {
		m_sample->Release();
		m_sample = 0;
	}
	m_unk0x08 = 0;
	m_unk0x18 = 0;
	m_unk0x0c = -1;
	m_pos = 0;
	m_running = 0;
	m_loop = 0;
}

// FUNCTION: ALIEN 0x41e560
int SFXBUFFER::Play(int p_pan, int p_volume)
{
	int result = m_unk0x0c;
	if (result >= 0 && m_sample) {
		m_volume = p_volume;
		m_pan = p_pan;
		if (!m_unk0x08) {
			SFX* sfx = Sound->m_sfx;
			int id = m_unk0x0c;
			m_loop = 0;
			if (sfx && id >= 0 && id <= Sound->m_noSfx && sfx[id].m_samples[0]) {
				m_loop = !sfx[id].m_unk0x20;
			}
			m_running = 1;
		}
		m_unk0x08 = 1;
		m_unk0x18 = CurrentTime;
	}
	return result;
}

// FUNCTION: ALIEN 0x41e780
void SFXBUFFER::SetSample(int p_id, SOUND_SAMPLE* p_sample)
{
	m_unk0x0c = p_id;
	if (m_sample) {
		m_sample->Release();
	}
	m_sample = p_sample;
	m_unk0x08 = 0;
	m_unk0x18 = 0;
	m_pos = 0;
	m_running = 0;
	m_loop = 0;
}

void SFXBUFFER::Mix(float* p_out, int p_frames)
{
	if (!m_running || !m_sample) {
		return;
	}

	float gain = Mixer_Gain(m_volume);
	float left = m_pan > 0 ? gain * Mixer_Gain(-m_pan) : gain;
	float right = m_pan < 0 ? gain * Mixer_Gain(m_pan) : gain;

	if (!m_sample->Mix(p_out, p_frames, &m_pos, m_loop, left, right)) {
		m_pos = 0;
		m_running = 0;
		m_unk0x08 = 0;
	}
}
