#include "gfx/sprite_rotate.h"

#include "gfx/gfxdefs.h"
#include "gfx/texture.h"
#include "util/angle.h"

#include <cmath>
#include <map>
#include <memory>
#include <utility>

namespace SPRITE_ROTATE
{
namespace
{

constexpr size_t kMaxEntries = 512;
constexpr size_t kMaxPixels = 4u * 1024u * 1024u;

size_t g_cachedPixels = 0;

struct KEY {
	const TEXTURE* m_source;
	int m_srcX;
	int m_srcY;
	int m_srcWidth;
	int m_srcHeight;
	int m_dstWidth;
	int m_dstHeight;
	unsigned char m_angle;

	bool operator<(const KEY& p_other) const
	{
		if (m_source != p_other.m_source) {
			return m_source < p_other.m_source;
		}
		if (m_srcX != p_other.m_srcX) {
			return m_srcX < p_other.m_srcX;
		}
		if (m_srcY != p_other.m_srcY) {
			return m_srcY < p_other.m_srcY;
		}
		if (m_srcWidth != p_other.m_srcWidth) {
			return m_srcWidth < p_other.m_srcWidth;
		}
		if (m_srcHeight != p_other.m_srcHeight) {
			return m_srcHeight < p_other.m_srcHeight;
		}
		if (m_dstWidth != p_other.m_dstWidth) {
			return m_dstWidth < p_other.m_dstWidth;
		}
		if (m_dstHeight != p_other.m_dstHeight) {
			return m_dstHeight < p_other.m_dstHeight;
		}
		return m_angle < p_other.m_angle;
	}
};

struct ENTRY {
	std::unique_ptr<TEXTURE> m_texture;
	TILE m_tile;
};

std::map<KEY, ENTRY>& Cache()
{
	static std::map<KEY, ENTRY> cache;
	return cache;
}

bool g_clearing = false;

} // namespace

const TILE* Acquire(TEXTURE* p_source, const int* p_srcRect, int p_dstWidth, int p_dstHeight, unsigned char p_angle)
{
	if (!p_source || !p_source->m_data || !p_srcRect || p_dstWidth <= 0 || p_dstHeight <= 0) {
		return 0;
	}
	const int srcW = p_srcRect[2] - p_srcRect[0];
	const int srcH = p_srcRect[3] - p_srcRect[1];
	if (srcW <= 0 || srcH <= 0 || p_srcRect[0] < 0 || p_srcRect[1] < 0 || p_srcRect[2] > p_source->m_width ||
		p_srcRect[3] > p_source->m_height) {
		return 0;
	}
	if (p_dstWidth > g_textureMaxWidth || p_dstHeight > g_textureMaxHeight) {
		return 0;
	}

	const KEY key = {p_source, p_srcRect[0], p_srcRect[1], srcW, srcH, p_dstWidth, p_dstHeight, p_angle};
	std::map<KEY, ENTRY>& cache = Cache();
	auto found = cache.find(key);
	if (found != cache.end()) {
		return &found->second.m_tile;
	}
	if (cache.size() >= kMaxEntries || g_cachedPixels >= kMaxPixels) {
		Clear();
	}

	const ANGLE angle(p_angle);
	const double cosine = angle.Cos();
	const double sine = angle.Sin();

	const double halfW = p_dstWidth * 0.5;
	const double halfH = p_dstHeight * 0.5;
	const int rotatedW = (int) std::ceil(std::abs(p_dstWidth * cosine) + std::abs(p_dstHeight * sine));
	const int rotatedH = (int) std::ceil(std::abs(p_dstWidth * sine) + std::abs(p_dstHeight * cosine));
	if (rotatedW <= 0 || rotatedH <= 0 || rotatedW > g_textureMaxWidth || rotatedH > g_textureMaxHeight) {
		return 0;
	}

	std::unique_ptr<TEXTURE> rotated(new TEXTURE(rotatedW, rotatedH, D3DFMT_A8R8G8B8, 0));
	if (!rotated->m_data) {
		return 0;
	}

	const double rotatedHalfW = rotatedW * 0.5;
	const double rotatedHalfH = rotatedH * 0.5;
	for (int y = 0; y < rotatedH; ++y) {
		unsigned int* row = (unsigned int*) ((unsigned char*) rotated->m_data + (size_t) y * rotated->m_pitch);
		const double localY = y + 0.5 - rotatedHalfH;
		for (int x = 0; x < rotatedW; ++x) {
			const double localX = x + 0.5 - rotatedHalfW;

			const double tileX = localX * cosine + localY * sine + halfW;
			const double tileY = -localX * sine + localY * cosine + halfH;
			if (tileX < 0.0 || tileX >= p_dstWidth || tileY < 0.0 || tileY >= p_dstHeight) {
				row[x] = 0;
				continue;
			}

			int sx = p_srcRect[0] + (int) (tileX * srcW / p_dstWidth);
			int sy = p_srcRect[1] + (int) (tileY * srcH / p_dstHeight);
			if (sx < p_srcRect[0]) {
				sx = p_srcRect[0];
			}
			else if (sx >= p_srcRect[2]) {
				sx = p_srcRect[2] - 1;
			}
			if (sy < p_srcRect[1]) {
				sy = p_srcRect[1];
			}
			else if (sy >= p_srcRect[3]) {
				sy = p_srcRect[3] - 1;
			}
			row[x] = TextureSampleArgb(p_source, sx, sy);
		}
	}

	ENTRY entry;
	entry.m_tile.m_texture = rotated.get();
	entry.m_tile.m_width = rotatedW;
	entry.m_tile.m_height = rotatedH;
	entry.m_texture = std::move(rotated);
	auto inserted = cache.emplace(key, std::move(entry));
	g_cachedPixels += (size_t) rotatedW * rotatedH;
	return &inserted.first->second.m_tile;
}

void Forget(const TEXTURE* p_source)
{
	if (g_clearing || !p_source) {
		return;
	}
	std::map<KEY, ENTRY>& cache = Cache();
	for (auto it = cache.begin(); it != cache.end();) {
		if (it->first.m_source == p_source || it->second.m_tile.m_texture == p_source) {
			const size_t pixels = (size_t) it->second.m_tile.m_width * it->second.m_tile.m_height;
			g_cachedPixels -= pixels < g_cachedPixels ? pixels : g_cachedPixels;
			g_clearing = true;
			it = cache.erase(it);
			g_clearing = false;
		}
		else {
			++it;
		}
	}
}

void Clear()
{
	std::map<KEY, ENTRY> dying;
	dying.swap(Cache());
	g_cachedPixels = 0;
	g_clearing = true;
	dying.clear();
	g_clearing = false;
}

} // namespace SPRITE_ROTATE
