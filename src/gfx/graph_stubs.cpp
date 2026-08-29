
#define DECOMP_COLOR_COPY_OUT_OF_LINE

#include "gfx/graph.h"

#include <stdlib.h>

#include "game/map.h"
#include "sprite/sprite.h"
#include "ui/mouse.h"
#include "video/vid.h"
#include "gfx/graph_core.h"

#include "util/string.h"

#include "game/map.h"
#include "video/vid.h"
#include <math.h>
#include <stdlib.h>
#include "game/gametime.h"
#include "util/myerror.h"
#include "gfx/picture.h"
#include "gfx/texture.h"

// GLOBAL: ALIEN 0x4b2c58
static float s_oldShiftX;

// GLOBAL: ALIEN 0x492b78
static float s_oldShiftY;

// FUNCTION: ALIEN 0x430870
void GRAPH::Tact(int p_draw)
{
	GRAPH_CORE* core = (GRAPH_CORE*) this;
	if ((m_env & 4) && !(Map->m_flag & 0x10)) {
		s_oldShiftX = Map->m_shiftX;
		s_oldShiftY = Map->m_shiftY;
		float height = Graph->m_height;
		int jy = rand() % 9;
		float width = core->m_width;
		int jx = rand() % 9;
		Map->SetShiftCoor(width * 0.5f + s_oldShiftX + 4.0f - jx,
			height * 0.5f + s_oldShiftY + 4.0f - jy, 0);
	}
	if (p_draw) {
		if (Map->m_noVid <= 1024 || !Map->m_vids[1024] || Map->m_shiftX < 0.0f
			|| core->m_viewXMax + Map->m_shiftX - core->m_viewXMin > (double) Map->m_w
			|| Map->m_shiftY < 0.0f
			|| core->m_viewYMax + Map->m_shiftY - core->m_viewYMin > (double) Map->m_h)
			core->ClearScreen(COLOR(0, 0, 0));
		core->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
		core->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
		core->SetRenderState(D3DRS_ZWRITEENABLE, 1);
		core->m_device->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_POINT);
		if (core->m_locked) {
			core->m_backBuffer->UnlockRect();
			core->m_locked = 0;
		}
		Map->DrawLayer(0);
		core->m_device->SetTexture(0, 0);
		core->Lock();
		Map->DrawLayer(1);
		Map->DrawLayer(2);
		Map->DrawLayer(3);
		if (core->m_locked) {
			core->m_backBuffer->UnlockRect();
			core->m_locked = 0;
		}
		Map->DrawLayer(4);
		core->Lock();
		Map->DrawLayer(5);
		Map->DrawLayer(6);
		Map->DrawLayer(7);
		if (core->m_locked) {
			core->m_backBuffer->UnlockRect();
			core->m_locked = 0;
		}
		core->SetRenderState(D3DRS_ZFUNC, D3DCMP_GREATEREQUAL);
		Map->DrawLayer(8);
		core->SetRenderState(D3DRS_ZWRITEENABLE, 0);
		core->SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
		core->m_device->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_POINT);
		Map->DrawLayer(9);
		Map->DrawLayer(10);
		core->m_device->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
		Map->DrawLayer(11);
		DrawSnow();
		DrawSnowflakes();
		core->m_device->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_POINT);
		Map->DrawLayer(12);
		DrawRain();
		core->SetRenderState(D3DRS_ZWRITEENABLE, 1);
		core->m_device->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_POINT);
		if (!core->m_locked) {
			int rect[2];
			if (core->m_backBuffer->LockRect((D3DLOCKED_RECT*) rect, 0, 0) < 0)
				core->Error(0, "backBuffer", 0);
			core->m_locked = rect[1];
			core->m_unk0x248 = rect[0] / ((core->m_flags & 2) ? 4 : 2);
		}
		Map->DrawLayer(13);
		if (core->m_locked) {
			core->m_backBuffer->UnlockRect();
			core->m_locked = 0;
		}
		Map->DrawLayer(14);
		MOUSE* child = Mouse;
		if (!Mouse->m_unk0x70 && (Mouse->m_vid->m_flag & 0x8000) && Mouse) {
			do {
				if (!(child->m_flag & 0x10000))
					child->Draw();
				child = (MOUSE*) child->m_child;
			} while (child);
		}
	}
	DrawSquall();
	if ((m_env & 4) && !(Map->m_flag & 0x10))
		Map->SetShiftCoor(core->m_width * 0.5f + s_oldShiftX,
			Graph->m_height * 0.5f + s_oldShiftY, 0);
	if (core->m_locked) {
		core->m_backBuffer->UnlockRect();
		core->m_locked = 0;
	}
	if (m_movie.m_graph && m_movie.IsComplete())
		m_movie.Stop();
	core->SetRenderState(D3DRS_SPECULARENABLE, 0);
	DrawEffect(p_draw);
	if (core->m_locked) {
		core->m_backBuffer->UnlockRect();
		core->m_locked = 0;
	}
}

