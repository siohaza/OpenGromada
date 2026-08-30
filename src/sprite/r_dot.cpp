#include "sprite/r_dot.h"

#include "game/engine.h"
#include "game/map.h"
#include "misc.h"
#include "sprite/r_map.h"
#include "sprite/r_pos.h"
#include "util/angle.h"
#include "util/myerror.h"
#include "util/polar.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

// GLOBAL: ALIEN 0x484188
int R_DOT::NoStep = 0xffff;

// GLOBAL: ALIEN 0x48418c
int R_DOT::NoStepForNotFound = 0xffff;

// GLOBAL: ALIEN 0x5d7c34
R_DOT* R_DOT::Goal;

// GLOBAL: ALIEN 0x5d7c38
R_MAP RailMap;

// GLOBAL: ALIEN 0x5d7c58
int R_DOT::repair_in_tail;

// GLOBAL: ALIEN 0x5d7c5c
char R_DOT::CurrentPath[0x9c4];

// GLOBAL: ALIEN 0x5da36c
int R_DOT::head_is_head;

// GLOBAL: ALIEN 0x5da370
SPRITE* R_DOT::Target;

// GLOBAL: ALIEN 0x5da508
int R_DOT::repair_in_head;

// GLOBAL: ALIEN 0x5da50c
R_DOT* R_DOT::FindedDot;

// GLOBAL: ALIEN 0x5da510
int R_DOT::train_length_in_rails;

// GLOBAL: ALIEN 0x5da514
char* R_DOT::FindedPath;

// GLOBAL: ALIEN 0x5da518
ENGINE* R_DOT::eng;

// GLOBAL: ALIEN 0x5da51c
int R_DOT::command;

// GLOBAL: ALIEN 0x5da520
int R_DOT::weaponrange;

// FUNCTION: ALIEN 0x43aed0
R_DOT::R_DOT()
{
	m_unk0x04 = 0;
	m_unk0x14 = 0;
	m_unk0x10 = 0;
	m_unk0x0c = -1;
	m_refCount = 0;
	m_busyEngine = 0;
	m_noLinks = 0;
}

// FUNCTION: ALIEN 0x43aef0
void R_DOT::ReleaseThunk()
{
	Release();
}

// FUNCTION: ALIEN 0x43af00
void R_DOT::Release()
{
	if (m_refCount) {
		int refs = m_refCount - 1;
		m_refCount = refs;
		if (!refs) {
			m_busyEngine = 0;
			int i = 0;
			if (m_noLinks > 0) {
				R_DOT_LINK* link = m_links;
				do {
					link->m_dot->UnLink(this);
					++i;
					++link;
				} while (i < m_noLinks);
			}
			m_noLinks = 0;

			int n = RailMap.m_list.m_n;
			if (n) {
				R_DOT** dots = RailMap.m_list.m_data + n;
				while (n) {
					R_DOT* dot = *--dots;
					--n;
					if (dot == this) {
						if (n >= 0 && n < RailMap.m_list.m_n) {
							--RailMap.m_list.m_n;
							RailMap.m_list.m_data[n] = RailMap.m_list.m_data[RailMap.m_list.m_n];
						}
						break;
					}
				}
			}
			// The original tested `this` for null here; that is always true.
			ReleaseThunk();
			operator delete(this);
		}
	}
}

// FUNCTION: ALIEN 0x43afa0
void R_DOT::SetIfIsBetter(int p_len, int p_noStep, int p_unused, int* p_out)
{
	FindedDot = this;
	NoStep = p_noStep;
	*p_out = -1;
	if (FindedPath) {
		if (p_len < 0x9c4) {
			if (eng) {
				eng->m_noPathLinks = p_len;
			}
			memcpy(FindedPath, CurrentPath, p_len);
		}
	}
}

inline static int NearDistance(int p_dx, int p_dy)
{
	int a = abs(p_dx);
	int b = abs(p_dy);
	if (a > b) {
		return a + b / 2;
	}
	return b + a / 2;
}

