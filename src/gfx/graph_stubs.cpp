
#include "game/game_descriptor.h"
#include "game/gametime.h"
#include "game/map.h"
#include "game/viewport_math.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "gfx/picture.h"
#include "gfx/render_math.h"
#include "gfx/texture.h"
#include "gfx/weather_raster.h"
#include "sprite/sprite.h"
#include "ui/mouse.h"
#include "util/game_random.h"
#include "util/myerror.h"
#include "util/packed.h"
#include "util/string.h"
#include "video/vid.h"

#include <bit>
#include <math.h>
#include <stdlib.h>

// GLOBAL: ALIEN 0x4b2c58
static float s_oldShiftX;

// GLOBAL: ALIEN 0x492b78
static float s_oldShiftY;

static void DrawLocolandLayers(GRAPH* p_graph)
{



	p_graph->SetRenderState(D3DRS_ZFUNC, 8);
	p_graph->SetRenderState(D3DRS_ZWRITEENABLE, 1);
	for (int layer = 0; layer <= 4; ++layer) {
		Map->DrawLayer(layer);
	}
	p_graph->SetRenderState(D3DRS_ZFUNC, 7);
	Map->DrawLayer(5);
	p_graph->SetRenderState(D3DRS_ZWRITEENABLE, 0);
	p_graph->SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
	Map->DrawLayer(6);
	Map->DrawLayer(7);

	if ((p_graph->m_env & 3) && !(p_graph->m_flags & 0x20)) {
		const float width = p_graph->m_viewXMax - p_graph->m_viewXMin;
		const float height = p_graph->m_viewYMax - p_graph->m_viewYMin;
		const int cx =
			VIEWPORT_MATH::LegacyCoordinate(((double) p_graph->m_viewXMin + p_graph->m_viewXMax) * 0.5 + Map->m_shiftX);
		const int cy =
			VIEWPORT_MATH::LegacyCoordinate(((double) p_graph->m_viewYMin + p_graph->m_viewYMax) * 0.5 + Map->m_shiftY);


		const int halfWidth = (int) ceil(width * 0.5) + 192 > 512 ? (int) ceil(width * 0.5) + 192 : 512;
		const int halfHeight = (int) ceil(height * 0.5) + 272 > 512 ? (int) ceil(height * 0.5) + 272 : 512;
		for (int layer = 2; layer <= 3; ++layer) {
			int iter;
			for (SPRITE* sprite = Map->FirstSprite(layer, &iter); sprite; sprite = Map->NextSprite(layer, &iter)) {
				const int x = VIEWPORT_MATH::LegacyCoordinate(sprite->m_x);
				const int y = VIEWPORT_MATH::LegacyCoordinate(sprite->m_y);
				const int z = VIEWPORT_MATH::LegacyCoordinate(sprite->m_z);
				const int dx = VIEWPORT_MATH::LegacyCoordinateDifference(x, cx);
				const int dy =
					VIEWPORT_MATH::LegacyCoordinateDifference(VIEWPORT_MATH::LegacyCoordinateDifference(y, z), cy);
				if ((dx >= -halfWidth && dx < halfWidth && dy >= -halfHeight && dy < halfHeight) ||
					VIEWPORT_MATH::LegacyCoordinateDifference(y, cy) >= halfHeight) {
					sprite->m_vid->DrawShadow(sprite);
				}
			}
		}
	}
	p_graph->SetTextureStageState(D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
	Map->DrawLayer(8);
	p_graph->DrawSnow();
	p_graph->DrawSnowflakes();
	p_graph->SetTextureStageState(D3DTSS_MAGFILTER, D3DTEXF_POINT);
	Map->DrawLayer(9);
	p_graph->DrawRain();
	p_graph->SetRenderState(D3DRS_ZWRITEENABLE, 1);
	Map->DrawLayer(10);
}

// FUNCTION: ALIEN 0x430870
void GRAPH::Tact(int p_draw)
{
	GRAPH_CORE* core = (GRAPH_CORE*) this;
	if ((m_env & 4) && !(Map->m_flag & 0x10)) {
		s_oldShiftX = Map->m_shiftX;
		s_oldShiftY = Map->m_shiftY;
		float height = Graph->m_height;
		int jy = GameRand() % 9;
		float width = core->m_width;
		int jx = GameRand() % 9;
		Map->SetShiftCoor(width * 0.5f + s_oldShiftX + 4.0f - jx, height * 0.5f + s_oldShiftY + 4.0f - jy, 0);
	}
	if (p_draw) {
		const bool extendedLayers = GameDesc->m_layerRules == GAME_LAYERS_ZS1;


		if (extendedLayers || Map->m_menuFrameActive || Map->m_noVid <= 1024 || !Map->m_vids[1024] ||
			Map->m_shiftX < 0.0f || core->m_viewXMax + Map->m_shiftX - core->m_viewXMin > (double) Map->m_w ||
			Map->m_shiftY < 0.0f || core->m_viewYMax + Map->m_shiftY - core->m_viewYMin > (double) Map->m_h) {
			core->ClearScreen(COLOR(0, 0, 0));
		}

		core->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
		if (extendedLayers) {
			core->SetRenderState(D3DRS_ZFUNC, 8);
			core->SetRenderState(D3DRS_ZWRITEENABLE, 1);
		}
		core->SetTextureStageState(D3DTSS_MAGFILTER, D3DTEXF_POINT);
		if (GameDesc->m_layerRules == GAME_LAYERS_LOCOLAND) {
			DrawLocolandLayers(this);
		}
		else {
			Map->DrawLayer(0);
			Map->DrawLayer(1);
			Map->DrawLayer(2);
			Map->DrawLayer(3);
			Map->DrawLayer(4);
			Map->DrawLayer(5);
			Map->DrawLayer(6);
			Map->DrawLayer(7);
			if (extendedLayers) {
				core->SetRenderState(D3DRS_ZFUNC, 7);
			}
			Map->DrawLayer(8);
			if (extendedLayers) {
				core->SetRenderState(D3DRS_ZWRITEENABLE, 0);
			}
			core->SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
			Map->DrawLayer(9);
			Map->DrawLayer(10);
			core->SetTextureStageState(D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
			Map->DrawLayer(11);
			DrawSnow();
			DrawSnowflakes();
			core->SetTextureStageState(D3DTSS_MAGFILTER, D3DTEXF_POINT);
			Map->DrawLayer(12);
			DrawRain();
			if (extendedLayers) {
				Map->DrawLayer(14);
				Map->DrawLayer(13);
				Map->DrawLayer(15);
				core->SetRenderState(D3DRS_ZWRITEENABLE, 1);
				Map->DrawLayer(16);
				Map->DrawLayer(17);
				Map->DrawLayer(18);
			}
			else {
				Map->DrawLayer(13);
				Map->DrawLayer(14);
			}
			MOUSE* child = Mouse;
			if (Mouse && !Mouse->m_unk0x70 && (Mouse->m_vid->m_flag & 0x8000)) {
				do {
					if (!(child->m_flag & 0x10000)) {
						child->Draw();
					}
					child = (MOUSE*) child->m_child;
				} while (child);
			}
		}
	}
	DrawSquall();
	if ((m_env & 4) && !(Map->m_flag & 0x10)) {
		Map->SetShiftCoor(core->m_width * 0.5f + s_oldShiftX, Graph->m_height * 0.5f + s_oldShiftY, 0);
	}
	core->SetRenderState(D3DRS_SPECULARENABLE, 0);
	DrawEffect(p_draw);
}

inline static GAMMA InterpolateGamma(const GAMMA& p_cur, const GAMMA& p_tgt, double p_t)
{
	int base;
	int want;
	int cur;
	base = (p_cur.m_a & 0xff000000) ? -(int) ((unsigned int) p_cur.m_a >> 24) : (int) ((unsigned int) p_cur.m_b >> 24);
	want = ((unsigned int) p_tgt.m_a & 0xff000000) ? -(int) ((unsigned int) p_tgt.m_a >> 24)
												   : (int) ((unsigned int) p_tgt.m_b >> 24);
	cur = (p_cur.m_a & 0xff000000) ? -(int) ((unsigned int) p_cur.m_a >> 24) : (int) ((unsigned int) p_cur.m_b >> 24);
	int a = (int) ((want - cur) * p_t + base);
	base = (p_cur.m_a & 0xff0000) ? -(int) ((const unsigned char*) &p_cur.m_a)[2]
								  : (int) ((const unsigned char*) &p_cur.m_b)[2];
	want = (p_tgt.m_a & 0xff0000) ? -(int) (unsigned char) ((unsigned int) p_tgt.m_a >> 16)
								  : (int) (unsigned char) ((unsigned int) p_tgt.m_b >> 16);
	cur = (p_cur.m_a & 0xff0000) ? -(int) ((const unsigned char*) &p_cur.m_a)[2]
								 : (int) ((const unsigned char*) &p_cur.m_b)[2];
	int r = (int) ((want - cur) * p_t + base);
	base = (p_cur.m_a & 0xff00) ? -(int) ((const unsigned char*) &p_cur.m_a)[1]
								: (int) ((const unsigned char*) &p_cur.m_b)[1];
	want = ((unsigned char) ((unsigned int) p_tgt.m_a >> 8)) ? -(int) (unsigned char) ((unsigned int) p_tgt.m_a >> 8)
															 : (int) (unsigned char) ((unsigned int) p_tgt.m_b >> 8);
	cur = (p_cur.m_a & 0xff00) ? -(int) ((const unsigned char*) &p_cur.m_a)[1]
							   : (int) ((const unsigned char*) &p_cur.m_b)[1];
	int g = (int) ((want - cur) * p_t + base);
	base = ((unsigned char) p_cur.m_a) ? -(int) (unsigned char) p_cur.m_a : (int) (unsigned char) p_cur.m_b;
	want = ((unsigned char) p_tgt.m_a) ? -(int) (unsigned char) p_tgt.m_a : (int) (unsigned char) p_tgt.m_b;
	cur = ((unsigned char) p_cur.m_a) ? -(int) (unsigned char) p_cur.m_a : (int) (unsigned char) p_cur.m_b;
	int b = (int) ((want - cur) * p_t + base);
	return GAMMA(a, r, g, b);
}

inline static unsigned int QuantizeAlphaAppearSource(unsigned int p_color)
{
	unsigned int r5 = (p_color >> 19) & 0x1f;
	unsigned int g6 = (p_color >> 10) & 0x3f;
	unsigned int b5 = (p_color >> 3) & 0x1f;
	return 0xff000000u | (((r5 << 3) | (r5 >> 2)) << 16) | (((g6 << 2) | (g6 >> 4)) << 8) | ((b5 << 3) | (b5 >> 2));
}

static void BlendAlphaAppear(GRAPH_CORE* p_core, unsigned int p_alpha)
{
	if (!p_core || !p_core->m_screen || !p_core->m_color) {
		return;
	}

	int frameWidth = (int) p_core->m_width;
	int frameHeight = (int) p_core->m_height;
	if (frameWidth <= 0 || frameHeight <= 0 || p_core->m_pitch < frameWidth) {
		return;
	}
	int left = (int) p_core->m_viewXMin;
	int top = (int) p_core->m_viewYMin;
	int right = (int) p_core->m_viewXMax;
	int bottom = (int) p_core->m_viewYMax;
	if (left < 0) {
		left = 0;
	}
	if (top < 0) {
		top = 0;
	}
	if (right > frameWidth) {
		right = frameWidth;
	}
	if (bottom > frameHeight) {
		bottom = frameHeight;
	}
	int width = right - left;
	int height = bottom - top;
	if (width <= 0 || height <= 0) {
		return;
	}

	const unsigned int inverse = 255 - p_alpha;
	const unsigned int* saved =
		(const unsigned int*) p_core->m_screen + (size_t) top * (size_t) p_core->m_pitch + (size_t) left;
	unsigned int* current = (unsigned int*) p_core->m_color + (size_t) top * (size_t) p_core->m_pitch + (size_t) left;
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			unsigned int oldColor = QuantizeAlphaAppearSource(saved[x]);
			unsigned int newColor = current[x];
			unsigned int r = ((((oldColor >> 16) & 0xff) * inverse) + (((newColor >> 16) & 0xff) * p_alpha)) / 255;
			unsigned int g = ((((oldColor >> 8) & 0xff) * inverse) + (((newColor >> 8) & 0xff) * p_alpha)) / 255;
			unsigned int b = (((oldColor & 0xff) * inverse) + ((newColor & 0xff) * p_alpha)) / 255;
			current[x] = 0xff000000u | (r << 16) | (g << 8) | b;
		}
		saved += p_core->m_pitch;
		current += p_core->m_pitch;
	}
}

