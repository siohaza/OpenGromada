#include "audio/sound.h"

#include "game/game_descriptor.h"
#include "game/gametime.h"
#include "util/myerror.h"

#include <stdlib.h>

// FUNCTION: ALIEN 0x41d430
int SOUND::StartSFX(int p_sfx)
{
	if (m_disabled) {
		return -1;
	}
	SFX* sfx = m_sfx;
	if (sfx && !(p_sfx >= 0 && p_sfx <= m_noSfx && sfx[p_sfx].m_samples[0])) {
		MYERROR::Error(
			::Error,
			"SOUND",
			4,
			// STRING: ALIEN 0x482ed0
			"nsfx",
			p_sfx
		);
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
		if (q->m_unk0x0c < 0) {
			break;
		}
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
	m_channel[free].SetSample(p_sfx, m_sfx[p_sfx].Play());
	return free;
}

// FUNCTION: ALIEN 0x41d560
int SOUND::StopSFX(int p_sfxId)
{
	int result = m_disabled;
	if (!result) {
		if (p_sfxId == -1) {
			return StopAllSFX();
		}
		SFXBUFFER* channel = m_channel;
		int n = 16;
		do {
			if (channel->m_unk0x0c == p_sfxId) {
				channel->Release();
			}
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
	if (volX < volY) {
		volY = volX;
	}
	int pan = x * 4;

	if (GameDesc->m_sfxSchema != GAME_SFX_AS1 && ValidateSFX(p_sfx)) {
		unsigned int property = m_sfx[p_sfx].m_property;
		if (property & 2) {
			volY = 0;
		}
		if (property & 4) {
			pan = 0;
		}
	}
	PlaySFX(p_sfx, pan, volY);
}

// FUNCTION: ALIEN 0x41d600
void SOUND::PlaySFX(int p_sfx, int p_pan, int p_volume)
{
	if (m_disabled) {
		return;
	}
	if (!ValidateSFX(p_sfx)) {
		MYERROR::Error(
			::Error,
			// STRING: ALIEN 0x482e20
			"SOUND",
			4,
			"nsfx",
			p_sfx
		);
		return;
	}
	int sfxBase = m_sfx[p_sfx].m_volume;
	p_volume += sfxBase + 32 * (m_volume - 100);
	if (p_volume < -3000) {
		return;
	}
	if (p_pan > 10000) {
		p_pan = 10000;
	}
	else if (p_pan < -10000) {
		p_pan = -10000;
	}
	const bool vip = GameDesc->m_sfxSchema == GAME_SFX_AS1 ? m_sfx[p_sfx].m_unk0x20 == 100 :
		(m_sfx[p_sfx].m_property & 8) != 0;

	if (vip) {
		p_volume = sfxBase + 32 * (m_volume - 100);
	}
	if (GameDesc->m_sfxSchema == GAME_SFX_ZS1) {
		if (p_volume > 10000) p_volume = 10000;
		if (p_volume < -10000) p_volume = -10000;
	}
	int i;
	for (i = 0; i < 32; i++) {
		if (m_playing[i].m_sfx < 0) {
			m_playing[i].m_sfx = p_sfx;
			m_playing[i].m_vol = p_volume;
			m_playing[i].m_pan = p_pan;
			break;
		}
	}
	if (i < 32) {
		return;
	}
	int foundSame = 0;
	int best = 0;
	for (int j = 1; j < 32; j++) {
		if (m_playing[j].m_sfx == p_sfx) {
			foundSame = 1;
		}
		if (m_sfx[m_playing[j].m_sfx].m_unk0x20 < m_sfx[m_playing[best].m_sfx].m_unk0x20) {
			best = j;
		}
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
			channel->m_unk0x08 = 0;
			channel->m_running = 0;
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
	if (!result) {
		SFXBUFFER* channel = m_channel;
		int n = 16;
		do {
			// The cursor stays where it is, so ResumeSFX picks the sound up
			// where the pause cut it.
			if (channel->m_unk0x08) {
				channel->m_running = 0;
			}
			++channel;
			--n;
		} while (n);
	}
	return result;
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