float R_DOT::SizeTo(float p_x, float p_y) const
{
	float dx = (float) fabs(p_x - m_x);
	float dy = (float) fabs(p_y - m_y);
	float d;
	if (dx > dy) {
		d = dy;
		d *= 0.5f;
		d += dx;
	}
	else {
		d = dx;
		d *= 0.5f;
		d += dy;
	}
	return d;
}

// GLOBAL: ALIEN 0x5da374
int dots_num;
// GLOBAL: ALIEN 0x5da378
int dots_finded_for_return[95];

// GLOBAL: ALIEN 0x47a7e8
double g_dbl47A7E8 = 0.03;

// GLOBAL: ALIEN 0x5da524
static int cur_step;
// GLOBAL: ALIEN 0x5da528
static int cur_realstep;
// GLOBAL: ALIEN 0x5da52c
static int cur_raillength;

// STUB: ALIEN 0x43b000
int R_DOT::FindNewDot(int p_backLink, ANGLE p_dir)
{
	int savedHead = head_is_head;
	int result = -2;
	if (MaxPathDots > cur_step) {
		if (p_backLink >= 0) {
			m_unk0x94[p_backLink] = cur_step;
			m_unk0xac[p_backLink] = cur_realstep;
			m_unk0xc4[p_backLink] = cur_raillength;
		}
		if (this != Goal) {
			if (!Target || !m_busyEngine || !m_busyEngine->InTrain(Target)) {
				goto nearTarget;
			}
			if (command == 26) {
				if (p_backLink < 0) {
					goto notFound;
				}
				ENGINE* busy = m_busyEngine;
				if (busy->m_prevEngine || (busy->m_curDotRef.LinkedDot() != m_links[p_backLink].m_dot &&
										   busy->m_curDotRef.LinkedDot() != this)) {
					if (busy->m_nextEngine) {
						return -2;
					}
					if (busy->m_lastDotRef.LinkedDot() != m_links[p_backLink].m_dot &&
						busy->m_lastDotRef.LinkedDot() != this) {
						return -2;
					}
				}
			}
		}
		SetIfIsBetter(cur_step, cur_realstep, weaponrange, &result);

	nearTarget:
		if ((command == 28 || command == 29) && Target &&
			(!m_busyEngine || (eng && eng->InTrain((SPRITE*) m_busyEngine)))) {
			int dist = (int) SizeTo(Target->m_x, Target->m_y);
			if (dist <= weaponrange) {
				if (FindedDot) {
					if (FindedDot->SizeTo(Target->m_x, Target->m_y) > dist) {
						if (NoStep > cur_realstep) {
							SetIfIsBetter(cur_step, cur_realstep, weaponrange, &result);
						}
					}
					else if (NoStep > cur_realstep) {
						SetIfIsBetter(cur_step, cur_realstep, weaponrange, &result);
					}
				}
				else if (NoStep > cur_realstep) {
					SetIfIsBetter(cur_step, cur_realstep, weaponrange, &result);
				}
			}
		}

	notFound:
		if (NoStep >= 0xffff) {
			if (Goal) {
				int keep = 0;
				if (FindedDot) {
					if (NearDistance(Goal->m_x - FindedDot->m_x, Goal->m_y - FindedDot->m_y) <=
							NearDistance(Goal->m_x - m_x, Goal->m_y - m_y) &&
						(FindedDot != this || NoStepForNotFound <= cur_realstep)) {
						keep = 1;
					}
				}
				if (!keep) {
					NoStepForNotFound = cur_realstep;
					FindedDot = this;
					if (FindedPath && cur_step < 2500) {
						if (eng) {
							eng->m_noPathLinks = cur_step;
						}
						memcpy(FindedPath, CurrentPath, cur_step);
					}
					result = -1;
				}
			}
			if (Target) {
				int better = 1;
				if (FindedDot) {
					better = FindedDot->SizeTo(Target->m_x, Target->m_y) > SizeTo(Target->m_x, Target->m_y) ||
							 (FindedDot == this && NoStepForNotFound > cur_realstep);
				}
				if (better) {
					FindedDot = this;
					NoStepForNotFound = cur_realstep;
					if (FindedPath && cur_step < 2500) {
						if (eng) {
							eng->m_noPathLinks = cur_step;
						}
						memcpy(FindedPath, CurrentPath, cur_step);
					}
					result = -1;
				}
			}
		}

		{
			int canPassBack = 1;
			if (p_backLink >= 0 && eng) {
				canPassBack = m_links[p_backLink].m_dot->CanEnginePassTo(m_links[p_backLink].m_backLink, eng);
			}
			if (cur_realstep < NoStep) {
				++cur_step;
				++cur_realstep;
				for (int i = 0; i < m_noLinks; ++i) {
					if (!canPassBack) {
						if (cur_step != 1 || i != p_backLink) {
							continue;
						}
					}
					else if (i == p_backLink && cur_step >= 3) {
						continue;
					}
					R_DOT_LINK* link = &m_links[i];
					if (p_backLink >= 0 && link->m_dot->m_unk0xac[link->m_backLink] <= cur_realstep) {
						continue;
					}
					head_is_head = savedHead;
					int reversed = 0;
					int railsForReturn = 1;
					if (p_backLink >= 0) {
						unsigned char d1 = (unsigned char) (p_dir.m_dir - link->m_dir.m_dir);
						unsigned char d2 = (unsigned char) (link->m_dir.m_dir - p_dir.m_dir);
						unsigned char delta = d1 < d2 ? d1 : d2;
						if (delta > 0x1f) {
							if ((cur_step > 1 || !eng || eng->m_speed != 0.0f) && i != p_backLink) {
								if (train_length_in_rails) {
									dots_num = 0;
									railsForReturn = TryToFindRailsForReturn(
										train_length_in_rails,
										m_links[p_backLink].m_dot,
										link->m_dot,
										eng,
										p_dir
									);
								}
								reversed = 1;
							}
							head_is_head ^= 1;
						}
					}
					int dist = link->m_dist;
					cur_raillength += dist;
					if (reversed) {
						cur_realstep += train_length_in_rails;
					}
					int passable = 1;
					R_DOT_LINK* cross = link->m_crossLink;
					if (cross) {
						R_DOT* other = cross->m_dot->m_links[cross->m_backLink].m_dot;
						ENGINE* busy = cross->m_dot->m_busyEngine;
						if ((busy && fabs(busy->m_speed) < g_dbl47A7E8 && !busy->InTrain((SPRITE*) eng)) ||
							((busy = other->m_busyEngine) != 0 && fabs(busy->m_speed) < g_dbl47A7E8 &&
							 !busy->InTrain((SPRITE*) eng))) {
							passable = 0;
						}
					}
					if (railsForReturn && passable) {
						if (cur_step - 1 < 2500) {
							CurrentPath[cur_step - 1] = (char) i;
						}
						if (link->m_dot->FindNewDot(link->m_backLink, link->m_dir) >= -1) {
							result = i;
						}
					}
					cur_raillength -= dist;
					if (reversed) {
						cur_realstep -= train_length_in_rails;
					}
				}
				--cur_step;
				--cur_realstep;
			}
		}
	}
	head_is_head = savedHead;
	return result;
}

