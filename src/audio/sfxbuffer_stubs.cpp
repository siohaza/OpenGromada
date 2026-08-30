#include "audio/sfxbuffer.h"
#include "audio/sound.h"
#include "game/gametime.h"

// FUNCTION: ALIEN 0x41e650
int SFXBUFFER::Resume()
{
	int result = m_unk0x0c;
	if (result >= 0 && m_sample && m_unk0x08) {
		m_running = 1;
	}
	return result;
}

// FUNCTION: ALIEN 0x41e6e0
int SFXBUFFER::IsPlaying()
{
	if (m_unk0x0c < 0) {
		return 0;
	}
	if (!m_sample || !m_unk0x08) {
		return 0;
	}

	SFX* sfx = Sound->m_sfx;
	int id = m_unk0x0c;
	if (sfx && id >= 0 && id <= Sound->m_noSfx) {
		SFX* entry = &sfx[id];
		int looped = 0;
		if (entry->m_samples[0]) {
			looped = !entry->m_unk0x20;
		}
		if (looped && CurrentTime - m_unk0x18 > 100) {
			m_unk0x08 = 0;
			m_running = 0;
		}
	}
	return m_unk0x08;
}