static inline GAMMA InterpolateGamma(const GAMMA& p_cur, const GAMMA& p_tgt, double p_t)
{
	int base;
	int want;
	int cur;
	base = (p_cur.m_a & 0xff000000) ? -(int) ((unsigned int) p_cur.m_a >> 24)
								    : (int) ((unsigned int) p_cur.m_b >> 24);
	want = ((unsigned int) p_tgt.m_a & 0xff000000) ? -(int) ((unsigned int) p_tgt.m_a >> 24)
												   : (int) ((unsigned int) p_tgt.m_b >> 24);
	cur = (p_cur.m_a & 0xff000000) ? -(int) ((unsigned int) p_cur.m_a >> 24)
								   : (int) ((unsigned int) p_cur.m_b >> 24);
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
	base = ((unsigned char) p_cur.m_a) ? -(int) (unsigned char) p_cur.m_a
									   : (int) (unsigned char) p_cur.m_b;
	want = ((unsigned char) p_tgt.m_a) ? -(int) (unsigned char) p_tgt.m_a
									   : (int) (unsigned char) p_tgt.m_b;
	cur = ((unsigned char) p_cur.m_a) ? -(int) (unsigned char) p_cur.m_a
									  : (int) (unsigned char) p_cur.m_b;
	int b = (int) ((want - cur) * p_t + base);
	return GAMMA(a, r, g, b);
}

// STUB: ALIEN 0x4311a0
void GRAPH::DrawEffect(int p_draw)
{
	GRAPH_CORE* core = (GRAPH_CORE*) this;
	unsigned int t;
	if (core->m_effectStart[5]) {
		if (RealCurrentTime - core->m_effectStart[5] < (unsigned int) core->m_effectDuration[5]) {
			core->SetAlphaBlend(6, 5);
			float y = core->m_viewYMin;
			if (y < core->m_viewYMax) {
				do {
					t = RealCurrentTime;
					float x = core->m_viewXMin;
					if (x < core->m_viewXMax) {
						do {
						float w;
						if (x + 256.0f >= core->m_viewXMax)
							w = core->m_viewXMax - x;
						else
							w = 256.0f;
						float h;
						if (y + 256.0f >= core->m_viewYMax)
							h = core->m_viewYMax - y;
						else
							h = 256.0f;
						RECT src;
						src.left = (int) (x - core->m_viewXMin);
						src.top = (int) (y - core->m_viewYMin);
						src.right = (int) (w + x - core->m_viewXMin);
						src.bottom = (int) (h + y - core->m_viewYMin);
						POINT pt;
						pt.x = 0;
						pt.y = 0;
						RECT dst;
						dst.left = 0;
						dst.top = 0;
						dst.right = (int) w;
						dst.bottom = (int) h;
						int alpha = (256 * t - 256 * core->m_effectStart[5])
							/ core->m_effectDuration[5];
						if (p_draw) {
							if (!core->m_texE10->CopyFromSurface(core->m_screenSurf, &src, &pt)) {
								core->m_texE10->Draw(&src, &dst,
									&GAMMA(COLOR(alpha, 255, 255, 255),
										COLOR((int) 0xff000000)));
							}
							t = RealCurrentTime;
						}
						x += 256.0f;
						} while (x < core->m_viewXMax);
					}
					y += 256.0f;
				} while (y < core->m_viewYMax);
			}
			else
				t = RealCurrentTime;
		}
		else {
			core->m_effectStart[5] = 0;
			t = RealCurrentTime;
		}
	}
	else
		t = RealCurrentTime;
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
			float sy = ((float) core->m_effectB[2] - core->m_unk0xcd0) * fe / fd + core->m_unk0xcd0;
			float sx = ((float) core->m_effectA[2] - core->m_unk0xccc) * fe / fd + core->m_unk0xccc;
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
			int scale = num / dur;
			int color = ((((core->m_effectA[1] & 0xff00) * scale) >> 8) & 0xff00)
				+ (((core->m_effectA[1] & 0xff) * scale) >> 8)
				+ (((core->m_effectA[1] * scale) >> 8) & 0xff0000);
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
			if (elapsed < 4 * dur / 5)
				a = (1280 * t - 1280 * start) / (4 * dur);
			else
				a = 255;
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
			SetGamma(GAMMA(GAMMA::RAW_COPY,
				InterpolateGamma(core->m_gammaCur, tgt, frac)));
		}
	}
}