// FUNCTION: ALIEN 0x43b7d0
int R_DOT::TryToFindRailsForReturn(int p_depth, R_DOT* p_prev, R_DOT* p_avoid, ENGINE* p_engine, ANGLE p_dir)
{
	if (!p_depth) {
		return 1;
	}
	for (int i = 0; i < m_noLinks; ++i) {
		R_DOT_LINK* link = &m_links[i];
		R_DOT* dot = link->m_dot;
		if (link->m_dot != p_prev && link->m_dot != p_avoid && p_engine && CanEnginePassTo(i, p_engine) &&
			link->m_dot->CanEnginePassTo(link->m_backLink, p_engine)) {
			unsigned char linkDir = link->m_dir.m_dir;
			unsigned char d1 = (unsigned char) (p_dir.m_dir - linkDir);
			unsigned char d2 = (unsigned char) (linkDir - p_dir.m_dir);
			unsigned char delta = (d1 < d2) ? d1 : d2;
			if (delta <= 0x1f) {
				if (dots_num < 95) {
					dots_finded_for_return[dots_num] = i;
					++dots_num;
				}
				else {
					MYERROR::Error(
						::Error,
						// STRING: ALIEN 0x484190
						"R_DOT %i,%i",
						10,
						// STRING: ALIEN 0x48419c
						"dots_num is large",
						dots_num,
						m_x,
						m_y
					);
				}
				if (link->m_dot->TryToFindRailsForReturn(p_depth - 1, this, p_avoid, p_engine, link->m_dir)) {
					return 1;
				}
				--dots_num;
			}
		}
	}
	return 0;
}

