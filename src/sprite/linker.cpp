#include "sprite/linker.h"

// FUNCTION: ALIEN 0x40ec60
LINKER::LINKER(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent)
	: SPRITE(p_vid, p_x, p_y, p_z, p_dir, p_parent)
{
	m_owner = p_parent;
	if (p_parent) {
		p_parent->AddLinkToLast(this);
		m_dx = p_x - p_parent->X();
		m_dy = p_y - p_parent->Y();
		m_dz = p_z - p_parent->Z();
		if (p_parent->HasMenuScriptLayout()) {
			const float scale = p_parent->UIDrawScale();
			if (scale != 1.0f) {



				m_dx /= scale;
				m_dy = (m_dy - m_dz) / scale + m_dz;
			}
		}
		AngleAssign(&m_ddir, p_dir);
	}
	else {
		m_dx = 0;
		m_dy = 0;
		m_dz = 0;
		AngleAssign(&m_ddir, p_dir);
	}
}

// FUNCTION: ALIEN 0x40ed10
void* LINKER::ScalarDeletingDestructor(unsigned int p_flags)
{
	LINKER* result = this;
	this->~LINKER();
	if (p_flags & 1) {
		operator delete(result);
	}
	return result;
}

// FUNCTION: ALIEN 0x442e50
void LINKER::LinkRotate(ANGLE p_dir)
{
	unsigned char d = p_dir.m_dir - m_ddir.m_dir;
	float xoff = ANGLE::CosTable[d] * m_dx - ANGLE::SinTable[d] * m_dy;
	d = p_dir.m_dir - m_ddir.m_dir;
	float yoff = ANGLE::SinTable2[d] * m_dx + ANGLE::CosTable2[d] * m_dy;
	SPRITE* s = m_owner;
	if (!s) {
		s = m_parent;
	}
	if (HasMenuScriptLayout()) {
		const float scale = UIDrawScale();
		if (scale != 1.0f) {
			const float dz = Z() - s->Z();
			xoff *= scale;
			yoff = scale * (yoff - dz) + dz;
		}
	}
	ChangeCoor(s->X() + xoff, s->Y() + yoff, Z());
}
