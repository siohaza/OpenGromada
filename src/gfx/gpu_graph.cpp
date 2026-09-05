#include "gfx/gpu_graph.h"

#include "game/game_descriptor.h"
#include "gfx/gpu_backend.h"
#include "gfx/graph_core.h"
#include "gfx/texture.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <limits>

namespace GPU_GRAPH
{
namespace
{

GPU_RENDER::Command Rect(unsigned op, int left, int top, int right, int bottom)
{
	GPU_RENDER::Command command;
	command.op = op;
	command.left = left;
	command.top = top;
	command.right = right;
	command.bottom = bottom;
	return command;
}

bool Clip(GRAPH_CORE* graph, int& left, int& top, int& right, int& bottom, bool centers = false)
{
	if (!graph || !std::isfinite(graph->m_viewXMin) || !std::isfinite(graph->m_viewYMin) ||
		!std::isfinite(graph->m_viewXMax) || !std::isfinite(graph->m_viewYMax)) {
		return false;
	}
	const auto bound = [centers](float value, int extent) {
		value = std::clamp(value, 0.0f, (float) extent);
		return (int) (centers ? std::ceil(value) : value);
	};
	left = std::max(left, bound(graph->m_viewXMin, GPU_RENDER::Width()));
	top = std::max(top, bound(graph->m_viewYMin, GPU_RENDER::Height()));
	right = std::min(right, bound(graph->m_viewXMax, GPU_RENDER::Width()));
	bottom = std::min(bottom, bound(graph->m_viewYMax, GPU_RENDER::Height()));
	return left < right && top < bottom;
}

void State(GPU_RENDER::Command& command, const GRAPH_CORE* graph)
{
	command.p[40] = graph->m_state.m_alphaBlend;
	command.p[41] = graph->m_state.m_srcBlend;
	command.p[42] = graph->m_state.m_dstBlend;
	command.p[43] = graph->m_state.m_zWrite;
	command.p[44] = graph->m_state.m_zFunc;
	command.p[45] = graph->m_state.m_alphaTest;
	command.p[46] = graph->m_state.m_alphaFunc;
	command.p[47] = graph->m_state.m_alphaRef;
}

uint32_t Ramp(const unsigned short* ramp, size_t count)
{
	if (!ramp || !count || count > 65536) {
		return 0;
	}
	return GPU_RENDER::Upload(ramp, ramp, count * sizeof(*ramp));
}

}

void RawRect(int left, int top, int right, int bottom, uint32_t color)
{
	left = std::max(0, left);
	top = std::max(0, top);
	right = std::min(GPU_RENDER::Width(), right);
	bottom = std::min(GPU_RENDER::Height(), bottom);
	if (left >= right || top >= bottom) {
		return;
	}
	auto command = Rect(300, left, top, right, bottom);
	command.p[0] = color;
	GPU_RENDER::Submit(command);
}

void Fill(GRAPH_CORE* graph, int left, int top, int right, int bottom, uint32_t color, uint32_t specular)
{
	if (!Clip(graph, left, top, right, bottom)) {
		return;
	}
	auto command = Rect(301, left, top, right, bottom);
	command.p[0] = color;
	command.p[1] = specular;
	command.p[2] = graph->m_state.m_specular;
	State(command, graph);
	GPU_RENDER::Submit(command);
}

void Triangle(GRAPH_CORE* graph,
			  const float* a,
			  const float* b,
			  const float* c,
			  uint32_t ca,
			  uint32_t cb,
			  uint32_t cc,
			  bool shadow)
{
	for (const float* vertex : {a, b, c}) {
		for (int i = 0; i < 3; ++i) {
			if (!std::isfinite(vertex[i])) {
				return;
			}
		}
	}
	double area = (double(b[0]) - a[0]) * (double(c[1]) - a[1]) - (double(b[1]) - a[1]) * (double(c[0]) - a[0]);
	if (area == 0 || (graph->m_state.m_cull == D3DCULL_CW && area > 0) ||
		(graph->m_state.m_cull == D3DCULL_CCW && area < 0)) {
		return;
	}
	if (area < 0) {
		std::swap(b, c);
		std::swap(cb, cc);
	}
	int left = (int) std::clamp(std::ceil((double) std::min({a[0], b[0], c[0]})), 0.0, double(GPU_RENDER::Width()));
	int top = (int) std::clamp(std::ceil((double) std::min({a[1], b[1], c[1]})), 0.0, double(GPU_RENDER::Height()));
	int right = (int) std::clamp(std::ceil((double) std::max({a[0], b[0], c[0]})), 0.0, double(GPU_RENDER::Width()));
	int bottom = (int) std::clamp(std::ceil((double) std::max({a[1], b[1], c[1]})), 0.0, double(GPU_RENDER::Height()));
	if (!Clip(graph, left, top, right, bottom, true)) {
		return;
	}
	auto command = Rect(302, left, top, right, bottom);
	for (int i = 0; i < 3; ++i) {
		command.p[i] = std::bit_cast<uint32_t>(a[i]);
		command.p[3 + i] = std::bit_cast<uint32_t>(b[i]);
		command.p[6 + i] = std::bit_cast<uint32_t>(c[i]);
	}
	command.p[9] = ca;
	command.p[10] = cb;
	command.p[11] = cc;
	command.p[12] = shadow;
	const auto pair = [&](unsigned index, double value) {
		const float high = (float) value;
		command.p[index] = std::bit_cast<uint32_t>(high);
		command.p[index + 1] = std::bit_cast<uint32_t>((float) (value - high));
	};
	const float* edgeVertices[] = {a, b, c, a};
	for (unsigned edge = 0; edge < 3; ++edge) {
		const float* start = edgeVertices[edge];
		const float* end = edgeVertices[edge + 1];
		const double dx = double(end[0]) - start[0], dy = double(end[1]) - start[1];
		pair(13 + edge * 6, dx * (top - double(start[1])) - dy * (left - double(start[0])));
		pair(15 + edge * 6, -dy);
		pair(17 + edge * 6, dx);
	}
	pair(31, std::abs(area));
	State(command, graph);
	if (shadow) {
		command.p[45] = 0;
		command.p[44] = GameDesc->m_layerRules == GAME_LAYERS_LOCOLAND && graph->m_state.m_zFunc == D3DCMP_GREATEREQUAL
							? D3DCMP_GREATEREQUAL
							: D3DCMP_ALWAYS;
		if (command.p[44] == D3DCMP_ALWAYS) {
			command.p[43] = 0;
		}
	}
	GPU_RENDER::Submit(command);
}

void Primitive(GRAPH_CORE* graph, int type, unsigned fvf, const void* vertices, unsigned stride, int count)
{
	if (!vertices || count <= 0 || count > 1000000 || (stride != 20 && stride != 24) || (fvf != 0x44 && fvf != 0xc4)) {
		return;
	}
	const auto* bytes = static_cast<const unsigned char*>(vertices);
	const auto vertex = [&](int index, float* position, uint32_t& color) {
		std::memcpy(position, bytes + size_t(index) * stride, 3 * sizeof(float));
		std::memcpy(&color, bytes + size_t(index) * stride + 16, sizeof(color));
	};
	if (type == D3DPT_TRIANGLESTRIP || type == D3DPT_TRIANGLELIST || type == D3DPT_TRIANGLEFAN) {
		for (int i = 0; i + 2 < count; i += type == D3DPT_TRIANGLELIST ? 3 : 1) {
			int ia = type == D3DPT_TRIANGLEFAN ? 0 : i;
			int ib = i + 1, ic = i + 2;
			if (type == D3DPT_TRIANGLESTRIP && (i & 1)) {
				std::swap(ib, ic);
			}
			float a[3], b[3], c[3];
			uint32_t ca, cb, cc;
			vertex(ia, a, ca);
			vertex(ib, b, cb);
			vertex(ic, c, cc);
			if (fvf == 0xc4) {
				std::memcpy(&ca, bytes + 16, sizeof(ca));
			}
			Triangle(graph, a, b, c, ca, fvf == 0xc4 ? ca : cb, fvf == 0xc4 ? ca : cc, fvf == 0xc4);
		}
		return;
	}
	if (type != D3DPT_LINELIST && type != D3DPT_LINESTRIP && type != D3DPT_POINTLIST) {
		return;
	}
	for (int i = 0; i < count - (type != D3DPT_POINTLIST); i += type == D3DPT_LINELIST ? 2 : 1) {
		float a[3], b[3];
		uint32_t ca, cb;
		vertex(i, a, ca);
		vertex(type == D3DPT_POINTLIST ? i : i + 1, b, cb);
		bool valid = true;
		for (float value : {a[0], a[1], a[2], b[0], b[1], b[2], b[0] - a[0], b[1] - a[1], b[2] - a[2]}) {
			valid &= std::isfinite(value);
		}
		if (!valid) {
			continue;
		}
		int left = (int) std::clamp(std::floor((double) std::min(a[0], b[0])), 0.0, double(GPU_RENDER::Width()));
		int top = (int) std::clamp(std::floor((double) std::min(a[1], b[1])), 0.0, double(GPU_RENDER::Height()));
		int right = (int) std::clamp(std::ceil((double) std::max(a[0], b[0])) + 1, 0.0, double(GPU_RENDER::Width()));
		int bottom = (int) std::clamp(std::ceil((double) std::max(a[1], b[1])) + 1, 0.0, double(GPU_RENDER::Height()));
		if (!Clip(graph, left, top, right, bottom)) {
			continue;
		}
		auto command = Rect(303, left, top, right, bottom);
		for (int j = 0; j < 3; ++j) {
			command.p[j] = std::bit_cast<uint32_t>(a[j]);
			command.p[3 + j] = std::bit_cast<uint32_t>(b[j]);
		}
		command.p[6] = ca;
		command.p[7] = cb;
		const float extent = std::max(std::abs(b[0] - a[0]), std::abs(b[1] - a[1]));
		command.p[8] =
			std::bit_cast<uint32_t>(extent == 0 ? 1.0f : std::scalbn(1.0f, -std::clamp(std::ilogb(extent), -100, 100)));
		State(command, graph);
		GPU_RENDER::Submit(command);
	}
}

void Snapshot(GRAPH_CORE* graph)
{
	const size_t words = size_t(GPU_RENDER::Width()) * GPU_RENDER::Height();
	if (graph->m_gpuScreen) {
		GPU_RENDER::Release(graph->m_gpuScreen);
	}
	graph->m_gpuScreen = GPU_RENDER::Allocate(words);
	if (!graph->m_gpuScreen) {
		graph->m_effectStart[5] = 0;
		return;
	}
	auto command = Rect(304, 0, 0, GPU_RENDER::Width(), GPU_RENDER::Height());
	command.p[0] = graph->m_gpuScreen;
	GPU_RENDER::Submit(command, true);
}

void AlphaAppear(GRAPH_CORE* graph, unsigned alpha)
{
	if (!graph->m_gpuScreen) {
		return;
	}
	int left = 0, top = 0, right = GPU_RENDER::Width(), bottom = GPU_RENDER::Height();
	if (!Clip(graph, left, top, right, bottom)) {
		return;
	}
	auto command = Rect(305, left, top, right, bottom);
	command.p[0] = graph->m_gpuScreen;
	command.p[1] = std::min(255u, alpha);
	GPU_RENDER::Submit(command);
}

void CopyDepth(TEXTURE* texture, int dx, int dy, int sx, int sy, int width, int height)
{
	const uint32_t data = texture->GpuData();
	if (!data) {
		return;
	}
	auto command = Rect(306, dx, dy, dx + width, dy + height);
	command.p[0] = data;
	command.p[1] = texture->m_width;
	command.p[2] = sx;
	command.p[3] = sy;
	GPU_RENDER::Submit(command);
}

void LightMap(GRAPH_CORE* graph,
			  TEXTURE* texture,
			  float x,
			  float y,
			  int halfWidth,
			  int halfHeight,
			  int zHeight,
			  int zCenter,
			  int falloff)
{
	if (!texture || halfWidth <= 0 || halfHeight <= 0 || texture->m_width < halfWidth / 2 ||
		texture->m_height < halfHeight / 2) {
		return;
	}
	const uint32_t data = texture->GpuData(), ramp = Ramp(graph->m_snowRamp, 256);
	if (!data || !ramp) {
		return;
	}
	auto command = Rect(307, 0, 0, halfWidth / 2, halfHeight / 2);
	command.p[0] = data;
	command.p[1] = texture->m_width;
	command.p[2] = texture->m_format == D3DFMT_P8;
	command.p[3] = ramp;
	command.p[4] = std::bit_cast<uint32_t>(x);
	command.p[5] = std::bit_cast<uint32_t>(y);
	command.p[6] = halfWidth;
	command.p[7] = halfHeight;
	command.p[8] = zHeight;
	command.p[9] = zCenter;
	command.p[10] = falloff;
	command.p[11] = std::bit_cast<uint32_t>(graph->m_viewXMin);
	command.p[12] = std::bit_cast<uint32_t>(graph->m_viewYMin);
	command.p[13] = std::bit_cast<uint32_t>(graph->m_viewXMax);
	command.p[14] = std::bit_cast<uint32_t>(graph->m_viewYMax);
	GPU_RENDER::Submit(command, true);
	texture->GpuWritten();
}

bool FogMap(GRAPH_CORE* graph,
			TEXTURE* texture,
			int left,
			int top,
			int right,
			int bottom,
			const unsigned short* ramp,
			int zBase,
			int zFar)
{
	(void) graph;
	const int width = (right - left + 3) / 4, height = (bottom - top + 3) / 4;
	if (!texture || width <= 0 || height <= 0 || width > texture->m_width || height > texture->m_height ||
		zBase < zFar || (int64_t) zBase - zFar >= 65536) {
		return false;
	}
	const uint32_t target = texture->GpuData(), table = Ramp(ramp, size_t((int64_t) zBase - zFar + 1));
	const uint32_t count = uint32_t(width) * uint32_t(height);
	const uint32_t first = GPU_RENDER::Allocate(count), second = GPU_RENDER::Allocate(count);
	if (!target || !table || !first || !second) {
		GPU_RENDER::Release(first);
		GPU_RENDER::Release(second);
		return false;
	}
	auto command = Rect(308, 0, 0, width, height);
	command.p[0] = first;
	command.p[1] = width;
	command.p[2] = texture->m_format == D3DFMT_P8;
	command.p[3] = table;
	command.p[4] = left;
	command.p[5] = top;
	command.p[6] = right;
	command.p[7] = zBase;
	command.p[8] = zFar;
	GPU_RENDER::Submit(command, true);
	uint32_t source = first, destination = second;
	for (uint32_t step = 1; step < count; step <<= 1) {
		command.op = 309;
		command.p[0] = source;
		command.p[2] = destination;
		command.p[3] = step;
		GPU_RENDER::Submit(command, true);
		std::swap(source, destination);
	}
	command.op = 310;
	command.p[0] = source;
	command.p[2] = target;
	command.p[3] = texture->m_width;
	GPU_RENDER::Submit(command, true);
	GPU_RENDER::Release(first);
	GPU_RENDER::Release(second);
	texture->GpuWritten();
	return true;
}

bool SnowMap(GRAPH_CORE* graph, TEXTURE* texture, int xPhase, int yPhase, unsigned fade)
{
	const int width = (GPU_RENDER::Width() + 3) / 4, height = (GPU_RENDER::Height() + 3) / 4;
	if (!texture || width > texture->m_width || height > texture->m_height) {
		return false;
	}
	const uint32_t data = texture->GpuData(), ramp = Ramp(graph->m_snowRamp, 256);
	if (!data || !ramp) {
		return false;
	}
	auto command = Rect(311, 0, 0, width, height);
	command.p[0] = data;
	command.p[1] = texture->m_width;
	command.p[2] = texture->m_format == D3DFMT_P8;
	command.p[3] = ramp;
	command.p[4] = xPhase;
	command.p[5] = yPhase;
	command.p[6] = fade;
	GPU_RENDER::Submit(command, true);
	texture->GpuWritten();
	return true;
}

}
