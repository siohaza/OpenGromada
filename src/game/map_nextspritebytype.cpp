#include "game/map.h"
#include "sprite/sprite.h"
#include "video/vid.h"

// FUNCTION: ALIEN 0x40fb40
SPRITE* MAP::NextSpriteByType(int p_type, int* p_iter, int p_flag)
{
	SPRITE* result = NextSprite(p_type, p_iter);
	while (result) {
		if (p_flag & result->m_vid->m_unk0x0c) {
			break;
		}
		result = NextSprite(p_type, p_iter);
	}
	return result;
}
