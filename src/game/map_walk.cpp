#include "game/engine.h"
#include "game/gametime.h"
#include "game/map.h"
#include "gfx/graph.h"
#include "sprite/sprite.h"

// FUNCTION: ALIEN 0x40fa00
void MAP::PauseOff()
{
	if (m_flag & 0x10) {
		for (int layer = 0; layer < 17; ++layer) {
			int iterator;
			SPRITE* sprite = FirstSprite(layer, &iterator);
			while (sprite) {
				sprite->m_tactTime = PauseOldClock;
				sprite = NextSprite(layer, &iterator);
			}
		}
		CurrentTime = PauseOldClock;
		PrevCurrentTime = PauseOldClock - 10;
	}
	m_flag &= 0xffffffef;
}

// FUNCTION: ALIEN 0x411440
void MAP::DeleteExtraVid()
{
	for (int layer = 0; layer < 17; ++layer) {
		int iter;
		for (SPRITE* sprite = FirstSprite(layer, &iter); sprite; sprite = NextSprite(layer, &iter)) {
			if (sprite->m_vid->m_pixelFlag16 & 0x200) {
				sprite->ScalarDeletingDestructor(1);
			}
		}
	}
	for (int i = m_noVid - 1; i >= 0; --i) {
		VID* vid = m_vids[i];
		if (vid && (vid->m_pixelFlag16 & 0x200)) {
			vid->ScalarDeletingDestructor(1);
			m_vids[i] = 0;
		}
	}
	while (m_noVid > 0 && !m_vids[m_noVid - 1]) {
		--m_noVid;
	}
}
