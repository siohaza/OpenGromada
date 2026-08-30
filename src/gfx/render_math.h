#ifndef RENDER_MATH_H
#define RENDER_MATH_H

#include <cmath>
#include <cstddef>

namespace RENDER_MATH
{

inline int QuarterExtent(float p_frameExtent)
{
	if (p_frameExtent <= 0.0f) {
		return 0;
	}
	return (int) std::ceil((double) p_frameExtent * 0.25);
}

inline int ScratchExtent(float p_frameExtent)
{
	int extent = QuarterExtent(p_frameExtent);
	if (extent < 256) {
		extent = 256;
	}
	return extent;
}

inline bool AlphaSpriteAnchorVisible(
	const unsigned short* p_depth,
	int p_pitch,
	int p_width,
	int p_height,
	int p_viewLeft,
	int p_viewTop,
	int p_viewRight,
	int p_viewBottom,
	int p_x,
	int p_y,
	int p_threshold,
	float p_footprintWidth,
	float p_footprintHeight
)
{
	(void) p_footprintWidth;
	(void) p_footprintHeight;
	if (!p_depth || p_width <= 0 || p_height <= 0 || p_pitch < p_width) {
		return true;
	}

	int left = p_viewLeft < 0 ? 0 : p_viewLeft;
	int top = p_viewTop < 0 ? 0 : p_viewTop;
	int right = p_viewRight > p_width ? p_width : p_viewRight;
	int bottom = p_viewBottom > p_height ? p_height : p_viewBottom;
	if (p_x < left || p_x >= right || p_y < top || p_y >= bottom) {
		return false;
	}

	return p_depth[(std::size_t) p_y * p_pitch + p_x] <= p_threshold;
}

inline bool AlphaSpriteChildVisible(
	const unsigned short* p_depth,
	int p_pitch,
	int p_width,
	int p_height,
	int p_viewLeft,
	int p_viewTop,
	int p_viewRight,
	int p_viewBottom,
	const int* p_dst,
	int p_threshold
)
{
	if (!p_depth || p_width <= 0 || p_height <= 0 || p_pitch < p_width) {
		return true;
	}
	if (!p_dst) {
		return false;
	}

	int left = p_dst[0];
	if (left < p_viewLeft) {
		left = p_viewLeft;
	}
	if (left < 0) {
		left = 0;
	}
	int top = p_dst[1];
	if (top < p_viewTop) {
		top = p_viewTop;
	}
	if (top < 0) {
		top = 0;
	}
	int right = p_dst[2];
	if (right > p_viewRight) {
		right = p_viewRight;
	}
	if (right > p_width) {
		right = p_width;
	}
	int bottom = p_dst[3];
	if (bottom > p_viewBottom) {
		bottom = p_viewBottom;
	}
	if (bottom > p_height) {
		bottom = p_height;
	}
	if (left >= right || top >= bottom) {
		return false;
	}

	const int xs[3] = {left, left + (right - left) / 2, right - 1};
	const int ys[3] = {top, top + (bottom - top) / 2, bottom - 1};
	for (int y : ys) {
		for (int x : xs) {
			if (p_depth[(std::size_t) y * p_pitch + x] <= p_threshold) {
				return true;
			}
		}
	}
	return false;
}

} // namespace RENDER_MATH

#endif
