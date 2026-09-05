#include "audio/sfxbuffer.h"
#include "audio/sound.h"
#include "game/game_descriptor.h"
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

	if (Sound->IsLooped(m_unk0x0c) && CurrentTime - m_unk0x18 > GameDesc->SfxLoopTimeoutMs()) {
		m_unk0x08 = 0;
		m_running = 0;
	}
	return m_unk0x08;
}
