#include "video/vid_software.h"

#include "util/stream.h"

// FUNCTION: ALIEN 0x415280
VID_SOFTWARE::VID_SOFTWARE(STREAM* p_stream) throw()
{
	struct DOT_COORD {
		float x;
		float y;
		float z;
	};

	VID_SOFTWARE& o = *(VID_SOFTWARE*) p_stream;
	m_weaponPtr = o.m_weaponPtr;
	o.m_weaponPtr = this;
	m_layer = o.m_layer;
	*(unsigned short*) &m_pixelFlag = *(unsigned short*) &o.m_pixelFlag;
	*(unsigned short*) &m_unk0x2f2[2] = *(unsigned short*) &o.m_unk0x2f2[2];
	*(unsigned short*) &m_unk0x2f2[0] = *(unsigned short*) &o.m_unk0x2f2[0];
	*(unsigned short*) &m_unk0x2f2[4] = *(unsigned short*) &o.m_unk0x2f2[4];
	*(unsigned short*) &m_unk0x2f2[6] = *(unsigned short*) &o.m_unk0x2f2[6];
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
			m_unk0x470 = operator new(4 * *(short*) &m_unk0x2f2[2]);
			for (int j = 0; j < *(short*) &m_unk0x2f2[2]; ++j)
				((int*) m_unk0x470)[j] = ((int*) o.m_unk0x470)[j];
		}
	}
}
