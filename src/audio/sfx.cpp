#include "audio/sfx.h"

#include "audio/sound.h"
#include "util/game_random.h"

#include <stdlib.h>
#include <string.h>

// FUNCTION: ALIEN 0x41d3d0
SFX::SFX()
{
	memset(m_samples, 0, sizeof(m_samples));
	m_unk0x20 = 0;
	m_unk0x44 = 0;
	m_property = 0;
	m_volume = 0;
}

// FUNCTION: ALIEN 0x41d400
SFX::~SFX()
{
	Release();
}

// FUNCTION: ALIEN 0x41e7e0
void SFX::Release()
{
	for (int i = 0; i < 8; ++i) {
		if (m_samples[i]) {
			m_samples[i]->Release();
			m_samples[i] = 0;
		}
	}
}

// STUB: ALIEN 0x41e810
void SFX::Load(STRING* p_names, int p_flag, SOUND* p_sound)
{
	Release();
	for (int i = 0; i < 8; ++i) {
		m_names[i] = p_names[i];
	}
	m_unk0x20 = (unsigned char) p_flag;
	m_unk0x44 = 0;

	for (int i = 0; i < 8; ++i) {
		if (!*m_names[i].m_str) {
			break;
		}
		const char* dot = strrchr(m_names[i].m_str, '.');
		if (dot && _stricmp(dot, ".ogg") == 0) {
			m_samples[i] = p_sound->CreateOggSample(&m_names[i]);
		}
		else {
			m_samples[i] = p_sound->CreateWavSample(&m_names[i]);
		}
		if (!m_samples[i]) {
			return;
		}
		m_unk0x44 = i + 1;
	}
}

// FUNCTION: ALIEN 0x41eb30
SOUND_SAMPLE* SFX::Play()
{
	if (m_unk0x44 <= 0) {
		return 0;
	}
	SOUND_SAMPLE* sample = m_samples[GameRand() % m_unk0x44];
	if (sample) {
		sample->AddRef();
	}
	return sample;
}
