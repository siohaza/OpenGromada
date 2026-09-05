#ifndef UI_SCALING_H
#define UI_SCALING_H

#include <float.h>
#include <stddef.h>
#include <stdint.h>

namespace UI_SCALING
{

constexpr float RETAIL_WIDTH = 640.0f;
constexpr float RETAIL_HEIGHT = 480.0f;

enum AXIS_ANCHOR {
	ANCHOR_MIN_EDGE,
	ANCHOR_CENTER,
	ANCHOR_MAX_EDGE
};

struct MENU_POINT {
	float m_x;
	float m_y;
	float m_z;
	AXIS_ANCHOR m_anchorX;
	AXIS_ANCHOR m_anchorY;
};

inline int NormalizeScale(int p_scale)
{
	if (p_scale < 1) {
		return 1;
	}
	if (p_scale > 3) {
		return 3;
	}
	return p_scale;
}

inline float NormalizeDrawScale(float p_scale)
{
	if (!(p_scale > 0.0f) || p_scale > FLT_MAX) {
		return 1.0f;
	}
	if (p_scale > 3.0f) {
		return 3.0f;
	}
	return p_scale;
}

inline float ScaleMetric(float p_value, float p_scale)
{
	return p_value * NormalizeDrawScale(p_scale);
}

inline bool HitTestSprite(
	float p_centerX,
	float p_top,
	float p_halfWidth,
	float p_upperOffset,
	float p_lowerOffset,
	float p_scale,
	float p_x,
	float p_y
)
{
	float halfWidth = ScaleMetric(p_halfWidth, p_scale);
	if (p_x < p_centerX - halfWidth || p_x > p_centerX + halfWidth) {
		return false;
	}
	if (p_top - ScaleMetric(p_upperOffset + p_lowerOffset, p_scale) >= p_y) {
		return false;
	}
	if (p_top + ScaleMetric(p_lowerOffset, p_scale) <= p_y) {
		return false;
	}
	return true;
}

inline bool HitTestCentered(
	float p_centerX,
	float p_centerY,
	float p_halfWidth,
	float p_halfHeight,
	float p_scale,
	float p_x,
	float p_y
)
{
	float halfWidth = ScaleMetric(p_halfWidth, p_scale);
	float halfHeight = ScaleMetric(p_halfHeight, p_scale);
	return p_x >= p_centerX - halfWidth && p_x <= p_centerX + halfWidth && p_y >= p_centerY - halfHeight &&
		   p_y <= p_centerY + halfHeight;
}

struct MOUSE_TIP_PLACEMENT {
	float m_x;
	float m_y;
	float m_width;
};

inline MOUSE_TIP_PLACEMENT PlaceMouseTip(
	float p_cursorX,
	float p_cursorY,
	float p_viewXMax,
	float p_viewYMin,
	float p_glyphWidth,
	float p_glyphHeight,
	int p_columns,
	float p_scale,
	float p_depth
)
{
	p_scale = NormalizeDrawScale(p_scale);
	float glyphWidth = ScaleMetric(p_glyphWidth, p_scale);
	float glyphHeight = ScaleMetric(p_glyphHeight, p_scale);
	float cursorGap = ScaleMetric(5.0f, p_scale);
	float verticalGap = ScaleMetric(10.0f, p_scale);
	MOUSE_TIP_PLACEMENT result =
		{p_cursorX + cursorGap, p_cursorY - glyphHeight + p_depth - verticalGap, (p_columns + 2) * glyphWidth};
	float halfHeight = glyphHeight * 0.5f;
	if (halfHeight + result.m_y - p_depth <= p_viewYMin) {
		result.m_y = halfHeight + p_cursorY + p_depth + verticalGap;
	}
	if (result.m_width + result.m_x >= p_viewXMax) {
		result.m_x = p_viewXMax - result.m_width;
	}
	return result;
}

inline AXIS_ANCHOR ClassifyAxis(float p_designPosition, float p_retailExtent)
{
	if (p_designPosition < p_retailExtent * 0.25f) {
		return ANCHOR_MIN_EDGE;
	}
	if (p_designPosition > p_retailExtent * 0.75f) {
		return ANCHOR_MAX_EDGE;
	}
	return ANCHOR_CENTER;
}

inline float TransformMenuAxis(
	float p_authoredPosition,
	int p_origin,
	int p_menuExtent,
	float p_viewExtent,
	float p_scale,
	float p_retailExtent
)
{
	float authoredPosition = p_authoredPosition - (float) p_origin;
	float menuHalf = (float) (p_menuExtent / 2);
	p_scale = NormalizeDrawScale(p_scale);

	float authoredExtent = p_menuExtent > 0 ? (float) p_menuExtent : p_retailExtent;

	switch (ClassifyAxis(authoredPosition, authoredExtent)) {
	case ANCHOR_MIN_EDGE:
		return authoredPosition * (float) p_scale;
	case ANCHOR_MAX_EDGE:
		return p_viewExtent - (authoredExtent - authoredPosition) * (float) p_scale;
	default:
		return p_viewExtent * 0.5f + (authoredPosition - menuHalf) * (float) p_scale;
	}
}

inline bool IsShippedLegacyViewport(float p_width, float p_height, float p_scale)
{
	if (NormalizeDrawScale(p_scale) != 1.0f) {
		return false;
	}
	return (p_width == 640.0f && p_height == 480.0f) || (p_width == 800.0f && p_height == 600.0f) ||
		   (p_width == 1024.0f && p_height == 768.0f) || (p_width == 1280.0f && p_height == 720.0f);
}

inline MENU_POINT TransformMenuPoint(
	float p_x,
	float p_y,
	float p_z,
	int p_originX,
	int p_originY,
	int p_menuWidth,
	int p_menuHeight,
	float p_viewWidth,
	float p_viewHeight,
	float p_scale
)
{
	bool legacyViewport = IsShippedLegacyViewport(p_viewWidth, p_viewHeight, p_scale);
	float x = legacyViewport ? p_x - p_originX - p_menuWidth / 2 + p_viewWidth * 0.5f
							 : TransformMenuAxis(p_x, p_originX, p_menuWidth, p_viewWidth, p_scale, RETAIL_WIDTH);
	float y = legacyViewport
				  ? p_y - p_originY - p_menuHeight / 2 + p_viewHeight * 0.5f
				  : TransformMenuAxis(p_y - p_z, p_originY, p_menuHeight, p_viewHeight, p_scale, RETAIL_HEIGHT) + p_z;
	float authoredX = p_x - (float) p_originX;
	float authoredY = p_y - p_z - (float) p_originY;
	float authoredWidth = p_menuWidth > 0 ? (float) p_menuWidth : RETAIL_WIDTH;
	float authoredHeight = p_menuHeight > 0 ? (float) p_menuHeight : RETAIL_HEIGHT;
	MENU_POINT result = {x, y, p_z, ClassifyAxis(authoredX, authoredWidth), ClassifyAxis(authoredY, authoredHeight)};
	return result;
}

inline MENU_POINT TransformCenteredMenuPoint(
	float p_x,
	float p_y,
	float p_z,
	int p_originX,
	int p_originY,
	int p_menuWidth,
	int p_menuHeight,
	float p_viewWidth,
	float p_viewHeight,
	float p_scale
)
{
	p_scale = NormalizeDrawScale(p_scale);
	float menuHalfWidth = (float) (p_menuWidth / 2);
	float menuHalfHeight = (float) (p_menuHeight / 2);
	float x = p_viewWidth * 0.5f + (p_x - (float) p_originX - menuHalfWidth) * (float) p_scale;
	float projectedY = p_viewHeight * 0.5f + (p_y - p_z - (float) p_originY - menuHalfHeight) * (float) p_scale;
	MENU_POINT result = {x, projectedY + p_z, p_z, ANCHOR_CENTER, ANCHOR_CENTER};
	return result;
}

inline float UntransformCanvasAxis(float p_position, float p_viewExtent, float p_canvasExtent, float p_scale)
{
	return p_canvasExtent * 0.5f + (p_position - p_viewExtent * 0.5f) / NormalizeDrawScale(p_scale);
}

inline float TransformAnchoredReferenceAxis(
	float p_position,
	float p_referenceExtent,
	float p_viewExtent,
	float p_scale,
	AXIS_ANCHOR p_anchor
)
{
	p_scale = NormalizeDrawScale(p_scale);
	switch (p_anchor) {
	case ANCHOR_MIN_EDGE:
		return p_position * (float) p_scale;
	case ANCHOR_MAX_EDGE:
		return p_viewExtent - (p_referenceExtent - p_position) * (float) p_scale;
	default:
		return p_viewExtent * 0.5f + (p_position - p_referenceExtent * 0.5f) * (float) p_scale;
	}
}

inline bool IsGamebarInventoryRoot(int p_nvid)
{
	switch (p_nvid) {
	case 727: // lives
	case 728:
	case 729:
	case 730:
	case 731:
	case 747: // inventory-cell background
	case 748: // explosives
		return true;
	default:
		return false;
	}
}

inline int GamebarInventoryOffset(float p_viewWidth, float p_viewHeight, float p_scale, int p_backingWidth)
{
	p_scale = NormalizeDrawScale(p_scale);
	if (p_viewWidth * RETAIL_HEIGHT <= p_viewHeight * RETAIL_WIDTH) {
		return 0;
	}
	float exactGap = p_viewWidth - p_backingWidth * p_scale;
	int gap = exactGap > 0.0f ? (int) (exactGap + 0.5f) : 0;
	return gap > 0 ? gap : 0;
}

inline float NormalizeWidescreenWeaponProjectedY(
	float p_projectedY,
	float p_nominalWidth,
	float p_nominalHeight,
	float p_viewWidth,
	float p_viewHeight,
	float p_scale,
	int p_nvid
)
{
	if (p_nvid != 710 && p_nvid != 745) {
		return p_projectedY;
	}
	p_scale = NormalizeDrawScale(p_scale);
	if (p_viewWidth * RETAIL_HEIGHT <= p_viewHeight * RETAIL_WIDTH || p_viewHeight < RETAIL_HEIGHT * p_scale) {
		return p_projectedY;
	}

	float selectorBase = 0.0f;
	float ammoBase = 0.0f;
	if (p_nominalWidth == 640.0f && p_nominalHeight == 480.0f) {
		selectorBase = 52.0f;
		ammoBase = 60.0f;
	}
	else if (p_nominalWidth == 800.0f && p_nominalHeight == 600.0f) {
		selectorBase = 47.0f;
		ammoBase = 55.0f;
	}
	else if (p_nominalWidth == 1024.0f && p_nominalHeight == 768.0f) {
		selectorBase = 53.0f;
		ammoBase = 61.0f;
	}
	else {
		return p_projectedY;
	}

	float sourceBase = p_nvid == 710 ? selectorBase : ammoBase;
	float targetBase = p_nvid == 710 ? 70.0f : 78.0f;
	for (int slot = 0; slot < 10; ++slot) {
		if (p_projectedY == sourceBase + slot * 36.0f) {
			return targetBase + slot * 36.0f;
		}
	}
	return p_projectedY;
}

inline MENU_POINT TransformGamebarPoint(
	float p_x,
	float p_y,
	float p_z,
	int p_originX,
	int p_originY,
	int p_menuWidth,
	int p_menuHeight,
	float p_nominalWidth,
	float p_nominalHeight,
	float p_viewWidth,
	float p_viewHeight,
	float p_scale,
	int p_nvid = -1
)
{
	if (p_nominalWidth <= 0.0f || p_nominalHeight <= 0.0f) {
		return TransformCenteredMenuPoint(
			p_x,
			p_y,
			p_z,
			p_originX,
			p_originY,
			p_menuWidth,
			p_menuHeight,
			p_viewWidth,
			p_viewHeight,
			p_scale
		);
	}

	float nominalX = p_x - (float) p_originX - (float) (p_menuWidth / 2) + p_nominalWidth * 0.5f;
	float nominalProjectedY = p_y - p_z - (float) p_originY - (float) (p_menuHeight / 2) + p_nominalHeight * 0.5f;
	bool wideViewport = p_viewWidth * RETAIL_HEIGHT > p_viewHeight * RETAIL_WIDTH;
	nominalProjectedY = NormalizeWidescreenWeaponProjectedY(
		nominalProjectedY,
		p_nominalWidth,
		p_nominalHeight,
		p_viewWidth,
		p_viewHeight,
		p_scale,
		p_nvid
	);
	bool rightInventory = wideViewport && IsGamebarInventoryRoot(p_nvid);
	AXIS_ANCHOR anchorX = rightInventory ? ANCHOR_MAX_EDGE : ANCHOR_MIN_EDGE;
	float referenceWidth = rightInventory ? RETAIL_WIDTH : p_nominalWidth;
	AXIS_ANCHOR anchorY = nominalProjectedY > p_nominalHeight * 0.875f ? ANCHOR_MAX_EDGE : ANCHOR_MIN_EDGE;
	float transformedY =
		TransformAnchoredReferenceAxis(nominalProjectedY, p_nominalHeight, p_viewHeight, p_scale, anchorY);
	if (rightInventory) {
		transformedY += 6.0f * p_scale;
	}
	MENU_POINT result = {
		TransformAnchoredReferenceAxis(nominalX, referenceWidth, p_viewWidth, p_scale, anchorX),
		transformedY + p_z,
		p_z,
		anchorX,
		anchorY
	};
	return result;
}

inline float TransformScriptAxis(float p_position, float p_viewExtent, float p_scale, AXIS_ANCHOR p_anchor)
{
	p_scale = NormalizeDrawScale(p_scale);
	if (p_scale == 1.0f) {
		return p_position;
	}
	switch (p_anchor) {
	case ANCHOR_MIN_EDGE:
		return p_position * (float) p_scale;
	case ANCHOR_MAX_EDGE:
		return p_viewExtent - (p_viewExtent - p_position) * (float) p_scale;
	default:
		return p_viewExtent * 0.5f + (p_position - p_viewExtent * 0.5f) * (float) p_scale;
	}
}

inline float UntransformScriptAxis(float p_position, float p_viewExtent, float p_scale, AXIS_ANCHOR p_anchor)
{
	p_scale = NormalizeDrawScale(p_scale);
	if (p_scale == 1.0f) {
		return p_position;
	}
	switch (p_anchor) {
	case ANCHOR_MIN_EDGE:
		return p_position / (float) p_scale;
	case ANCHOR_MAX_EDGE:
		return p_viewExtent - (p_viewExtent - p_position) / (float) p_scale;
	default:
		return p_viewExtent * 0.5f + (p_position - p_viewExtent * 0.5f) / (float) p_scale;
	}
}

inline MENU_POINT TransformAnchoredScriptPoint(
	float p_x,
	float p_y,
	float p_z,
	float p_viewWidth,
	float p_viewHeight,
	float p_scale,
	AXIS_ANCHOR p_anchorX,
	AXIS_ANCHOR p_anchorY
)
{
	MENU_POINT result = {
		TransformScriptAxis(p_x, p_viewWidth, p_scale, p_anchorX),
		TransformScriptAxis(p_y - p_z, p_viewHeight, p_scale, p_anchorY) + p_z,
		p_z,
		p_anchorX,
		p_anchorY
	};
	return result;
}

inline MENU_POINT TransformScriptPoint(
	float p_x,
	float p_y,
	float p_z,
	float p_viewWidth,
	float p_viewHeight,
	float p_scale
)
{
	AXIS_ANCHOR anchorX = ClassifyAxis(p_x, p_viewWidth);
	AXIS_ANCHOR anchorY = ClassifyAxis(p_y - p_z, p_viewHeight);
	return TransformAnchoredScriptPoint(p_x, p_y, p_z, p_viewWidth, p_viewHeight, p_scale, anchorX, anchorY);
}

struct RECT_I {
	int m_left;
	int m_top;
	int m_right;
	int m_bottom;
};

struct RASTER_TARGET32 {
	uint32_t* m_pixels;
	int m_width;
	int m_height;
	int m_pitch;
	RECT_I m_clip;
};

inline bool OpaquePalettedDepthPass(short p_source, unsigned short p_destination, bool p_horizontallyUnclipped)
{
	return p_horizontallyUnclipped ? (unsigned short) p_source >= p_destination : p_source > (short) p_destination;
}

inline RASTER_TARGET32 MakeRasterTarget32(uint32_t* p_pixels, int p_width, int p_height, int p_pitch, RECT_I p_clip)
{
	if (p_clip.m_left < 0) {
		p_clip.m_left = 0;
	}
	if (p_clip.m_top < 0) {
		p_clip.m_top = 0;
	}
	if (p_clip.m_right > p_width) {
		p_clip.m_right = p_width;
	}
	if (p_clip.m_bottom > p_height) {
		p_clip.m_bottom = p_height;
	}
	if (p_clip.m_right < p_clip.m_left) {
		p_clip.m_right = p_clip.m_left;
	}
	if (p_clip.m_bottom < p_clip.m_top) {
		p_clip.m_bottom = p_clip.m_top;
	}
	RASTER_TARGET32 result = {p_pixels, p_width, p_height, p_pitch, p_clip};
	return result;
}

inline bool PixelBlockBounds(
	const RASTER_TARGET32& p_target,
	int p_destinationX,
	int p_destinationY,
	int p_width,
	int p_height,
	RECT_I* p_out
)
{
	if (!p_out || !p_target.m_pixels || p_target.m_width <= 0 || p_target.m_height <= 0 ||
		p_target.m_pitch < p_target.m_width || p_width <= 0 || p_height <= 0) {
		return false;
	}
	long long left64 = p_destinationX;
	long long top64 = p_destinationY;
	long long right64 = left64 + p_width;
	long long bottom64 = top64 + p_height;
	if (right64 <= p_target.m_clip.m_left || left64 >= p_target.m_clip.m_right || bottom64 <= p_target.m_clip.m_top ||
		top64 >= p_target.m_clip.m_bottom) {
		return false;
	}

	p_out->m_left = left64 < p_target.m_clip.m_left ? p_target.m_clip.m_left : (int) left64;
	p_out->m_top = top64 < p_target.m_clip.m_top ? p_target.m_clip.m_top : (int) top64;
	p_out->m_right = right64 > p_target.m_clip.m_right ? p_target.m_clip.m_right : (int) right64;
	p_out->m_bottom = bottom64 > p_target.m_clip.m_bottom ? p_target.m_clip.m_bottom : (int) bottom64;
	return p_out->m_left < p_out->m_right && p_out->m_top < p_out->m_bottom;
}

inline bool ScaledPixelBounds(
	const RASTER_TARGET32& p_target,
	int p_destinationX,
	int p_destinationY,
	int p_scale,
	RECT_I* p_out
)
{
	p_scale = NormalizeScale(p_scale);
	return PixelBlockBounds(p_target, p_destinationX, p_destinationY, p_scale, p_scale, p_out);
}

inline int StoreNearestPixel32(
	const RASTER_TARGET32& p_target,
	int p_destinationX,
	int p_destinationY,
	int p_scale,
	uint32_t p_color
)
{
	RECT_I block;
	if (!ScaledPixelBounds(p_target, p_destinationX, p_destinationY, p_scale, &block)) {
		return 0;
	}
	int written = 0;
	for (int y = block.m_top; y < block.m_bottom; ++y) {
		uint32_t* row = p_target.m_pixels + (size_t) y * p_target.m_pitch;
		for (int x = block.m_left; x < block.m_right; ++x) {
			row[x] = p_color;
			++written;
		}
	}
	return written;
}

inline int BlitNearest32(
	const RASTER_TARGET32& p_target,
	int p_destinationX,
	int p_destinationY,
	const uint32_t* p_source,
	int p_sourceWidth,
	int p_sourceHeight,
	int p_sourcePitch,
	int p_scale
)
{
	if (!p_source || p_sourceWidth <= 0 || p_sourceHeight <= 0 || p_sourcePitch < p_sourceWidth) {
		return 0;
	}
	int written = 0;
	for (int y = 0; y < p_sourceHeight; ++y) {
		for (int x = 0; x < p_sourceWidth; ++x) {
			written += StoreNearestPixel32(
				p_target,
				p_destinationX + x * p_scale,
				p_destinationY + y * p_scale,
				p_scale,
				p_source[(size_t) y * p_sourcePitch + x]
			);
		}
	}
	return written;
}

} // namespace UI_SCALING

#endif
