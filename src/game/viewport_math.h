#ifndef VIEWPORT_MATH_H
#define VIEWPORT_MATH_H

#include <cmath>

namespace VIEWPORT_MATH
{

inline float ClampCameraAxis(float p_value, float p_min, float p_max)
{
	if (p_max < p_min) {
		return (p_min + p_max) * 0.5f;
	}
	if (p_value < p_min) {
		return p_min;
	}
	if (p_value > p_max) {
		return p_max;
	}
	return p_value;
}

inline float ClampDirectionalAimAxis(float p_value, float p_extent, float p_retailExtent)
{
	if (!(p_extent > 0.0f) || !(p_retailExtent > 0.0f)) {
		return p_value;
	}
	float window = p_retailExtent < p_extent ? p_retailExtent : p_extent;
	float minimum = (p_extent - window) * 0.5f;
	float maximum = minimum + window;
	if (p_value < minimum) {
		return minimum;
	}
	if (p_value > maximum) {
		return maximum;
	}
	return p_value;
}

inline unsigned int ResolveShiftFlag(unsigned int p_flag, float p_viewWidth)
{
	(void) p_viewWidth;
	return p_flag;
}

inline bool CoarseSpriteVisible(
	int p_x,
	int p_yMinusZ,
	int p_worldY,
	int p_centerX,
	int p_centerY,
	float p_viewWidth,
	float p_viewHeight
)
{
	int halfWidth = (int) std::ceil((double) p_viewWidth * 0.5) + 704;
	int halfHeight = (int) std::ceil((double) p_viewHeight * 0.5) + 272;
	if (halfWidth < 1024) {
		halfWidth = 1024;
	}
	if (halfHeight < 512) {
		halfHeight = 512;
	}

	int dx = p_x - p_centerX;
	int dy = p_yMinusZ - p_centerY;
	return (dx >= -halfWidth && dx < halfWidth && dy >= -halfHeight && dy < halfHeight) ||
		   p_worldY - p_centerY >= halfHeight;
}

inline float RelativeAudioAxis(float p_source, float p_listener)
{
	return p_source - p_listener;
}

} // namespace VIEWPORT_MATH

#endif
