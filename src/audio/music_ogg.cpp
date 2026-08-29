#define DECOMP_INLINE_STRING_COPY_LIFETIME
#include "audio/music_ogg.h"

#include <stdio.h>
#include <string.h>

#include "audio/sound.h"
#include "util/myerror.h"

// FUNCTION: ALIEN 0x41c310
MUSIC_OGG::MUSIC_OGG(STRING* p_name)
	: MUSIC(*p_name)
{
	m_unk0x0c = 0;
	m_buffer = Sound->CreateOggBuffer(p_name, &m_file, &m_fileHandle, 0x40000);
	if (!m_buffer)
		MYERROR::Error(::Error,
			"MUSIC '%s'", 3,
			// STRING: ALIEN 0x482d3c
			"SoundBuffer", 0, m_name.m_str);
}

// FUNCTION: ALIEN 0x41c3e0
MUSIC_OGG::~MUSIC_OGG()
{
	Stop();
	if (m_buffer) {
		int released = m_buffer->Release();
		if (released)
			MYERROR::Error(::Error,
				"MUSIC '%s'", 10,
				// STRING: ALIEN 0x482d48
				"SoundBuffer release !=0", released, m_name.m_str);
		m_buffer = 0;
	}
	if (m_fileHandle) {
		ov_clear(&m_file);
		fclose(m_fileHandle);
		m_fileHandle = 0;
	}
}

// FUNCTION: ALIEN 0x41c4c0
void MUSIC_OGG::Play()
{
	int result = (int) m_buffer;
	if (result) {
		if (m_fileHandle) {
			((IDirectSoundBuffer*) result)->Stop();
			rewind(m_fileHandle);
			m_writePos = 16;
			m_state = 0;
			m_buffer->SetCurrentPosition(0);
			ov_pcm_seek(&m_file, 0);
			Tact();
			result = m_buffer->Play(0, 0, 1);
			m_unk0x0c = 1;
		}
	}
}

// FUNCTION: ALIEN 0x41c540
void MUSIC_OGG::Stop()
{
	m_unk0x0c = 0;
	IDirectSoundBuffer* buffer = m_buffer;
	if (buffer)
		buffer->Stop();
}

// FUNCTION: ALIEN 0x41c560
void MUSIC_OGG::Pause()
{
	int result = m_unk0x0c;
	if (result) {
		result = (int) m_buffer;
		if (result)
			result = ((IDirectSoundBuffer*) result)->Stop();
	}
}

// FUNCTION: ALIEN 0x41c580
void MUSIC_OGG::Resume()
{
	int result = m_unk0x0c;
	if (result) {
		result = (int) m_buffer;
		if (result)
			result = ((IDirectSoundBuffer*) result)->Play(0, 0, 1);
	}
}

// FUNCTION: ALIEN 0x41c5a0
int MUSIC_OGG::IsPlaying()
{
	IDirectSoundBuffer* buffer = m_buffer;
	if (!buffer || !m_unk0x0c)
		return 0;
	int status;
	buffer->GetStatus((DWORD*) &status);
	if (!(status & 1))
		m_unk0x0c = 0;
	return m_unk0x0c;
}

// FUNCTION: ALIEN 0x41c5e0
void MUSIC_OGG::SetVolume(int p_volume)
{
	int result = (int) m_buffer;
	if (result)
		result = ((IDirectSoundBuffer*) result)->SetVolume(p_volume);
}

// GLOBAL: ALIEN 0x490750
char OggBuffer[4096];

// FUNCTION: ALIEN 0x41c600
void MUSIC_OGG::Tact()
{
	unsigned int result = (unsigned int) m_buffer;
	if (result) {
		int bitstream;
		unsigned int write;
		void* p1;
		char* p2;
		int l1;
		unsigned int l2;
		unsigned int play;
		m_buffer->GetCurrentPosition((DWORD*) &play, (DWORD*) &write);
		for (;;) {
			result = m_state;
			unsigned int pos = play;
			if (result)
				break;
			unsigned int avail;
			if (pos < m_writePos)
				avail = pos - m_writePos + 0x40000;
			else
				avail = pos - m_writePos;
			if (avail < 0x1000)
				break;
			int n = ov_read(&m_file, OggBuffer, 4096, 0, 2, 1, &bitstream);
			if (n == 0) {
				m_state = play > m_writePos ? 2 : 1;
			}
			else if (n < 0) {
				MYERROR::Error(::Error,
							   "MUSIC '%s'", 10,
							   // STRING: ALIEN 0x482d60
							   "decode", 0, m_name);
			}
			else {
				if (m_buffer->Lock(m_writePos, n, &p1, (DWORD*) &l1, (void**) &p2,
								   (DWORD*) &l2, 0) >= 0) {
					if (n <= l1) {
						l1 = n;
						l2 = 0;
					}
					else {
						l2 = n - l1;
					}
					memcpy(p1, OggBuffer, l1);
					unsigned int u = l2;
					if (l2) {
						memcpy(p2, OggBuffer + l1, l2);
						u = l2;
					}
					m_buffer->Unlock(p1, l1, p2, u);
				}
				m_writePos = (n + m_writePos) & 0x3FFFF;
			}
		}
		if (m_state == 2 && play < m_writePos)
			m_state = 1;
		if (m_state == 1 && play >= m_writePos)
			((MUSIC*) this)->Stop();
	}
}
