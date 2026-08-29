#include "sprite/r_map.h"

#include <stdlib.h>

#include "game/engine.h"
#include "sprite/r_dot.h"
#include <math.h>

#include "misc.h"
#include "util/myerror.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "gfx/color.h"
#include "game/map.h"
#include "video/vid.h"

// GLOBAL: ALIEN 0x4b2cb0
int R_DOT::MaxPathDots;

// GLOBAL: ALIEN 0x4b2cb4
int R_MAP_dotArray[300000];

// FUNCTION: ALIEN 0x43c480
int R_MAP::AddDotToArray(int p_x, int p_y, int p_w, int p_h, int p_dot)
{
	int result = p_x;
	if (p_x < p_w && p_y < p_h && p_x >= 0 && p_y >= 0) {
		int cell = p_y + 100 * p_x;
		int count = ++R_MAP_dotArray[30 * cell];
		if (count >= 30) {
			MYERROR::Error(::Error,
				// STRING: ALIEN 0x484210
				"R_MAP", 10,
				// STRING: ALIEN 0x484218
				"AddDotToArray() a[x][y][0]>=ARR_SIZE", 0);
			result = R_MAP_dotArray[30 * cell] - 1;
			R_MAP_dotArray[30 * cell] = result;
		}
		else {
			result = count + 30 * cell;
			R_MAP_dotArray[result] = p_dot;
		}
	}
	return result;
}

// FUNCTION: ALIEN 0x43c510
void R_MAP::CreateAdditionalDots()
{
	int xCells = 2 * (m_unk0x08 / 150) + 10;
	int yCells = 2 * (m_unk0x0c / 150) + 10;
	if (xCells >= 100)
		xCells = 100;
	if (yCells >= 100)
		yCells = 100;
	for (int gx = 0; gx < xCells; ++gx) {
		for (int gy = 0; gy < yCells; ++gy)
			R_MAP_dotArray[3000 * gx + 30 * gy] = 0;
	}
	for (int i = 0; i < m_list.m_n; ++i) {
		m_list.m_data[i]->m_unk0x94[0] = 0;
		int cx = m_list.m_data[i]->m_x / 75;
		int cy = m_list.m_data[i]->m_y / 75;
		AddDotToArray(cx, cy, xCells, yCells, i);
		AddDotToArray(cx - 1, cy, xCells, yCells, i);
		AddDotToArray(cx, cy - 1, xCells, yCells, i);
		AddDotToArray(cx - 1, cy - 1, xCells, yCells, i);
	}
	for (int cgx = 0; cgx < xCells; ++cgx) {
		for (int cgy = 0; cgy < yCells; ++cgy) {
			for (int a = 1; a <= R_MAP_dotArray[30 * (cgy + 100 * cgx)]; ++a) {
				for (int b = a + 1; b <= R_MAP_dotArray[30 * (cgy + 100 * cgx)]; ++b) {
					int ai = R_MAP_dotArray[30 * (cgy + 100 * cgx) + a];
					int bi = R_MAP_dotArray[30 * (cgy + 100 * cgx) + b];
					for (int la = 0; la < m_list.m_data[ai]->m_noLinks; ++la) {
						for (int lb = 0; lb < m_list.m_data[bi]->m_noLinks; ++lb)
							CreateIntersectedDot(m_list.m_data[ai],
								m_list.m_data[ai]->m_links[la].m_dot,
								m_list.m_data[bi],
								m_list.m_data[bi]->m_links[lb].m_dot);
					}
				}
			}
		}
	}
}

static inline int Between(int p_v, int p_a, int p_b)
{
	if (p_a >= p_b)
		return p_v >= p_b && p_v <= p_a;
	return p_v >= p_a && p_v <= p_b;
}

