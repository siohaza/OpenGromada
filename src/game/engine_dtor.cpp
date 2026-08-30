#include "game/engine.h"
#include "game/map.h"
#include "sprite/sprite.h"
#include "video/vid.h"
#include "video/vid_exdata.h"
#include "world/hash_map.h"

// FUNCTION: ALIEN 0x44e9f0
ENGINE::~ENGINE()
{
	if (!globaldeleting) {
		Map->ScriptRun(EvFunctionNumber[24], this, 0, 0);
	}
	SPRITE* owner = m_commandOwner;
	if (owner == this) {
		SetCommandToTrain(0, 0, 0, 0);
	}
	else if (m_commandOwner) {
		m_commandOwner->ReleaseRef();
		m_commandOwner = 0;
	}
	if ((SPRITE*) this == (SPRITE*) Map->Flagman(Map->m_curArmy)) {
		PathDots.DeleteAll();
	}
	if (!globaldeleting) {
		ClearDotBusy();
	}
	if (m_prevEngine) {
		m_prevEngine->m_nextEngine = 0;
		m_prevEngine->ReCalcMoveParameters();
	}
	if (m_nextEngine) {
		m_nextEngine->m_prevEngine = 0;
		m_nextEngine->ReCalcMoveParameters();
	}
	if (!globaldeleting) {
		if (m_prevEngine && m_nextEngine) {
			Map->ScriptRun(EvFunctionNumber[5], m_prevEngine, m_nextEngine, 0);
		}
		if (m_vid->m_exData->m_unk0x10 - m_vid->m_exData->m_unk0x0c > 1.0f) {
			Map->ScriptRun(EvFunctionNumber[7], this, 0, 0);
		}
		if (!m_prevEngine && !m_nextEngine) {
			Map->ScriptRun(EvFunctionNumber[6], this, 0, 0);
		}
		if (m_unk0xdc) {
			HASH_MAP* h = Hash;
			SPRITE* s = (SPRITE*) h->m_list.LastIterate(&h->m_iter);
			while (s) {
				if (s->m_vid->m_sprClass == 21 && !((m_flag ^ s->m_flag) & 0x1800) &&
					((ENGINE*) s)->m_unk0xf4 == m_unk0xf4) {
					break;
				}
				h = Hash;
				s = (SPRITE*) h->m_list.NextIterate(&h->m_iter);
			}
			if (s) {
				Map->ScriptRun(EvFunctionNumber[4], s, 0, 0);
			}
		}
	}
}
