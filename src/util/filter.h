#ifndef FILTER_H
#define FILTER_H

#include "util/decomp.h"

#include <stdio.h>

// VTABLE: ALIEN 0x47a604
class FILTER {
public:
	virtual ~FILTER() {}                                                // vtable+0x00
	virtual void StartEncoding(FILE*) {}                                // vtable+0x04
	virtual int StartDecoding(FILE* p_file);                            // vtable+0x08
	virtual int EndEncoding() { return 0; }                             // vtable+0x0c
	virtual void EndDecoding() {}                                       // vtable+0x10
	virtual void EncodeByte(int) {}                                     // vtable+0x14
	virtual int DecodeByte();                                           // vtable+0x18
	virtual void Reset() {}                                             // vtable+0x1c
	virtual int Write(const void* p_buf, int p_size, FILE* p_file) = 0; // vtable+0x20
	virtual int Read(void* p_buf, int p_size, FILE* p_file) = 0;        // vtable+0x24
};

// SYNTHETIC: ALIEN 0x415e30
// FILTER::`scalar deleting destructor'

#endif
