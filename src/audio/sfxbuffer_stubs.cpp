#include "audio/sfxbuffer.h"

#include "audio/sound.h"
#include "game/gametime.h"
#include "util/myerror.h"

// FUNCTION: ALIEN 0x41e650
int SFXBUFFER::Resume()
{
	int result = m_unk0x0c;
	if (result >= 0) {
		IDirectSoundBuffer* buffer = m_buffer;
		if (buffer) {
			if (m_unk0x08) {
				SOUND* sound = Sound;
				SFX* sfx = sound->m_sfx;
				int id = m_unk0x0c;
				int loop = 0;
				if (sfx && id >= 0 && id <= sound->m_noSfx && sfx[id].m_buffers[0])
					loop = !sfx[id].m_unk0x20;
				result = buffer->Play(0, 0, (loop && DSBPLAY_LOOPING) != 0);
				if (result == -2005401450) {
					// STRING: ALIEN 0x482fcc
					MYERROR::Log(::Error, "!!!ERROR!!! SFXBUFFER::buffer is lost");
					return Sound->ReloadSFX();
				}
			}
		}
	}
	return result;
}

// FUNCTION: ALIEN 0x41e6e0
int SFXBUFFER::IsPlaying()
{
	if (m_unk0x0c < 0)
		return 0;
	IDirectSoundBuffer* buffer = m_buffer;
	if (!buffer || !m_unk0x08)
		return 0;

	int status;
	buffer->GetStatus((DWORD*) &status);
	if (!(status & 1))
		m_unk0x08 = 0;

	SFX* sfx = Sound->m_sfx;
	int id = m_unk0x0c;
	if (sfx && id >= 0 && id <= Sound->m_noSfx) {
		SFX* entry = &sfx[id];
		int oneShot = 0;
		if (entry->m_buffers[0])
			oneShot = !entry->m_unk0x20;
		if (oneShot && CurrentTime - m_unk0x18 > 100) {
			buffer = m_buffer;
			m_unk0x08 = 0;
			if (buffer) {
				buffer->Stop();
			}
		}
	}
	return m_unk0x08;
}
