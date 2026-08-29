#define DECOMP_INLINE_STRING_DTOR
#include "audio/sound.h"

#include "util/myerror.h"
#include "util/resource.h"

#include <mmsystem.h>

// FUNCTION: ALIEN 0x41cd30
void SOUND::InitDS()
{
	if (!m_disabled)
		return;
	if (m_directSound)
		return;

	HRESULT hr = DirectSoundCreate8(0, &m_directSound, 0);
	if (hr < 0) {
		m_disabled = 2;
		MYERROR::Error(::Error,
			"SOUND", 3,
			// STRING: ALIEN 0x482e28
			"DirectSound", hr);
		return;
	}

	if (m_unk0x00 & 1) {
		hr = m_directSound->SetCooperativeLevel(m_hwnd, DSSCL_PRIORITY);
		if (hr < 0) {
			m_unk0x00 &= ~1u;
			MYERROR::Error(::Error,
				"SOUND", 8,
				// STRING: ALIEN 0x482e10
				"PriorityLevel", hr);
		}
	}
	if (!(m_unk0x00 & 1))
		hr = m_directSound->SetCooperativeLevel(m_hwnd, DSSCL_NORMAL);
	if (hr < 0) {
		m_directSound->Release();
		m_directSound = 0;
		m_disabled = 3;
		MYERROR::Error(::Error,
			"SOUND", 8,
			// STRING: ALIEN 0x482dfc
			"CooperativeLevel", hr);
		return;
	}

	DSBUFFERDESC desc;
	memset(&desc, 0, sizeof(desc));
	desc.dwSize = sizeof(desc);
	desc.dwFlags = DSBCAPS_PRIMARYBUFFER;
	IDirectSoundBuffer* primary;
	hr = m_directSound->CreateSoundBuffer(&desc, &primary, 0);
	if (hr >= 0) {
		if (primary->Play(0, 0, DSBPLAY_LOOPING) < 0)
			MYERROR::Error(::Error,
				"SOUND", 4,
				// STRING: ALIEN 0x482de8
				"unable play Primary", 0);
		if (m_unk0x00 & 1) {
			WAVEFORMATEX format;
			memset(&format, 0, sizeof(format));
			format.wFormatTag = WAVE_FORMAT_PCM;
			format.nChannels = 2;
			format.nSamplesPerSec = 44100;
			format.wBitsPerSample = 16;
			format.nBlockAlign = 4;
			format.nAvgBytesPerSec = 176400;
			hr = primary->SetFormat(&format);
			if (hr < 0)
				MYERROR::Error(::Error,
					"SOUND", 8,
					// STRING: ALIEN 0x482dd0
					"format primary buffer", hr);
		}
		primary->Release();
	}
	else {
		MYERROR::Error(::Error,
			"SOUND", 3,
			// STRING: ALIEN 0x482db8
			"Primary sound buffer", hr);
	}

	m_disabled = 0;
	for (int i = 0; i < m_noSfx; ++i) {
		SFX* sfx = &m_sfx[i];
		sfx->Load((STRING*) sfx->m_names, sfx->m_unk0x20, this);
	}
}

// FUNCTION: ALIEN 0x41cf70
void SOUND::ReleaseDS()
{
	if (!m_disabled) {
		SFXBUFFER* channel = m_channel;
		int n = 16;
		do {
			channel->Release();
			++channel;
			--n;
		} while (n);

		int i = 0;
		if (m_noSfx > 0) {
			SFX* sfx = m_sfx;
			do {
				m_sfx[i].Release();
				++i;
				sfx = m_sfx + i;
			} while (i < m_noSfx);
		}

		int result = m_directSound->Release();
		if (result)
			MYERROR::Error(::Error,
				"SOUND", 10,
				// STRING: ALIEN 0x482e34
				"DirectSound release !=0", result);
		m_directSound = 0;

		SFX_CHANNEL* playing = m_playing;
		n = 32;
		do {
			playing->m_sfx = -1;
			++playing;
			--n;
		} while (n);
		m_disabled = 1;
	}
}

// FUNCTION: ALIEN 0x41d110
void SOUND::LoadSFX(RESOURCE* p_res)
{
	STRING names[8];
	DWORD start = timeGetTime();
	if (m_disabled)
		return;
	if (!p_res->m_file) {
		MYERROR::Error(::Error,
			"SOUND", 7,
			// STRING: ALIEN 0x482ecc
			"res", 0);
		return;
	}

	delete[] m_sfx;
	m_sfx = 0;
	m_noSfx = p_res->GetNoSubRes(0x20584653);
	if (!m_noSfx) {
		MYERROR::Error(::Error,
			"SOUND", 11,
			// STRING: ALIEN 0x482ec4
			"SFX ", 0);
		return;
	}
	m_sfx = new SFX[m_noSfx + 1];
	if (!m_sfx) {
		MYERROR::Error(::Error,
			"SOUND", 2,
			// STRING: ALIEN 0x482ebc
			"LoadSfx", m_noSfx);
		return;
	}
	if (p_res->GoBegin(0x20584653))
		return;

	int i = 0;

	if (i)
		return;
	if (i)
		return;
	do {

		int quality;
		p_res->Read(&quality, 1);
		for (int j = 0; j != 8; ++j)
			names[j].Read_res(p_res);
		SFX* sfx = &m_sfx[i];
		++i;
		sfx->Load(names, quality, this);
	} while (!p_res->GoNextSub(0x20584653));

	MYERROR::Log(::Error,
		// STRING: ALIEN 0x482e6c
		"LoadSFX::No   =%-15i   sizeof(SFX)   =%-5i    load time     =%ims Quality=%i",
		m_noSfx, sizeof(SFX), timeGetTime() - start, m_unk0x00 & 1);
}