// FUNCTION: ALIEN 0x4311a0
void GRAPH::DrawEffect(int p_draw)
{
	GRAPH_CORE* core = (GRAPH_CORE*) this;
	unsigned int t;
	if (core->m_effectStart[5]) {
		unsigned int elapsed = RealCurrentTime - core->m_effectStart[5];
		if (core->m_effectDuration[5] > 0 && elapsed < (unsigned int) core->m_effectDuration[5]) {
			core->SetAlphaBlend(6, 5);
			if (p_draw) {
				unsigned int alpha = (unsigned int) ((256ull * elapsed) / (unsigned int) core->m_effectDuration[5]);
				core->SetRenderState(D3DRS_SPECULARENABLE, 1);
				BlendAlphaAppear(core, alpha);
			}
			t = RealCurrentTime;
		}
		else {
			core->m_effectStart[5] = 0;
			t = RealCurrentTime;
		}
	}
	else {
		t = RealCurrentTime;
	}
	if (core->m_effectStart[2]) {
		unsigned int elapsed = t - core->m_effectStart[2];
		unsigned int dur = core->m_effectDuration[2];
		if (elapsed > dur) {
			Map->SetShiftCoor((float) core->m_effectA[2], (float) core->m_effectB[2], 0);
			core->m_effectStart[2] = 0;
		}
		else {
			float fe = (float) (int) elapsed;
			float fd = (float) (int) dur;
			float sy = ((float) core->m_effectB[2] - core->m_shiftBaseY) * fe / fd + core->m_shiftBaseY;
			float sx = ((float) core->m_effectA[2] - core->m_shiftBaseX) * fe / fd + core->m_shiftBaseX;
			Map->SetShiftCoor(sx, sy, 0);
		}
		t = RealCurrentTime;
	}
	if (core->m_effectStart[1]) {
		unsigned int start = core->m_effectStart[1];
		unsigned int dur = core->m_effectDuration[1];
		unsigned int elapsed = t - start;
		if (elapsed >= dur) {
			core->m_effectStart[1] = 0;
			t = RealCurrentTime;
		}
		else if (p_draw) {
			unsigned int num;
			if (elapsed < dur / 9) {
				num = 2304 * t - 2304 * start;
			}
			else {
				num = (start + dur - t) << 8;
				dur -= dur / 9;
			}
			unsigned int scale = num / dur;


			unsigned int sourceColor = (unsigned int) core->m_effectA[1];
			unsigned int color = ((((sourceColor & 0xff00u) * scale) >> 8) & 0xff00u) +
						(((sourceColor & 0xffu) * scale) >> 8) + (((sourceColor * scale) >> 8) & 0xff0000u);
			LightBar(0, 0, core->m_width, core->m_height, color);
			LightBar(0, 0, core->m_width, core->m_height, color);
			LightBar(0, 0, core->m_width, core->m_height, color);
			t = RealCurrentTime;
		}
	}
	if (core->m_effectStart[3]) {
		unsigned int start = core->m_effectStart[3];
		unsigned int dur = core->m_effectDuration[3];
		unsigned int elapsed = t - start;
		if (elapsed >= dur) {
			core->m_effectStart[3] = 0;
			t = RealCurrentTime;
		}
		else if (p_draw) {
			unsigned int q = 4 * dur;
			unsigned int a;
			if (elapsed < 4 * dur / 9) {
				a = (2304 * t - 2304 * start) / q;
			}
			else if (elapsed <= 5 * dur / 9) {
				a = 255;
			}
			else {
				a = (2304 * (dur + start) - 2304 * t) / q;
			}
			ShadowBar(0, 0, core->m_width, core->m_height, a | ((a | (a << 8)) << 8));
			t = RealCurrentTime;
		}
	}
	if (core->m_effectStart[9]) {
		unsigned int start = core->m_effectStart[9];
		unsigned int dur = core->m_effectDuration[9];
		unsigned int elapsed = t - start;
		if (elapsed >= dur) {
			core->m_effectStart[9] = 0;
			t = RealCurrentTime;
		}
		else if (p_draw) {
			unsigned int a;
			if (elapsed < 4 * dur / 5) {
				a = (1280 * t - 1280 * start) / (4 * dur);
			}
			else {
				a = 255;
			}
			ShadowBar(0, 0, core->m_width, core->m_height, a | ((a | (a << 8)) << 8));
			t = RealCurrentTime;
		}
	}
	if (core->m_effectStart[10]) {
		unsigned int start = core->m_effectStart[10];
		unsigned int dur = core->m_effectDuration[10];
		if (t - start >= dur) {
			core->m_effectStart[10] = 0;
			t = RealCurrentTime;
		}
		else if (p_draw) {
			unsigned int a = ((start + dur - t) << 8) / dur;
			ShadowBar(0, 0, core->m_width, core->m_height, a | ((a | (a << 8)) << 8));
			t = RealCurrentTime;
		}
	}
	if (core->m_effectStart[11]) {
		unsigned int start = core->m_effectStart[11];
		unsigned int dur = core->m_effectDuration[11];
		unsigned int elapsed = t - start;
		if (elapsed >= dur) {
			SetGamma(GAMMA(GAMMA::DECODE, (unsigned int) core->m_effectA[11]));
			core->m_effectStart[11] = 0;
		}
		else if (p_draw) {
			double frac = (double) elapsed / (int) dur;
			GAMMA tgt(GAMMA::DECODE, (unsigned int) core->m_effectA[11]);
			SetGamma(GAMMA(GAMMA::RAW_COPY, InterpolateGamma(core->m_gammaCur, tgt, frac)));
		}
	}
}

