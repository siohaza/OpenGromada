#ifndef LOGICSTACK_H
#define LOGICSTACK_H

#include "util/decomp.h"
#include "util/string.h"

#include <stdint.h>
#include <stdlib.h>

// script: A VM stack value tagged as an integer, string, or object.
class LOGICSTACK {
public:
	unsigned char m_type;   // 0x00
	undefined m_unk0x01[3]; // 0x01
	intptr_t m_num;         // 0x04
	STRING m_str;           // 0x08

	LOGICSTACK();
	~LOGICSTACK();
	LOGICSTACK(const LOGICSTACK& p_other);
	LOGICSTACK(int p_value);
	LOGICSTACK(const STRING& p_value);
	LOGICSTACK(const void* p_object);
	LOGICSTACK& operator=(const LOGICSTACK& p_other);
	void BinarOperator(int p_operation, const LOGICSTACK& p_other);
	void Inc();
	void Dec();
	int Int();
	decomp_intptr Value() const;
	void AssignValue(const LOGICSTACK& p_source);
	STRING* String();
	char Read(STREAM* p_stream);
	char Write(STREAM* p_stream) const;
};

// SYNTHETIC: ALIEN 0x4041e0
// LOGICSTACK::`vector deleting destructor'

#endif
