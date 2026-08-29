#include "audio/sound.h"

#include <stdlib.h>

#include "game/gametime.h"

#include "util/myerror.h"

// FUNCTION: ALIEN 0x41d430
int SOUND::StartSFX(int p_sfx)
{
	if (m_disabled) {
		return -1;
	}
	SFX* sfx = m_sfx;
	if (sfx && !(p_sfx >= 0 && p_sfx <= m_noSfx && sfx[p_sfx].m_buffers[0])) {
		MYERROR::Error(::Error,
			"SOUND", 4,
			// STRING: ALIEN 0x482ed0
			"nsfx", p_sfx);
		return -1;
	}
	int ch = 0;
	SFXBUFFER* p = m_channel;
	do {
		if (p->m_unk0x0c == p_sfx) {
			if (CurrentTime - p->m_unk0x18 <= 0x32) {
				return -1;
			}
			if (sfx[p_sfx].m_unk0x44 == 1 && !p->m_unk0x08) {
				return ch;
			}
		}
		++ch;
		++p;
	} while (ch < 16);
	int free;
	SFXBUFFER* q = m_channel;
	for (free = 0; free < 16; ++free, ++q) {
		if (q->m_unk0x0c < 0)
			break;
	}
	if (free >= 16) {
		free = 0;
		SFXBUFFER* r = m_channel;
		while (free < 16 && r->m_unk0x08) {
			++free;
			++r;
			if (free >= 16) {
				return -1;
			}
		}
		m_channel[free].Release();
	}
	if (free >= 16) {
		return -1;
	}
	m_channel[free].SetBuffer(p_sfx, m_sfx[p_sfx].Play(m_directSound));
	return free;
}

// FUNCTION: ALIEN 0x41d560
int SOUND::StopSFX(int p_sfxId)
{
	int result = m_disabled;
	if (!result) {
		if (p_sfxId == -1)
			return StopAllSFX();
		SFXBUFFER* channel = m_channel;
		int n = 16;
		do {
			if (channel->m_unk0x0c == p_sfxId)
				result = channel->Release();
			++channel;
			--n;
		} while (n);
	}
	return result;
}

// FUNCTION: ALIEN 0x41d5b0
void SOUND::PlaySFXFromCoor(int p_sfx, float p_x, float p_y)
{
	int x = (int) p_x;
	int volX = -2 * abs(x);
	int volY = -2 * abs((int) p_y);
	if (volX < volY)
		volY = volX;
	PlaySFX(p_sfx, x * 4, volY);
}

// FUNCTION: ALIEN 0x41d600
void SOUND::PlaySFX(int p_sfx, int p_pan, int p_volume)
{
	if (m_disabled) {
		return;
	}
	if (!ValidateSFX(p_sfx)) {
		MYERROR::Error(::Error,
			// STRING: ALIEN 0x482e20
			"SOUND", 4,
			"nsfx", p_sfx);
		return;
	}
	p_volume += 32 * (m_volume - 100);
	if (p_volume < -3000) {
		return;
	}
	if (p_pan > 10000)
		p_pan = 10000;
	else if (p_pan < -10000)
		p_pan = -10000;
	if (m_sfx[p_sfx].m_unk0x20 == 100)
		p_volume = 32 * (m_volume - 100);
	int i;
	for (i = 0; i < 32; i++) {
		if (m_playing[i].m_sfx < 0) {
			m_playing[i].m_sfx = p_sfx;
			m_playing[i].m_vol = p_volume;
			m_playing[i].m_pan = p_pan;
			break;
		}
	}
	if (i < 32)
		return;
	int foundSame = 0;
	int best = 0;
	for (int j = 1; j < 32; j++) {
		if (m_playing[j].m_sfx == p_sfx)
			foundSame = 1;
		if (m_sfx[m_playing[j].m_sfx].m_unk0x20 < m_sfx[m_playing[best].m_sfx].m_unk0x20)
			best = j;
	}
	if (!best && foundSame) {
		return;
	}
	m_playing[best].m_sfx = p_sfx;
	m_playing[best].m_vol = p_volume;
	m_playing[best].m_pan = p_pan;
}

// FUNCTION: ALIEN 0x41dcd0
int SOUND::StopAllSFX()
{
	int result = m_disabled;
	if (!result) {
		SFXBUFFER* channel = m_channel;
		int n = 16;
		do {
			result = (int) channel->m_buffer;
			channel->m_unk0x08 = 0;
			if (channel->m_buffer)
				result = channel->m_buffer->Stop();
			++channel;
			--n;
		} while (n);
	}
	return result;
}

// FUNCTION: ALIEN 0x41dd10
int SOUND::PauseSFX()
{
	int result = m_disabled;
	if (result)
		return result;
	SFXBUFFER* channel = m_channel;
	int n = 16;
	do {
		if (channel->m_unk0x08 && channel->m_buffer)
			channel->m_buffer->Stop();
		++channel;
		--n;
	} while (n);

}

// FUNCTION: ALIEN 0x41dd50
int SOUND::ResumeSFX()
{
	int result = m_disabled;
	if (!result) {
		SFXBUFFER* channel = m_channel;
		int n = 16;
		do {
			result = channel->Resume();
			++channel;
			--n;
		} while (n);
	}
	return result;
}

// FUNCTION: ALIEN 0x41e190
int SOUND::ReloadSFX()
{
	int result = (int) m_sfx;
	if (result) {
		result = m_noSfx;
		int i = 0;
		if (result > 0) {
			int off = 0;
			do {
				SFX* p = (SFX*) ((char*) m_sfx + off);
				p->Load((STRING*) p->m_names, p->m_unk0x20, this);
				result = m_noSfx;
				++i;
				off += 72;
			} while (i < result);
		}
	}
	return result;
}