// FUNCTION: ALIEN 0x431c50
void GRAPH::SetGamma(const GAMMA& p_gamma)
{
	if (m_gammaSet.m_a != p_gamma.m_a || m_gammaSet.m_b != p_gamma.m_b) {
		m_gammaSet.m_a = p_gamma.m_a;
		m_gammaSet.m_b = p_gamma.m_b;
		for (int i = 0; i < Map->m_noVid && i < MAP::MAX_VIDS; ++i) {
			VID* vid = Map->GetVid(i);
			if (vid != EmptyVid) {
				vid = Map->GetVid(i);
				vid->SetGamma(p_gamma, 4);
			}
		}
	}
}

// GLOBAL: ALIEN 0x4b2c70
static unsigned int g_startSquall;

// GLOBAL: ALIEN 0x483d38
static float g_oldWindSpeed = -1.0f;

// FUNCTION: ALIEN 0x431d50
void GRAPH::DrawSquall()
{
	if (Map->m_flag & 0x10) {
		return;
	}
	if (m_env & 0x80) {
		if (g_oldWindSpeed == -1.0f) {
			g_oldWindSpeed = m_windForce;
		}
		unsigned int elapsed = CurrentTime - g_startSquall;
		if (elapsed <= 0x800) {
			float ramp = elapsed * g_oldWindSpeed;
			m_windForce = ramp * 0.001953125f + g_oldWindSpeed;
			return;
		}
		if (elapsed <= 0x1000) {
			float ramp = (g_startSquall - CurrentTime + 0x1000) * g_oldWindSpeed;
			m_windForce = ramp * 0.001953125f + g_oldWindSpeed;
			return;
		}
		m_env &= ~0x80u;
		m_windForce = g_oldWindSpeed;
	}
	else {
		g_startSquall = CurrentTime;
		if (g_oldWindSpeed != -1.0f) {
			m_windForce = g_oldWindSpeed;
		}
	}
	g_oldWindSpeed = -1.0f;
}

