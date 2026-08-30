#include "world/hash_map.h"

#include "game/map.h"
#include "sprite/sprite.h"
#include "ui/mouse.h"
#include "util/myerror.h"
#include "video/vid.h"

#include <math.h>

// GLOBAL: ALIEN 0x492b6c
HASH_MAP* Hash;

// FUNCTION: ALIEN 0x42d380
HASH_MAP::HASH_MAP(float p_w, float p_h, VID** p_vids, int p_noVid)
{
	m_iter = 0;
	m_curIdx = 0;
	float maxW = 0.0f;
	float maxH = 0.0f;
	for (int i = 0; i < p_noVid; ++i) {
		VID* vid = p_vids[i];
		if (vid && (vid->m_flag & 0x40)) {
			if (vid->m_footprintWidth > maxW) {
				maxW = vid->m_footprintWidth;
			}
			if (maxH < vid->m_footprintHeight) {
				maxH = vid->m_footprintHeight;
			}
		}
	}
	maxW = maxW * 0.5f;
	maxH = maxH * 0.5f;
	int shiftX = 0;
	if (1.0f < maxW) {
		do {
			++shiftX;
		} while ((1 << shiftX) < maxW);
	}
	int shiftY = 0;
	if (1.0f < maxH) {
		do {
			++shiftY;
		} while ((1 << shiftY) < maxH);
	}
	m_cellW = 1.0f / (1 << shiftX);
	m_cellH = 1.0f / (1 << shiftY);
	m_maxY = (int) ((p_h - 1.0f) * m_cellH + 3.0f);
	m_shift = 0;

	float cols;
	(void) cols;
	if (1.0f < m_cellH * (p_w - 1.0f) + 1.0f) {
		do {
			++m_shift;
		} while ((1 << m_shift) < m_cellH * (p_w - 1.0f) + 1.0f);
	}
	m_maxX = 1 << m_shift;
	m_cells = new SPRITE_LIST[m_maxX * m_maxY];
	if (!m_cells) {
		MYERROR::LogExit(
			::Error,
			// STRING: ALIEN 0x483a78
			"!!!ERROR!!!HASH_MAP: Enough memory %i,%i",
			m_maxX,
			m_maxY
		);
	}
}

// FUNCTION: ALIEN 0x42d590
void* SPRITE_LIST::ScalarDeletingDestructor(unsigned int p_flags)
{
	SPRITE_LIST* result = this;
	this->~SPRITE_LIST();
	if (p_flags & 1) {
		operator delete(result);
	}
	return result;
}

// FUNCTION: ALIEN 0x42d630
HASH_MAP::~HASH_MAP()
{
	m_list.Release();
	if (m_cells) {
		for (int i = m_maxY * m_maxX - 1; i >= 0; --i) {
			m_cells[i].Release();
		}
		delete[] m_cells;
		m_cells = 0;
	}
}

// FUNCTION: ALIEN 0x42d6a0
void HASH_MAP::Insert(SPRITE* p_sprite)
{
	if (p_sprite->m_vid->m_flag & 0x40) {
		float fx = p_sprite->m_x * m_cellW;
		int cx;
		if (fx < 0.0f) {
			cx = 0;
		}
		else if (m_maxX <= fx) {
			cx = m_maxX - 1;
		}
		else {
			cx = (int) fx;
		}
		float fy = p_sprite->m_y * m_cellH;
		int cy;
		if (fy < 0.0f) {
			cy = 0;
		}
		else if (m_maxY <= fy) {
			cy = m_maxY - 1;
		}
		else {
			cy = (int) fy;
		}
		m_cells[cx + (cy << m_shift)].Insert(p_sprite);
	}
	if (p_sprite->m_vid->m_unk0x0c & 0xc) {
		m_list.Insert(p_sprite);
	}
}

