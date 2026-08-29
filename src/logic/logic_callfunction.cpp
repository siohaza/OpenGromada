
#define DECOMP_LOGICSTACK_STRING_MEMBER
#define DECOMP_INLINE_LOGICSTACK_DEFAULT_CTOR
#define DECOMP_INLINE_LOGICSTACK_ASSIGN
#define DECOMP_INLINE_LOGICSTACK_INT
#define DECOMP_INLINE_LOGICSTACK_INCDEC
#define DECOMP_INLINE_STRING_INT
#define DECOMP_INLINE_LOGICSTACK_OBJECT_CTOR
#define DECOMP_INLINE_LOGICSTACK_STRING
#include "logic/logic.h"

#include "logic/logicstack.h"
#include "logic/logicvar.h"
#include "util/myerror.h"
#include "util/named_list_struct_logicvar.h"
#include "util/string.h"

#include <stdlib.h>
#include <string.h>

class VID;
VID** ScriptExecFunc(int p_cmd);

// script: The VM stack stores the caller frame base and return instruction pointer as sentinels.
// STUB: ALIEN 0x423430
int LOGIC::CallFunction(int p_fn, const SPRITE* p_a, const SPRITE* p_b, int p_c)
{
	int result = 0;
	int indexActive = 0;
	int arrayIndex = 0;

	if (!m_stackData)
		return 0;

	int function = p_fn < 0 ? m_main : p_fn;
	if (function < 0 || function >= m_variables.m_n) {
		MYERROR::Log(::Error,
			"!!!ERROR!!! SCRIPT Call unexisted function %i", function);
		return 0;
	}
	if (m_variables.m_data[function].m_var.m_flag != 3) {
		MYERROR::Log(::Error,
			"!!!ERROR!!!LOGIC: Call unexisted function %s()",
			m_variables.m_data[function].m_name.m_str);
		return 0;
	}

	int initialStack = m_stack.m_n;
	m_stack.Push(LOGICSTACK(initialStack));
	m_stack.Push(LOGICSTACK(m_stackPos));
	int frameBase = m_stack.m_n;
	int instruction = m_variables.m_data[function].m_var.m_a;
	int statementStart = instruction;

	if (m_variables.m_data[function].m_var.m_extra >= 1)
		((LOGICSTACK*) m_stack.m_data)[m_variables.m_data[function].m_var.m_type] =
			LOGICSTACK((const void*) p_a);
	if (m_variables.m_data[function].m_var.m_extra >= 2)
		((LOGICSTACK*) m_stack.m_data)[m_variables.m_data[function].m_var.m_type + 1] =
			LOGICSTACK((const void*) p_b);
	if (m_variables.m_data[function].m_var.m_extra >= 3)
		((LOGICSTACK*) m_stack.m_data)[m_variables.m_data[function].m_var.m_type + 2] =
			LOGICSTACK(p_c);

	while (instruction < m_stackPos) {
		if (m_stack.m_n < frameBase) {
			MYERROR::Log(::Error,
				"!!!ERROR!!!LOGIC: '%s' stack error %i",
				"pop, but not push", instruction);
			exit(1);
		}

		int stackCount = m_stack.m_n;
		unsigned int operation = (unsigned char) m_stackData[instruction];
		if (operation <= 23 && operation >= 6) {
			++instruction;
			LOGICSTACK* stack = (LOGICSTACK*) m_stack.m_data;
			stack[stackCount - 2].BinarOperator(operation, stack[stackCount - 1]);
			--m_stack.m_n;
		}
		else if (operation <= 39 && operation >= 32) {
			int variable = *(int*) (m_stackData + instruction + 1);
			++instruction;
			LOGICSTACK* stack = (LOGICSTACK*) m_stack.m_data;
			if ((stack[variable].m_type & 0x20) && indexActive)
				variable = stack[variable].m_num;
			int effective = variable + arrayIndex;

			switch (operation) {
			case 39: {
				--m_stack.m_n;
				stack[effective].BinarOperator(
					(unsigned char) m_stackData[instruction + 4], stack[stackCount - 1]);
				m_stack.Insert(((LOGICSTACK*) m_stack.m_data)[effective]);
				instruction += 5;
				indexActive = 0;
				arrayIndex = 0;
				break;
			}
			case 38:
				if (stack[*(int*) (m_stackData + instruction)].m_type & 1) {
					if (!indexActive
						|| (stack[*(int*) (m_stackData + instruction)].m_type & 4))
						*(STRING*) &stack[effective].m_str =
							*stack[stackCount - 1].String();
					else
						stack[*(int*) (m_stackData + instruction)]
							.m_str.m_str[arrayIndex] =
							(char) stack[stackCount - 1].Int();
				}
				else {
					stack[effective].m_type &= 0xaf;
					stack[effective].m_num =
						((LOGICSTACK*) m_stack.m_data)[m_stack.m_n - 1].Int();
					if (((LOGICSTACK*) m_stack.m_data)[m_stack.m_n - 1].m_type & 0x10)
						stack[effective].m_type |= 0x10;
				}
				instruction += 4;
				indexActive = 0;
				arrayIndex = 0;
				break;
			case 36:
				if (indexActive || !(stack[effective].m_type & 4))
					m_stack.Insert(stack[effective]);
				else
					m_stack.Push(LOGICSTACK(*(int*) (m_stackData + instruction)));
				instruction += 4;
				indexActive = 0;
				arrayIndex = 0;
				break;
			case 37:
				if (stack[effective].m_type & 1)
					m_stack.Push(LOGICSTACK((int) &stack[effective].m_str));
				else
					m_stack.Push(LOGICSTACK(0));
				instruction += 4;
				indexActive = 0;
				arrayIndex = 0;
				break;
			case 34:
				stack[effective].Inc();
				m_stack.Insert(stack[effective]);
				instruction += 4;
				indexActive = 0;
				arrayIndex = 0;
				break;
			case 35:
				stack[effective].Dec();
				m_stack.Insert(stack[effective]);
				instruction += 4;
				indexActive = 0;
				arrayIndex = 0;
				break;
			case 32: {
				LOGICSTACK value(stack[effective]);
				m_stack.Insert(value);
				stack = (LOGICSTACK*) m_stack.m_data;
				stack[effective].Inc();
				instruction += 4;
				indexActive = 0;
				arrayIndex = 0;
				break;
			}
			case 33: {
				LOGICSTACK value(stack[effective]);
				m_stack.Insert(value);
				stack = (LOGICSTACK*) m_stack.m_data;
				stack[effective].Dec();
				instruction += 4;
				indexActive = 0;
				arrayIndex = 0;
				break;
			}
			}
		}
		else {
			++instruction;
			switch (operation) {
			case 3:
				((LOGICSTACK*) m_stack.m_data)[m_stack.m_n - 1].m_num =
					-((LOGICSTACK*) m_stack.m_data)[m_stack.m_n - 1].Int();
				((LOGICSTACK*) m_stack.m_data)[m_stack.m_n - 1].m_type = 2;
				break;
			case 4:
				((LOGICSTACK*) m_stack.m_data)[m_stack.m_n - 1].m_num =
					~((LOGICSTACK*) m_stack.m_data)[m_stack.m_n - 1].Int();
				((LOGICSTACK*) m_stack.m_data)[m_stack.m_n - 1].m_type = 2;
				break;
			case 5:
				((LOGICSTACK*) m_stack.m_data)[m_stack.m_n - 1].m_num =
					((LOGICSTACK*) m_stack.m_data)[m_stack.m_n - 1].Int() == 0;
				((LOGICSTACK*) m_stack.m_data)[m_stack.m_n - 1].m_type = 2;
				break;
			case 1:
				m_stack.Push(LOGICSTACK(*(int*) (m_stackData + instruction)));
				instruction += 4;
				break;
			case 2: {
				STRING text(m_stackData + instruction, STRING::CALL_COPY);
				m_stack.Push(LOGICSTACK(text));
				instruction += strlen(m_stackData + instruction) + 1;
				break;
			}
			case 26:
				m_stack.m_n = stackCount - 1;
				break;
			case 25:
				instruction += 4;
				statementStart = instruction;

				if (stackCount - frameBase > 1)
					MYERROR::Log(::Error,
						"!!!ERROR!!!LOGIC: '%s' stack error %i", ";", instruction);
				m_stack.m_n = frameBase;
				if (frameBase > m_stack.m_max)
					m_stack.Expand(frameBase);
				break;
			case 24: {
				LOGICSTACK* condition = &((LOGICSTACK*) m_stack.m_data)[--m_stack.m_n];
				if (condition->Int())
					instruction += 4;
				else
					instruction += *(int*) (m_stackData + instruction);
				if (m_stack.m_n - frameBase > 1)
					MYERROR::Log(::Error,
						"!!!ERROR!!!LOGIC: '%s' stack error %i", "if", instruction);
				statementStart = instruction;
				m_stack.m_n = frameBase;
				if (frameBase > m_stack.m_max)
					m_stack.Expand(frameBase);
				break;
			}
			case 28:
				instruction += *(int*) (m_stackData + instruction);
				break;
			case 29: {
				LOGICSTACK* condition = &((LOGICSTACK*) m_stack.m_data)[--m_stack.m_n];
				if (!condition->Int())
					instruction += *(int*) (m_stackData + instruction);
				else {
					m_stackData[statementStart] = 28;
					*(int*) (m_stackData + statementStart + 1) =
						*(int*) (m_stackData + instruction) + instruction - statementStart - 1;
					instruction += 4;
				}
				statementStart = instruction;
				if (m_stack.m_n - frameBase > 1)
					MYERROR::Log(::Error,
						"!!!ERROR!!!LOGIC: '%s' stack error %i", "iff", instruction);
				m_stack.m_n = frameBase;
				if (frameBase > m_stack.m_max)
					m_stack.Expand(frameBase);
				break;
			}
			case 30:
				m_stack.Push(LOGICSTACK(frameBase));
				m_stack.Push(LOGICSTACK(instruction + 4));
				instruction = *(int*) (m_stackData + instruction);
				frameBase = m_stack.m_n;
				statementStart = instruction;
				break;
			case 31: {
				if (stackCount - frameBase > 1)
					MYERROR::Log(::Error,
						"!!!ERROR!!!LOGIC: '%s' stack error %i", "return", instruction);
				if (m_stack.m_n > frameBase) {
					LOGICSTACK returnValue(((LOGICSTACK*) m_stack.m_data)[--m_stack.m_n]);
					m_stack.m_n = frameBase;
					if (frameBase > m_stack.m_max)
						m_stack.Expand(frameBase);
					instruction = ((LOGICSTACK*) m_stack.m_data)[--m_stack.m_n].Int();
					frameBase = ((LOGICSTACK*) m_stack.m_data)[--m_stack.m_n].Int();
					m_stack.Insert(returnValue);
					if (instruction >= m_stackPos && !result)
						result = returnValue.Int();
				}
				else {
					m_stack.m_n = frameBase;
					if (frameBase > m_stack.m_max)
						m_stack.Expand(frameBase);
					instruction = ((LOGICSTACK*) m_stack.m_data)[--m_stack.m_n].Int();
					frameBase = ((LOGICSTACK*) m_stack.m_data)[--m_stack.m_n].Int();
				}
				statementStart = instruction;
				break;
			}
			case 40: {
				LOGICSTACK* index = &((LOGICSTACK*) m_stack.m_data)[--m_stack.m_n];
				arrayIndex = index->Int();
				indexActive = 1;
				break;
			}
			default:
				if ((unsigned char) m_stackData[instruction - 1] == 84) {
					LOGICSTACK* top = &((LOGICSTACK*) m_stack.m_data)[stackCount - 1];
					if (top->Int() == (int) p_a)
						result = 1;
				}
				ScriptExecFunc((unsigned char) m_stackData[instruction - 1]);
				break;
			}
		}
	}

	m_stack.m_n = initialStack;
	if (initialStack > m_stack.m_max) {
		LOGICSTACK* oldData = (LOGICSTACK*) m_stack.m_data;
		LOGICSTACK* newData = new LOGICSTACK[initialStack];
		m_stack.m_data = (int*) newData;
		if (!newData)
			MYERROR::LogExit(::Error,
				"!!!ERROR!!!::LIST: Not enough memory %i", initialStack);
		if (oldData) {
			for (int i = 0; i < m_stack.m_max; ++i)
				((LOGICSTACK*) m_stack.m_data)[i] = oldData[i];
			delete[] oldData;
		}
		m_stack.m_max = initialStack;
	}
	return result;
}
