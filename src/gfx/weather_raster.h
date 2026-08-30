#ifndef WEATHER_RASTER_H
#define WEATHER_RASTER_H

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <stdlib.h>

namespace WEATHER_RASTER
{

inline int QuarterExtent(int p_extent)
{
	return p_extent > 0 ? (p_extent + 3) / 4 : 0;
}

template <bool Paletted>
inline bool FillFog(
	unsigned char* p_dst,
	int p_dstPitch,
	int p_dstWidth,
	int p_dstHeight,
	const unsigned short* p_depth,
	int p_depthPitch,
	int p_frameWidth,
	int p_frameHeight,
	int p_left,
	int p_top,
	int p_right,
	int p_bottom,
	const unsigned short* p_ramp,
	int p_zBase,
	int p_zFar
)
{
	const int cols = p_right - p_left;
	const int rows = p_bottom - p_top;
	const int outWidth = QuarterExtent(cols);
	const int outHeight = QuarterExtent(rows);
	const int bytesPerPixel = Paletted ? 1 : 2;
	if (!p_dst || !p_depth || !p_ramp || p_depthPitch < p_frameWidth || p_frameWidth <= 0 || p_frameHeight <= 0 ||
		p_left < 0 || p_top < 0 || p_right > p_frameWidth || p_bottom > p_frameHeight || cols <= 0 || rows <= 0 ||
		p_dstWidth < outWidth || p_dstHeight < outHeight || p_dstPitch < outWidth * bytesPerPixel) {
		return false;
	}

	unsigned short carry16 = 0;
	unsigned char carry8 = 0;
	for (int outY = 0, y = p_top; y < p_bottom; ++outY, y += 4) {
		unsigned char* dstRow = p_dst + (std::size_t) outY * p_dstPitch;
		for (int outX = 0, x = p_left; x < p_right; ++outX, x += 4) {
			const int xEnd = std::min(x + 3, p_right - 1);
			const unsigned short* depthRow = p_depth + (std::size_t) y * p_depthPitch;
			const int z = std::min(depthRow[x], depthRow[xEnd]) - 1024;
			if constexpr (Paletted) {
				if (z <= p_zBase) {
					carry8 = z <= p_zFar ? 0xff : ((const unsigned char*) p_ramp)[2 * (p_zBase - z)];
				}
				else if (z <= p_zBase + 10) {
					carry8 = 0;
				}
				dstRow[outX] = carry8;
			}
			else {
				if (z <= p_zBase) {
					carry16 = p_ramp[p_zBase - (z <= p_zFar ? p_zFar : z)];
				}
				else if (z <= p_zBase + 10) {
					carry16 = 0;
				}
				std::memcpy(dstRow + 2 * outX, &carry16, sizeof(carry16));
			}
		}
	}
	return true;
}

inline int SnowDepthDelta(const unsigned short* p_depth, int p_pitch, int p_width, int p_height, int p_x, int p_y)
{
	if (!p_depth || p_pitch < p_width || p_x < 0 || p_x >= p_width || p_y < 0 || p_y >= p_height) {
		return -1;
	}
	const int current = p_depth[(std::size_t) p_y * p_pitch + p_x];
	if (p_y >= 4) {
		const int previous = p_depth[(std::size_t) (p_y - 4) * p_pitch + p_x];
		const int dz = abs(current - previous);
		if (dz <= 6) {
			return dz;
		}
	}
	if (p_y + 4 < p_height) {
		const int next = p_depth[(std::size_t) (p_y + 4) * p_pitch + p_x];
		const int dz = abs(current - next);
		if (dz <= 6) {
			return dz;
		}
	}
	return -1;
}

template <bool Paletted>
inline bool FillSnow(
	unsigned char* p_dst,
	int p_dstPitch,
	int p_dstWidth,
	int p_dstHeight,
	const unsigned short* p_depth,
	int p_depthPitch,
	int p_width,
	int p_height,
	int p_xPhase,
	int p_yPhase,
	unsigned int p_fade,
	const unsigned short* p_ramp
)
{
	const int outWidth = QuarterExtent(p_width);
	const int outHeight = QuarterExtent(p_height);
	const int bytesPerPixel = Paletted ? 1 : 2;
	if (!p_dst || !p_depth || (!Paletted && !p_ramp) || p_width <= 0 || p_height <= 0 || p_depthPitch < p_width ||
		p_xPhase < 0 || p_xPhase > 3 || p_yPhase < 0 || p_yPhase > 3 || p_dstWidth < outWidth ||
		p_dstHeight < outHeight || p_dstPitch < outWidth * bytesPerPixel) {
		return false;
	}

	for (int outY = 0; outY < outHeight; ++outY) {
		const int y = std::min(p_yPhase + 4 * outY, p_height - 1);
		unsigned char* dstRow = p_dst + (std::size_t) outY * p_dstPitch;
		for (int outX = 0; outX < outWidth; ++outX) {
			const int x = std::min(p_xPhase + 4 * outX, p_width - 1);
			const int dz = SnowDepthDelta(p_depth, p_depthPitch, p_width, p_height, x, y);
			const unsigned int intensity = dz < 0 ? 0 : std::min(255u, ((6u - (unsigned int) dz) * p_fade) >> 3);
			if constexpr (Paletted) {
				dstRow[outX] = (unsigned char) intensity;
			}
			else {
				const unsigned short value = p_ramp[intensity];
				std::memcpy(dstRow + 2 * outX, &value, sizeof(value));
			}
		}
	}
	return true;
}

} // namespace WEATHER_RASTER

#endif