// FUNCTION: ALIEN 0x42d770
int HASH_MAP::Delete(SPRITE* p_sprite)
{
	int result = 0;
	if (m_cells && (p_sprite->m_vid->m_flag & 0x40)) {
		float v = p_sprite->m_x * m_cellW;
		if (v >= 0.0f) {
			result = m_maxX;
			if ((float) m_maxX <= v) {
				result--;
			}
			else {
				result = (int) v;
			}
		}
		int y;
		v = p_sprite->m_y * m_cellH;
		if (v < 0.0f) {
			y = 0;
		}
		else if ((float) m_maxY <= v) {
			y = m_maxY - 1;
		}
		else {
			y = (int) v;
		}
		if (result == m_curX && y == m_y0 && m_curIdx > 0 && m_curIdx < m_cells[(y << m_shift) + result].m_n &&
			m_cells[(y << m_shift) + result].m_data[m_curIdx - 1] == p_sprite) {
			m_curIdx = m_curIdx - 1;
		}
		result = m_cells[(y << m_shift) + result].Delete(p_sprite);
	}
	if (p_sprite->m_vid->m_unk0x0c & 0xc) {
		result |= 2 * m_list.Delete(p_sprite);
	}
	if (result && p_sprite != Mouse && p_sprite != Mouse->m_child) {
		VID* vid = p_sprite->m_vid;
		MYERROR::Error(
			::Error,
			"SPRITE %i",
			10,
			// STRING: ALIEN 0x483aa4
			"hash can't delete",
			result,
			vid ? vid->m_idx : -1
		);
	}
	return result;
}

// FUNCTION: ALIEN 0x42d8c0
void HASH_MAP::ChangeCoor(SPRITE* p_sprite, float p_x, float p_y)
{
	int oldX;
	float v = p_sprite->m_x * m_cellW;
	if (v < 0.0f) {
		oldX = 0;
	}
	else if ((float) m_maxX <= v) {
		oldX = m_maxX - 1;
	}
	else {
		oldX = (int) v;
	}
	int oldY;
	v = p_sprite->m_y * m_cellH;
	if (v < 0.0f) {
		oldY = 0;
	}
	else if ((float) m_maxY <= v) {
		oldY = m_maxY - 1;
	}
	else {
		oldY = (int) v;
	}
	int newX;
	float w = p_x * m_cellW;
	if (w < 0.0f) {
		newX = 0;
	}
	else if ((float) m_maxX <= w) {
		newX = m_maxX - 1;
	}
	else {
		newX = (int) w;
	}
	int newY;
	w = p_y * m_cellH;
	if (w < 0.0f) {
		newY = 0;
	}
	else if ((float) m_maxY <= w) {
		newY = m_maxY - 1;
	}
	else {
		newY = (int) w;
	}
	if ((oldX != newX || oldY != newY) && !m_cells[(oldY << m_shift) + oldX].Delete(p_sprite)) {
		m_cells[(newY << m_shift) + newX].Insert(p_sprite);
	}
}

// FUNCTION: ALIEN 0x42da30
// Returns the blocking sprite, or null when the position is free. The
// global Mouse is used as the sentinel for "blocked by terrain".
SPRITE* HASH_MAP::CanPlace(VID* p_vid, float p_x, float p_y, float p_z)
{
	if (Map->GetGroundZ_vid(p_vid, p_x, p_y) > p_z) {
		return Mouse;
	}
	if (!p_vid->m_unk0x18) {
		return 0;
	}
	SPRITE* s =
		FirstInBox(p_x - p_vid->m_unk0x384, p_y - p_vid->m_unk0x388, p_x + p_vid->m_unk0x384, p_y + p_vid->m_unk0x388);
	while (s) {
		if (s->m_ani < 0xf) {
			VID* v = s->m_vid;
			if ((float) fabs(s->m_x - p_x) < v->m_unk0x384 + p_vid->m_unk0x384 &&
				(float) fabs(s->m_y - p_y) < v->m_unk0x388 + p_vid->m_unk0x388 && v->m_unk0x24 + s->m_z >= p_z &&
				p_z + p_vid->m_unk0x24 >= s->m_z && (v->m_unk0x18 & p_vid->m_unk0x18)) {
				return s;
			}
		}
		s = NextInBox();
	}
	return 0;
}

