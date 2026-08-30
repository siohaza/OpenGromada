#ifndef DISPLAY_MATH_H
#define DISPLAY_MATH_H

#include <stdint.h>

namespace DISPLAY_MATH
{

struct RESOLUTION {
	int m_width;
	int m_height;
};

inline RESOLUTION SanitizeOutput(int p_width, int p_height)
{
	if (p_width <= 0 || p_height <= 0) {
		return {640, 480};
	}
	return {p_width, p_height};
}

inline int PositiveMinimum(int p_a, int p_b)
{
	if (p_a <= 0) {
		return p_b;
	}
	if (p_b <= 0) {
		return p_a;
	}
	return p_a < p_b ? p_a : p_b;
}

inline RESOLUTION FitAspectWithin(RESOLUTION p_aspect, int p_maxWidth, int p_maxHeight)
{
	p_aspect = SanitizeOutput(p_aspect.m_width, p_aspect.m_height);
	if (p_maxWidth <= 0 || p_maxHeight <= 0) {
		return {640, 480};
	}

	int width = p_maxWidth;
	int64_t scaledHeight = (int64_t) width * p_aspect.m_height + p_aspect.m_width / 2;
	int height = (int) (scaledHeight / p_aspect.m_width);
	if (height > p_maxHeight) {
		height = p_maxHeight;
		int64_t scaledWidth = (int64_t) height * p_aspect.m_width + p_aspect.m_height / 2;
		width = (int) (scaledWidth / p_aspect.m_height);
	}
	if (width < 1) {
		width = 1;
	}
	if (height < 1) {
		height = 1;
	}
	if (width > p_maxWidth) {
		width = p_maxWidth;
	}
	if (height > p_maxHeight) {
		height = p_maxHeight;
	}
	return {width, height};
}

inline RESOLUTION ResolveOutput(
	int p_width,
	int p_height,
	int p_desktopWidth,
	int p_desktopHeight,
	int p_availableWidth,
	int p_availableHeight,
	bool p_automatic,
	bool p_fullscreen
)
{
	if (!p_automatic) {
		return SanitizeOutput(p_width, p_height);
	}

	RESOLUTION desktop = SanitizeOutput(p_desktopWidth, p_desktopHeight);
	if (p_fullscreen) {
		return desktop;
	}
	int maxWidth = PositiveMinimum(1280, p_availableWidth > 0 ? p_availableWidth : desktop.m_width);
	int maxHeight = PositiveMinimum(800, p_availableHeight > 0 ? p_availableHeight : desktop.m_height);
	return FitAspectWithin(desktop, maxWidth, maxHeight);
}

inline RESOLUTION ResolveOutput(int p_width, int p_height, int p_desktopWidth, int p_desktopHeight, bool p_automatic)
{
	if (p_automatic) {
		return SanitizeOutput(p_desktopWidth, p_desktopHeight);
	}
	return SanitizeOutput(p_width, p_height);
}

inline RESOLUTION ResolveInternal(int p_outputWidth, int p_outputHeight, int p_renderWidth, bool p_native)
{
	RESOLUTION output = SanitizeOutput(p_outputWidth, p_outputHeight);
	if (p_native) {
		return output;
	}

	if (p_renderWidth > 0) {
		int width = p_renderWidth < output.m_width ? p_renderWidth : output.m_width;
		int64_t scaled = (int64_t) width * output.m_height + output.m_width / 2;
		int height = (int) (scaled / output.m_width);
		if (height < 1) {
			height = 1;
		}
		return {width, height};
	}

	int height = output.m_height < 480 ? output.m_height : 480;
	int64_t scaled = (int64_t) height * output.m_width + output.m_height / 2;
	int width = (int) (scaled / output.m_height);
	if (width < 1) {
		width = 1;
	}
	if (width > output.m_width) {
		width = output.m_width;
	}
	return {width, height};
}

inline int GreatestCommonDivisor(int p_a, int p_b)
{
	while (p_b) {
		int next = p_a % p_b;
		p_a = p_b;
		p_b = next;
	}
	return p_a;
}

inline RESOLUTION ResolveMapSafeInternal(RESOLUTION p_frame, RESOLUTION p_outputAspect, int p_mapWidth, int p_mapHeight)
{
	const int gameplayWidthCeiling = 2000;
	if (p_frame.m_width <= 0 || p_frame.m_height <= 0 || p_mapWidth <= 0 || p_mapHeight <= 0) {
		return p_frame;
	}
	if (p_frame.m_width <= p_mapWidth && p_frame.m_height <= p_mapHeight && p_frame.m_width <= gameplayWidthCeiling) {
		return p_frame;
	}

	p_outputAspect = SanitizeOutput(p_outputAspect.m_width, p_outputAspect.m_height);
	const int divisor = GreatestCommonDivisor(p_outputAspect.m_width, p_outputAspect.m_height);
	const int aspectWidth = p_outputAspect.m_width / divisor;
	const int aspectHeight = p_outputAspect.m_height / divisor;
	int maxWidth = p_frame.m_width < p_mapWidth ? p_frame.m_width : p_mapWidth;
	if (maxWidth > gameplayWidthCeiling) {
		maxWidth = gameplayWidthCeiling;
	}
	const int maxHeight = p_frame.m_height < p_mapHeight ? p_frame.m_height : p_mapHeight;
	const int widthScale = maxWidth / aspectWidth;
	const int heightScale = maxHeight / aspectHeight;
	const int scale = widthScale < heightScale ? widthScale : heightScale;
	if (scale <= 0) {
		return p_frame;
	}

	RESOLUTION target = {aspectWidth * scale, aspectHeight * scale};
	const RESOLUTION retail = ResolveInternal(p_outputAspect.m_width, p_outputAspect.m_height, 0, false);
	if (target.m_width < retail.m_width || target.m_height < retail.m_height) {
		return p_frame;
	}
	return target;
}

inline int ResolveUIScale(int p_width, int p_height, int p_override)
{
	if (p_override >= 1 && p_override <= 3) {
		return p_override;
	}
	if (p_width <= 0 || p_height <= 0) {
		return 1;
	}
	int scaleX = p_width / 640;
	int scaleY = p_height / 480;
	int scale = scaleX < scaleY ? scaleX : scaleY;
	if (scale < 1) {
		scale = 1;
	}
	if (scale > 3) {
		scale = 3;
	}
	return scale;
}

} // namespace DISPLAY_MATH

#endif
