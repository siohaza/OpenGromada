#ifndef SFX_H
#define SFX_H

#include "audio/mixer.h"
#include "util/decomp.h"
#include "util/string.h"

class SFX {
public:
	SFX();
	~SFX();

	STRING m_names[8];          // 0x00
	unsigned char m_unk0x20;    // 0x20
	SOUND_SAMPLE* m_samples[8]; // 0x24
	int m_unk0x44;              // 0x44

	unsigned int m_property;
	int m_volume;

	STRING m_forceFeedbackNames[8];

	int MaxVoices() const
	{
		if (m_unk0x20 == 100) {
			return 999999;
		}
		if (!m_unk0x20) {
			return 1;
		}
		return m_unk0x20 / 10;
	}

	void Load(STRING* p_names, int p_flag, class SOUND* p_sound);
	SOUND_SAMPLE* Play();
	void Release();
};

#endif