// FUNCTION: ALIEN 0x42db40
int HASH_MAP::AskLine(VID* p_vid, float p_x, float p_y, float p_z, float* p_lx, float* p_ly, float* p_lz)
{
	if (!p_vid || !p_vid->m_unk0x18) {
		return 0;
	}
	int major = (int) p_x;
	int minor = (int) p_y;
	int iz = (int) p_z;
	unsigned int dMajor = abs((int) *p_lx - (int) p_x);
	unsigned int dMinor = abs((int) *p_ly - (int) p_y);
	int steep = 0;
	int stepMajor = (*p_lx > p_x) ? 1 : -1;
	int stepMinor = (*p_ly > p_y) ? 1 : -1;
	if (dMinor > dMajor) {
		steep = 1;
		major ^= minor;
		minor ^= major;
		major ^= minor;
		dMajor ^= dMinor;
		dMinor ^= dMajor;
		dMajor ^= dMinor;
		stepMajor ^= stepMinor;
		stepMinor ^= stepMajor;
		stepMajor ^= stepMinor;
	}
	int twoDMinor = 2 * (int) dMinor;
	int err = twoDMinor - (int) dMajor;
	int dz = dMajor ? ((((int) *p_lz - iz) << 4) / (int) dMajor) : 0;
	for (unsigned int i = 0; i < dMajor; ++i) {
		if ((i & 0xf) == 0 && i > 0) {
			iz += dz;
			if (steep) {
				if (CanPlace(p_vid, (float) minor, (float) major, (float) iz)) {
					*p_lx = (float) minor;
					*p_ly = (float) major;
					*p_lz = (float) iz;
					return 1;
				}
			}
			else if (CanPlace(p_vid, (float) major, (float) minor, (float) iz)) {
				*p_lx = (float) major;
				*p_ly = (float) minor;
				*p_lz = (float) iz;
				return 1;
			}
		}
		while (err >= 0) {
			minor += stepMinor;
			err -= 2 * (int) dMajor;
		}
		major += stepMajor;
		err += twoDMinor;
	}
	return 0;
}

// FUNCTION: ALIEN 0x42dd60
SPRITE* HASH_MAP::FirstInBox(float p_left, float p_top, float p_right, float p_bot)
{
	float invW = 1.0f / m_cellW;
	int x0;
	float v = (p_left - invW) * m_cellW;
	if (v < 0.0f) {
		x0 = 0;
	}
	else if ((float) m_maxX <= v) {
		x0 = m_maxX - 1;
	}
	else {
		x0 = (int) v;
	}
	float invH = 1.0f / m_cellH;
	m_x0 = x0;
	int y0;
	v = (p_top - invH) * m_cellH;
	if (v < 0.0f) {
		y0 = 0;
	}
	else if ((float) m_maxY <= v) {
		y0 = m_maxY - 1;
	}
	else {
		y0 = (int) v;
	}
	m_y0 = y0;
	int x1;
	v = (invW + p_right) * m_cellW;
	if (v < 0.0f) {
		x1 = 0;
	}
	else if ((float) m_maxX <= v) {
		x1 = m_maxX - 1;
	}
	else {
		x1 = (int) v;
	}
	m_x1 = x1;
	int y1;
	v = (invH + p_bot) * m_cellH;
	if (v < 0.0f) {
		y1 = 0;
	}
	else if ((float) m_maxY <= v) {
		y1 = m_maxY - 1;
	}
	else {
		y1 = (int) v;
	}
	m_y1 = y1;
	m_curX = x0;
	m_curIdx = 0;
	return NextInBox();
}

// FUNCTION: ALIEN 0x42deb0
SPRITE* HASH_MAP::NextInBox()
{
	while (m_y0 <= m_y1) {
		while (m_curX <= m_x1) {
			if (m_curIdx >= m_cells[(m_y0 << m_shift) + m_curX].m_n) {
				m_curX++;
				m_curIdx = 0;
			}
			else {
				int i = m_curIdx;
				m_curIdx = i + 1;
				return (SPRITE*) m_cells[(m_y0 << m_shift) + m_curX].m_data[i];
			}
		}
		m_y0++;
		m_curX = m_x0;
		m_curIdx = 0;
	}
	return 0;
}
