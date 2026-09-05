#ifndef GPU_GRAPH_H
#define GPU_GRAPH_H

#include <cstdint>

class GRAPH_CORE;
class TEXTURE;

namespace GPU_GRAPH
{
void RawRect(int left, int top, int right, int bottom, uint32_t color);
void Fill(GRAPH_CORE* graph, int left, int top, int right, int bottom, uint32_t color, uint32_t specular);
void Triangle(GRAPH_CORE* graph,
			  const float* a,
			  const float* b,
			  const float* c,
			  uint32_t ca,
			  uint32_t cb,
			  uint32_t cc,
			  bool shadow);
void Primitive(GRAPH_CORE* graph, int type, unsigned fvf, const void* vertices, unsigned stride, int count);
void Snapshot(GRAPH_CORE* graph);
void AlphaAppear(GRAPH_CORE* graph, unsigned alpha);
void CopyDepth(TEXTURE* texture, int dx, int dy, int sx, int sy, int width, int height);
void LightMap(GRAPH_CORE* graph,
			  TEXTURE* texture,
			  float x,
			  float y,
			  int halfWidth,
			  int halfHeight,
			  int zHeight,
			  int zCenter,
			  int falloff);
bool FogMap(GRAPH_CORE* graph,
			TEXTURE* texture,
			int left,
			int top,
			int right,
			int bottom,
			const unsigned short* ramp,
			int zBase,
			int zFar);
bool SnowMap(GRAPH_CORE* graph, TEXTURE* texture, int xPhase, int yPhase, unsigned fade);
}

#endif
