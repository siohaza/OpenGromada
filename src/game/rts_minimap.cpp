#include "game/rts_minimap.h"

#include "game/map.h"
#include "game/viewport_math.h"
#include "gfx/gpu_backend.h"
#include "gfx/gpu_graph.h"
#include "gfx/graph.h"
#include "sprite/r_dot.h"
#include "sprite/sprite.h"
#include "video/vid.h"
#include "world/hash_map.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace
{

struct MINIMAP_CANVAS {
	float x, y, width, height, scale;
	int screenWidth, screenHeight;
	unsigned int* pixels;

	bool Project(double p_x, double p_y, float& p_screenX, float& p_screenY) const
	{
		p_screenX = (float) (p_x * width / Map->m_w + x);
		p_screenY = (float) (p_y * height / Map->m_h + y);
		return p_screenX >= x && p_screenX < x + width && p_screenY >= y && p_screenY < y + height;
	}

	void Stamp(float p_x, float p_y, int p_nativeSize, unsigned int p_colour) const
	{

		const int size = std::max(1, (int) std::ceil(p_nativeSize * scale));
		if (!(p_x >= Graph->m_viewXMin && p_y >= Graph->m_viewYMin && p_x < Graph->m_viewXMax - (size - 1) &&
			  p_y < Graph->m_viewYMax - (size - 1) && p_x >= 0 && p_y >= 0 && p_x < screenWidth - (size - 1) &&
			  p_y < screenHeight - (size - 1))) {
			return;
		}
		const int px = (int) p_x, py = (int) p_y;
		if (GPU_RENDER::Active()) {
			GPU_GRAPH::RawRect(px, py, px + size, py + size, p_colour);
			return;
		}
		for (int row = 0; row < size; ++row) {
			unsigned int* dst = pixels + size_t(py + row) * Graph->m_pitch + px;
			std::fill_n(dst, size, p_colour);
		}
	}

	void Marker(double p_x, double p_y, int p_size, unsigned int p_colour) const
	{
		float sx, sy;
		if (!Project(p_x, p_y, sx, sy)) {
			return;
		}
		if (p_size == 4) {

			Stamp(sx - 2 * scale, sy - 2 * scale, 2, p_colour);
			Stamp(sx - 2 * scale, sy, 2, p_colour);
			Stamp(sx, sy - 2 * scale, 2, p_colour);
			Stamp(sx, sy, 2, p_colour);
		}
		else {
			Stamp(sx, sy, p_size, p_colour);
		}
	}

	void Horizontal(double p_x0, double p_x1, double p_y) const
	{
		if (!std::isfinite(p_x0) || !std::isfinite(p_x1) || !std::isfinite(p_y) || p_y < 0 || p_y >= screenHeight ||
			p_y < Graph->m_viewYMin || p_y >= Graph->m_viewYMax) {
			return;
		}
		if (p_x1 < p_x0) {
			std::swap(p_x0, p_x1);
		}
		const double first = std::max({std::trunc(p_x0), 0.0, std::ceil((double) Graph->m_viewXMin)});
		const double last =
			std::min({std::trunc(p_x1), double(screenWidth - 1), std::ceil((double) Graph->m_viewXMax) - 1});
		if (first > last) {
			return;
		}
		if (GPU_RENDER::Active()) {
			GPU_GRAPH::RawRect((int) first, (int) p_y, (int) last + 1, (int) p_y + 1, 0x00cfcfcfu);
			return;
		}
		unsigned int* dst = pixels + size_t((int) p_y) * Graph->m_pitch;
		std::fill(dst + (int) first, dst + (int) last + 1, 0x00cfcfcfu);
	}

	void Vertical(double p_x, double p_y0, double p_y1) const
	{
		if (!std::isfinite(p_x) || !std::isfinite(p_y0) || !std::isfinite(p_y1) || p_x < 0 || p_x >= screenWidth ||
			p_x < Graph->m_viewXMin || p_x >= Graph->m_viewXMax) {
			return;
		}
		if (p_y1 < p_y0) {
			std::swap(p_y0, p_y1);
		}
		const double first = std::max({std::trunc(p_y0), 0.0, std::ceil((double) Graph->m_viewYMin)});
		const double last =
			std::min({std::trunc(p_y1), double(screenHeight - 1), std::ceil((double) Graph->m_viewYMax) - 1});
		if (first > last) {
			return;
		}
		if (GPU_RENDER::Active()) {
			GPU_GRAPH::RawRect((int) p_x, (int) first, (int) p_x + 1, (int) last + 1, 0x00cfcfcfu);
			return;
		}
		for (int row = (int) first; row <= (int) last; ++row) {
			pixels[size_t(row) * Graph->m_pitch + (int) p_x] = 0x00cfcfcfu;
		}
	}

	void Viewport() const
	{
		const double x0 = Map->m_shiftX * width / Map->m_w + x;
		const double y0 = Map->m_shiftY * height / Map->m_h + y;
		const double x1 = (Graph->m_viewXMax + Map->m_shiftX - Graph->m_viewXMin) * width / Map->m_w + x;
		const double y1 = (Graph->m_viewYMax + Map->m_shiftY - Graph->m_viewYMin) * height / Map->m_h + y;
		Horizontal(x0, x1, y0);
		Horizontal(x0, x1, y1);
		Vertical(x0, y0, y1);
		Vertical(x1, y0, y1);
	}
};

bool IsArmed(const SPRITE* p_sprite)
{
	const VID* vid = p_sprite->m_vid;
	if (vid->m_sprClass == 21 || (vid->m_weaponVid && vid->m_weapon)) {
		return true;
	}
	const SPRITE* child = p_sprite->m_child;
	return child && child->m_vid == vid->m_linkVid && child->m_vid && child->m_vid->m_weaponVid &&
		   child->m_vid->m_weapon;
}

void UnitMarker(const MINIMAP_CANVAS& p_canvas, const SPRITE* p_sprite)
{
	if (!p_sprite || !p_sprite->m_vid) {
		return;
	}
	const VID* vid = p_sprite->m_vid;
	const unsigned int army = (p_sprite->m_flag >> 11) & 3;
	unsigned int colour;
	int size = 2;
	if (army < 2) {
		if (vid->m_unk0x0c & 8) {
			colour = army ? 0xffff8080u : 0xff00ffd2u;
		}
		else {
			colour = army ? 0xffff0000u : 0xff00ff00u;
			if (!IsArmed(p_sprite)) {
				if (vid->m_sprClass != 3 && vid->m_sprClass != 24) {
					return;
				}
				size = 4;
			}
		}
	}
	else {
		colour = 0xffffff00u;
		if (vid->m_sprClass != 21) {
			if (vid->m_sprClass != 3 && vid->m_sprClass != 24) {
				return;
			}
			size = 4;
		}
	}
	p_canvas.Marker(p_sprite->m_x, (double) p_sprite->m_y - p_sprite->m_z, size, colour);
}

}

