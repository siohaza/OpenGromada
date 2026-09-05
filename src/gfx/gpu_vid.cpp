#include "game/game_descriptor.h"
#include "game/terrain_camera.h"
#include "gfx/gpu_backend.h"
#include "gfx/gpu_texture.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "gfx/texture.h"
#include "video/vid_software.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <list>
#include <memory>
#include <unordered_map>
#include <vector>

namespace GPU_VID
{
struct Frame {
	uint32_t data = 0;
	uint64_t generation = 0;
	size_t bytes = 0;
	std::list<Frame*>::iterator recent;
	std::vector<uint32_t> rowOffsets;
	unsigned rawKey = 0;
	unsigned rowKey = 0;
};
static std::unordered_map<const unsigned char*, std::unique_ptr<Frame>> s_frames;
static std::list<Frame*> s_recent;
static constexpr size_t DecodedCacheBytes = 64 * 1024 * 1024;
static size_t s_decodedBytes = 0;
static uint64_t s_generation = 0;
static unsigned s_paletteKey, s_xKey, s_yKey, s_depthKey;

static void SynchronizeGeneration()
{
	if (s_generation == GPU_RENDER::Generation()) {
		return;
	}
	for (Frame* frame : s_recent) {
		frame->data = 0;
		frame->bytes = 0;
		std::vector<uint32_t>().swap(frame->rowOffsets);
	}
	s_recent.clear();
	s_decodedBytes = 0;
	s_generation = GPU_RENDER::Generation();
}

static void Evict(Frame& frame)
{
	if (frame.bytes) {
		s_decodedBytes -= frame.bytes;
		s_recent.erase(frame.recent);
	}
	if (frame.generation == GPU_RENDER::Generation()) {
		GPU_RENDER::Release(frame.data);
	}
	GPU_RENDER::Forget(&frame.rawKey);
	GPU_RENDER::Forget(&frame.rowKey);
	frame.data = 0;
	frame.bytes = 0;
	std::vector<uint32_t>().swap(frame.rowOffsets);
}

void Forget(const void* begin, size_t bytes)
{
	SynchronizeGeneration();
	GPU_RENDER::Forget(begin);
	uintptr_t start = reinterpret_cast<uintptr_t>(begin);
	for (auto iter = s_frames.begin(); iter != s_frames.end();) {
		uintptr_t address = reinterpret_cast<uintptr_t>(iter->first);
		if (address >= start && address - start < bytes) {
			Evict(*iter->second);
			iter = s_frames.erase(iter);
		}
		else {
			++iter;
		}
	}
}

static uint32_t FrameData(VID_SOFTWARE* vid, const unsigned char* rle, int rows)
{
	SynchronizeGeneration();
	uintptr_t begin = reinterpret_cast<uintptr_t>(vid->m_unk0x48c);
	uintptr_t input = reinterpret_cast<uintptr_t>(rle);
	if (!begin || vid->m_unk0x488 <= 0 || input < begin || input - begin > size_t(vid->m_unk0x488)) {
		GPU_RENDER::Fail("GPU VID frame lies outside its allocation");
		return 0;
	}
	auto& entry = s_frames[rle];
	if (!entry) {
		entry = std::make_unique<Frame>();
	}
	Frame& frame = *entry;
	if (frame.generation == GPU_RENDER::Generation() && frame.data) {
		s_recent.splice(s_recent.end(), s_recent, frame.recent);
		return frame.data;
	}
	frame.generation = GPU_RENDER::Generation();
	frame.rowOffsets.clear();
	if (vid->m_unk0x2f6 <= 0 || rows <= 0 ||
		size_t(vid->m_unk0x2f6) > SIZE_MAX / size_t(rows) / (2 * sizeof(uint32_t))) {
		GPU_RENDER::Fail("Invalid GPU VID decoded frame size");
		return 0;
	}
	const size_t bytes = size_t(vid->m_unk0x2f6) * rows * 2 * sizeof(uint32_t);
	const bool alpha = (vid->m_pixelFlag16 & 10) == 10;
	const unsigned kind = !(vid->m_pixelFlag16 & 8) ? 2 : !alpha && (vid->m_pixelFlag16 & 4) ? 3 : 1;
	const unsigned char* cursor = rle;
	const unsigned char* end = static_cast<const unsigned char*>(vid->m_unk0x48c) + vid->m_unk0x488;
	for (int row = 0; row < rows; ++row) {
		frame.rowOffsets.push_back(uint32_t(cursor - rle));
		unsigned x = 0;
		for (;;) {
			if (cursor > end || size_t(end - cursor) < 2) {
				GPU_RENDER::Fail("Truncated GPU VID row");
				return 0;
			}
			unsigned skip = cursor[0], count = cursor[1];
			cursor += 2;
			if (!skip && !count) {
				break;
			}
			x += skip + count;
			if (x > unsigned(vid->m_unk0x2f6) || size_t(end - cursor) < kind * count) {
				GPU_RENDER::Fail("Invalid GPU VID span");
				return 0;
			}
			cursor += kind * count;
		}
	}
	while (!s_recent.empty() && (bytes > DecodedCacheBytes || s_decodedBytes > DecodedCacheBytes - bytes)) {
		Evict(*s_recent.front());
	}
	uint32_t raw = GPU_RENDER::Upload(&frame.rawKey, rle, size_t(cursor - rle));
	uint32_t offsets = GPU_RENDER::Upload(&frame.rowKey, frame.rowOffsets.data(), frame.rowOffsets.size() * 4);
	frame.data = raw && offsets ? GPU_RENDER::Allocate(bytes / sizeof(uint32_t)) : 0;
	if (!frame.data) {
		Evict(frame);
		return 0;
	}
	frame.bytes = bytes;
	s_decodedBytes += bytes;
	s_recent.push_back(&frame);
	frame.recent = std::prev(s_recent.end());
	GPU_RENDER::Command command;
	command.op = DecodeFrame;
	command.right = vid->m_unk0x2f6;
	command.bottom = rows;
	command.p[EncodedFrame] = raw;
	command.p[RowOffsets] = offsets;
	command.p[DecodedFrame] = frame.data;
	command.p[RowWidth] = vid->m_unk0x2f6;
	command.p[PixelEncoding] = kind;
	GPU_RENDER::Submit(command, true);
	return frame.data;
}

static int Boundary(int value, float scale)
{
	double scaled = double(value) * scale;
	return int(scaled >= 0 ? std::floor(scaled + 0.5) : std::ceil(scaled - 0.5));
}

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
			   TEXTURE* terrainColor,
			   TEXTURE* terrainDepth)
{
	if (!(vid->m_pixelFlag16 & 1) || rows <= 0) {
		return;
	}
	const int sourceWidth = vid->m_unk0x2f6;
	if (sourceWidth <= 0 || !(scale > 0) || !std::isfinite(scale) || horizontalGap < 0 ||
		double(sourceWidth) * scale + horizontalGap > 65536 || double(rows) * scale > 65536) {
		GPU_RENDER::Fail("GPU VID dimensions exceed bounded coordinates");
		return;
	}
	const int width = Boundary(sourceWidth, scale) + horizontalGap, height = Boundary(rows, scale);
	if (width <= 0 || height <= 0 || width > 65536 || height > 65536) {
		return;
	}
	const int targetWidth = terrainColor ? terrainColor->m_width : GPU_RENDER::Width();
	const int targetHeight = terrainColor ? terrainColor->m_height : GPU_RENDER::Height();
	GPU_RENDER::Command command;
	command.op = CompositeFrame;
	command.left = std::max({x, int(Graph->m_viewXMin), 0});
	command.top = std::max({y, int(Graph->m_viewYMin), 0});
	command.right = int(std::min({int64_t(x) + width, int64_t(Graph->m_viewXMax), int64_t(targetWidth)}));
	command.bottom = int(std::min({int64_t(y) + height, int64_t(Graph->m_viewYMax), int64_t(targetHeight)}));
	if (command.left >= command.right || command.top >= command.bottom) {
		return;
	}
	uint32_t frame = FrameData(vid, rle, rows);
	if (!frame) {
		return;
	}
	std::vector<uint32_t> xs(size_t(width), UINT32_MAX), ys(size_t(height), UINT32_MAX), depths(size_t(height), 0);
	int split = sourceWidth / 2;
	for (int sx = 0; sx < sourceWidth; ++sx) {
		if (horizontalGap > 0 && sx >= split && sx < split + 2) {
			continue;
		}
		int offset = sx >= split + 2 ? horizontalGap : 0;
		for (int dx = Boundary(sx, scale) + offset; dx < Boundary(sx + 1, scale) + offset; ++dx) {
			if (dx >= 0 && dx < width) {
				xs[size_t(dx)] = sx;
			}
		}
	}
	if (horizontalGap > 0) {
		int tile = Boundary(2, scale), first = Boundary(1, scale), start = Boundary(split, scale);
		for (int dx = 0; tile > 0 && dx < horizontalGap + tile && start + dx < width; ++dx) {
			if (start + dx >= 0) {
				xs[size_t(start + dx)] = 0x80000000u | uint32_t(split + (dx % tile >= first));
			}
		}
	}
	bool alpha = (vid->m_pixelFlag16 & 10) == 10;
	bool delta = !alpha && (vid->m_pixelFlag16 & 12) == 12;
	bool direct = !(vid->m_pixelFlag16 & 8);
	bool locoland = !software16 && GameDesc->m_layerRules == GAME_LAYERS_LOCOLAND;
	bool sloped = !delta && vid->m_unk0x24 > vid->m_footprintHeight;
	int firstVisibleRow = 0;
	while (firstVisibleRow < rows && y + Boundary(firstVisibleRow + 1, scale) <= command.top) {
		++firstVisibleRow;
	}
	if (!delta) {
		depth = std::min(std::bit_cast<int32_t>(uint32_t(depth) + 1024u), 32767);
	}
	if (locoland && alpha) {
		depth &= 65535;
	}
	for (int sy = 0; sy < rows; ++sy) {
		int change = locoland ? -((alpha && sloped && sy > firstVisibleRow) ? 8 * (sy - firstVisibleRow) : 0)
							  : (sloped ? 8 * (rows - sy) : 0);
		int rowDepth = std::bit_cast<int32_t>(uint32_t(depth) + uint32_t(change));
		for (int dy = Boundary(sy, scale); dy < Boundary(sy + 1, scale); ++dy) {
			ys[size_t(dy)] = sy;
			depths[size_t(dy)] = uint32_t(rowDepth);
		}
	}
	command.p[FramePixels] = frame;
	command.p[ColorTarget] = terrainColor ? terrainColor->GpuData() : GPU_RENDER::Color();
	command.p[DepthTarget] = terrainDepth ? terrainDepth->GpuData() : GPU_RENDER::Depth();
	command.p[TargetPitch] = targetWidth;
	command.p[FrameWidth] = sourceWidth;
	command.p[DrawPalette] = direct ? 0 : GPU_RENDER::Upload(&s_paletteKey, palette, vid->PaletteSize());
	command.p[DepthComparison] =
		alpha   ? 0
		: delta ? 1
		: (software16 || direct || (!locoland && x >= Graph->m_viewXMin && int64_t(x) + width <= Graph->m_viewXMax))
			? 2
			: 1;
	command.p[HasDepthDelta] = delta;
	command.p[DirectRgb16] = direct;
	command.p[PaletteRgb16] = software16 && !alpha;
	command.p[HorizontalMap] = GPU_RENDER::Upload(&s_xKey, xs.data(), xs.size() * 4);
	command.p[VerticalMap] = GPU_RENDER::Upload(&s_yKey, ys.data(), ys.size() * 4);
	command.p[RowDepths] = GPU_RENDER::Upload(&s_depthKey, depths.data(), depths.size() * 4);
	command.p[OriginX] = x;
	command.p[OriginY] = y;
	command.p[TargetRgb16] = terrainColor != nullptr;
	command.p[RedShift] = RGB16_rShift;
	command.p[GreenShift] = RGB16_gShift;
	command.p[RedMask] = RGB16_rMask;
	command.p[GreenMask] = RGB16_gMask;
	command.p[BridgeSplit] = split;
	command.p[ScaledCoordinates] = scale != 1.0f || horizontalGap > 0;
	if (terrainColor) {
		command.p[CoverageTarget] =
			GPU_RENDER::Allocate(size_t(command.right - command.left) * (command.bottom - command.top));
		command.p[CoveragePitch] = command.right - command.left;
		terrainColor->GpuWritten();
		terrainDepth->GpuWritten();
	}
	GPU_RENDER::Submit(command);
	if (terrainColor) {
		std::vector<uint32_t> coverage(size_t(command.p[CoveragePitch]) * (command.bottom - command.top));
		if (GPU_RENDER::Read(command.p[CoverageTarget], coverage.data(), coverage.size() * 4)) {
			for (int cy = command.top; cy < command.bottom; ++cy) {
				for (int cx = command.left; cx < command.right; ++cx) {
					if (coverage[size_t(cy - command.top) * command.p[CoveragePitch] + cx - command.left]) {
						TerrainCoverageMarkPixel(cx, cy);
					}
				}
			}
		}
		GPU_RENDER::Release(command.p[CoverageTarget]);
	}
}
}
