
#define DECOMP_INLINE_MAP_NEXTSPRITE
#define DECOMP_INLINE_MAP_NEXTSPRITE_BYVALUE

#include "game/map.h"

// FUNCTION: ALIEN 0x43a800
SPRITE* MAP::FirstSprite(int p_layer, int* p_iter) const
{
	*p_iter = m_layers[p_layer].m_n;
	return NextSprite(p_layer, p_iter);
}
