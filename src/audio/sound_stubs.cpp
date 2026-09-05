#include "audio/sound.h"
#include "game/game_descriptor.h"
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
	unsigned int start = Platform_Ticks();
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

	const int count = p_res->GetNoSubRes(0x20584653);
	if (count <= 0) {
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
	const bool hasProperty = GameDesc->m_sfxSchema != GAME_SFX_AS1;
	const bool hasVolume = GameDesc->m_sfxSchema == GAME_SFX_ZS1;
	const int minimumBytes = hasVolume ? 25 : hasProperty ? 21 : 9;
	if ((size_t) count > (size_t) p_res->m_end / (minimumBytes + 4)) {
		p_res->Fail("SFX count exceeds selected schema bounds");
		return;
	}
	if (p_res->GoBegin(0x20584653)) {
		return;
	}
	SFX* records = new SFX[(size_t) count + 1];


	for (int i = 0; i < count; ++i) {
		SFX& sfx = records[i];
		if (hasProperty) {
			p_res->ReadWords(&sfx.m_property, 4);
		}
		p_res->Read(&sfx.m_unk0x20, 1);
		if (hasVolume) {
			p_res->ReadWords(&sfx.m_volume, 4);
			if (sfx.m_volume < -1000 || sfx.m_volume > 1000) {
				p_res->Fail("SFX volume is outside its documented range");
				sfx.m_volume = 0;
			}
			sfx.m_volume *= 10;
		}
		for (int j = 0; j != 8; ++j) {
			p_res->ReadString(sfx.m_names[j]);
		}
		if (hasProperty) {
			for (int j = 0; j != 8; ++j) {
				p_res->ReadString(sfx.m_forceFeedbackNames[j]);
			}
		}
		if (!p_res->RequireEnd() || (i + 1 < count && p_res->GoNextSub(0x20584653))) {
			delete[] records;
			return;
		}
	}
	delete[] m_sfx;
	m_sfx = records;
	m_noSfx = count;
	if (!m_disabled) {
		for (int i = 0; i < count; ++i) {
			m_sfx[i].Load(m_sfx[i].m_names, m_sfx[i].m_unk0x20, this);
		}
	}

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