// FUNCTION: ALIEN 0x43bb90
int R_DOT::CanEnginePassTo(int p_link, ENGINE* p_engine)
{
	if (p_link < 0 || p_link >= m_noLinks) {
		return 0;
	}
	ENGINE* busy;
	if (p_engine &&
		((m_unk0x14 == ((p_engine->m_flag >> 11) & 3) + 4 && m_links[p_link].m_dot->m_unk0x14 == m_unk0x14) ||
		 ((busy = m_links[p_link].m_dot->m_busyEngine) != 0 && !p_engine->InTrain((SPRITE*) busy) &&
		  ((R_DOT::command == 26 && busy->InTrain(R_DOT::Target)) ||
		   ((fabs(busy->m_speed) < g_dbl47A7E8 || busy->m_curDotRef.LinkedDot() == this) &&
			(!busy->m_goal || busy->m_goal != p_engine->m_goal) &&
			(!busy->m_commandDot || busy->m_commandDot != p_engine->m_commandDot)))))) {
		return 0;
	}
	return m_links[p_link].m_dot->m_unk0x0c != m_links[p_link].m_backLink;
}

// FUNCTION: ALIEN 0x43bc90
void R_DOT::UnLink(const R_DOT* p_dot)
{
	int n = m_noLinks;
	for (int i = 0; i < n; i++) {
		if (m_links[i].m_dot == p_dot) {
			if (m_unk0x0c == i) {
				m_unk0x0c = -1;
			}
			--n;
			m_noLinks = n;
			m_links[i] = m_links[n];
			m_links[i].m_dot->m_links[m_links[i].m_backLink].m_backLink = i;
			if (m_unk0x0c == m_noLinks) {
				m_unk0x0c = i;
			}
			return;
		}
	}
}

// FUNCTION: ALIEN 0x43bd20
void R_DOT::Link(R_DOT* p_dot)
{
	if (!p_dot) {
		return;
	}
	if (GetLink_idx(p_dot) >= 0) {
		return;
	}
	R_DOT_LINK link[1];
	AngleAssign(&link[0].m_dir, Decart2Polar(p_dot->m_x - m_x, p_dot->m_y - m_y, 0));
	int dy = m_x - p_dot->m_x;
	int dx = m_y - p_dot->m_y;
	int dist = Sqrt(dx * dx * 9 / 4 + dy * dy);
	int idx = p_dot->m_noLinks;
	if (m_noLinks < 6) {
		R_DOT_LINK* l = &m_links[m_noLinks];
		m_noLinks++;
		l->m_dot = p_dot;
		l->m_dist = dist;
		l->m_backLink = idx;
		l->m_crossLink = 0;
		AngleAssign(&l->m_dir, link[0].m_dir);
	}
	else {
		MYERROR::Log(
			::Error,
			// STRING: ALIEN 0x4841e0
			"!!!ERROR!!!R_DOT: Too many links in %i,%i,%i",
			m_x,
			m_y,
			m_z
		);
	}
	link[0].m_dir.m_dir += 0x80;
	int myIdx = m_noLinks - 1;
	if (p_dot->m_noLinks < 6) {
		R_DOT_LINK* l = &p_dot->m_links[p_dot->m_noLinks];
		p_dot->m_noLinks++;
		l->m_dot = this;
		l->m_dist = dist;
		l->m_backLink = myIdx;
		l->m_crossLink = 0;
		AngleAssign(&l->m_dir, link[0].m_dir);
	}
	else {
		MYERROR::Log(
			::Error,
			// STRING: ALIEN 0x4841b0
			"!!!ERROR!!!R_DOT: Too many links2 in %i,%i,%i",
			p_dot->m_x,
			p_dot->m_y,
			p_dot->m_z
		);
	}
}

