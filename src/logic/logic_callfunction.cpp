
#include "logic/logic.h"
#include "logic/logicstack.h"
#include "logic/logicvar.h"
#include "logic/script_commands.h"
#include "game/data_version.h"
#include "game/game_descriptor.h"
#include "util/myerror.h"
#include "util/named_list_struct_logicvar.h"
#include "util/string.h"

#include <stdlib.h>
#include <string.h>

class VID;
VID** ScriptExecFunc(int p_cmd);

namespace {





int NativeMinimumArgs(unsigned int p_operation)
{
	const int dialectMinimum = ScriptCommandMinimumArgs(p_operation);
	if (dialectMinimum != -2) return dialectMinimum;
	const bool steamAS1 = GameDesc->m_scriptDialect == GAME_SCRIPT_AS1 && GameData_IsSteam();
	switch (p_operation) {
	case 102: return steamAS1 ? 2 : 1;
	case 134: return Game_IsZS1() ? 1 : 0;
	case 136: return Game_IsZS1() ? 3 : 2;
	case 150: return steamAS1 ? 3 : 0;
	case 212: return steamAS1 ? 1 : 2;
	case 231: return Game_IsZS1() ? 1 : 4;
	case 232: return Game_IsZS1() ? 0 : -1;
	case 78: case 255: return Game_IsZS1() ? 5 : -1;



	case 210: case 211: case 215: case 217: case 218: case 221: case 222:
		return steamAS1 ? 1 : -1;
	case 214: case 219: return steamAS1 ? 2 : -1;
	case 220: return steamAS1 ? 3 : -1;
	case 216: case 223: return steamAS1 ? 0 : -1;
	case 213: return steamAS1 || GameDesc->m_unitCountLayers > 0 ? 0 : -1;
	case 69: case 75: case 76: case 77: case 91: case 92:
	case 104: case 105: case 108: case 109: case 110: case 111:
	case 115: case 118: case 119: case 120: case 127:
	case 143: case 144: case 145: case 147: case 149: case 151: case 157:
	case 167: case 168: case 173: case 185: case 189: case 241: case 244: case 245:
		return 0;
	case 66: case 68: case 83: case 84: case 85: case 86: case 87: case 88: case 89:
	case 96: case 98: case 99: case 100: case 103: case 112: case 113:
	case 117: case 121: case 122: case 124: case 125:
	case 130: case 131: case 132: case 133: case 138: case 139: case 140: case 142:
	case 148: case 152: case 154: case 155: case 159: case 164: case 165: case 166:
	case 169: case 171: case 175: case 176: case 177: case 178: case 179:
	case 188: case 205: case 206: case 207: case 240: case 246: case 247:
	case 250: case 252:
		return 1;
	case 97: case 101: case 116: case 123: case 126: case 128: case 129:
	case 141: case 153: case 156: case 158: case 160: case 162: case 170:
	case 172: case 174: case 184: case 186: case 187: case 208: case 251: case 254:
		return 2;
	case 71: case 80: case 90: case 114: case 135: case 146: case 163:
	case 182: case 183: case 239: case 242: case 249: case 253:
		return 3;
	case 70: case 74: case 137: case 161:
		return 4;
	case 72: case 79: case 82: case 107: case 243:
		return 5;
	case 65: case 106:
		return 6;
	default:
		return -1;
	}
}

}