// STUB: ALIEN 0x43c7d0
void R_MAP::CreateIntersectedDot(R_DOT* p_a, R_DOT* p_b, R_DOT* p_c, R_DOT* p_d)
{
	if (p_a == p_c || p_a == p_d)
		return;
	if (p_b == p_c || p_b == p_d)
		return;
	if (p_a->m_unk0x94[0] || p_b->m_unk0x94[0] || p_c->m_unk0x94[0] || p_d->m_unk0x94[0])
		return;
	int cx = p_c->m_x;
	int bx = p_a->m_x;
	int ax = p_b->m_x;
	int by = p_a->m_y;
	int ay = p_b->m_y;
	int cy = p_c->m_y;
	int dx = p_d->m_x;
	int dy = p_d->m_y;
	int abx = bx - ax;
	int crossA = abx * ay - (by - ay) * ax;
	int cdx = dx - cx;
	int cyDy = cy - dy;
	int ayBy = ay - by;
	int crossC = cdx * cy - (p_d->m_y - cy) * cx;
	int denom = cdx * ayBy - cyDy * abx;
	if (denom == 0.0)
		return;
	double x = (double) (cdx * crossA - crossC * abx) / denom;
	double y;
	if (abx)
		y = (crossA - ayBy * x) / abx;
	else if (cdx)
		y = (crossC - cyDy * x) / cdx;
	int ix = (int) x;
	if (!Between(ix, p_a->m_x, ax))
		return;
	int iy = (int) y;
	if (!Between(iy, p_a->m_y, ay))
		return;
	if (!Between(ix, cx, p_d->m_x))
		return;
	if (!Between(iy, cy, p_d->m_y))
		return;
	int idxAB = p_a->GetLink_idx(p_b);
	int idxBA = p_b->GetLink_idx(p_a);
	int idxCD = p_c->GetLink_idx(p_d);
	int idxDC = p_d->GetLink_idx(p_c);
	if (idxAB >= 0 && idxBA >= 0 && idxCD >= 0 && idxDC >= 0) {
		R_DOT_LINK* linkC = &p_c->m_links[idxCD];
		p_a->m_links[idxAB].m_crossLink = linkC;
		p_b->m_links[idxBA].m_crossLink = linkC;
		R_DOT_LINK* linkA = &p_a->m_links[idxAB];
		p_c->m_links[idxCD].m_crossLink = linkA;
		p_d->m_links[idxDC].m_crossLink = linkA;
	}
}

// FUNCTION: ALIEN 0x43cac0
R_MAP::R_MAP()
{
	m_unk0x04 = 10000;
	m_unk0x00 = 10000;
	m_unk0x0c = 0;
	m_unk0x08 = 0;
}

// STUB: ALIEN 0x43caf0
void R_MAP::SetPushLine(int p_x1, int p_y1, int p_x2, int p_y2, int p_value)
{
	R_DOT* startDot = GetNearestDot_xy(p_x1, p_y1);
	R_DOT* endDot = GetNearestDot_xy(p_x2, p_y2);
	if (!startDot)
		MYERROR::Log(::Error,
			// STRING: ALIEN 0x4842a4
			"!!!ERROR!!!R_MAP: Can't found dot in %i,%i", p_x1, p_y1);
	else if (!endDot)
		MYERROR::Log(::Error, "!!!ERROR!!!R_MAP: Can't found dot in %i,%i", p_x2, p_y2);
	else {
		int n = startDot->m_noLinks;
		if (n <= 0)
			MYERROR::Log(::Error,
				// STRING: ALIEN 0x484274
				"!!!ERROR!!!R_MAP: Can't SetPushLine in %i,%i", p_x1, p_y1);
		else {
		if (endDot == startDot) {
			int best = 999999;
			for (int i = 0; i < n; ++i) {
				R_DOT* d = startDot->m_links[i].m_dot;
				int dx = d->m_x - p_x2;
				int dy = d->m_y - d->m_z - p_y2;
				if (dx * dx + dy * dy < best) {
					best = dx * dx + dy * dy;
					endDot = d;
				}
			}
		}
		RailMap.PrepareForFindDot(startDot, 0, 0, 0);
		if (endDot->FindNewDotWithoutBusyDots() >= 0) {
			R_DOT* dot = startDot;
			while (dot && dot != endDot) {
				for (int i = 0; i < dot->m_noLinks; ++i) {
					if (dot->m_unk0x94[0] == dot->m_links[i].m_dot->m_unk0x94[0] + 1) {
						dot->m_unk0x0c = i;
						*(int*) &dot->m_unk0x10 = p_value;
						dot = dot->m_links[i].m_dot;
						break;
					}
				}
			}
		}
		else
			MYERROR::Log(::Error,
				// STRING: ALIEN 0x484240
				"!!!ERROR!!!R_MAP: Can't PushLineFindDot in %i,%i", p_x1, p_y1);
		}
	}
}

// FUNCTION: ALIEN 0x43cc80
char* R_MAP::SetSemaphoreOrMine(int p_x, int p_y, int p_value, int p_d)
{
	R_DOT* dot = GetNearestDot_xy(p_x, p_y);
	R_DOT* linked = dot;
	if (dot) {
		double bestDist = 10000.0;
		int best = -1;
		int i = 0;
		if (i < dot->m_noLinks) {
			do {
				double dx2 = sqr(p_x - dot->m_links[i].m_dot->m_x);
				double d = sqrt(sqr(p_y - dot->m_links[i].m_dot->m_y) + dx2);
				if (d < bestDist) {
					bestDist = d;
					best = i;
				}
				++i;
			} while (i < dot->m_noLinks);
		}
		if (best < 0)
			return MYERROR::Error(::Error, "R_MAP", 10,
				// STRING: ALIEN 0x4842d0
				"\xed\xe5\xf2\xf3 \xf1\xe2\xff\xe7\xe5\xe9 \xf3 \xf2\xee\xf7\xea\xe8 - \xf2\xe0\xea\xee\xe3\xee \xe1\xfb\xf2\xfc \xed\xe5 \xec\xee\xe6\xe5\xf2",
				0);
		linked = dot->m_links[best].m_dot;
		dot->m_unk0x14 = p_value;
		if (linked)
			linked->m_unk0x14 = p_value;
	}
	return (char*) linked;
}

