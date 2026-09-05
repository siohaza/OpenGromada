#ifndef GPU_TEXTURE_H
#define GPU_TEXTURE_H

#include <cstddef>
#include <cstdint>

class TEXTURE;
class GAMMA;
class VID_SOFTWARE;

namespace GPU_TEXTURE
{
enum Operation : uint32_t {
	Expand = 100,
	Quad = 101,
	CopyDepth = 102,
	CopyColor = 103,
	Predicate = 104
};
enum Parameter : unsigned {
	Source,
	Destination,
	Depth,
	Pitch,
	SourceWidth,
	SourceHeight,
	Format,
	Palette,
	RawPitch,
	DestinationLeft,
	DestinationTop,
	DestinationWidth,
	DestinationHeight,
	SourceLeft,
	SourceTop,
	StepU,
	StepV,
	Bilinear,
	Diffuse,
	Specular,
	ModulateShift,
	UseSpecular,
	Blend,
	SourceBlend,
	DestinationBlend,
	AlphaTest,
	AlphaReference,
	DepthTest,
	DepthWrite,
	DepthRows,
	PredicateOffset,
	SourceExtentWidth,
	SourceExtentHeight
};
void Draw(TEXTURE* texture,
		  const int* destination,
		  const int* source,
		  const GAMMA* gamma,
		  bool depthTest,
		  double depthTop,
		  double depthBottom);
void CopyDepthToScreen(TEXTURE* texture, const int* destination, const int* source, bool scaled);
uint32_t CurrentPredicate();
void SetPredicate(uint32_t predicate);
uint32_t Visibility(int x, int y, int threshold, const int* child = nullptr, uint32_t anchor = 0);
}

namespace GPU_VID
{
enum Operation : uint32_t {
	DecodeFrame = 200,
	CompositeFrame = 201
};
enum DecodeParameter : unsigned {
	EncodedFrame,
	RowOffsets,
	DecodedFrame,
	RowWidth,
	PixelEncoding
};
enum DrawParameter : unsigned {
	FramePixels,
	ColorTarget,
	DepthTarget,
	TargetPitch,
	FrameWidth,
	DrawPalette,
	DepthComparison,
	HasDepthDelta,
	DirectRgb16,
	PaletteRgb16,
	HorizontalMap,
	VerticalMap,
	RowDepths,
	OriginX,
	OriginY,
	TargetRgb16,
	RedShift,
	GreenShift,
	RedMask,
	GreenMask,
	BridgeSplit,
	ScaledCoordinates,
	CoverageTarget,
	CoveragePitch
};
void Forget(const void* begin, size_t bytes);
void DrawFrame(VID_SOFTWARE* vid,
			   const unsigned char* rle,
			   int rows,
			   int x,
			   int y,
			   int depth,
			   float scale,
			   int horizontalGap,
			   const void* palette,
			   bool software16,
			   TEXTURE* terrainColor = nullptr,
			   TEXTURE* terrainDepth = nullptr);
}

#endif