// STUB: ALIEN 0x423430
int LOGIC::CallFunction(int p_fn, const SPRITE* p_a, const SPRITE* p_b, int p_c)
{
	int result = 0;
	int indexActive = 0;
	int arrayIndex = 0;

	if (!m_stackData || m_runtimeFault) {
		return 0;
	}

	int function = p_fn < 0 ? m_main : p_fn;
	if (function < 0 || function >= m_variables.m_n) {
		MYERROR::Log(::Error, "!!!ERROR!!! SCRIPT Call unexisted function %i", function);
		return 0;
	}
	if (m_variables.m_data[function].m_var.m_flag != 3) {
		MYERROR::Log(
			::Error,
			"!!!ERROR!!!LOGIC: Call unexisted function %s()",
			m_variables.m_data[function].m_name.m_str
		);
		return 0;
	}
	const LOGICVAR& entry = m_variables.m_data[function].m_var;
	if (entry.m_a < 0 || entry.m_a >= m_stackPos) {
		RuntimeError(m_variables.m_data[function].m_name.m_str, entry.m_a);
		return 0;
	}
	if (entry.m_extra < 0 || entry.m_type < 0 ||
		entry.m_type > m_stack.m_n || entry.m_extra > m_stack.m_n - entry.m_type) {
		RuntimeError("invalid function parameter slots", function);
		return 0;
	}

	int initialStack = m_stack.m_n;
	m_stack.Push(LOGICSTACK(initialStack));
	m_stack.Push(LOGICSTACK(m_stackPos));
	int frameBase = m_stack.m_n;
	int instruction = m_variables.m_data[function].m_var.m_a;
	int statementStart = instruction;

	if (m_variables.m_data[function].m_var.m_extra >= 1) {
		((LOGICSTACK*) m_stack.m_data)[m_variables.m_data[function].m_var.m_type] = LOGICSTACK((const void*) p_a);
	}
	if (m_variables.m_data[function].m_var.m_extra >= 2) {
		((LOGICSTACK*) m_stack.m_data)[m_variables.m_data[function].m_var.m_type + 1] = LOGICSTACK((const void*) p_b);
	}
	if (m_variables.m_data[function].m_var.m_extra >= 3) {
		((LOGICSTACK*) m_stack.m_data)[m_variables.m_data[function].m_var.m_type + 2] = LOGICSTACK(p_c);
	}

	while (instruction != m_stackPos && !m_runtimeFault) {
		m_runtimeOffset = instruction;
		if (instruction < 0 || instruction >= m_stackPos) {
			RuntimeError("invalid instruction address", instruction);
			break;
		}
		if (m_stack.m_n < frameBase || frameBase < initialStack + 2) {
			RuntimeError("stack underflow", instruction);
			break;
		}

		int stackCount = m_stack.m_n;
		unsigned int operation = (unsigned char) m_stackData[instruction];
		int operandBytes = operation == 39 ? 5 :
			(operation == 1 || operation == 24 || operation == 25 || operation == 28 ||
			 operation == 29 || operation == 30 || (operation >= 32 && operation <= 38) ||
			 operation == 44 || operation == 45) ? 4 : 0;
		if (operandBytes > m_stackPos - instruction - 1 ||
			(operation == 2 && !memchr(m_stackData + instruction + 1, 0, m_stackPos - instruction - 1))) {
			RuntimeError("truncated bytecode operand", operation);
			break;
		}
		int needed = operation >= 6 && operation <= 23 ? 2 :
			(operation == 3 || operation == 4 || operation == 5 || operation == 24 ||
			 operation == 26 || operation == 29 || operation == 38 || operation == 39 ||
			 operation == 40 || operation == 44 || operation == 45) ? 1 : 0;
		if (stackCount - frameBase < needed) {
			RuntimeError("missing operand on stack", operation);
			break;
		}
		if (operation <= 23 && operation >= 6) {
			++instruction;
			LOGICSTACK* stack = (LOGICSTACK*) m_stack.m_data;
			stack[stackCount - 2].BinarOperator(operation, stack[stackCount - 1]);
			--m_stack.m_n;
		}
		else if (operation <= 39 && operation >= 32) {
			const int encodedVariable = LOGIC_BYTECODE::ReadInt32(m_stackData + instruction + 1);
			int variable = encodedVariable;
			++instruction;
			LOGICSTACK* stack = (LOGICSTACK*) m_stack.m_data;
			if (variable < 0 || variable >= initialStack) {
				RuntimeError("invalid variable slot", variable);
				break;
			}
			if ((stack[variable].m_type & 0x20) && indexActive) {
				variable = stack[variable].m_num;
			}


			const bool characterAssignment = operation == 38 && indexActive &&
				(stack[encodedVariable].m_type & 1) && !(stack[encodedVariable].m_type & 4);
			const int64_t effectiveIndex = characterAssignment ? encodedVariable : int64_t(variable) + arrayIndex;
			if (effectiveIndex < 0 || effectiveIndex >= initialStack) {
				RuntimeError("array index outside variable storage", arrayIndex);
				break;
			}
			int effective = int(effectiveIndex);

			switch (operation) {
			case 39: {
				--m_stack.m_n;
				stack[effective].BinarOperator((unsigned char) m_stackData[instruction + 4], stack[stackCount - 1]);
				m_stack.Insert(((LOGICSTACK*) m_stack.m_data)[effective]);
				instruction += 5;
				indexActive = 0;
				arrayIndex = 0;
				break;
			}
			case 38:
				if (stack[encodedVariable].m_type & 1) {
					if (!indexActive || (stack[encodedVariable].m_type & 4)) {
						stack[effective].m_str = *stack[stackCount - 1].String();
					}
					else {
						if (arrayIndex < 0 || size_t(arrayIndex) >= strlen(stack[encodedVariable].m_str.m_str)) {
							RuntimeError("string index outside storage", arrayIndex);
							break;
						}
						stack[encodedVariable].m_str.m_str[arrayIndex] = (char) stack[stackCount - 1].Int();
					}
				}
				else {
					stack[effective].AssignValue(((LOGICSTACK*) m_stack.m_data)[m_stack.m_n - 1]);
				}
				instruction += 4;
				indexActive = 0;
				arrayIndex = 0;
				break;
			case 36:
				if (indexActive || !(stack[effective].m_type & 4)) {
					m_stack.Insert(stack[effective]);
				}
				else {
					m_stack.Push(LOGICSTACK(encodedVariable));
				}
				instruction += 4;
				indexActive = 0;
				arrayIndex = 0;
				break;
			case 37:
				if (stack[effective].m_type & 1) {
					m_stack.Push(LOGICSTACK((const void*) &stack[effective].m_str));
				}
				else {
					m_stack.Push(LOGICSTACK(0));
				}
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
				m_stack.Push(LOGICSTACK(LOGIC_BYTECODE::ReadInt32(m_stackData + instruction)));
				instruction += 4;
				break;
			case 2: {
				STRING text(m_stackData + instruction);
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

				if (stackCount - frameBase > 1) {
					MYERROR::Log(::Error, "!!!ERROR!!!LOGIC: '%s' stack error %i", ";", instruction);
				}
				m_stack.m_n = frameBase;
				if (frameBase > m_stack.m_max) {
					m_stack.Expand(frameBase);
				}
				break;
			case 24: {
				LOGICSTACK* condition = &((LOGICSTACK*) m_stack.m_data)[--m_stack.m_n];
				if (condition->Int()) {
					instruction += 4;
				}
				else {
					instruction += LOGIC_BYTECODE::ReadInt32(m_stackData + instruction);
				}
				if (m_stack.m_n - frameBase > 1) {
					MYERROR::Log(::Error, "!!!ERROR!!!LOGIC: '%s' stack error %i", "if", instruction);
				}
				statementStart = instruction;
				m_stack.m_n = frameBase;
				if (frameBase > m_stack.m_max) {
					m_stack.Expand(frameBase);
				}
				break;
			}
			case 28:
				instruction += LOGIC_BYTECODE::ReadInt32(m_stackData + instruction);
				break;
			case 29: {
				LOGICSTACK* condition = &((LOGICSTACK*) m_stack.m_data)[--m_stack.m_n];
				if (!condition->Int()) {
					instruction += LOGIC_BYTECODE::ReadInt32(m_stackData + instruction);
				}
				else {
					m_stackData[statementStart] = 28;
					LOGIC_BYTECODE::WriteInt32(
						m_stackData + statementStart + 1,
						LOGIC_BYTECODE::ReadInt32(m_stackData + instruction) + instruction - statementStart - 1
					);
					instruction += 4;
				}
				statementStart = instruction;
				if (m_stack.m_n - frameBase > 1) {
					MYERROR::Log(::Error, "!!!ERROR!!!LOGIC: '%s' stack error %i", "iff", instruction);
				}
				m_stack.m_n = frameBase;
				if (frameBase > m_stack.m_max) {
					m_stack.Expand(frameBase);
				}
				break;
			}
			case 30:
				if (LOGIC_BYTECODE::ReadInt32(m_stackData + instruction) < 0) {
					const char* name = "unresolved function";
					for (const auto& fixup : m_protoFixups) {
						if (fixup.first == instruction) name = m_variables.m_data[fixup.second].m_name.m_str;
					}
					RuntimeError(name, -1);
					break;
				}
				m_stack.Push(LOGICSTACK(frameBase));
				m_stack.Push(LOGICSTACK(instruction + 4));
				instruction = LOGIC_BYTECODE::ReadInt32(m_stackData + instruction);
				frameBase = m_stack.m_n;
				statementStart = instruction;
				break;
			case 31: {
				if (stackCount - frameBase > 1) {
					MYERROR::Log(::Error, "!!!ERROR!!!LOGIC: '%s' stack error %i", "return", instruction);
				}
				if (m_stack.m_n > frameBase) {
					LOGICSTACK returnValue(((LOGICSTACK*) m_stack.m_data)[--m_stack.m_n]);
					m_stack.m_n = frameBase;
					if (frameBase > m_stack.m_max) {
						m_stack.Expand(frameBase);
					}
					instruction = ((LOGICSTACK*) m_stack.m_data)[--m_stack.m_n].Int();
					frameBase = ((LOGICSTACK*) m_stack.m_data)[--m_stack.m_n].Int();
					m_stack.Insert(returnValue);
					if (instruction >= m_stackPos && !result) {
						result = returnValue.Int();
					}
				}
				else {
					m_stack.m_n = frameBase;
					if (frameBase > m_stack.m_max) {
						m_stack.Expand(frameBase);
					}
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
			case 44:
			case 45: {
				int value = ((LOGICSTACK*) m_stack.m_data)[m_stack.m_n - 1].Int();
				if (operation == 44 ? value == 0 : value != 0) {
					instruction += LOGIC_BYTECODE::ReadInt32(m_stackData + instruction);
				}
				else {
					instruction += 4;
				}
				break;
			}
			default: {



				const int nativeArgs = NativeMinimumArgs(operation);
				if (m_externalArgs[operation] < 0)
					RuntimeError("undeclared external command", operation);
				else if (nativeArgs < 0)
					RuntimeError("unsupported external command for script dialect", operation);
				else if (m_externalArgs[operation] < nativeArgs)
					RuntimeError("external declaration has fewer arguments than native handler", operation);
				else if (stackCount - frameBase < m_externalArgs[operation])
					RuntimeError("external command argument underflow", operation);
				if (m_runtimeFault) break;
				if ((unsigned char) m_stackData[instruction - 1] == 84) {
					LOGICSTACK* top = &((LOGICSTACK*) m_stack.m_data)[stackCount - 1];
					if (top->m_num == (intptr_t) p_a) {
						result = 1;
					}
				}
				ScriptExecFunc((unsigned char) m_stackData[instruction - 1]);
				break;
			}
			}
		}
	}

	m_stack.m_n = initialStack;
	if (initialStack > m_stack.m_max) {
		LOGICSTACK* oldData = (LOGICSTACK*) m_stack.m_data;
		LOGICSTACK* newData = new LOGICSTACK[initialStack];
		m_stack.m_data = (int*) newData;
		if (!newData) {
			MYERROR::LogExit(::Error, "!!!ERROR!!!::LIST: Not enough memory %i", initialStack);
		}
		if (oldData) {
			for (int i = 0; i < m_stack.m_max; ++i) {
				((LOGICSTACK*) m_stack.m_data)[i] = oldData[i];
			}
			delete[] oldData;
		}
		m_stack.m_max = initialStack;
	}
	return m_runtimeFault ? 0 : result;
}
