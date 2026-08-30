#include "audio/sound.h"
#include "platform/timing.h"
#include "util/myerror.h"
#include "util/resource.h"

#include <SDL3/SDL.h>

// FUNCTION: ALIEN 0x41cd30
void SOUND::InitDS()
{
	if (!m_disabled) {
		return;
	}

	if (Mixer_Open()) {
		m_disabled = 2;
		MYERROR::Error(::Error, "SOUND", 3, "audio device", 0);
		MYERROR::Log(::Error, "!!!ERROR!!!SOUND: %s", SDL_GetError());
		return;
	}

	m_disabled = 0;
	for (int i = 0; i < m_noSfx; ++i) {
		SFX* sfx = &m_sfx[i];
		sfx->Load(sfx->m_names, sfx->m_unk0x20, this);
	}
}

// FUNCTION: ALIEN 0x41cf70
void SOUND::ReleaseDS()
{
	if (m_disabled) {
		return;
	}

	for (int i = 0; i < 16; ++i) {
		m_channel[i].Release();
	}

	if (m_sfx) {
		for (int i = 0; i < m_noSfx; ++i) {
			m_sfx[i].Release();
		}
	}

	Mixer_Close();

	for (int i = 0; i < 32; ++i) {
		m_playing[i].Clear();
	}
	m_disabled = 1;
}

// FUNCTION: ALIEN 0x41d110
void SOUND::LoadSFX(RESOURCE* p_res)
{
	STRING names[8];
	unsigned int start = Platform_Ticks();
	if (m_disabled) {
		return;
	}
	if (!p_res->m_file) {
		MYERROR::Error(
			::Error,
			"SOUND",
			7,
			// STRING: ALIEN 0x482ecc
			"res",
			0
		);
		return;
	}

	delete[] m_sfx;
	m_sfx = 0;
	m_noSfx = p_res->GetNoSubRes(0x20584653);
	if (!m_noSfx) {
		MYERROR::Error(
			::Error,
			"SOUND",
			11,
			// STRING: ALIEN 0x482ec4
			"SFX ",
			0
		);
		return;
	}
	m_sfx = new SFX[m_noSfx + 1];
	if (!m_sfx) {
		MYERROR::Error(
			::Error,
			"SOUND",
			2,
			// STRING: ALIEN 0x482ebc
			"LoadSfx",
			m_noSfx
		);
		return;
	}
	if (p_res->GoBegin(0x20584653)) {
		return;
	}

	int i = 0;

	do {

		int quality = 0;
		p_res->Read(&quality, 1);
		for (int j = 0; j != 8; ++j) {
			names[j].Read_res(p_res);
		}
		SFX* sfx = &m_sfx[i];
		++i;
		sfx->Load(names, quality, this);
	} while (!p_res->GoNextSub(0x20584653));

	MYERROR::Log(
		::Error,
		// STRING: ALIEN 0x482e6c
		"LoadSFX::No   =%-15i   sizeof(SFX)   =%-5i    load time     =%ims Quality=%i",
		m_noSfx,
		(int) sizeof(SFX),
		Platform_Ticks() - start,
		m_unk0x00 & 1
	);
}