// GLOBAL: ALIEN 0x4b2c74
static unsigned int s_snowStart;

// GLOBAL: ALIEN 0x4b2c78
static unsigned int s_snowFade;

// FUNCTION: ALIEN 0x431e40
void GRAPH::DrawFog(
	float p_x0,
	float p_y0,
	float p_x1,
	float p_y1,
	int p_zTop,
	int p_zBottom,
	COLOR p_color,
	const unsigned short* p_ramp,
	int p_zBase,
	int p_blend
)
{
	GRAPH_CORE* core = (GRAPH_CORE*) this;
	int x0 = (int) p_x0;
	int y0 = (int) p_y0;
	int x1 = (int) p_x1;
	int y1 = (int) p_y1;
	if ((core->m_flags & 0x20) || !p_ramp || p_x1 < core->m_viewXMin || p_x0 >= core->m_viewXMax ||
		p_y1 < core->m_viewYMin || p_y0 >= core->m_viewYMax) {
		return;
	}

	if (p_x0 < core->m_viewXMin) {
		x0 = (int) core->m_viewXMin;
	}
	if (p_y0 < core->m_viewYMin) {
		y0 = (int) core->m_viewYMin;
	}
	if (p_x1 >= core->m_viewXMax) {
		x1 = (int) core->m_viewXMax;
	}
	if (p_y1 >= core->m_viewYMax) {
		y1 = (int) core->m_viewYMax;
	}
	if (x0 < 0) {
		x0 = 0;
	}
	if (y0 < 0) {
		y0 = 0;
	}
	if (x1 > (int) core->m_width) {
		x1 = (int) core->m_width;
	}
	if (y1 > (int) core->m_height) {
		y1 = (int) core->m_height;
	}
	int cols = x1 - x0;
	int rows = y1 - y0;
	if (cols < 4 || rows < 4) {
		return;
	}
	if (!core->m_texE0C || !core->m_zbuffer || core->m_zpitch < (int) core->m_width) {
		return;
	}

	int zFar = p_zBase + 8 * (p_zTop - p_zBottom);
	int srcRect[4];
	srcRect[0] = 0;
	srcRect[1] = 0;
	srcRect[2] = RENDER_MATH::QuarterExtent((float) cols);
	srcRect[3] = RENDER_MATH::QuarterExtent((float) rows);
	int dstRect[4];
	dstRect[0] = x0;
	dstRect[1] = y0;
	dstRect[2] = x1;
	dstRect[3] = y1;
	int pitch;
	unsigned char* dst = (unsigned char*) core->m_texE0C->Lock(&pitch, (const RECT*) srcRect);
	if (!dst) {
		if (::Error) {
			MYERROR::Error(
				::Error,
				"GRAPH",
				0,
				// STRING: ALIEN 0x483f44
				"fog buffer",
				0
			);
		}
		return;
	}

	if (core->m_texE0C->m_format != D3DFMT_P8) {
		if (!WEATHER_RASTER::FillFog<false>(
				dst,
				pitch,
				core->m_texE0C->m_width,
				core->m_texE0C->m_height,
				(const unsigned short*) core->m_zbuffer,
				core->m_zpitch,
				(int) core->m_width,
				(int) core->m_height,
				x0,
				y0,
				x1,
				y1,
				p_ramp,
				p_zBase,
				zFar
			)) {
			return;
		}
	}
	else {
		if (!WEATHER_RASTER::FillFog<true>(
				dst,
				pitch,
				core->m_texE0C->m_width,
				core->m_texE0C->m_height,
				(const unsigned short*) core->m_zbuffer,
				core->m_zpitch,
				(int) core->m_width,
				(int) core->m_height,
				x0,
				y0,
				x1,
				y1,
				p_ramp,
				p_zBase,
				zFar
			)) {
			return;
		}
	}

	core->SetAlphaBlend((p_blend == 0) + 1, 4);
	float zScale = (p_zBase + 1022) * 0.000015258789f;
	GAMMA fogGamma(p_color, COLOR((int) 0xff000000));
	core->m_texE0C->Draw_z(zScale, std::bit_cast<int>(zScale), dstRect, srcRect, &fogGamma);
}

