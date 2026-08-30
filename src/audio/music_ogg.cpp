#include "audio/music_ogg.h"

#include "audio/mixer.h"
#include "audio/sound.h"
#include "util/myerror.h"

#include <stdio.h>
#include <string.h>

// FUNCTION: ALIEN 0x41c310
MUSIC_OGG::MUSIC_OGG(STRING* p_name) : MUSIC(*p_name)
{
	m_unk0x0c = 0;
	m_state = 0;
	m_running = 0;
	m_volume = 0;
	m_channels = 0;
	m_rate = MIXER_RATE;
	m_frames = 0;
	m_pos = 0;
	m_fileHandle = 0;

	if (Sound->OpenOgg(p_name, &m_file, &m_fileHandle)) {
		return;
	}

	vorbis_info* info = ov_info(&m_file, -1);
	if (!info || info->channels < 1) {
		MYERROR::Error(
			::Error,
			"MUSIC '%s'",
			4,
			// STRING: ALIEN 0x482d3c
			"SoundBuffer",
			0,
			m_name.m_str
		);
		ov_clear(&m_file);
		fclose(m_fileHandle);
		m_fileHandle = 0;
		return;
	}
	m_channels = info->channels;
	m_rate = info->rate;
}

// FUNCTION: ALIEN 0x41c3e0
MUSIC_OGG::~MUSIC_OGG()
{
	Stop();
	if (m_fileHandle) {
		ov_clear(&m_file);
		fclose(m_fileHandle);
		m_fileHandle = 0;
	}
}

// FUNCTION: ALIEN 0x41c4c0
void MUSIC_OGG::Play()
{
	if (!m_fileHandle) {
		return;
	}

	ov_pcm_seek(&m_file, 0);
	m_frames = 0;
	m_pos = 0;
	m_state = 0;
	Fill();
	m_unk0x0c = 1;
	m_running = 1;
}

// FUNCTION: ALIEN 0x41c540
void MUSIC_OGG::Stop()
{
	m_unk0x0c = 0;
	m_running = 0;
}

// FUNCTION: ALIEN 0x41c560
void MUSIC_OGG::Pause()
{
	if (m_unk0x0c) {
		m_running = 0;
	}
}

// FUNCTION: ALIEN 0x41c580
void MUSIC_OGG::Resume()
{
	if (m_unk0x0c) {
		m_running = 1;
	}
}

// FUNCTION: ALIEN 0x41c5a0
int MUSIC_OGG::IsPlaying()
{
	if (!m_running) {
		m_unk0x0c = 0;
	}
	return m_unk0x0c;
}

// FUNCTION: ALIEN 0x41c5e0
void MUSIC_OGG::SetVolume(int p_volume)
{
	m_volume = p_volume;
}

void MUSIC_OGG::Fill()
{
	if (!m_fileHandle || m_channels < 1) {
		return;
	}

	int done = (int) m_pos;
	if (done > 0) {
		if (m_frames > done) {
			memmove(m_pcm, m_pcm + done * m_channels, (size_t) (m_frames - done) * m_channels * sizeof(short));
		}
		m_frames = m_frames > done ? m_frames - done : 0;
		m_pos -= done;
	}

	int capacity = MUSIC_OGG_SAMPLES / m_channels;
	int errors = 0;
	while (!m_state && m_frames < capacity) {
		int bitstream;
		long got = ov_read(
			&m_file,
			(char*) (m_pcm + m_frames * m_channels),
			(capacity - m_frames) * m_channels * (int) sizeof(short),
			0,
			2,
			1,
			&bitstream
		);
		if (!got) {
			m_state = 1;
		}
		else if (got < 0) {
			MYERROR::Error(
				::Error,
				"MUSIC '%s'",
				10,
				// STRING: ALIEN 0x482d60
				"decode",
				(int) got,
				m_name.m_str
			);
			if (++errors >= 8) {
				m_state = 1;
			}
		}
		else {
			m_frames += (int) (got / (m_channels * (long) sizeof(short)));
		}
	}
}

// FUNCTION: ALIEN 0x41c600
void MUSIC_OGG::Tact()
{
	if (m_running) {
		Fill();
	}
}

void MUSIC_OGG::Mix(float* p_out, int p_frames)
{
	if (!m_running || !m_fileHandle || m_channels < 1) {
		return;
	}

	Fill();

	float gain = Mixer_Gain(m_volume);
	const double step = (double) m_rate / MIXER_RATE;
	const double scale = 1.0 / 32768.0;
	double pos = m_pos;
	float* out = p_out;

	for (int i = 0; i < p_frames; ++i) {
		int index = (int) pos;
		int next = index + 1;
		if (next >= m_frames) {
			if (index >= m_frames || !m_state) {
				break;
			}
			next = index;
		}

		const short* a = m_pcm + index * m_channels;
		const short* b = m_pcm + next * m_channels;
		double frac = pos - index;
		double left = a[0] + (b[0] - a[0]) * frac;
		double right = m_channels > 1 ? a[1] + (b[1] - a[1]) * frac : left;

		*out++ += (float) (left * scale) * gain;
		*out++ += (float) (right * scale) * gain;
		pos += step;
	}

	m_pos = pos;
	if (m_state && (int) m_pos >= m_frames) {
		Stop();
	}
}
