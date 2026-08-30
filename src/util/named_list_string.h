#ifndef NAMED_LIST_STRING_H
#define NAMED_LIST_STRING_H

#include "util/decomp.h"
#include "util/named_list_string_base.h"

class NAMED_LIST_STRUCT_STRING;
class STRING;

// VTABLE: ALIEN 0x47a334

class NAMED_LIST_STRING : public NAMED_LIST_STRING_BASE {
public:
	NAMED_LIST_STRING();

	void Insert(STRING p_name, STRING p_value);
};

// SYNTHETIC: ALIEN 0x424d30
// NAMED_LIST_STRUCT_STRING::`vector deleting destructor'

#endif