// FUNCTION: ALIEN 0x4322a0
void GRAPH::DrawSnow()
{
	int pitch;
	char* bits;
	int yPhase;
	int xPhase;
	RECT quarter;
	RECT full;

	GRAPH_CORE* core = (GRAPH_CORE*) this;
	xPhase = -((int) Map->m_shiftX & 3) & 3;
	yPhase = -((int) Map->m_shiftY & 3) & 3;
	if (core->m_flags & 0x20) {
		return;
	}
	if (Map->m_flag & 0x10) {
		return;
	}
	if (!(m_env & 0x40)) {
		s_snowStart = CurrentTime;
		s_snowFade = 0;
		return;
	}
	if (s_snowFade < 0x100) {
		s_snowFade = std::min(0x100u, (CurrentTime - s_snowStart) >> 7);
	}
	quarter.left = 0;
	quarter.top = 0;
	int w = (int) m_width;
	quarter.right = RENDER_MATH::QuarterExtent((float) w);
	int h = (int) m_height;
	quarter.bottom = RENDER_MATH::QuarterExtent((float) h);
	full.left = 0;
	full.top = 0;
	full.right = w;
	full.bottom = h;
	bits = (char*) core->m_texE0C->Lock(&pitch, &quarter);
	if (!bits) {
		if (::Error) {
			MYERROR::Error(
				::Error,
				"GRAPH",
				0,
				// STRING: ALIEN 0x483f50
				"snow buffer",
				0
			);
		}
		return;
	}
	if (core->m_texE0C->m_format != D3DFMT_P8) {
		if (!WEATHER_RASTER::FillSnow<false>(
				(unsigned char*) bits,
				pitch,
				core->m_texE0C->m_width,
				core->m_texE0C->m_height,
				(const unsigned short*) m_zbuffer,
				m_zpitch,
				w,
				h,
				xPhase,
				yPhase,
				s_snowFade,
				core->m_snowRamp
			)) {
			return;
		}
	}
	else {
		if (!WEATHER_RASTER::FillSnow<true>(
				(unsigned char*) bits,
				pitch,
				core->m_texE0C->m_width,
				core->m_texE0C->m_height,
				(const unsigned short*) m_zbuffer,
				m_zpitch,
				w,
				h,
				xPhase,
				yPhase,
				s_snowFade,
				core->m_snowRamp
			)) {
			return;
		}
	}
	core->SetRenderState(D3DRS_SPECULARENABLE, 0);
	core->SetAlphaBlend(2, 4);
	GAMMA plain(GAMMA::RAW_COPY, 0, 0);
	core->m_texE0C->Draw(&full, &quarter, &plain);
}

