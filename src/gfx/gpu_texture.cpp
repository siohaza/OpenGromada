#include "gfx/gpu_texture.h"

#include "game/game_descriptor.h"
#include "gfx/gpu_backend.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "gfx/texture.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

uint32_t TEXTURE::GpuData()
{
	if (!GPU_RENDER::Active() || !m_data || m_width <= 0 || m_height <= 0) {
		return 0;
	}
	if (m_gpuGeneration != GPU_RENDER::Generation()) {
		if (m_gpuWritten) {
			GPU_RENDER::Fail("GPU texture contents lost during device recreation");
			return 0;
		}
		m_gpuData = 0;
		m_gpuCpuDirty = true;
		m_gpuGeneration = GPU_RENDER::Generation();
	}
	if (!m_gpuData) {
		m_gpuData = GPU_RENDER::Allocate(size_t(m_width) * m_height);
	}
	if (!m_gpuData || m_gpuWritten || !m_gpuCpuDirty) {
		return m_gpuData;
	}
	uint32_t raw = GPU_RENDER::Upload(this, m_data, size_t(m_pitch) * m_height);
	if (!raw) {
		return 0;
	}
	GPU_RENDER::Command command;
	command.op = GPU_TEXTURE::Expand;
	command.right = m_width;
	command.bottom = m_height;
	command.p[GPU_TEXTURE::Source] = raw;
	command.p[GPU_TEXTURE::Destination] = m_gpuData;
	command.p[GPU_TEXTURE::Pitch] = m_width;
	command.p[GPU_TEXTURE::Format] = m_format;
	command.p[GPU_TEXTURE::RawPitch] = m_pitch;
	GPU_RENDER::Submit(command, true);
	GPU_RENDER::Forget(this);
	m_gpuCpuDirty = false;
	return m_gpuData;
}

uint32_t TEXTURE::GpuPalette()
{
	return m_palette ? GPU_RENDER::Upload(&m_palette, m_palette, 1024) : 0;
}

void TEXTURE::GpuWritten()
{
	m_gpuWritten = true;
	m_gpuCpuDirty = false;
}

bool TEXTURE::GpuReadback()
{
	if (!m_gpuWritten) {
		return true;
	}
	if (m_gpuGeneration != GPU_RENDER::Generation() || !m_gpuData) {
		GPU_RENDER::Fail("GPU texture readback references a lost resource");
		return false;
	}
	std::vector<uint32_t> pixels(size_t(m_width) * m_height);
	if (!GPU_RENDER::Read(m_gpuData, pixels.data(), pixels.size() * sizeof(uint32_t))) {
		return false;
	}
	int bytes = BitsPerPixel() / 8;
	for (int y = 0; y < m_height; ++y) {
		for (int x = 0; x < m_width; ++x) {
			uint32_t value = pixels[size_t(y) * m_width + x];
			unsigned char* destination = static_cast<unsigned char*>(m_data) + size_t(y) * m_pitch + x * bytes;
			for (int i = 0; i < bytes; ++i) {
				destination[i] = static_cast<unsigned char>(value >> (i * 8));
			}
		}
	}
	return true;
}

