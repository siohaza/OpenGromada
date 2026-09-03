#ifndef LOGIC_H
#define LOGIC_H

#include "logic/list_logicstack.h"
#include "util/decomp.h"
#include "util/named_list_logicvar.h"
#include "util/named_list_string.h"
#include "util/string.h"

#include <cstring>
#include <utility>
#include <vector>

namespace LOGIC_BYTECODE
{

inline int ReadInt32(const char* p_data)
{
	int value;
	std::memcpy(&value, p_data, sizeof(value));
	return value;
}

inline void WriteInt32(char* p_data, int p_value)
{
	std::memcpy(p_data, &p_value, sizeof(p_value));
}

} // namespace LOGIC_BYTECODE

class SPRITE;
class STRING;
class STREAM;

// script: Owns the value stack, symbol tables, bytecode, and parser cursor.
class LOGIC {
public:
	LIST_LOGICSTACK m_stack;         // 0x00
	NAMED_LIST_LOGICVAR m_variables; // 0x10
	NAMED_LIST_STRING m_strings;     // 0x20
	STRING m_name;                   // 0x30
	// script: Compiled bytecode buffer.
	char* m_stackData; // 0x34
	// script: Current bytecode length or instruction cursor.
	int m_stackPos; // 0x38
	// script: Current source parser position.
	char* m_pos; // 0x3c
	// script: End of the source buffer.
	char* m_end;     // 0x40
	void* m_unk0x44; // 0x44
	// script: Zero based source line.
	int m_line;    // 0x48
	int m_unk0x4c; // 0x4c
	// script: Symbol-table index of the main entry.
	int m_main;    // 0x50
	int m_unk0x54; // 0x54

	std::vector<std::pair<int, int>> m_protoFixups;
	int m_inBody = 0;
	int m_declStatic = 0;
	int m_actionN[256];

	LOGIC()
	{
		for (int i = 0; i < 256; ++i) {
			m_actionN[i] = -1;
		}
		m_stackData = 0;
		m_stackPos = 0;
		m_pos = 0;
		m_end = 0;
		m_unk0x44 = 0;
		m_line = -1;
		m_unk0x4c = 0;
		m_main = -1;
		m_unk0x54 = 0;
	}

	~LOGIC() { Release(); }

	void Release();
	int LoadLGC(const STRING& p_name);
	int Load(const STRING& p_name);
	int LoadVar(STREAM* p_stream);
	int func();
	int GetActionN(int p_n);
	void SetActionN(int p_n, int p_value);
	int Error(int p_type, const char* p_word, int p_line);
	char** GetVariableStr(char** p_out, const STRING& p_name);
	int skipempty();
	int skipempty2();
	void mnog();
	void SetOperation(int p_pos, int p_op);
	void slag();
	void cmpslag();
	void logicslag();
	void vyrag();
	void vyragAnd();
	void vyragXor();
	void vyragOr();
	void vyragCmpAnd();
	int vyrag_oper();
	void oper(int* p_breakFixups);
	int GetInt();
	int GetString(char* p_out);
	void IntVar();
	void StringVar();
	int GetName(STRING* p_out);
	int GetLine(STRING* p_out);
	int Word(const char* p_word);
	int WordEnd(const char* p_word);
	int CallFunction(int p_fn, const SPRITE* p_a, const SPRITE* p_b, int p_c);
	void PushStr(const STRING& p_value);
	void PushInt(int p_value);
	void PushObject(const void* p_object);
	int SetNoElement(int p_value);
	int SaveVar(STREAM* p_stream);
};

#endif