// FUNCTION: ALIEN 0x43be90
int R_DOT::SetNearestPos(int p_x, int p_y, int p_z, R_POS* p_out) const
{
	int result = m_noLinks;
	if (result) {
		int best = 0xffff;
		for (int i = 0; i < m_noLinks; ++i) {
			R_DOT* dot = m_links[i].m_dot;
			for (int sub = 0; sub < m_links[i].m_dot->m_noLinks; ++sub) {
				int d = m_links[i].m_dot->GetDistance(p_x, p_y, p_z, sub);
				if (d < best) {
					best = d;
					p_out->m_dot = m_links[i].m_dot;
					p_out->m_link = sub;
				}
			}
		}

		R_DOT* winner = p_out->m_dot;
		int pos = p_out->m_dot->GetPos(p_x, p_y, p_z, p_out->m_link);
		p_out->m_pos = pos;
		if (pos < 0) {
			p_out->m_pos = 0;
		}
		// The original returned the dot pointer here as a truthy int; both
		// callers ignore the result.
		result = p_out->m_dot ? 1 : 0;
		int cap = p_out->m_dot ? p_out->m_dot->m_links[p_out->m_link].m_dist : 0;
		if (p_out->m_pos >= cap) {
			if (p_out->m_dot) {
				result = p_out->m_dot->m_links[p_out->m_link].m_dist;
			}
			else {
				result = 0;
			}
			--result;
			p_out->m_pos = result;
		}
	}
	return result;
}

// STUB: ALIEN 0x43bf80
int R_DOT::GetDistance(int p_x, int p_y, int p_z, int p_link) const
{
	R_DOT* dot = m_links[p_link].m_dot;
	int pos = GetPos(p_x, p_y, p_z, p_link);
	if (pos < 0 || pos >= m_links[p_link].m_dist) {
		int dz = (dot->m_z + m_z) / 2 - p_z + 4;
		int dy = (dot->m_y + m_y) / 2 - p_y;
		int dx = (dot->m_x + m_x) / 2 - p_x;
		return Sqrt(dz * dz + dy * dy + dx * dx);
	}
	else {
		int x = m_x;
		int y = m_y;
		int dpy = p_y - y;
		int dy2 = dot->m_y - y;
		int dx2 = dot->m_x - x;
		int z = m_z;
		int dz2 = dot->m_z - z;
		int cz = dy2 * (p_x - x) - dx2 * dpy;
		int cx = dz2 * dpy - dy2 * (p_z - z);
		int cy = dx2 * (p_z - z) - dz2 * (p_x - x);
		int len =
			Sqrt((x - dot->m_x) * (x - dot->m_x) + (y - dot->m_y) * (y - dot->m_y) + (z - dot->m_z) * (z - dot->m_z));
		return Sqrt(cy * cy + cx * cx + cz * cz) / len;
	}
}

// GLOBAL: ALIEN 0x5da530
static int s_findNewDotWithoutBusyDotsStep;