// GLOBAL: ALIEN 0x4b27a8
static ANGLE s_lastWindDir;

// GLOBAL: ALIEN 0x483d3c
static float s_lastWindForce = 99999.0f;

// GLOBAL: ALIEN 0x4b27d0
static float s_windDrift;

// GLOBAL: ALIEN 0x4b2c7c
static int s_rainCount;

struct RAINDROPS {
	~RAINDROPS() {}
	RHW_VERTEX m_v[500];
};

// STUB: ALIEN 0x432770
void GRAPH::DrawRain()
{
	// GLOBAL: ALIEN 0x4b0078
	static RAINDROPS s_drops;

	GRAPH_CORE* core = (GRAPH_CORE*) this;
	int env = m_env;
	if (!(env & 0xc00)) {
		s_rainCount = 0;
		return;
	}
	if (core->m_flags & 0x20) {
		return;
	}
	if (Map->m_flag & 0x10) {
		return;
	}
	if (s_lastWindDir.m_dir != m_windDirection || s_lastWindForce != m_windForce) {
		float old = s_windDrift;
		s_windDrift = ANGLE::SinTable[m_windDirection] * m_windForce * 1000.0f;
		RHW_VERTEX* v = s_drops.m_v;
		for (int n = s_rainCount; n > 0; --n) {
			v[1].m_x += (v[1].m_y - v->m_y) * (s_windDrift - old) * 0.005f;
			v += 2;
		}
		AngleAssign(&s_lastWindDir, ANGLE(m_windDirection));
		s_lastWindForce = m_windForce;
	}
	env = m_env;
	if ((env & 0xc00) == 0xc00) {
		s_rainCount = 250;
	}
	else if ((env & 0xc00) == 0x800) {
		int count = s_rainCount;
		if (count < 250) {
			++count;
			s_rainCount = count;
		}
		else {
			SetEnvironment(0xc00);
		}
	}
	else if ((env & 0xc00) == 0x400) {
		int count = s_rainCount;
		if (count > 0) {
			s_rainCount = count - 1;
		}
		else {
			m_env = env & ~0xc00;
		}
	}
	int i = 0;
	int total = 0;
	int count = s_rainCount;
	if (count > 0) {
		float* pz1 = &s_drops.m_v[1].m_z;
		unsigned int* pc = &s_drops.m_v[0].m_color;
		float* prhw = &s_drops.m_v[0].m_rhw;
		float* pz = &s_drops.m_v[0].m_z;
		float* py = &s_drops.m_v[0].m_y;
		float* px = &s_drops.m_v[0].m_x;
		do {
			float x;
			float y;
			if (*pc && (x = *px, y = *py, x) >= core->m_viewXMin && x < core->m_viewXMax && y >= core->m_viewYMin &&
				y < core->m_viewYMax && *pz >= 0.015625f) {
				unsigned int dt = CurrentTime - PrevCurrentTime;
				float fall = (*pz1 - *pz) * dt * 200.0f;
				float drift = s_windDrift * fall * 0.005f;
				if (drift + *px < 0.0f) {
					drift = drift + m_width;
				}
				if (drift + *px > m_width) {
					drift = drift - m_width;
				}
				*px += drift;
				*py += fall;
				float zd = fall * 0.0001220703125f;
				*pz -= zd;
				px += 5;
				py += 5;
				pz += 5;
				prhw += 5;
				pc += 5;
				pz1 += 5;
				*px += drift;
				*py += fall;
				*pz -= zd;
				px += 5;
				py += 5;
				pz += 5;
				prhw += 5;
				pc += 5;
				pz1 += 5;
			}
			else {
				++total;
				float len = GameRand() % 51 + 15.0f;
				float windX = s_windDrift * len * -0.005f;
				float xmax = core->m_viewXMax - 1.0f;
				float x0 = GameRand() * xmax * 3.0518509e-5f;
				float ymax;
				if (total < 50) {
					ymax = 40.0f;
				}
				else {
					ymax = core->m_viewYMax;
				}
				float y0 = GameRand() * ymax * 3.0518509e-5f;
				*px = x0;
				float z0 = (len + 10.0f) * 0.001220703125f + 0.015625f;
				*py = y0;
				*pz = z0;
				*prhw = 1.0f;
				*pc = 0x70e0e0ff;
				px += 5;
				py += 5;
				pz += 5;
				prhw += 5;
				pc += 5;
				pz1 += 5;
				*px = x0 + windX;
				*py = y0 - len;
				*pz = len * 0.0001220703125f + z0;
				*prhw = 1.0f;
				*pc = 0x308080ff;
				px += 5;
				py += 5;
				pz += 5;
				prhw += 5;
				pc += 5;
				pz1 += 5;
			}
			++i;
		} while (i < s_rainCount);
	}
	core->SetAlphaBlend(5, 6);
	core->DrawPrimitive(2, 0x44, s_drops.m_v, 0x14, 2 * s_rainCount);
}

// GLOBAL: ALIEN 0x4b0070
static ANGLE s_lastFlakeWindDir;

// GLOBAL: ALIEN 0x483d40
static float s_lastFlakeWindForce = 99999.0f;

// GLOBAL: ALIEN 0x4b27e0
static float s_flakeWindDrift;

