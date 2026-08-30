#ifndef FSTREAM_H
#define FSTREAM_H

#include "util/decomp.h"
#include "util/stream.h"

#include <stdio.h>

// VTABLE: ALIEN 0x47a2f0

class FSTREAM : public STREAM {
public:
	virtual ~FSTREAM()
	{
		if (m_file) {
			fclose(m_file);
		}
	}
	virtual int Read(void* p_dest, int p_size);
	virtual int Write(const void* p_src, int p_size);

	FILE* m_file; // 0x04
};

// SYNTHETIC: ALIEN 0x4083f0
// FSTREAM::`scalar deleting destructor'

#endif
