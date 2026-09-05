#include "game/gametime.h"
#include "gfx/gpu_backend.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "gfx/picture.h"
#include "platform/timing.h"
#include "util/myerror.h"
#include "video/vid.h"

#include <algorithm>
#include <math.h>
#include <stdlib.h>
#include <vector>

// GLOBAL: ALIEN 0x4b2c68
static int s_loadFrame;

// GLOBAL: ALIEN 0x4b2c6c
static unsigned int s_loadTime;

// FUNCTION: ALIEN 0x430dd0
void GRAPH::DrawLoadBar(VID* p_vid)
{
	GRAPH_CORE* core = (GRAPH_CORE*) this;
	RealCurrentTime = Platform_Ticks();
	if (!p_vid) {
		return;
	}
	if (RealCurrentTime - s_loadTime <= (unsigned int) p_vid->m_defaultAniPeriod) {
		return;
	}
	core->PreTact();
	core->ClearScreen(COLOR(0, 0, 0));
	DrawVid(p_vid, s_loadFrame, core->m_width * 0.5f, core->m_height * 0.5f + 1.0f, 1.0f);
	DrawEffect(1);
	core->PutsXY(200.0f, core->GetViewYMin() + 5.0f, m_fontName, GRAPH_CORE::GREEN);
	core->PostTact(1);
	s_loadTime = RealCurrentTime;
	if (++s_loadFrame >= p_vid->m_dotFrameCount) {
		s_loadFrame = 0;
	}
}

// FUNCTION: ALIEN 0x430f10
void GRAPH::DrawDebugText(const char* p_format, ...)
{
	GRAPH_CORE* core = (GRAPH_CORE*) this;
	m_fontName = p_format;
	core->PreTact();
	core->Bar(200.0f, core->GetViewYMin() + 5.0f, 800.0f, core->GetViewYMin() + 30.0f, COLOR(0, 0, 0));
	core->PutsXY(200.0f, core->GetViewYMin() + 5.0f, m_fontName, GRAPH_CORE::GREEN);
	core->PostTact(1);
}

inline static COLOR ScreenPixel(GRAPH_CORE* p_core, float p_x, float p_y)
{
	if (!p_core->m_color || p_x < p_core->m_viewXMin || p_x >= p_core->m_viewXMax || p_y < p_core->m_viewYMin ||
		p_y >= p_core->m_viewYMax) {
		return COLOR((int) 0xff000000);
	}
	return COLOR((int) ((const unsigned int*) p_core->m_color)[(int) p_y * p_core->m_pitch + (int) p_x]);
}

// FUNCTION: ALIEN 0x430fc0
int GRAPH::ScreenShot(STRING* p_name, int p_x, int p_y, int p_w, int p_h)
{
	GRAPH_CORE* core = (GRAPH_CORE*) this;
	PICTURE picture(p_w, p_h, 1);
	if (GPU_RENDER::Active()) {
		const int left = (int) std::max<int64_t>(0, p_x);
		const int top = (int) std::max<int64_t>(0, p_y);
		const int right = (int) std::min<int64_t>(GPU_RENDER::Width(), (int64_t) p_x + p_w);
		const int bottom = (int) std::min<int64_t>(GPU_RENDER::Height(), (int64_t) p_y + p_h);
		const int width = std::max(0, right - left), height = std::max(0, bottom - top);
		std::vector<uint32_t> pixels(size_t(width) * height);
		if (width && height && !GPU_RENDER::ReadColor(left, top, width, height, pixels.data(), width)) {
			return 1;
		}
		for (int y = 0; y < p_h; ++y) {
			for (int x = 0; x < p_w; ++x) {
				const int64_t fx = (int64_t) p_x + x, fy = (int64_t) p_y + y;
				uint32_t color = 0xff000000u;
				if (fx >= left && fx < right && fy >= top && fy < bottom && fx >= core->m_viewXMin &&
					fx < core->m_viewXMax && fy >= core->m_viewYMin && fy < core->m_viewYMax) {
					color = pixels[size_t(fy - top) * width + size_t(fx - left)];
				}
				picture.PutPixel(x, y, COLOR((int) color));
			}
		}
		picture.m_impl->SaveTGA(*p_name, 0, 0, -1, -1);
		return 0;
	}
	for (int y = 0; y < p_h; ++y) {
		for (int x = 0; x < p_w; ++x) {
			float fy = (float) (y + p_y);
			float fx = (float) (x + p_x);
			picture.PutPixel(x, y, ScreenPixel(core, fx, fy));
		}
	}
	picture.m_impl->SaveTGA(*p_name, 0, 0, -1, -1);
	return 0;
}

// FUNCTION: ALIEN 0x433da0
void GRAPH::Box(float p_x, float p_y, float p_x1, float p_y1, COLOR p_color)
{
	Line(p_x, p_y, p_x1, p_y, p_color);
	Line(p_x, p_y1, p_x1, p_y1, p_color);
	Line(p_x, p_y, p_x, p_y1, p_color);
	Line(p_x1, p_y, p_x1, p_y1, p_color);
}

// FUNCTION: ALIEN 0x433e10
void GRAPH::Line(float p_x, float p_y, float p_x1, float p_y1, COLOR p_color)
{
	if ((float) fabs(p_x) > 10000.0f) {
		p_x = 0.0;
		if (::Error) {
			MYERROR::Error(::Error,
						   "GRAPH",
						   4,
						   // STRING: ALIEN 0x483f94
						   "x in Line",
						   0);
		}
	}
	if ((float) fabs(p_x1) > 10000.0f) {
		p_x1 = 0.0;
		if (::Error) {
			MYERROR::Error(::Error,
						   "GRAPH",
						   4,
						   // STRING: ALIEN 0x483f88
						   "x1 in Line",
						   0);
		}
	}
	if ((float) fabs(p_y) > 10000.0f) {
		p_y = 0.0;
		if (::Error) {
			MYERROR::Error(::Error,
						   "GRAPH",
						   4,
						   // STRING: ALIEN 0x483f7c
						   "y in Line",
						   0);
		}
	}
	if ((float) fabs(p_y1) > 10000.0f) {
		p_y1 = 0.0;
		if (::Error) {
			MYERROR::Error(::Error,
						   "GRAPH",
						   4,
						   // STRING: ALIEN 0x483f70
						   "y1 in Line",
						   0);
		}
	}
	int x = (int) p_x;
	int y = (int) p_y;
	int stepY = -1;
	int steep = 0;
	int stepX;
	if (p_x1 > p_x) {
		stepX = 1;
	}
	else {
		stepX = -1;
	}
	if (p_y1 > p_y) {
		stepY = 1;
	}
	unsigned int dx = abs((int) (p_x1 - p_x));
	unsigned int dy = abs((int) (p_y1 - p_y));
	if (dy > dx) {
		steep = 1;
		x ^= y;
		y ^= x;
		x ^= y;
		dx ^= dy;
		dy ^= dx;
		dx ^= dy;
		stepX ^= stepY;
		stepY ^= stepX;
		stepX ^= stepY;
	}
	int d = 2 * dy - dx;
	int incr = 2 * dy;
	for (unsigned int count = 0; count < dx; ++count) {
		if (steep) {
			PutPixel((float) y, (float) x, p_color);
		}
		else {
			PutPixel((float) x, (float) y, p_color);
		}
		while (d >= 0) {
			y += stepY;
			d -= 2 * dx;
		}
		x += stepX;
		d += incr;
	}
	PutPixel(p_x1, p_y1, p_color);
}
