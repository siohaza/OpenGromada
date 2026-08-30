#ifndef STEXT_H
#define STEXT_H

#include "sprite/frame.h"
#include "util/string.h"

class VID;

// VTABLE: ALIEN 0x47a3b0
class STEXT : public FRAME {
public:
	STEXT(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent);
	virtual void* ScalarDeletingDestructor(unsigned int p_flags);

	virtual decomp_intptr Action(int p_action, decomp_intptr p_a, decomp_intptr p_b, decomp_intptr p_c);
	virtual void Draw();

	int CalcTextProperty();

	STRING m_text;   // 0x70
	STRING m_textId; // 0x74
	int m_len;       // 0x78
	int m_flag;      // 0x7c
	int m_rows;      // 0x80
	int m_cols;      // 0x84
};

#endif