// GLOBAL: ALIEN 0x4b2c80
static float s_lastShiftX;

// GLOBAL: ALIEN 0x4b2c84
static float s_lastShiftY;

// GLOBAL: ALIEN 0x4b2c88
static int s_flakeCount;

struct SNOWFLAKES {
	~SNOWFLAKES() {}
	RHW_VERTEX m_v[6000];
};

// STUB: ALIEN 0x432c10
void GRAPH::DrawSnowflakes()
{
	// GLOBAL: ALIEN 0x492b80
	static SNOWFLAKES s_flakes;

	GRAPH_CORE* core = (GRAPH_CORE*) this;
	int env = m_env;
	if (!(env & 0xc000)) {
		s_flakeCount = 0;
		return;
	}
	if (core->m_flags & 0x20) {
		return;
	}
	if (Map->m_flag & 0x10) {
		return;
	}
	if (s_lastFlakeWindDir.m_dir != m_windDirection || s_lastFlakeWindForce != m_windForce) {
		s_flakeWindDrift = ANGLE::SinTable[m_windDirection] * m_windForce * 1000.0f;
		AngleAssign(&s_lastFlakeWindDir, ANGLE(m_windDirection));
		s_lastFlakeWindForce = m_windForce;
	}
	env = m_env;
	if ((env & 0xc000) == 0xc000) {
		s_flakeCount = 1000;
	}
	else if ((env & 0xc000) == 0x8000) {
		if (s_flakeCount < 1000) {
			++s_flakeCount;
		}
		else {
			s_flakeCount = 1000;
			SetEnvironment(0xc000);
		}
	}
	else if ((env & 0xc000) == 0x4000) {
		if (s_flakeCount > 0) {
			--s_flakeCount;
		}
		else {
			m_env = env & ~0xc000;
		}
	}
	int i = 0;
	int total = 0;
	if (s_flakeCount > 0) {
		unsigned int* pc = &s_flakes.m_v[0].m_color;
		float* prhw = &s_flakes.m_v[1].m_z;
		float* pz = &s_flakes.m_v[0].m_z;
		float* py = &s_flakes.m_v[0].m_y;
		float* px = &s_flakes.m_v[0].m_x;
		float* pz1 = &s_flakes.m_v[0].m_rhw;
		for (; i < s_flakeCount; ++i) {
			if (*pc && *pz >= 0.015625f) {
				unsigned int dt = CurrentTime - PrevCurrentTime;
				float fall = (*prhw - *pz - 0.0001220703125f) * dt * 150.0f;
				int r = GameRand() % 101;
				float dx = (s_flakeWindDrift * fall + 50.0f - r) * 0.02f - (s_lastShiftX - (-Map->m_shiftX));
				float zd = fall * 0.0001220703125f;
				float dy = fall - (s_lastShiftY - (-Map->m_shiftY));
				if (dx + *px < core->m_viewXMin - 30.0f) {
					dx = dx + (core->m_viewXMax - core->m_viewXMin);
				}
				if (dx + *px > core->m_viewXMax + 30.0f) {
					dx = dx - (core->m_viewXMax - core->m_viewXMin);
				}
				if (dy + *py < core->m_viewYMin - 30.0f) {
					dy = dy + (core->m_viewYMax - core->m_viewYMin);
				}
				if (dy + *py > core->m_viewYMax + 30.0f) {
					dy = dy - (core->m_viewYMax - core->m_viewYMin);
				}
				*px += dx;
				*py += dy;
				*pz -= zd;
				px += 5;
				py += 5;
				pz += 5;
				prhw += 5;
				pc += 5;
				pz1 += 5;
				*px += dx;
				*py += dy;
				*pz -= zd;
				px += 5;
				py += 5;
				pz += 5;
				prhw += 5;
				pc += 5;
				pz1 += 5;
				*px += dx;
				*py += dy;
				*pz -= zd;
				px += 5;
				py += 5;
				pz += 5;
				prhw += 5;
				pc += 5;
				pz1 += 5;
				*px += dx;
				*py += dy;
				*pz -= zd;
				px += 5;
				py += 5;
				pz += 5;
				prhw += 5;
				pc += 5;
				pz1 += 5;
				*px += dx;
				*py += dy;
				*pz -= zd;
				px += 5;
				py += 5;
				pz += 5;
				prhw += 5;
				pc += 5;
				pz1 += 5;
				*px += dx;
				*py += dy;
				*pz -= zd;
				px += 5;
				py += 5;
				pz += 5;
				prhw += 5;
				pc += 5;
				pz1 += 5;
			}
			else {
				++total;
				float len = GameRand() % 3 + 2.0f;
				float xmax = core->m_viewXMax - 1.0f;
				float x0 = GameRand() * xmax * 3.0518509e-5f;
				float ymax;
				if (total < 50) {
					ymax = 40.0f;
				}
				else {
					ymax = core->m_viewYMax;
				}
				float y0 = GameRand() * ymax * 3.0518509e-5f;
				float z0 = GameRand() * (core->m_viewYMax + 50.0f) * 3.725404e-9f + 0.015625f;
				*px = x0 - len;
				*py = y0;
				*pz = z0;
				*pz1 = 1.0f;
				*pc = 0xffffffff;
				px += 5;
				py += 5;
				pz += 5;
				prhw += 5;
				pc += 5;
				pz1 += 5;
				*px = x0 + len;
				*py = y0;
				*pz = len * 0.0001220703125f + z0;
				*pz1 = 1.0f;
				*pc = 0xffffffff;
				px += 5;
				py += 5;
				pz += 5;
				prhw += 5;
				pc += 5;
				pz1 += 5;
				float half = len * 0.5f;
				*px = x0 - half;
				*py = y0 - len;
				*pz = z0;
				*pz1 = 1.0f;
				*pc = 0xffffffff;
				px += 5;
				py += 5;
				pz += 5;
				prhw += 5;
				pc += 5;
				pz1 += 5;
				*px = x0 + half;
				*py = y0 + len;
				*pz = z0;
				*pz1 = 1.0f;
				*pc = 0xffffffff;
				px += 5;
				py += 5;
				pz += 5;
				prhw += 5;
				pc += 5;
				pz1 += 5;
				*px = x0 - half;
				*py = y0 + len;
				*pz = z0;
				*pz1 = 1.0f;
				*pc = 0xffffffff;
				px += 5;
				py += 5;
				pz += 5;
				prhw += 5;
				pc += 5;
				pz1 += 5;
				*px = x0 + half;
				*py = y0 - len;
				*pz = z0;
				*pz1 = 1.0f;
				*pc = 0xffffffff;
				px += 5;
				py += 5;
				pz += 5;
				prhw += 5;
				pc += 5;
				pz1 += 5;
			}
		}
	}
	s_lastShiftX = -Map->m_shiftX;
	s_lastShiftY = -Map->m_shiftY;
	float* qx = &s_flakes.m_v[0].m_x;
	float* qy = &s_flakes.m_v[0].m_y;
	float* qx1 = &s_flakes.m_v[1].m_x;
	float* qy1 = &s_flakes.m_v[1].m_y;
	for (i = 0; i < s_flakeCount; ++i) {
		if (!(GameRand() % 5)) {
			float cx = (*qx1 + *qx) * 0.5f;
			float cy = (*qy1 + *qy) * 0.5f;
			float tmp = *qx;
			*qx = cx + *qy - cy;
			*qy = tmp + cy - cx;
			qx += 5;
			qy += 5;
			tmp = *qx;
			*qx = cx + *qy - cy;
			*qy = tmp + cy - cx;
			qx += 5;
			qy += 5;
			tmp = *qx;
			*qx = cx + *qy - cy;
			*qy = tmp + cy - cx;
			qx += 5;
			qy += 5;
			tmp = *qx;
			*qx = cx + *qy - cy;
			*qy = tmp + cy - cx;
			qx += 5;
			qy += 5;
			tmp = *qx;
			*qx = cx + *qy - cy;
			*qy = tmp + cy - cx;
			qx += 5;
			qy += 5;
			tmp = *qx;
			*qx = cx + *qy - cy;
			*qy = tmp + cy - cx;
			qx += 5;
			qy += 5;
			qx1 += 30;
			qy1 += 30;
		}
		else {
			qx += 30;
			qy += 30;
			qx1 += 30;
			qy1 += 30;
		}
	}
	core->SetAlphaBlend(5, 6);
	core->DrawPrimitive(2, 0x44, s_flakes.m_v, 0x14, 2 * s_flakeCount);
}

