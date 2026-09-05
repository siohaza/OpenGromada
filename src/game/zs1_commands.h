
#ifndef ZS1_COMMANDS_H
#define ZS1_COMMANDS_H

class MAP;
class STRING;

enum ZS1_CMD_RESULT {
	ZS1_CMD_INT = 0,
	ZS1_CMD_STR = 1,
	ZS1_CMD_OBJ = 2,
};

ZS1_CMD_RESULT ZS1_SendCommand2(
	MAP* p_map,
	int p_id,
	int p_var1,
	int p_var2,
	const char* p_str1,
	const char* p_str2,
	int* p_outInt,
	const void** p_outObj,
	STRING* p_outStr
);

#endif