namespace GPU_TEXTURE
{
static uint32_t s_predicate = 0;
uint32_t CurrentPredicate()
{
	return s_predicate;
}
void SetPredicate(uint32_t predicate)
{
	s_predicate = predicate;
}

uint32_t Visibility(int x, int y, int threshold, const int* child, uint32_t anchor)
{
	GPU_RENDER::Command command;
	command.op = Predicate;
	command.right = command.bottom = 1;
	command.p[Source] = GPU_RENDER::Depth();
	command.p[Destination] = GPU_RENDER::Allocate(1);
	command.p[Pitch] = GPU_RENDER::Width();
	command.p[SourceLeft] = x;
	command.p[SourceTop] = y;
	command.p[Diffuse] = threshold;
	command.p[DestinationLeft] = std::max(0, int(Graph->m_viewXMin));
	command.p[DestinationTop] = std::max(0, int(Graph->m_viewYMin));
	command.p[SourceWidth] = std::min(GPU_RENDER::Width(), int(Graph->m_viewXMax));
	command.p[SourceHeight] = std::min(GPU_RENDER::Height(), int(Graph->m_viewYMax));
	command.p[PredicateOffset] = anchor;
	if (child) {
		command.p[DepthTest] = 1;
		command.p[DestinationLeft] = std::max(int(command.p[DestinationLeft]), child[0]);
		command.p[DestinationTop] = std::max(int(command.p[DestinationTop]), child[1]);
		command.p[SourceWidth] = std::min(int(command.p[SourceWidth]), child[2]);
		command.p[SourceHeight] = std::min(int(command.p[SourceHeight]), child[3]);
	}
	GPU_RENDER::Submit(command, true);
	return command.p[Destination];
}

void Draw(TEXTURE* texture,
		  const int* destination,
		  const int* source,
		  const GAMMA* gamma,
		  bool depthTest,
		  double depthTop,
		  double depthBottom)
{
	GRAPH_CORE* graph = Graph;
	int width = destination[2] - destination[0], height = destination[3] - destination[1];
	int sourceWidth = source[2] - source[0], sourceHeight = source[3] - source[1];
	if (width <= 0 || height <= 0 || sourceWidth <= 0 || sourceHeight <= 0) {
		return;
	}
	GPU_RENDER::Command command;
	command.op = Quad;
	command.left = std::max({destination[0], int(graph->m_viewXMin), 0});
	command.top = std::max({destination[1], int(graph->m_viewYMin), 0});
	command.right = std::min({destination[2], int(graph->m_viewXMax), GPU_RENDER::Width()});
	command.bottom = std::min({destination[3], int(graph->m_viewYMax), GPU_RENDER::Height()});
	if (command.left >= command.right || command.top >= command.bottom) {
		return;
	}
	command.p[Source] = texture->GpuData();
	if (!command.p[Source]) {
		return;
	}
	command.p[Destination] = GPU_RENDER::Color();
	command.p[Depth] = GPU_RENDER::Depth();
	command.p[Pitch] = GPU_RENDER::Width();
	command.p[SourceWidth] = texture->m_width;
	command.p[SourceHeight] = texture->m_height;
	command.p[Format] = texture->m_format;
	command.p[Palette] = texture->GpuPalette();
	command.p[DestinationLeft] = destination[0];
	command.p[DestinationTop] = destination[1];
	command.p[DestinationWidth] = width;
	command.p[DestinationHeight] = height;
	command.p[SourceLeft] = source[0];
	command.p[SourceTop] = source[1];
	command.p[StepU] = uint32_t((int64_t(sourceWidth) << 16) / width);
	command.p[StepV] = uint32_t((int64_t(sourceHeight) << 16) / height);
	int filter = width > sourceWidth || height > sourceHeight ? graph->m_state.m_magFilter : graph->m_state.m_minFilter;
	command.p[Bilinear] = (width != sourceWidth || height != sourceHeight) && filter == D3DTEXF_LINEAR;
	command.p[Diffuse] = ~uint32_t(gamma->m_a);
	command.p[Specular] = gamma->m_b;
	command.p[ModulateShift] = graph->m_state.m_colorOp == D3DTOP_MODULATE2X ? 7 : 8;
	command.p[UseSpecular] = graph->m_state.m_specular;
	command.p[Blend] = graph->m_state.m_alphaBlend;
	command.p[SourceBlend] = graph->m_state.m_srcBlend;
	command.p[DestinationBlend] = graph->m_state.m_dstBlend;
	command.p[AlphaTest] = GameDesc->m_layerRules == GAME_LAYERS_ZS1 && graph->m_state.m_alphaTest &&
						   graph->m_state.m_alphaFunc == D3DCMP_GREATER;
	command.p[AlphaReference] = graph->m_state.m_alphaRef;
	command.p[DepthTest] = depthTest;
	command.p[DepthWrite] = graph->m_state.m_zWrite;
	command.p[PredicateOffset] = s_predicate;
	if (depthTest) {
		std::vector<uint32_t> rows(size_t(command.bottom - command.top));
		for (int y = command.top; y < command.bottom; ++y) {
			double depth = depthTop + (depthBottom - depthTop) * (double(y) - destination[1]) / height;
			rows[size_t(y - command.top)] = uint32_t(std::lround(std::clamp(depth, 0.0, 65535.0)));
		}
		command.p[DepthRows] = GPU_RENDER::Upload(&s_predicate, rows.data(), rows.size() * sizeof(uint32_t));
	}
	GPU_RENDER::Submit(command);
}

void CopyDepthToScreen(TEXTURE* texture, const int* destination, const int* source, bool scaled)
{
	GPU_RENDER::Command command;
	command.op = CopyDepth;
	command.left = std::max({destination[0], int(Graph->m_viewXMin), 0});
	command.top = std::max({destination[1], int(Graph->m_viewYMin), 0});
	command.right = std::min({destination[2], int(Graph->m_viewXMax), GPU_RENDER::Width()});
	command.bottom = std::min({destination[3], int(Graph->m_viewYMax), GPU_RENDER::Height()});
	if (command.left >= command.right || command.top >= command.bottom) {
		return;
	}
	command.p[Source] = texture->GpuData();
	command.p[Destination] = GPU_RENDER::Depth();
	command.p[Pitch] = GPU_RENDER::Width();
	command.p[SourceWidth] = texture->m_width;
	command.p[SourceHeight] = texture->m_height;
	command.p[DestinationLeft] = destination[0];
	command.p[DestinationTop] = destination[1];
	command.p[DestinationWidth] = destination[2] - destination[0];
	command.p[DestinationHeight] = destination[3] - destination[1];
	command.p[SourceLeft] = source[0];
	command.p[SourceTop] = source[1];
	command.p[SourceExtentWidth] = scaled ? source[2] - source[0] : command.p[DestinationWidth];
	command.p[SourceExtentHeight] = scaled ? source[3] - source[1] : command.p[DestinationHeight];
	command.p[PredicateOffset] = s_predicate;
	GPU_RENDER::Submit(command);
}
}