void DrawRtsMinimap(const SPRITE* p_frame)
{
	if (!p_frame || !p_frame->m_vid || (p_frame->m_flag & 0x10000) || !Map || !Graph ||
		(!Graph->m_color && !GPU_RENDER::Active()) || !(Map->m_w > 0) || !(Map->m_h > 0) || !std::isfinite(Map->m_w) ||
		!std::isfinite(Map->m_h)) {
		return;
	}
	const float bounds[] = {Graph->m_width,
							Graph->m_height,
							Graph->m_viewXMin,
							Graph->m_viewXMax,
							Graph->m_viewYMin,
							Graph->m_viewYMax,
							p_frame->m_x,
							p_frame->m_y,
							p_frame->m_z,
							Map->m_shiftX,
							Map->m_shiftY};
	for (float value : bounds) {
		if (!std::isfinite(value)) {
			return;
		}
	}
	if (Graph->m_width <= 0 || Graph->m_height <= 0 || (double) Graph->m_width >= std::numeric_limits<int>::max() ||
		(double) Graph->m_height >= std::numeric_limits<int>::max() || Graph->m_pitch < std::ceil(Graph->m_width)) {
		return;
	}
	const int w = p_frame->m_vid->m_unk0x2f6, h = p_frame->m_vid->m_messageLineHeight;
	const float scale = p_frame->UIDrawScale();
	if (w <= 14 || h <= 13 || !(scale > 0) || scale > 3) {
		return;
	}
	MINIMAP_CANVAS canvas = {p_frame->m_x - Map->m_shiftX - (w / 2 - 8) * scale,
							 p_frame->m_y - p_frame->m_z - Map->m_shiftY - (h / 2 - 10) * scale,
							 (w - 14) * scale,
							 (h - 13) * scale,
							 scale,
							 (int) Graph->m_width,
							 (int) Graph->m_height,
							 static_cast<unsigned int*>(Graph->m_color)};

	for (int i = 0; i < RailMap.m_list.m_n; ++i) {
		const R_DOT* dot = RailMap.m_list.m_data[i];
		if (dot) {
			canvas.Marker(dot->m_x, VIEWPORT_MATH::LegacyCoordinateDifference(dot->m_y, dot->m_z), 1, 0xff808080u);
		}
	}
	canvas.Viewport();
	if (Map->m_flag & 1) {
		for (int layer = 0; layer < 13; ++layer) {
			const SPRITE_LIST& sprites = Map->m_layers[layer];
			for (int i = sprites.m_n - 1; i >= 0; --i) {
				const SPRITE* sprite = sprites.m_data[i];
				if (sprite && sprite->m_vid && (sprite->m_vid->m_unk0x0c & 2) && (sprite->m_vid->m_flag & 0x68)) {
					canvas.Marker(sprite->m_x, (double) sprite->m_y - sprite->m_z, 1, 0xff808080u);
				}
			}
		}
	}
	if (Hash) {
		for (int i = Hash->m_list.m_n - 1; i >= 0; --i) {
			UnitMarker(canvas, Hash->m_list.m_data[i]);
		}
	}
}
