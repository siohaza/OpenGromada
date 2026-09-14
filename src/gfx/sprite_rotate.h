#ifndef SPRITE_ROTATE_H
#define SPRITE_ROTATE_H

class TEXTURE;

namespace SPRITE_ROTATE
{

struct TILE {
	TEXTURE* m_texture;
	int m_width;
	int m_height;
};

const TILE* Acquire(TEXTURE* p_source, const int* p_srcRect, int p_dstWidth, int p_dstHeight, unsigned char p_angle);

void Forget(const TEXTURE* p_source);

void Clear();

} // namespace SPRITE_ROTATE

#endif
