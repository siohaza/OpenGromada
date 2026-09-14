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

} // namespace RENDER_MATH

#endif
