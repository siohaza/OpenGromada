#ifndef STREAM_H
#define STREAM_H

// VTABLE: ALIEN 0x47a2e4

class STREAM {
public:
	virtual ~STREAM() {} // vtable+0x00
	virtual int Read(void* p_dest, int p_size) = 0; // vtable+0x04
	virtual int Write(const void* p_src, int p_size) = 0; // vtable+0x08
};

// SYNTHETIC: ALIEN 0x4077a0
// STREAM::`scalar deleting destructor'

#endif
