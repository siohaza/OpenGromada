#include "audio/sfxbuffer.h"

#include "audio/sfx.h"
#include "audio/sound.h"
#include "game/gametime.h"
#include "util/myerror.h"

// FUNCTION: ALIEN 0x41e4e0
SFXBUFFER::~SFXBUFFER()
{
	if (m_buffer) {
		int released = m_buffer->Release();
		if (released)
			MYERROR::Error(::Error,
				// STRING: ALIEN 0x482fac
				"SFXBUFFER[%i]", 10,
				"SoundBuffer release !=0", released, m_unk0x0c);
		m_buffer = 0;
	}
}

// FUNCTION: ALIEN 0x41e530
int SFXBUFFER::Release()
{
	int result = (int) m_buffer;
	if (result) {
		result = ((IDirectSoundBuffer*) result)->Release();
		m_buffer = 0;
	}
	m_buffer = 0;
	m_unk0x08 = 0;
	m_unk0x18 = 0;
	m_unk0x0c = -1;
	return result;
}

// FUNCTION: ALIEN 0x41e560
int SFXBUFFER::Play(int p_pan, int p_volume)
{
	int result = m_unk0x0c;
	if (result >= 0) {
		result = (int) m_buffer;
		if (result) {
			m_volume = p_volume;
			m_pan = p_pan;
			((IDirectSoundBuffer*) result)->SetPan(p_pan);
			m_buffer->SetVolume(p_volume);
			result = m_unk0x08;
			if (!result) {
				SFX* sfx = Sound->m_sfx;
				int id = m_unk0x0c;
				int loop = 0;
				if (sfx && id >= 0 && id <= Sound->m_noSfx && sfx[id].m_buffers[0])
					loop = !sfx[id].m_unk0x20;
				result = m_buffer->Play(0, 0, (loop && DSBPLAY_LOOPING) != 0);
				if (result == 0x88780096) {
					MYERROR::Error(::Error,
						"SFXBUFFER[%i]", 10,
						// STRING: ALIEN 0x482fbc
						"buffer is lost", 0, m_unk0x0c);
					result = Sound->ReloadSFX();
				}
			}
			m_unk0x08 = 1;
			m_unk0x18 = CurrentTime;
		}
	}
	return result;
}

// FUNCTION: ALIEN 0x41e780
char* SFXBUFFER::SetBuffer(int p_id, IDirectSoundBuffer* p_buffer)
{
	m_unk0x0c = p_id;
	int result = (int) m_buffer;
	if (result) {
		result = (int) ((IDirectSoundBuffer*) result)->Release();
		if (result)
			result = (int) MYERROR::Error(::Error,
				"SFXBUFFER[%i]", 10,
				// STRING: ALIEN 0x482ff4
				"SoundBuffer(Load()) release !=0", result, m_unk0x0c);
		m_buffer = 0;
	}
	m_buffer = p_buffer;
	m_unk0x08 = 0;
	m_unk0x18 = 0;
	return (char*) result;
}