// FUNCTION: ALIEN 0x43cd90
R_MAP::~R_MAP()
{
}

class RGB16 {
public:
	RGB16(const COLOR& p_color)
	{
		m_value = (unsigned short) ((((unsigned int) p_color.m_value >> 3) & 0x1f)
			| (RGB16_rMask & ((unsigned int) p_color.m_value >> (16 - RGB16_rShift)))
			| (RGB16_gMask & ((unsigned int) p_color.m_value >> (8 - RGB16_gShift))));
	}

	unsigned short m_value; // 0x00
};

DECOMP_SIZE_ASSERT(RGB16, 0x2)

// FUNCTION: ALIEN 0x43cdc0
int R_MAP::DebugDraw()
{
	RGB16 redRgb = GRAPH_CORE::RED;
	for (int i = 0; i < m_list.m_n; ++i) {
		R_DOT* dot = m_list.m_data[i];
		float sx = dot->m_x - Map->m_shiftX;
		float sy = (dot->m_y - dot->m_z) - Map->m_shiftY;
		if (sx <= -50.0f || sy <= -50.0f || sx >= 1000.0f || sy >= 1000.0f)
			continue;

		Graph->Line(sx - 1.0f, sy, sx - 1.0f, dot->m_z + sy, GRAPH_CORE::BLUE);
		Graph->Line(sx - 2.0f, dot->m_z + sy, sx, dot->m_z + sy, GRAPH_CORE::BLUE);

		RGB16 dotRgb = dot->m_busyEngine ? GRAPH_CORE::RED : GRAPH_CORE::WHITE;
		for (int k = 0; k < dot->m_noLinks; ++k) {
			R_DOT_LINK* link = &dot->m_links[k];
			R_DOT* linked = link->m_dot;
			float lx = linked->m_x - Map->m_shiftX;
			float ly = (linked->m_y - linked->m_z) - Map->m_shiftY;
			RGB16 lineRgb = GRAPH_CORE::WHITE;
			if ((int) dot->m_unk0x14 > 3 && (int) linked->m_unk0x14 > 3)
				lineRgb = GRAPH_CORE::BLACK;
			if (dot->m_unk0x04 && linked->m_unk0x04)
				lineRgb = redRgb;
			if (dot->m_unk0x0c == k || linked->m_unk0x0c == link->m_backLink)
				lineRgb = COLOR(255, 128, 128);
			Graph->Line(sx, sy, lx, ly, COLOR(&lineRgb.m_value));
			if (link->m_crossLink)
				Graph->Line(sx, sy, link->m_crossLink->m_dot->GetScreenX(),
					link->m_crossLink->m_dot->GetScreenY(), GRAPH_CORE::GREEN);
		}

		Graph->Line(sx - 2.0f, sy - 2.0f, sx + 2.0f, sy + 2.0f, COLOR(&dotRgb.m_value));
		Graph->Line(sx - 2.0f, sy + 2.0f, sx + 2.0f, sy - 2.0f, COLOR(&dotRgb.m_value));
	}
	if (0)
		return 0;
}
// FUNCTION: ALIEN 0x43d2d0
R_DOT* R_MAP::CreateDot(float p_x, float p_y, float p_z)
{
	int x = (int) p_x;
	int y = (int) p_y;
	int z = (int) p_z;
	R_DOT* dot = GetDot(x, y, z);
	if (!dot) {
		dot = new R_DOT;
		if (!dot)
			MYERROR::LogExit(::Error,
				// STRING: ALIEN 0x4842fc
				"!!!R_MAP::CreateDot- Not enough memory");
		dot->m_x = x;
		dot->m_y = y;
		dot->m_z = z;

		int i = m_list.m_n;
		if (i) {
			R_DOT** dots = m_list.m_data + i;
			while (1) {
				R_DOT* existing = *--dots;
				--i;
				if (existing != dot) {
					if (i)
						continue;
					i = -1;
				}
				break;
			}
		}
		else {
			i = -1;
		}
		if (i < 0) {
			int max = m_list.m_max;
			if (m_list.m_n >= max) {
				int newMax = 2 * max + 4;
				if (newMax > max) {
					R_DOT** oldDots = m_list.m_data;
					m_list.m_data = (R_DOT**) operator new(4 * newMax);
					if (!m_list.m_data)
						MYERROR::LogExit(::Error, "!!!ERROR!!!::LIST: Not enough memory %i", newMax);
					if (oldDots) {
						for (int j = 0; j < m_list.m_max; ++j)
							m_list.m_data[j] = oldDots[j];
						operator delete(oldDots);
					}
					m_list.m_max = newMax;
				}
			}
			m_list.m_data[m_list.m_n++] = dot;
		}

		if (dot->m_x > m_unk0x08)
			m_unk0x08 = dot->m_x;
		else if (dot->m_x < m_unk0x00)
			m_unk0x00 = dot->m_x;
		if (dot->m_y > m_unk0x0c)
			m_unk0x0c = dot->m_y;
		else if (dot->m_y < m_unk0x04)
			m_unk0x04 = dot->m_y;
	}
	++dot->m_refCount;
	return dot;
}

