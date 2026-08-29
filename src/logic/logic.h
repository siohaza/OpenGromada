#ifndef LOGIC_H
#define LOGIC_H

#include "util/decomp.h"
#include "logic/list_logicstack.h"
#include "util/named_list_logicvar.h"
#include "util/named_list_string.h"
#include "util/string.h"

class SPRITE;
class STRING;
class STREAM;

// script: Owns the value stack, symbol tables, bytecode, and parser cursor.
class LOGIC {
public:
	LIST_LOGICSTACK m_stack; // 0x00
	NAMED_LIST_LOGICVAR m_variables; // 0x10
	NAMED_LIST_STRING m_strings; // 0x20
	STRING m_name; // 0x30
	// script: Compiled bytecode buffer.
	char* m_stackData; // 0x34
	// script: Current bytecode length or instruction cursor.
	int m_stackPos; // 0x38
	// script: Current source parser position.
	char* m_pos; // 0x3c
	// script: End of the source buffer.
	char* m_end; // 0x40
	void* m_unk0x44; // 0x44
	// script: Zero-based source line.
	int m_line; // 0x48
	int m_unk0x4c; // 0x4c
	// script: Symbol-table index of the main entry.
	int m_main; // 0x50
	int m_unk0x54; // 0x54

	LOGIC()
	{
		m_stackData = 0;
		m_stackPos = 0;
		m_unk0x44 = 0;
		m_line = -1;
	}

	~LOGIC() { Release(); }

	void Release();
	int LoadLGC(const STRING& p_name);
	int Load(const STRING& p_name);
	int LoadVar(STREAM* p_stream);
	int func();
	char* Error(int p_type, const char* p_word, int p_line);
	char** GetVariableStr(char** p_out, const STRING& p_name);
	int skipempty();
	int skipempty2();
	void mnog();
	void SetOperation(int p_pos, int p_op);
	void slag();
	void cmpslag();
	void logicslag();
	void vyrag();
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

DECOMP_SIZE_ASSERT(LOGIC, 0x58)

#endif