// FUNCTION: ALIEN 0x43c120
int R_DOT::FindNewDotWithoutBusyDots()
{
	int result = -2;
	if (m_unk0x94[0] > s_findNewDotWithoutBusyDotsStep) {
		m_unk0x94[0] = s_findNewDotWithoutBusyDotsStep;
		if (this == Goal || (Target && m_busyEngine && m_busyEngine->InTrain(Target))) {
			FindedDot = this;
			NoStep = s_findNewDotWithoutBusyDotsStep;
			return -1;
		}
		{
			if (m_busyEngine && NoStep >= 0xffff) {
				if (Goal) {
					if (FindedDot) {
						int a = abs(Goal->m_x - FindedDot->m_x);
						int b = abs(Goal->m_y - FindedDot->m_y);
						int d1 = a > b ? b / 2 + a : a / 2 + b;
						int c = abs(Goal->m_x - m_x);
						int d = abs(Goal->m_y - m_y);
						int d2 = c > d ? d / 2 + c : c / 2 + d;
						if (d1 > d2) {
							FindedDot = this;
							result = -1;
						}
					}
					else {
						FindedDot = this;
						result = -1;
					}
				}
				else if (Target) {
					if (FindedDot) {
						float tx = Target->m_x;
						float a = (float) fabs(tx - FindedDot->m_x);
						float ty = Target->m_y;
						float b = (float) fabs(ty - FindedDot->m_y);
						float d1;
						if (a > b) {
							d1 = b;
							d1 *= 0.5f;
							d1 += a;
						}
						else {
							d1 = a;
							d1 *= 0.5f;
							d1 += b;
						}
						float ux = Target->m_x;
						float c = (float) fabs(ux - m_x);
						float uy = Target->m_y;
						float d = (float) fabs(uy - m_y);
						float d2;
						if (c > d) {
							d2 = d;
							d2 *= 0.5f;
							d2 += c;
						}
						else {
							d2 = c;
							d2 *= 0.5f;
							d2 += d;
						}
						if (d1 > d2) {
							FindedDot = this;
							result = -1;
						}
					}
					else {
						FindedDot = this;
						result = -1;
					}
				}
			}
			if (s_findNewDotWithoutBusyDotsStep < NoStep) {
				s_findNewDotWithoutBusyDotsStep = s_findNewDotWithoutBusyDotsStep + 1;
				for (int i = 0; i < m_noLinks; i++) {
					if (m_links[i].m_dot->FindNewDotWithoutBusyDots() >= -1) {
						result = i;
					}
				}
				s_findNewDotWithoutBusyDotsStep--;
			}
		}
	}
	return result;
}

// FUNCTION: ALIEN 0x43c360
int R_DOT::GetLink_idx(const R_DOT* p_dot)
{
	for (int i = 0; i < m_noLinks; i++) {
		if (m_links[i].m_dot == p_dot) {
			return i;
		}
	}
	return -1;
}

// STUB: ALIEN 0x43c390
int R_DOT::GetLink_dir(ANGLE p_dir)
{
	int best = 0;
	unsigned char dir = p_dir.m_dir;
	for (int i = 1; i < m_noLinks; i++) {
		unsigned char a = dir - m_links[best].m_dir.m_dir;
		unsigned char b = m_links[best].m_dir.m_dir - dir;
		if (a >= b) {
			a = b;
		}
		unsigned char c = dir - m_links[i].m_dir.m_dir;
		unsigned char d = m_links[i].m_dir.m_dir - dir;
		if (c >= d) {
			c = d;
		}
		if (c < a) {
			best = i;
		}
	}
	return best;
}

// FUNCTION: ALIEN 0x43c410
int R_DOT::GetPos(int p_x, int p_y, int p_unused, int p_link) const
{
	unsigned char dir = m_links[p_link].m_dir.m_dir - 0x40;
	return (int) ((p_y - m_y) * 3 * ANGLE::SinTable[dir] * 0.5f + (p_x - m_x) * ANGLE::CosTable[dir]);
}

// FUNCTION: ALIEN 0x43d290
float R_DOT::GetScreenX()
{
	return m_x - Map->m_shiftX;
}

// FUNCTION: ALIEN 0x43d2a0
float R_DOT::GetScreenY()
{
	return (m_y - m_z) - Map->m_shiftY;
}
