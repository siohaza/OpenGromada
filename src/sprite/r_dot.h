#ifndef R_DOT_H
#define R_DOT_H

#include "sprite/r_map.h"
#include "util/angle.h"
#include "util/decomp.h"

class ENGINE;
class R_DOT;
class R_POS;

class R_DOT_LINK {
public:
	R_DOT_LINK& operator=(const R_DOT_LINK& p_other)
	{
		m_dot = p_other.m_dot;
		m_dist = p_other.m_dist;
		m_backLink = p_other.m_backLink;
		m_crossLink = p_other.m_crossLink;
		AngleAssign(&m_dir, p_other.m_dir);
		return *this;
	}

	R_DOT* m_dot;            // 0x00
	int m_dist;              // 0x04
	int m_backLink;          // 0x08
	R_DOT_LINK* m_crossLink; // 0x0c
	ANGLE m_dir;             // 0x10
};

class R_DOT {
public:
	R_DOT();

	void ReleaseThunk();
	void Release();

	static int NoStep;
	static int NoStepForNotFound;
	static int MaxPathDots;
	static R_DOT* Goal;
	static class SPRITE* Target;
	static int head_is_head;
	static int repair_in_head;
	static int repair_in_tail;
	static int train_length_in_rails;
	static int command;
	static int weaponrange;
	static char CurrentPath[0x9c4];
	static R_DOT* FindedDot;
	static char* FindedPath;
	static ENGINE* eng;

	int m_refCount;        // 0x00
	undefined4 m_unk0x04;  // 0x04
	undefined4 m_unk0x08;  // 0x08
	int m_unk0x0c;         // 0x0c
	undefined4 m_unk0x10;  // 0x10
	undefined4 m_unk0x14;  // 0x14
	int m_noLinks;         // 0x18
	R_DOT_LINK m_links[6]; // 0x1c
	int m_unk0x94[6];      // 0x94
	int m_unk0xac[6];      // 0xac
	int m_unk0xc4[6];      // 0xc4
	ENGINE* m_busyEngine;  // 0xdc
	int m_x;               // 0xe0
	int m_y;               // 0xe4
	int m_z;               // 0xe8

	void SetIfIsBetter(int p_len, int p_noStep, int p_unused, int* p_out);

	int FindNewDot(int p_backLink, ANGLE p_dir);
	int FindNewDotWithoutBusyDots();
	int CanEnginePassTo(int p_link, ENGINE* p_engine);
	int GetLink_idx(const R_DOT* p_dot);

	int GetLink_dir(ANGLE p_dir);

	float SizeTo(float p_x, float p_y) const;
	int GetDistance(int p_x, int p_y, int p_z, int p_link) const;

	int GetPos(int p_x, int p_y, int p_unused, int p_link) const;
	int SetNearestPos(int p_x, int p_y, int p_z, R_POS* p_out) const;
	int TryToFindRailsForReturn(int p_depth, R_DOT* p_prev, R_DOT* p_avoid, ENGINE* p_engine, ANGLE p_dir);
	void UnLink(const R_DOT* p_dot);
	void Link(R_DOT* p_dot);
	void Link(float p_x, float p_y, float p_z);
	float GetScreenX();
	float GetScreenY();
};

inline void R_DOT::Link(float p_x, float p_y, float p_z)
{
	Link(RailMap.GetDot((int) p_x, (int) p_y, (int) p_z));
}

#endif
