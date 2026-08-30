#ifndef PICTURE_H
#define PICTURE_H

#include "gfx/picture_base.h"
#include "util/decomp.h"

// VTABLE: ALIEN 0x47a530

class PICTURE {
public:
	PICTURE();
	PICTURE(int p_w, int p_h, int p_format);
	virtual ~PICTURE()
	{
		if (m_impl) {
			m_impl->ScalarDeletingDestructor(1);
		}
	}

	// FUNCTION: ALIEN 0x412660
	virtual int NextFrame() { return m_impl->NextFrame(); }
	// FUNCTION: ALIEN 0x412670
	virtual void Rewind() { m_impl->Rewind(); }
	virtual int Load(const char** p_name);

	void PutPixel(int p_x, int p_y, COLOR p_color);

	PICTURE_BASE* m_impl; // 0x04
	int m_format;         // 0x08
};

// SYNTHETIC: ALIEN 0x412680
// PICTURE::`scalar deleting destructor'

#endif