// FUNCTION: ALIEN 0x431c50
void GRAPH::SetGamma(const GAMMA& p_gamma)
{
	if (m_gammaSet.m_a != p_gamma.m_a || m_gammaSet.m_b != p_gamma.m_b) {
		m_gammaSet.m_a = p_gamma.m_a;
		m_gammaSet.m_b = p_gamma.m_b;
		for (int i = 0; i < 0x800; ++i) {
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
	if (Map->m_flag & 0x10)
		return;
	if (m_env & 0x80) {
		if (g_oldWindSpeed == -1.0f)
			g_oldWindSpeed = m_windForce;
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
		if (g_oldWindSpeed != -1.0f)
			m_windForce = g_oldWindSpeed;
	}
	g_oldWindSpeed = -1.0f;
}

// GLOBAL: ALIEN 0x4b2c74
static unsigned int s_snowStart;

// GLOBAL: ALIEN 0x4b2c78
static unsigned int s_snowFade;

// FUNCTION: ALIEN 0x431e40
void GRAPH::DrawFog(float p_x0, float p_y0, float p_x1, float p_y1, int p_zTop,
	int p_zBottom, COLOR p_color, int p_ramp, int p_zBase, int p_blend)
{
	GRAPH_CORE* core = (GRAPH_CORE*) this;
	int x0 = (int) p_x0;
	int y0 = (int) p_y0;
	int x1 = (int) p_x1;
	int y1 = (int) p_y1;
	if ((core->m_flags & 0x20) || !p_ramp
		|| p_x1 < core->m_viewXMin || p_x0 >= core->m_viewXMax
		|| p_y1 < core->m_viewYMin || p_y0 >= core->m_viewYMax)
		return;

	if (p_x0 < core->m_viewXMin)
		x0 = (int) core->m_viewXMin;
	if (p_y0 < core->m_viewYMin)
		y0 = (int) core->m_viewYMin;
	if (p_x1 >= core->m_viewXMax)
		x1 = (int) core->m_viewXMax;
	if (p_y1 >= core->m_viewYMax)
		y1 = (int) core->m_viewYMax;
	int cols = x1 - x0;
	int rows = y1 - y0;
	if (cols < 4 || rows < 4)
		return;

	int zFar = p_zBase + 8 * (p_zTop - p_zBottom);
	int srcRect[4];
	srcRect[0] = 0;
	srcRect[1] = 0;
	srcRect[2] = cols / 4;
	srcRect[3] = rows / 4;
	int dstRect[4];
	dstRect[0] = x0;
	dstRect[1] = y0;
	dstRect[2] = x1;
	dstRect[3] = y1;
	int pitch;
	unsigned char* dst = (unsigned char*) core->m_texE0C->Lock(&pitch, (const RECT*) srcRect);
	if (!dst) {
		if (::Error)
			MYERROR::Error(::Error,
				"GRAPH", 0,
				// STRING: ALIEN 0x483f44
				"fog buffer", 0);
		return;
	}

	int cursor;
	if (core->m_texE0C->m_format != 41) {
		unsigned short carry = 0;
		cursor = core->m_unk0x250 * y0 + x0;
		for (int y = y0; y < y1; y += 4) {
			for (int x = x0; x < x1; x += 4) {
				int z = ((unsigned short*) core->m_zbuffer)[cursor]
						< ((unsigned short*) core->m_zbuffer)[cursor + 3]
					? ((unsigned short*) core->m_zbuffer)[cursor] - 1024
					: ((unsigned short*) core->m_zbuffer)[cursor + 3] - 1024;
				if (z <= p_zBase) {
					if (z <= zFar) {
						carry = ((const unsigned short*) p_ramp)[p_zBase - zFar];
						*(unsigned short*) dst = carry;
					}
					else {
						carry = ((const unsigned short*) p_ramp)[p_zBase - z];
						*(unsigned short*) dst = carry;
					}
				}
				else if (z > p_zBase + 10) {
					*(unsigned short*) dst = carry;
				}
				else {
					carry = 0;
					*(unsigned short*) dst = carry;
				}
				cursor += 4;
				dst += 2;
			}
			cursor += 4 * (core->m_unk0x250 - (cols + 3) / 4);
			dst += pitch - 2 * ((cols + 3) / 4);
		}
	}
	else {
		unsigned char carry = 0;
		cursor = core->m_unk0x250 * y0 + x0;
		for (int y = y0; y < y1; y += 4) {
			for (int x = x0; x < x1; x += 4) {
				int z = ((unsigned short*) core->m_zbuffer)[cursor]
						< ((unsigned short*) core->m_zbuffer)[cursor + 3]
					? ((unsigned short*) core->m_zbuffer)[cursor] - 1024
					: ((unsigned short*) core->m_zbuffer)[cursor + 3] - 1024;
				if (z <= p_zBase) {
					if (z <= zFar) {
						carry = 0xff;
						*dst = carry;
					}
					else {
						carry = ((const unsigned char*) p_ramp)[2 * (p_zBase - z)];
						*dst = carry;
					}
				}
				else if (z > p_zBase + 10) {
					*dst = carry;
				}
				else {
					carry = 0;
					*dst = 0;
				}
				++dst;
				cursor += 4;
			}
			cursor += 4 * (core->m_unk0x250 - (cols + 3) / 4);
			dst += pitch - (cols + 3) / 4;
		}
	}

	if (core->m_texE0C->m_texture)
		core->m_texE0C->m_texture->UnlockRect(0);
	core->SetAlphaBlend((p_blend == 0) + 1, 4);
	float zScale = (p_zBase + 1022) * 0.000015258789f;
	core->m_texE0C->Draw_z(zScale, *(int*) &zScale, dstRect, srcRect,
		&GAMMA(p_color, COLOR((int) 0xff000000)));
}

// FUNCTION: ALIEN 0x4322a0
void GRAPH::DrawSnow()
{
	int pitch;
	int bits;
	int y;
	int yPhase;
	int xPhase;
	int cursor;
	RECT quarter;
	RECT full;

	GRAPH_CORE* core = (GRAPH_CORE*) this;
	xPhase = -((int) Map->m_shiftX & 3) & 3;
	yPhase = -((int) Map->m_shiftY & 3) & 3;
	if (core->m_flags & 0x20)
		return;
	if (Map->m_flag & 0x10)
		return;
	if (!(m_env & 0x40)) {
		s_snowStart = CurrentTime;
		s_snowFade = 0;
		return;
	}
	if (s_snowFade < 0x100)
		s_snowFade = (CurrentTime - s_snowStart) >> 7;
	quarter.left = 0;
	quarter.top = 0;
	int w = (int) m_width;
	quarter.right = w / 4;
	int h = (int) m_height;
	quarter.bottom = h / 4;
	full.left = 0;
	full.top = 0;
	full.right = w;
	full.bottom = h;
	bits = core->m_texE0C->Lock(&pitch, &quarter);
	if (!bits) {
		if (::Error)
			MYERROR::Error(::Error,
				"GRAPH", 0,
				// STRING: ALIEN 0x483f50
				"snow buffer", 0);
		return;
	}
	if (core->m_texE0C->m_format != 41) {
		pitch /= 2;
		for (y = yPhase, cursor = xPhase + m_unk0x250 * yPhase; y < m_height; y += 4, cursor += 3 * m_unk0x250) {
			for (int x = xPhase; x < m_width; x += 4, cursor += 4) {
				int dz;
				if (y > 3 && (dz = abs(((unsigned short*) m_zbuffer)[cursor] -
									((unsigned short*) m_zbuffer)[cursor - 4 * m_unk0x250])) <= 6) {
					((unsigned short*) bits)[pitch * (y / 4) + x / 4] =
						core->m_snowRamp[(6 - dz) * s_snowFade >> 3];
				}
				else {
					dz = abs(((unsigned short*) m_zbuffer)[cursor] -
						((unsigned short*) m_zbuffer)[cursor + 4 * m_unk0x250]);
					if (dz <= 6)
						((unsigned short*) bits)[pitch * (y / 4) + x / 4] =
							core->m_snowRamp[(6 - dz) * s_snowFade >> 3];
					else
						((unsigned short*) bits)[pitch * (y / 4) + x / 4] = 0;
				}
			}
		}
	}
	else {
		for (y = yPhase, cursor = xPhase + m_unk0x250 * yPhase; y < m_height; y += 4, cursor += 3 * m_unk0x250) {
			for (int x = xPhase; x < m_width; x += 4, cursor += 4) {
				int dz;
				if (y > 3 && (dz = abs(((unsigned short*) m_zbuffer)[cursor] -
									((unsigned short*) m_zbuffer)[cursor - 4 * m_unk0x250])) <= 6) {
					((unsigned char*) bits)[pitch * (y / 4) + x / 4] =
						(unsigned char) ((6 - dz) * s_snowFade >> 3);
				}
				else {
					dz = abs(((unsigned short*) m_zbuffer)[cursor] -
						((unsigned short*) m_zbuffer)[cursor + 4 * m_unk0x250]);
					if (dz <= 6)
						((unsigned char*) bits)[pitch * (y / 4) + x / 4] =
							(unsigned char) ((6 - dz) * s_snowFade >> 3);
					else
						((unsigned char*) bits)[pitch * (y / 4) + x / 4] = 0;
				}
			}
		}
	}
	if (core->m_texE0C->m_texture)
		core->m_texE0C->m_texture->UnlockRect(0);
	core->SetRenderState(D3DRS_SPECULARENABLE, 0);
	core->SetAlphaBlend(2, 4);
	{
		POINT pt;
		pt.x = 0;
		pt.y = 0;
		core->m_texE0C->Draw(&full, &quarter, (const GAMMA*) &pt);
	}
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
	if (core->m_flags & 0x20)
		return;
	if (Map->m_flag & 0x10)
		return;
	if (s_lastWindDir.m_dir != m_windDirection || s_lastWindForce != m_windForce) {
		float old = s_windDrift;
		s_windDrift = ANGLE::SinTable[m_windDirection] * m_windForce * 1000.0f;
		RHW_VERTEX* v = s_drops.m_v;
		for (int n = s_rainCount; n > 0; --n) {
			v[1].m_x += (v[1].m_y - v->m_y) * (s_windDrift - old) * 0.005f;
			v += 2;
		}
		AngleAssign(&s_lastWindDir, *(const ANGLE*) &m_windDirection);
		s_lastWindForce = m_windForce;
	}
	env = m_env;
	if ((env & 0xc00) == 0xc00) {
		s_rainCount = 250;
	}
	else if ((env & 0xc00) == 0x800) {
		int count = *(volatile int*) &s_rainCount;
		if (count < 250) {
			++count;
			*(volatile int*) &s_rainCount = count;
		}
		else
			SetEnvironment(0xc00);
	}
	else if ((env & 0xc00) == 0x400) {
		int count = s_rainCount;
		if (count > 0) {
			s_rainCount = count - 1;
		}
		else
			m_env = env & ~0xc00;
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
		if (*pc && (x = *px, y = *py, x) >= core->m_viewXMin && x < core->m_viewXMax
			&& y >= core->m_viewYMin && y < core->m_viewYMax
			&& *pz >= 0.015625f) {
			unsigned int dt = CurrentTime - PrevCurrentTime;
			float fall = (*pz1 - *pz) * dt * 200.0f;
			float drift = s_windDrift * fall * 0.005f;
			if (drift + *px < 0.0f)
				drift = drift + m_width;
			if (drift + *px > m_width)
				drift = drift - m_width;
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
			float len = rand() % 51 + 15.0f;
			float windX = s_windDrift * len * -0.005f;
			float xmax = core->m_viewXMax - 1.0f;
			float x0 = rand() * xmax * 3.0518509e-5f;
			float ymax;
			if (total < 50)
				ymax = 40.0f;
			else
				ymax = core->m_viewYMax;
			float y0 = rand() * ymax * 3.0518509e-5f;
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
	core->m_device->SetTexture(0, 0);
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
	if (core->m_flags & 0x20)
		return;
	if (Map->m_flag & 0x10)
		return;
	if (s_lastFlakeWindDir.m_dir != m_windDirection || s_lastFlakeWindForce != m_windForce) {
		s_flakeWindDrift = ANGLE::SinTable[m_windDirection] * m_windForce * 1000.0f;
		AngleAssign(&s_lastFlakeWindDir, *(const ANGLE*) &m_windDirection);
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
		if (s_flakeCount > 0)
			--s_flakeCount;
		else
			m_env = env & ~0xc000;
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
			int r = rand() % 101;
			float dx = (s_flakeWindDrift * fall + 50.0f - r) * 0.02f
				- (s_lastShiftX - (-Map->m_shiftX));
			float zd = fall * 0.0001220703125f;
			float dy = fall - (s_lastShiftY - (-Map->m_shiftY));
			if (dx + *px < core->m_viewXMin - 30.0f)
				dx = dx + (core->m_viewXMax - core->m_viewXMin);
			if (dx + *px > core->m_viewXMax + 30.0f)
				dx = dx - (core->m_viewXMax - core->m_viewXMin);
			if (dy + *py < core->m_viewYMin - 30.0f)
				dy = dy + (core->m_viewYMax - core->m_viewYMin);
			if (dy + *py > core->m_viewYMax + 30.0f)
				dy = dy - (core->m_viewYMax - core->m_viewYMin);
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
			float len = rand() % 3 + 2.0f;
			float xmax = core->m_viewXMax - 1.0f;
			float x0 = rand() * xmax * 3.0518509e-5f;
			float ymax;
			if (total < 50)
				ymax = 40.0f;
			else
				ymax = core->m_viewYMax;
			float y0 = rand() * ymax * 3.0518509e-5f;
			float z0 = rand() * (core->m_viewYMax + 50.0f) * 3.725404e-9f + 0.015625f;
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
		if (!(rand() % 5)) {
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
	core->m_device->SetTexture(0, 0);
	core->SetAlphaBlend(5, 6);
	core->DrawPrimitive(2, 0x44, s_flakes.m_v, 0x14, 2 * s_flakeCount);
}

// FUNCTION: ALIEN 0x4333d0
void GRAPH::DrawVid(VID* p_vid, int p_cadr, float p_x, float p_y, float p_z)
{
	if (!p_vid || p_vid == EmptyVid)
		return;
	if (p_cadr < 0 || p_cadr >= p_vid->m_dotFrameCount) {
		if (::Error)
			MYERROR::Error(::Error,
				"GRAPH", 4,
				// STRING: ALIEN 0x483f5c
				"ncadr in DrawVid", p_cadr);
		return;
	}

	GRAPH_CORE* core = (GRAPH_CORE*) this;
	int wasLocked = core->m_locked != 0;
	int layer = p_vid->m_layer;
	(void) layer;
	if (p_vid->m_layer == 6 || p_vid->m_layer == 5 || p_vid->m_layer == 7) {
		core->Lock();
	}
	else if (core->m_locked) {
		core->m_backBuffer->UnlockRect();
		core->m_locked = 0;
	}

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

	if (wasLocked) {
		if (!core->m_locked) {
			D3DLOCKED_RECT rect;
			if (core->m_backBuffer->LockRect(&rect, 0, 0) < 0 && ::Error)
				MYERROR::Error(::Error, "GRAPH", 0,
					"backBuffer", 0);
			core->m_locked = (int) rect.pBits;
			core->m_unk0x248 = rect.Pitch / ((core->m_flags & 2) ? 4 : 2);
		}
	}
	else if (core->m_locked) {
		core->m_backBuffer->UnlockRect();
		core->m_locked = 0;
	}
}

struct RHW_SPECULAR_VERTEX {
	float m_x; // 0x00
	float m_y; // 0x04
	float m_z; // 0x08
	float m_rhw; // 0x0c
	unsigned int m_color; // 0x10
	unsigned int m_specular; // 0x14
};

// FUNCTION: ALIEN 0x434060
int GRAPH::ShadowBar(float p_x, float p_y, float p_x1, float p_y1, unsigned int p_color)
{
	GRAPH_CORE* core = (GRAPH_CORE*) this;

	RHW_SPECULAR_VERTEX v[4];
	int i = 0;
	v[i].m_x = p_x;
	v[i].m_y = p_y;
	v[i].m_z = 0.99999988f;
	v[i].m_rhw = 1.0f;
	v[i].m_color = p_color;
	v[i].m_specular = 0xffffffff;
	i = 1;
	v[i].m_x = p_x1;
	v[i].m_y = p_y;
	v[i].m_z = 0.99999988f;
	v[i].m_rhw = 1.0f;
	v[i].m_color = p_color;
	v[i].m_specular = 0xffffffff;
	i = 2;
	v[i].m_x = p_x1;
	v[i].m_y = p_y1;
	v[i].m_z = 0.99999988f;
	v[i].m_rhw = 1.0f;
	v[i].m_color = p_color;
	v[i].m_specular = 0xffffffff;
	i = 3;
	v[i].m_x = p_x;
	v[i].m_y = p_y1;
	v[i].m_z = 0.99999988f;
	v[i].m_rhw = 1.0f;
	v[i].m_color = p_color;
	v[i].m_specular = 0xffffffff;

	if (core->m_locked) {
		core->m_backBuffer->UnlockRect();
		core->m_locked = 0;
	}
	core->m_device->SetTexture(0, 0);
	core->SetAlphaBlend(1, 4);
	core->SetRenderState(D3DRS_ZWRITEENABLE, 0);
	core->DrawPrimitive(D3DPT_TRIANGLEFAN, 0xc4, v, 0x18, 4);
	return core->SetRenderState(D3DRS_ZWRITEENABLE, 1);
}

// FUNCTION: ALIEN 0x434180
int GRAPH::LightBar(float p_x, float p_y, float p_x1, float p_y1, unsigned int p_color)
{
	GRAPH_CORE* core = (GRAPH_CORE*) this;

	RHW_SPECULAR_VERTEX v[4] = {
		{ p_x, p_y, 0.99999988f, 1.0f, p_color, 0xffffffff },
		{ p_x1, p_y, 0.99999988f, 1.0f, p_color, 0xffffffff },
		{ p_x1, p_y1, 0.99999988f, 1.0f, p_color, 0xffffffff },
		{ p_x, p_y1, 0.99999988f, 1.0f, p_color, 0xffffffff }
	};

	if (core->m_locked) {
		core->m_backBuffer->UnlockRect();
		core->m_locked = 0;
	}
	core->m_device->SetTexture(0, 0);
	core->SetRenderState(D3DRS_SPECULARENABLE, 0);
	core->SetAlphaBlend(9, 2);
	core->SetRenderState(D3DRS_ZWRITEENABLE, 0);
	core->DrawPrimitive(D3DPT_TRIANGLEFAN, 0xc4, v, 0x18, 4);
	return core->SetRenderState(D3DRS_ZWRITEENABLE, 1);
}
