#include "game/rail.h"

#include "sprite/r_dot.h"
#include "sprite/r_map.h"
#include "video/vid.h"

// GLOBAL: ALIEN 0x484840
static const float x1[12] = {0.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f,
	1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f};
// GLOBAL: ALIEN 0x484870
static const float y1[12] = {-1.0f, 0.0f, -1.0f, -1.0f, -1.0f, -1.0f,
	1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f};
// GLOBAL: ALIEN 0x4848d0
static const float x2[12] = {0.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, -1.0f, -1.0f, 1.0f, 1.0f};
// GLOBAL: ALIEN 0x484900
static const float y2[12] = {1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	-1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f};

// FUNCTION: ALIEN 0x44a140
RAIL::RAIL(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent)
	: TERRAIN(p_vid, p_x, p_y, p_z, p_dir, p_parent)
{
	m_dot2 = 0;
	m_dot1 = 0;
	Action(60, p_dir.m_dir, 0, 0);
}

// FUNCTION: ALIEN 0x44a1a0
void* RAIL::ScalarDeletingDestructor(unsigned int p_flags)
{
	RAIL* result = this;
	this->~RAIL();
	if (p_flags & 1)
		operator delete(result);
	return result;
}

// FUNCTION: ALIEN 0x44a1c0
RAIL::~RAIL()
{
	if (m_dot1)
		m_dot1->Release();
	if (m_dot2)
		m_dot2->Release();
}

// FUNCTION: ALIEN 0x44a1f0
void RAIL::UnBreak(R_DOT* p_dot)
{
	if (m_dot1 == p_dot || m_dot2 == p_dot) {
		VID* vid = m_vid;
		int v = vid->m_unk0x20c[15];
		if (v && v < vid->m_idx) {
			Action(62, v, 0, 0);
			ChangeHp(m_vid->m_maxHp[(m_flag >> 11) & 3]);
		}
	}
}

// STUB: ALIEN 0x44a240
int RAIL::Action(int p_action, int p_dir, int p_a, int p_b)
{

	// GLOBAL: ALIEN 0x5da558
	static float z1[4] = {0.0f, m_vid->m_unk0x24, 0.0f, 0.0f};
	// GLOBAL: ALIEN 0x5da568
	static float z2[4] = {0.0f, 0.0f, m_vid->m_unk0x24, 0.0f};
	float altitude = m_vid->m_unk0x60;

	if (p_action != 60) {
		if (p_action != 85)
			return TERRAIN::Action(p_action, p_dir, p_a, p_b);

		SPRITE::Action(85, p_dir, p_a, p_b);
		if (m_ani >= 15) {
			VID* vid = m_vid;
			VID* childVid = (VID*) vid->m_aniChildVid[15];
			if (childVid && childVid->m_sprClass == 22) {
				if (vid->m_unk0x20c[15] > vid->m_idx) {
					m_dot1->m_unk0x04 = 1;
					m_dot2->m_unk0x04 = 1;
				}
				Action(62, m_vid->m_unk0x20c[15], 0, 0);
				ChangeHp(m_vid->m_maxHp[(m_flag >> 11) & 3]);
			}
		}
		return 0;
	}

	ChangeDirection(ANGLE((char) p_dir));
	if (m_dot1)
		m_dot1->Release();
	if (m_dot2)
		m_dot2->Release();

	unsigned int direction = m_vid->RealDirection(ANGLE(m_dir));

	m_dot1 = RailMap.CreateDot(m_x + m_vid->m_footprintWidth * x1[direction % 12] * 0.25f,
		m_y + m_vid->m_footprintHeight * y1[direction % 12] * 0.25f, m_z + altitude + z1[direction / 12]);
	m_dot2 = RailMap.CreateDot(m_x + m_vid->m_footprintWidth * x2[direction % 12] * 0.25f,
		m_y + m_vid->m_footprintHeight * y2[direction % 12] * 0.25f, m_z + altitude + z2[direction / 12]);

	m_dot1->Link(m_x + m_vid->m_footprintWidth * x1[direction % 12] * 0.75f,
		m_y + m_vid->m_footprintHeight * y1[direction % 12] * 0.75f, m_z + altitude + z1[direction / 12]);
	m_dot2->Link(m_x + m_vid->m_footprintWidth * x2[direction % 12] * 0.75f,
		m_y + m_vid->m_footprintHeight * y2[direction % 12] * 0.75f, m_z + altitude + z2[direction / 12]);
	m_dot2->Link(m_dot1);

	if (m_vid->m_unk0x20c[15] < m_vid->m_idx) {
		m_dot1->m_unk0x04 = 1;
		m_dot2->m_unk0x04 = 1;
	}
	return 0;
}
