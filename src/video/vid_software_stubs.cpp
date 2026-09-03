#include "util/stream.h"
#include "video/vid_software.h"

// FUNCTION: ALIEN 0x415280
VID_SOFTWARE::VID_SOFTWARE(STREAM* p_stream) throw()
{
	struct DOT_COORD {
		float x;
		float y;
		float z;
	};

	VID_SOFTWARE& o = *(VID_SOFTWARE*) p_stream;
	m_recolorBase = 0;
	m_weaponPtr = o.m_weaponPtr;
	o.m_weaponPtr = this;
	m_layer = o.m_layer;
	m_pixelFlag16 = o.m_pixelFlag16;
	m_defaultAniPeriod = o.m_defaultAniPeriod;
	m_dotFrameCount = o.m_dotFrameCount;
	m_unk0x2f6 = o.m_unk0x2f6;
	m_messageLineHeight = o.m_messageLineHeight;
	m_unk0x484 = o.m_unk0x484;
	m_unk0x48c = o.m_unk0x48c;
	m_unk0x488 = o.m_unk0x488;
	m_unk0x468 = o.m_unk0x468;
	if (m_unk0x468) {
		m_unk0x46c = operator new(12 * m_unk0x468);
		for (int i = 0; i < m_unk0x468; ++i) {
			DOT_COORD& src = ((DOT_COORD*) o.m_unk0x46c)[i];
			DOT_COORD& dst = ((DOT_COORD*) m_unk0x46c)[i];
			dst = src;
		}
		if (o.m_unk0x470) {
			m_unk0x470 = operator new(4 * m_dotFrameCount);
			for (int j = 0; j < m_dotFrameCount; ++j) {
				((int*) m_unk0x470)[j] = ((int*) o.m_unk0x470)[j];
			}
		}
	}
}
