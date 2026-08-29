#define DECOMP_INLINE_STRING_COPY_LIFETIME
#include "audio/sfx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OV_EXCLUDE_STATIC_CALLBACKS
#include "audio/minivorbis.h"
#include "audio/sound.h"
#include "util/myerror.h"
#include "util/resource.h"

// FUNCTION: ALIEN 0x41d3d0
SFX::SFX()
{
	for (int i = 0; i < 8; ++i)
		m_names[i] = STRING::EMPTY;
	memset(m_buffers, 0, sizeof(m_buffers));
}

// FUNCTION: ALIEN 0x41d400
SFX::~SFX()
{
	Release();
	char** p = &m_names[8];
	int n = 8;
	do {
		char* name = *--p;
		if (name != STRING::EMPTY)
			operator delete(name);
		--n;
	} while (n);
}

// FUNCTION: ALIEN 0x41e7e0
int SFX::Release()
{
	IDirectSoundBuffer** p = m_buffers;
	int n = 8;
	int result;
	do {
		result = (int) *p;
		if (result) {
			result = ((IDirectSoundBuffer*) result)->Release();
			*p = 0;
		}
		++p;
		--n;
	} while (n);
	return result;
}

// GLOBAL: ALIEN 0x491758
static char OggDecodeBuffer[4096];

// STUB: ALIEN 0x41e810
void SFX::Load(STRING* p_names, int p_flag, SOUND* p_sound)
{
	RESOURCE res;
	Release();
	char** name = m_names;
	char** srcNames = (char**) p_names;
	for (int n = 0; n < 8; ++n)
		*(STRING*) &m_names[n] = *(STRING*) &srcNames[n];
	m_unk0x20 = (unsigned char) p_flag;
	void* part1;
	void* part2;
	DWORD size1;
	DWORD size2;

	for (int i = 0; i < 8; ++i) {
		if (!strcmp(*name, empty_str))
			break;
		if (!strstr(*name,
				// STRING: ALIEN 0x482f18
				".ogg")
			&& !strstr(*name,
				// STRING: ALIEN 0x482f10
				".OGG")
			&& !strstr(*name,
				// STRING: ALIEN 0x482f08
				".Ogg")) {
			m_buffers[i] = p_sound->CreateWavBuffer((STRING*) name, &res, 0);
			if (!m_buffers[i])
				return;
			if (m_buffers[i]->Lock(0, res.m_resSize, &part1, &size1, &part2, &size2, 0) >= 0) {
				res.Read(part1, size1);
				if (size2)
					res.Read(part2, size2);
				m_buffers[i]->Unlock(part1, size1, part2, size2);
			}
			res.Close();
		}
		else {
			OggVorbis_File vorbisFile;
			FILE* file;
			m_buffers[i] = p_sound->CreateOggBuffer((STRING*) name, &vorbisFile, &file, 0);
			if (!m_buffers[i])
				return;
			volatile int position = 0;
			char bitstream[4];
			for (;;) {
				int got = ov_read(&vorbisFile, OggDecodeBuffer, 4096, 0, 2, 1, (int*) bitstream);
				if (!got)
					break;
				if (got < 0) {
					MYERROR::Error(::Error,
						// STRING: ALIEN 0x483014
						"SFX", 10,
						// STRING: ALIEN 0x483018
						"decode ogg", got);
					continue;
				}
				if (m_buffers[i]->Lock(position, got, &part1, &size1, &part2, &size2, 0) >= 0) {
					memcpy(part1, OggDecodeBuffer, size1);
					if (size2)
						memcpy(part2, OggDecodeBuffer + size1, size2);
					m_buffers[i]->Unlock(part1, size1, part2, size2);
				}
				position += got;
			}
			if (file) {
				fclose(file);
				ov_clear(&vorbisFile);
			}
		}
		m_unk0x44 = i + 1;
		++name;
	}
}

// FUNCTION: ALIEN 0x41eb30
IDirectSoundBuffer* SFX::Play(IDirectSound* p_ds)
{
	IDirectSoundBuffer* dup = 0;
	int last = m_unk0x44 - 1;
	int i = rand() % (last + 1);
	if (m_buffers[i]->AddRef() <= 2) {
		return m_buffers[i];
	}
	m_buffers[i]->Release();
	int result = p_ds->DuplicateSoundBuffer(m_buffers[i], &dup);
	if (result < 0)
		MYERROR::Log(::Error,
			// STRING: ALIEN 0x483024
			"!!!ERROR!!!SFX:'%s' %X Couldn't duplicate buffer", (char*) this, result);
	return dup;
}
