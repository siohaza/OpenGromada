#ifndef RESOURCE_H
#define RESOURCE_H

#include "util/decomp.h"
#include "util/stream.h"
#include "util/string.h"

#include <stdio.h>

class FILTER;

// VTABLE: ALIEN 0x47a2d8

class RESOURCE : public STREAM {
public:
	RESOURCE();
	virtual ~RESOURCE();                              // vtable+0x00
	virtual int Read(void* p_buf, int p_size);        // vtable+0x04
	virtual int Write(const void* p_buf, int p_size); // vtable+0x08

	int m_flag;      // 0x04
	int m_state;     // 0x08
	STRING m_name;   // 0x0c
	int m_signature; // 0x10
	int m_resSize;   // 0x14
	int m_resPos;    // 0x18
	int m_begin;     // 0x1c
	int m_end;       // 0x20
	int m_subFlags;  // 0x24
	int m_noSubRes;  // 0x28
	int m_subPos;    // 0x2c
	int m_subSize;   // 0x30
	int m_packedPos; // 0x34
	FILE* m_file;    // 0x38
	int m_type;      // 0x3c


	bool m_readOnly;
	bool m_failed;
	int m_readBegin;
	int m_readEnd;
	int m_recordIndex;
	int m_fileEnd;
	int m_containerType = 0;

	bool Good() const { return !m_failed; }
	int Remaining() const;
	int ReadWords(void* p_buf, int p_size, int p_wordSize = 4);
	bool ReadString(STRING& p_string);
	bool Skip(int p_size);
	bool RequireEnd();
	bool Fail(const char* p_reason);

	STRING* Close();
	int Open(FILE* p_file, unsigned int p_type);
	int OpenForRead(const STRING& p_name, unsigned int p_type);
	int OpenForWrite(const STRING& p_name, unsigned int p_type);
	int Load(unsigned int p_type, void** p_out, int p_size);
	int SubLoad(void** p_out, FILTER* p_filter);
	int ReadPacked(void* p_buf, unsigned int p_size, FILTER* p_filter);
	int WritePacked(const void* p_buf, unsigned int p_size, FILTER* p_filter);
	int Append(RESOURCE* p_src, unsigned int p_type);
	int PreAppend(unsigned int p_sig, FILTER* p_filter);
	int PostAppend();
	int GoBegin(int p_type);
	int GoNext(int p_type);
	int GoNextSub(int p_type);
	int GetNoSubRes(int p_type);
};

// SYNTHETIC: ALIEN 0x407760
// RESOURCE::`scalar deleting destructor'

#endif
