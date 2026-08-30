#ifndef QS1_CODER_H
#define QS1_CODER_H

#include "compress/qsmodel.h"
#include "util/decomp.h"
#include "util/filter.h"

#include <stdio.h>

// VTABLE: ALIEN 0x47a5dc

class QS1_CODER : public FILTER {
public:
	int m_mode;            // 0x04
	QSMODEL m_models[256]; // 0x08

	QS1_CODER(int p_mode) { m_mode = p_mode; }

	void Reset();                                           // vtable+0x1c
	int Write(const void* p_buf, int p_size, FILE* p_file); // vtable+0x20
	int Read(void* p_buf, int p_size, FILE* p_file);        // vtable+0x24
};

// SYNTHETIC: ALIEN 0x415e50
// QS1_CODER::`scalar deleting destructor'

#endif