// FUNCTION: ALIEN 0x4333d0
void GRAPH::DrawVid(VID* p_vid, int p_cadr, float p_x, float p_y, float p_z)
{
	if (!p_vid || p_vid == EmptyVid) {
		return;
	}
	if (p_cadr < 0 || p_cadr >= p_vid->m_dotFrameCount) {
		if (::Error) {
			MYERROR::Error(
				::Error,
				"GRAPH",
				4,
				// STRING: ALIEN 0x483f5c
				"ncadr in DrawVid",
				p_cadr
			);
		}
		return;
	}

	GRAPH_CORE* core = (GRAPH_CORE*) this;
	if (p_vid->m_layer <= 8) {
		core->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
		core->SetRenderState(D3DRS_ZWRITEENABLE, 1);
	}
	else {
		core->SetRenderState(D3DRS_ZWRITEENABLE, 0);
		core->SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
		core->SetRenderState(D3DRS_ZFUNC, D3DCMP_GREATEREQUAL);
	}

	SPRITE sprite(p_vid, Map->AbsX(p_x), Map->AbsY(p_y), p_z, ANGLE((unsigned char) 0), 0);
	sprite.m_noCadr = p_cadr;
	p_vid->Draw(&sprite);
}

// FUNCTION: ALIEN 0x434060
int GRAPH::ShadowBar(float p_x, float p_y, float p_x1, float p_y1, unsigned int p_color)
{
	GRAPH_CORE* core = (GRAPH_CORE*) this;
	core->SetAlphaBlend(D3DBLEND_ZERO, D3DBLEND_INVSRCCOLOR);
	return core->FillRect(p_x, p_y, p_x1, p_y1, p_color, 0xffffffff);
}

// FUNCTION: ALIEN 0x434180
int GRAPH::LightBar(float p_x, float p_y, float p_x1, float p_y1, unsigned int p_color)
{
	GRAPH_CORE* core = (GRAPH_CORE*) this;
	core->SetRenderState(D3DRS_SPECULARENABLE, 0);
	core->SetAlphaBlend(D3DBLEND_DESTCOLOR, D3DBLEND_ONE);
	return core->FillRect(p_x, p_y, p_x1, p_y1, p_color, 0xffffffff);
}