// FUNCTION: ALIEN 0x43d440
R_DOT* R_MAP::GetDot(int p_x, int p_y, int p_z)
{
	for (int i = 0; i < m_list.m_n; i++) {
		R_DOT* dot = m_list.m_data[i];
		if (abs(dot->m_x - p_x) <= 1 && abs(dot->m_y - p_y) <= 1 && dot->m_z == p_z)
			return m_list.m_data[i];
	}
	return 0;
}

// FUNCTION: ALIEN 0x43d4c0
R_DOT* R_MAP::GetNearestDot_xy(int p_x, int p_y)
{
	int distance = 0x0fffffff;
	R_DOT* result = 0;
	for (int i = 0; i < m_list.m_n; ++i) {
		R_DOT* dot = m_list.m_data[i];
		if (dot->m_noLinks > 0) {
			int dx = dot->m_x - p_x;
			int dy = dot->m_y - dot->m_z - p_y;
			int newDistance = dy * dy + dx * dx;
			if (newDistance < distance) {
				result = dot;
				distance = newDistance;
			}
		}
	}
	return result;
}

// FUNCTION: ALIEN 0x43d550
R_DOT* R_MAP::GetNearestDot_xyr(int p_x, int p_y, int p_z)
{
	int distance = 0x0fffffff;
	R_DOT* result = 0;
	for (int i = 0; i < m_list.m_n; ++i) {
		R_DOT* dot = m_list.m_data[i];
		if (dot->m_noLinks > 0) {
			int dx = dot->m_x - p_x;
			int dy = dot->m_y - p_y;
			int dz = p_z - dot->m_z;
			int newDistance = dz * dz + dy * dy + dx * dx;
			if (newDistance < distance) {
				result = dot;
				distance = newDistance;
			}
		}
	}
	return result;
}

static inline VID* GetVidOf(SPRITE* p_sprite)
{
	return p_sprite->m_vid;
}

// FUNCTION: ALIEN 0x43d5f0
void R_MAP::PrepareForFindDot(R_DOT* p_goal, SPRITE* p_target, unsigned int p_command, ENGINE* p_eng)
{
	int weaponrange = 0;
	R_DOT::NoStep = 0xffff;
	R_DOT::NoStepForNotFound = 0xffff;
	R_DOT::MaxPathDots = 0xffff;
	R_DOT::Goal = p_goal;
	R_DOT::FindedDot = 0;
	R_DOT::eng = p_eng;
	if (p_eng) {
		p_eng->m_noPathLinks = 0;
		R_DOT::train_length_in_rails = p_eng->GetTrainLengthInRails() + 1;
		R_DOT::FindedPath = (char*) p_eng->m_pathLinks;
		R_DOT::repair_in_head = GetVidOf(p_eng->FirstEngine())->m_idx == 0x55;
		R_DOT::repair_in_tail = GetVidOf(p_eng->LastEngine())->m_idx == 0x55;
		if (p_command == 0x1c || p_command == 0x1d) {
			weaponrange = p_eng->TrainWeaponRange() - 0x96 - 20 * R_DOT::train_length_in_rails;
			if (weaponrange <= 20)
				weaponrange = 20;
		}
	}
	else {
		R_DOT::train_length_in_rails = 0;
		R_DOT::FindedPath = 0;
		R_DOT::repair_in_head = 0;
		R_DOT::repair_in_tail = 0;
	}
	R_DOT::Target = p_target;
	R_DOT::command = p_command;
	R_DOT::weaponrange = weaponrange;
	R_DOT::head_is_head = 0;
	for (int i = m_list.m_n - 1; i >= 0; i--) {
		for (int j = 0; j < m_list.m_data[i]->m_noLinks; j++) {
			m_list.m_data[i]->m_unk0x94[j] = 0xFFFFFFF;
			m_list.m_data[i]->m_unk0xac[j] = 0xFFFFFFF;
			m_list.m_data[i]->m_unk0xc4[j] = 0xFFFFFFF;
		}
	}
}
