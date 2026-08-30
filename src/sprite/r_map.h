#ifndef R_MAP_H
#define R_MAP_H

#include "util/decomp.h"

class ENGINE;
class R_DOT;
class SPRITE;

// VTABLE: ALIEN 0x47a800

class LIST_RDOT {
public:
	LIST_RDOT() : m_data(0)
	{
		m_n = 0;
		m_max = 0;
	}
	virtual ~LIST_RDOT()
	{
		if (m_data) {
			operator delete(m_data);
		}
		m_data = 0;
		m_n = 0;
	}

	int m_n;        // 0x04
	int m_max;      // 0x08
	R_DOT** m_data; // 0x0c
};

// SYNTHETIC: ALIEN 0x43da40
// LIST_RDOT::`scalar deleting destructor'

class R_MAP {
public:
	R_MAP();
	~R_MAP();

	int m_unk0x00;    // 0x00
	int m_unk0x04;    // 0x04
	int m_unk0x08;    // 0x08
	int m_unk0x0c;    // 0x0c
	LIST_RDOT m_list; // 0x10

	R_DOT* GetDot(int p_x, int p_y, int p_z);
	R_DOT* CreateDot(float p_x, float p_y, float p_z);
	R_DOT* GetNearestDot_xy(int p_x, int p_y);
	R_DOT* GetNearestDot_xyr(int p_x, int p_y, int p_z);

	void CreateIntersectedDot(R_DOT* p_a, R_DOT* p_b, R_DOT* p_c, R_DOT* p_d);
	void PrepareForFindDot(R_DOT* p_goal, SPRITE* p_target, unsigned int p_command, ENGINE* p_eng);
	int AddDotToArray(int p_x, int p_y, int p_w, int p_h, int p_dot);
	R_DOT* SetSemaphoreOrMine(int p_x, int p_y, int p_value, int p_d);
	void CreateAdditionalDots();

	void SetPushLine(int p_x1, int p_y1, int p_x2, int p_y2, int p_value);
	int DebugDraw();
};

extern R_MAP RailMap;

#endif
