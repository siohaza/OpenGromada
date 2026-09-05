
#include "logic/logic.h"

#include "game/game_descriptor.h"
#include "logic/logicstack.h"
#include "logic/logicvar.h"
#include "platform/paths.h"
#include "util/fstream.h"
#include "util/myerror.h"
#include "util/named_list_struct_logicvar.h"
#include "util/named_list_struct_string.h"
#include "util/stream.h"
#include "util/string.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class VID;
VID** ScriptExecFunc(int p_cmd);

inline static void InsertLocalLogicVar(
	NAMED_LIST_LOGICVAR* p_list,
	const STRING& p_name,
	LOGICVAR* p_value,
	int p_type,
	int p_extra
)
{
	p_value->m_type = p_type;
	p_value->m_extra = p_extra;
	p_list->Insert(p_name, *p_value);
}

// FUNCTION: ALIEN 0x41f330
void LOGIC::Release()
{
	delete[] m_stackData;
	m_stackData = 0;
	delete[] (char*) m_unk0x44;
	m_unk0x44 = 0;

	m_stack.Release();
	m_variables.Release();
	m_strings.Release();

	m_stackPos = 0;
	m_unk0x4c = 0;
	m_pos = 0;
	m_end = 0;
	m_line = -1;
	m_unk0x54 = 0;
	m_main = -1;
	m_protoFixups.clear();
	m_runtimeFault = false;
	m_runtimeOffset = -1;
	for (int i = 0; i < 256; ++i) {
		m_actionN[i] = -1;
		m_externalArgs[i] = -1;
	}
}

void LOGIC::RuntimeError(const char* p_reason, int p_detail)
{
	if (!m_runtimeFault) {
		MYERROR::Log(::Error, "SCRIPT [%s] %s byte %d: %s (%d); script halted",
			GameDesc->m_profileId, m_name.m_str, m_runtimeOffset, p_reason, p_detail);
	}
	m_runtimeFault = true;
}

void LOGIC::RebuildExternalSignatures()
{
	for (int& count : m_externalArgs) count = -1;
	for (int i = 0; i < m_variables.m_n; ++i) {
		const LOGICVAR& external = m_variables.m_data[i].m_var;
		if (external.m_flag == 2 && external.m_a >= 0 && external.m_a < 256)
			m_externalArgs[external.m_a] = external.m_extra;
	}
}

int LOGIC::GetActionN(int p_n)
{
	if (p_n < 0 || p_n >= 256 || m_actionN[p_n] < 0 || m_actionN[p_n] >= m_stack.m_n) {
		MYERROR::Log(::Error, "!!!ERROR!!!SCRIPT: variable for Get Action%i", p_n);
		return 0;
	}
	return ((LOGICSTACK*) m_stack.m_data)[m_actionN[p_n]].Int();
}

void LOGIC::SetActionN(int p_n, int p_value)
{
	if (p_n < 0 || p_n >= 256 || m_actionN[p_n] < 0 || m_actionN[p_n] >= m_stack.m_n) {
		MYERROR::Log(::Error, "!!!ERROR!!!SCRIPT: variable for Set Action%i", p_n);
		return;
	}
	LOGICSTACK* slot = (LOGICSTACK*) m_stack.m_data + m_actionN[p_n];
	slot->m_num = p_value;
	slot->m_type &= ~1;
}

// STUB: ALIEN 0x41f450
int LOGIC::LoadLGC(const STRING& p_name)
{
	FILE* file = *p_name.m_str ? Platform_FOpen(p_name.m_str, "rb") : 0;
	Release();
	m_name = p_name;
	if (!file) {
		Error(7, empty_str, 0);
		return 1;
	}
	int size = compat_filelength(file);
	m_stackData = new char[256000];
	if (!m_stackData) {
		// STRING: ALIEN 0x483128
		Error(2, "data", 0);
		exit(1);
	}
	m_unk0x44 = new char[size + 4096];
	if (!m_unk0x44) {
		// STRING: ALIEN 0x483124
		Error(2, "ini", 0);
		exit(1);
	}
	m_pos = (char*) m_unk0x44 + 4066;
	m_end = m_pos + size;
	fread(m_pos, 1, size, file);
	if (m_stack.m_max < 128) {
		LOGICSTACK* oldData = (LOGICSTACK*) m_stack.m_data;
		LOGICSTACK* newData = new LOGICSTACK[128];
		m_stack.m_data = (int*) newData;
		if (!newData) {
			MYERROR::LogExit(::Error, "!!!ERROR!!!::LIST: Not enough memory %i", 128);
		}
		if (oldData) {
			for (int i = 0; i < m_stack.m_max; ++i) {
				LOGICSTACK& item = ((LOGICSTACK*) m_stack.m_data)[i];
				item = oldData[i];
			}
			delete[] oldData;
		}
		m_stack.m_max = 128;
	}
	m_variables.NAMED_LIST_LOGICVAR_BASE::Expand(128);
	m_line = 0;
	while (!func()) {
	}



	MYERROR::Log(
		::Error,
		// STRING: ALIEN 0x4830ec
		"LoadScript::ByteCode=%i varNo=%i DefineNo=%i stackNo=%i",
		m_stackPos,
		m_variables.m_n,
		m_strings.m_n,
		m_stack.m_n
	);
	m_strings.Release();
	if (m_stackPos) {
		if (m_stackPos > 256000) {
			// STRING: ALIEN 0x4830dc
			Error(2, "byte code size", m_stackPos);
		}
		char* data = new char[m_stackPos];
		if (!data) {
			// STRING: ALIEN 0x4830d8
			Error(2, "tmp", 0);
			exit(1);
		}
		memcpy(data, m_stackData, m_stackPos);
		delete[] m_stackData;
		m_stackData = data;
	}
	else {
		Release();
	}
	delete[] (char*) m_unk0x44;
	m_unk0x44 = 0;
	fclose(file);
	RebuildExternalSignatures();
	return 0;
}

// FUNCTION: ALIEN 0x41f7d0
int LOGIC::Load(const STRING& p_name)
{
	FSTREAM stream;
	stream.m_file = *p_name.m_str ? Platform_FOpen(p_name.m_str, "rb") : nullptr;
	Release();
	m_name = p_name;
	if (!stream.m_file) {
		Error(7, empty_str, 0);
		return 1;
	}
	const int fileSize = compat_filelength(stream.m_file);
	if (fileSize < 4 || fileSize > 64 * 1024 * 1024) {
		RuntimeError("invalid compiled script size", fileSize);
		return 1;
	}
	std::vector<char> bytes(fileSize);
	if (fread(bytes.data(), 1, bytes.size(), stream.m_file) != bytes.size()) {
		RuntimeError("truncated compiled script", fileSize);
		return 1;
	}
	const int stackCount = LOGIC_BYTECODE::ReadInt32(bytes.data());
	if (stackCount & 0xff000000) return LoadLGC(m_name);
	size_t offset = 4;
	auto require = [&](size_t count) {
		if (count <= bytes.size() - offset) return true;
		m_runtimeOffset = int(offset);
		RuntimeError("truncated LGD field", int(count));
		return false;
	};
	auto readString = [&](STRING& out) {
		const char* begin = bytes.data() + offset;
		const char* end = (const char*) memchr(begin, 0, bytes.size() - offset);
		if (!end) {
			m_runtimeOffset = int(offset);
			RuntimeError("unterminated LGD string");
			return false;
		}
		out = begin;
		offset += size_t(end - begin) + 1;
		return true;
	};
	if (stackCount < 0 || size_t(stackCount) > bytes.size() - offset) {
		RuntimeError("invalid LGD stack count", stackCount);
		return 1;
	}
	m_stack.Expand(stackCount);
	m_stack.m_n = stackCount;
	for (int i = 0; i < stackCount; ++i) {
		if (!require(1)) return 1;
		LOGICSTACK& item = ((LOGICSTACK*) m_stack.m_data)[i];
		item.m_type = bytes[offset++];
		if (!(item.m_type & 8)) continue;
		if (item.m_type & 1) {
			if (!readString(item.m_str)) return 1;
			for (size_t j = 0, n = strlen(item.m_str.m_str); j < n; ++j) item.m_str.m_str[j] ^= 0x17;
		}
		else {
			if (!require(4)) return 1;
			item.m_num = LOGIC_BYTECODE::ReadInt32(bytes.data() + offset);
			offset += 4;
		}
	}
	if (!require(4)) return 1;
	const int variableCount = LOGIC_BYTECODE::ReadInt32(bytes.data() + offset);
	offset += 4;
	if (variableCount < 0 || size_t(variableCount) > (bytes.size() - offset) / 21) {
		RuntimeError("invalid LGD symbol count", variableCount);
		return 1;
	}
	m_variables.Expand(variableCount);
	m_variables.m_n = variableCount;
	for (int i = 0; i < variableCount; ++i) {
		auto& symbol = m_variables.m_data[i];
		if (!readString(symbol.m_name) || !require(20)) return 1;
		const char* record = bytes.data() + offset;
		symbol.m_var.m_flag = record[0];

		symbol.m_var.m_a = LOGIC_BYTECODE::ReadInt32(record + 8);
		symbol.m_var.m_type = LOGIC_BYTECODE::ReadInt32(record + 12);
		symbol.m_var.m_extra = LOGIC_BYTECODE::ReadInt32(record + 16);
		offset += 20;
	}
	for (int i = 0; i < variableCount; ++i) {
		if (!readString(m_variables.m_data[i].m_var.m_value)) return 1;
	}
	if (!require(4)) return 1;
	m_stackPos = LOGIC_BYTECODE::ReadInt32(bytes.data() + offset);
	offset += 4;
	if (m_stackPos < 0) {
		RuntimeError("negative bytecode size", m_stackPos);
		return 1;
	}
	if (!require(size_t(m_stackPos))) return 1;
	m_stackData = new char[m_stackPos];
	memcpy(m_stackData, bytes.data() + offset, m_stackPos);
	for (int i = 0; i < m_variables.m_n; ++i) {
		if (!strcmp(m_variables.GetName(i), "main")) m_main = i;
	}
	RebuildExternalSignatures();
	return 0;
}

// FUNCTION: ALIEN 0x41fa30
int LOGIC::SaveVar(STREAM* p_stream)
{
	for (int i = 0; i < m_stack.m_n; ++i) {
		((LOGICSTACK*) m_stack.m_data)[i].Write(p_stream);
	}
	m_variables.Write(p_stream);
	int result = 0;
	while (result < m_variables.m_n) {
		const STRING& value = m_variables.m_data[result].m_var.m_value;
		p_stream->Write(value.m_str, strlen(value.m_str) + 1);
		++result;
	}
	return result;
}

// FUNCTION: ALIEN 0x41fb20
int LOGIC::LoadVar(STREAM* p_stream)
{
	int i = 0;
	if (m_stack.m_n > 0) {
		do {
			((LOGICSTACK*) m_stack.m_data)[i].Read(p_stream);
			++i;
		} while (i < m_stack.m_n);
	}
	m_variables.Read(p_stream);
	int result = m_variables.m_n;
	i = 0;
	if (result > 0) {
		do {
			m_variables.m_data[i].m_var.m_value.Read_res(p_stream);
			result = m_variables.m_n;
			++i;
		} while (i < result);
	}
	return result;
}

// FUNCTION: ALIEN 0x41fbe0
int LOGIC::Error(int p_type, const char* p_word, int p_line)
{
	// STRING: ALIEN 0x483140
	int result = MYERROR::Error(::Error, "LOGIC '%s' line %i", p_type, p_word, p_line, m_name.m_str, m_line + 1);
	if (m_pos) {
		char buf[61];
		int i;
		for (i = 0; i < 60; ++i) {
			char c = m_pos[i - 30];
			buf[i] = c;
			if (c == 10 || c == 13 || c == 9) {
				buf[i] = '?';
			}
		}
		buf[60] = 0;
		// STRING: ALIEN 0x483138
		MYERROR::Error(::Error, "LOGIC", 10, buf, 0);
		for (i = 0; i < 60; ++i) {
			buf[i] = i == 30 ? '^' : ' ';
		}
		buf[60] = 0;
		result = MYERROR::Error(::Error, "LOGIC", 10, buf, 0);
	}
	return result;
}

// STUB: ALIEN 0x41fcb0
int LOGIC::skipempty2()
{
	int comment = 0;
	int skipDepth = 0;
	char tokenBuffer[4096];

	while (m_pos < m_end) {
		if (!comment) {
			if ((isalpha(*m_pos) || *m_pos == '_') && !m_unk0x54) {
				int length = 0;
				while (isalnum(m_pos[length]) || m_pos[length] == '_') {
					if (length >= 4095) {
						Error(
							10,
							// STRING: ALIEN 0x483178
							"Very long name",
							0
						);
						exit(1);
					}
					tokenBuffer[length] = m_pos[length];
					++length;
				}
				tokenBuffer[length] = 0;
				int define;
				if ((define = m_strings.Location(STRING(tokenBuffer))) >= 0) {
					int replacementLength = strlen(m_strings.GetValue(define));
					m_pos += length - replacementLength;
					memcpy(m_pos, m_strings.GetValue(define), replacementLength);
				}
			}

			if (*m_pos == '#' && m_pos[1] == 'i' && m_pos[2] == 'f' && m_pos[3] == 'd' && m_pos[4] == 'e' &&
				m_pos[5] == 'f') {
				m_pos += 6;
				++m_unk0x4c;
				if (!skipDepth) {
					STRING name;
					m_unk0x54 = 1;
					GetName(&name);
					m_unk0x54 = 0;
					if (m_variables.Location(name) < 0 && m_strings.Location(name) < 0) {
						skipDepth = m_unk0x4c;
					}
				}
			}
			else if (
				*m_pos == '#' && m_pos[1] == 'i' && m_pos[2] == 'f' && m_pos[3] == 'n' && m_pos[4] == 'd' &&
				m_pos[5] == 'e' && m_pos[6] == 'f'
			) {
				m_pos += 7;
				++m_unk0x4c;
				if (!skipDepth) {
					STRING name;
					m_unk0x54 = 1;
					GetName(&name);
					m_unk0x54 = 0;
					if (m_variables.Location(name) >= 0 || m_strings.Location(name) >= 0) {
						skipDepth = m_unk0x4c;
					}
				}
			}
			else if (
				*m_pos == '#' && m_pos[1] == 'e' && m_pos[2] == 'n' && m_pos[3] == 'd' && m_pos[4] == 'i' &&
				m_pos[5] == 'f'
			) {
				m_pos += 6;
				if (skipDepth == m_unk0x4c) {
					skipDepth = 0;
				}
				--m_unk0x4c;
				if (m_unk0x4c < 0) {
					Error(
						10,
						// STRING: ALIEN 0x4831b8
						"#endif without #ifdef",
						0
					);
				}
			}
			else if (*m_pos == '#' && m_pos[1] == 'e' && m_pos[2] == 'l' && m_pos[3] == 's' && m_pos[4] == 'e') {
				m_pos += 5;
				if (!skipDepth && m_unk0x4c > 0) {
					skipDepth = m_unk0x4c;
				}
				else if (skipDepth == m_unk0x4c) {
					skipDepth = 0;
				}
				if (m_unk0x4c <= 0) {
					Error(
						10,
						// STRING: ALIEN 0x4831a0
						"#else without #ifdef",
						0
					);
				}
			}

			if (*m_pos == '/' && m_pos[1] == '/') {
				comment = 1;
			}
			else if (*m_pos == '/' && m_pos[1] == '*') {
				comment = 2;
			}
			else {
				if (*m_pos == '?') {
					Error(
						10,
						// STRING: ALIEN 0x483154
						"?: not supported in this version",
						0
					);
					exit(1);
				}
				if (!skipDepth && !isspace(*m_pos) && *m_pos) {
					return 0;
				}
			}
		}
		else if ((comment == 1 && *m_pos == '\n') || (comment == 2 && *m_pos == '/' && m_pos[-1] == '*')) {
			comment = 0;
		}

		char c = *m_pos++;
		if (c == '\n') {
			++m_line;
		}
	}

	if (m_unk0x4c > 0) {
		Error(
			10,
			// STRING: ALIEN 0x483188
			"#ifdef without #endif",
			m_unk0x4c
		);
	}
	return 1;
}

// FUNCTION: ALIEN 0x4202a0
int LOGIC::skipempty()
{
	int result = skipempty2();
	if (result) {
		// STRING: ALIEN 0x4831d0
		Error(10, "End of file", 0);
		exit(1);
	}
	// The original fell off the end here; the non-zero path always exits, so
	// reaching this point means result == 0.
	return 0;
}

// FUNCTION: ALIEN 0x4202d0
int LOGIC::GetLine(STRING* p_out)
{
	skipempty();
	char buf[4096];
	int n = 0;
	while (*m_pos != '\n') {
		char* p = m_pos;
		char c = *p;
		if (c == '\r') {
			break;
		}
		if (n >= 4095) {
			// STRING: ALIEN 0x4831e0
			Error(10, "Very long line", 0);
			exit(1);
		}
		buf[n] = c;
		m_pos = p + 1;
		++n;
	}
	buf[n] = 0;
	*p_out = buf;
	if (!buf[0]) {
		// STRING: ALIEN 0x4831f0
		Error(10, "empty line", 0);
		exit(1);
	}
	char* line;
	p_out->Before(
		&line,
		// STRING: ALIEN 0x4831dc
		"//"
	);
	*p_out = line;
	if (line != STRING::EMPTY) {
		operator delete(line);
	}
	p_out->RemoveEndChars(
		// STRING: ALIEN 0x481a08
		" \n\r\t"
	);
	return skipempty();
}

// FUNCTION: ALIEN 0x4203b0
int LOGIC::GetName(STRING* p_out)
{
	skipempty();
	char buf[4096];
	int n = 0;
	while (isalnum(*m_pos) || *m_pos == '_') {
		if (n >= 4095) {

			Error(10, "Very long name", 0);
			exit(1);
		}
		char* p = m_pos;
		buf[n] = *p;
		m_pos = p + 1;
		++n;
	}
	buf[n] = 0;
	*p_out = buf;
	if (!buf[0]) {
		// STRING: ALIEN 0x4831fc
		Error(4, "name", 0);
		exit(1);
	}
	skipempty();
	return n;
}

// FUNCTION: ALIEN 0x420460
int LOGIC::Word(const char* p_word)
{
	int len = strlen(p_word);
	skipempty();
	if (strncmp(m_pos, p_word, len) ||
		((isalpha(*p_word) || *p_word == '#') && (isalnum(m_pos[len]) || m_pos[len] == '_'))) {
		return 0;
	}
	m_pos += len;
	skipempty2();
	return 1;
}

// FUNCTION: ALIEN 0x4204f0
int LOGIC::WordEnd(const char* p_word)
{
	if (Word(p_word)) {
		return 1;
	}
	Error(13, p_word, 0);
	exit(1);
}

// FUNCTION: ALIEN 0x420520
int LOGIC::GetInt()
{
	skipempty();
	int pos = m_stackPos;
	vyrag();
	char* data = m_stackData;
	if (data[pos] != 1 || m_stackPos - pos != 5) {
		// STRING: ALIEN 0x483204
		Error(4, "constant int value", 0);
		exit(1);
	}
	m_stackPos -= 5;
	return LOGIC_BYTECODE::ReadInt32(data + m_stackPos + 1);
}

// FUNCTION: ALIEN 0x420570
int LOGIC::GetString(char* p_out)
{
	char* start = p_out;
	if (*m_pos == '"') {
		++m_pos;
		while (*m_pos != '"' && m_pos < m_end) {
			if (*m_pos == '\\') {
				if (m_pos[1] == '\r' && m_pos[2] == '\n') {
					++m_line;
					m_pos += 2;
				}
				else if (m_pos[1] == '\n') {
					++m_line;
					++m_pos;
				}
				else if (isdigit(m_pos[1]) && isdigit(m_pos[2]) && isdigit(m_pos[3])) {
					*p_out++ = (char) (((m_pos[1] - '0') * 8 + m_pos[2] - '0') * 8 + m_pos[3] - '0');
					m_pos += 3;
				}
				else {
					char escaped = m_pos[1];
					if (escaped == 'n') {
						*p_out++ = '\n';
						++m_pos;
					}
					else if (escaped == 'r') {
						*p_out++ = '\r';
						++m_pos;
					}
					else if (escaped == 't') {
						*p_out++ = '\t';
						++m_pos;
					}
					else if (escaped == '\'') {
						*p_out++ = '\'';
						++m_pos;
					}
					else if (!escaped) {
						*p_out++ = 0;
						++m_pos;
					}
					else if (escaped == '"') {
						*p_out++ = '"';
						++m_pos;
					}
					else {
						++m_pos;
						*p_out++ = *m_pos;
					}
				}
			}
			else {
				*p_out++ = *m_pos;
			}
			++m_pos;
		}
		char* close = m_pos++;
		if (close >= m_end) {
			Error(10, "End of file", 0);
			exit(1);
		}
		*p_out++ = 0;
	}
	return p_out - start;
}

// FUNCTION: ALIEN 0x420700
int LOGIC::SetNoElement(int p_value)
{
	int result = 3 * m_variables.m_n;
	if (m_variables.m_n > 0) {
		m_variables.m_data[m_variables.m_n - 1].m_var.m_extra = p_value;
	}
	return result;
}

// STUB: ALIEN 0x420720
void LOGIC::IntVar()
{
	int count = 0;
	int flags = 0;
	STRING name;

	skipempty();
	if (*m_pos == '*') {
		flags = 0x20;
		++m_pos;
	}
	GetName(&name);

	if (m_variables.Location(name) >= 0) {
		Error(
			10,
			Printf(
				// STRING: ALIEN 0x48324c
				"int redefinition '%s'",
				name.m_str
			),
			0
		);
		exit(1);
	}

	int slot = m_stack.GetNo();
	m_variables.Insert(name, LOGICVAR(1, slot));
	if (!strncmp(name.m_str, "Action", 6) && isdigit((unsigned char) name.m_str[6])) {
		int actionIndex = atoi(name.m_str + 6);
		if (actionIndex >= 0 && actionIndex < 256) {
			m_actionN[actionIndex] = slot;
		}
	}

	if (Word("[")) {
		flags |= 4;
		if (*m_pos == ']') {
			++m_pos;
			if (Word(
					// STRING: ALIEN 0x481a00
					"="
				)) {
				// STRING: ALIEN 0x48326c
				WordEnd("{");
				do {
					m_stack.Push(LOGICSTACK(0));
					((LOGICSTACK*) m_stack.m_data)[m_stack.m_n - 1].m_type |= flags;
					LOGICSTACK* value = &((LOGICSTACK*) m_stack.m_data)[m_stack.m_n - 1];
					value->m_num = GetInt();
					value->m_type |= 8;
					++count;
				} while (Word(","));
				// STRING: ALIEN 0x483264
				WordEnd("}");
				SetNoElement(count);
				return;
			}
			// STRING: ALIEN 0x483230
			Error(10, "for [] need initialisation", 0);
			exit(1);
		}
		count = GetInt();
		WordEnd("]");
	}
	else {
		count = 1;
	}

	for (int element = 0; element < count; ++element) {
		m_stack.Push(LOGICSTACK(0));
		((LOGICSTACK*) m_stack.m_data)[m_stack.m_n - 1].m_type |= flags | 0x40;
	}

	if (Word("=")) {
		if (flags & 4) {
			WordEnd("{");
			int initializer = 0;
			do {
				if (initializer >= count) {
					// STRING: ALIEN 0x483218
					Error(10, "too many initializers", 0);
					exit(1);
				}
				((LOGICSTACK*) m_stack.m_data)[m_stack.m_n + initializer - count].m_type &= ~0x40;
				LOGICSTACK* value = &((LOGICSTACK*) m_stack.m_data)[m_stack.m_n + initializer - count];
				++initializer;
				value->m_num = GetInt();
				value->m_type |= 8;
			} while (Word(","));
			WordEnd("}");
		}
		else if (GameDesc->m_scriptDialect == GAME_SCRIPT_ZS1 && m_inBody && !m_declStatic) {
			int slot = m_stack.m_n - 1;
			vyrag();
			m_stackData[m_stackPos++] = 38;
			LOGIC_BYTECODE::WriteInt32(m_stackData + m_stackPos, slot);
			m_stackPos += 4;
			m_stackData[m_stackPos++] = 26;
			((LOGICSTACK*) m_stack.m_data)[slot].m_type &= ~0x40;
		}
		else {
			LOGICSTACK* value = &((LOGICSTACK*) m_stack.m_data)[m_stack.m_n - 1];
			value->m_num = GetInt();
			value->m_type |= 8;
			((LOGICSTACK*) m_stack.m_data)[m_stack.m_n - 1].m_type &= ~0x40;
		}
	}
	SetNoElement(count);
}

// STUB: ALIEN 0x420aa0
void LOGIC::StringVar()
{
	int flags = 0;
	STRING name;
	GetName(&name);

	if (m_variables.Location(name) >= 0) {
		Error(
			10,
			Printf(
				// STRING: ALIEN 0x483270
				"string redifinition '%s'",
				name.m_str
			),
			0
		);
		exit(1);
	}

	int count;
	if (Word("[")) {
		flags = 4;
		count = GetInt();
		WordEnd("]");
	}
	else {
		count = 1;
	}

	m_variables.Insert(name, LOGICVAR(1, m_stack.GetNo()));
	for (int element = 0; element < count; ++element) {
		m_stack.Push(LOGICSTACK(STRING(empty_str)));
		((LOGICSTACK*) m_stack.m_data)[m_stack.m_n - 1].m_type |= flags;
	}

	if (Word("=")) {
		char buffer[4096];
		if ((flags & 4) && Word("{")) {
			int initializer = 0;
			do {
				if (initializer >= count) {
					Error(10, "too many initializers", 0);
					exit(1);
				}
				GetString(buffer);
				LOGICSTACK* value = &((LOGICSTACK*) m_stack.m_data)[m_stack.m_n + initializer - count];
				value->m_str = buffer;
				value->m_type |= 8;
				++initializer;
			} while (Word(","));
			WordEnd("}");
		}
		else if (GameDesc->m_scriptDialect == GAME_SCRIPT_ZS1 && m_inBody && !m_declStatic && !(flags & 4)) {
			int slot = m_stack.m_n - 1;
			vyrag();
			m_stackData[m_stackPos++] = 38;
			LOGIC_BYTECODE::WriteInt32(m_stackData + m_stackPos, slot);
			m_stackPos += 4;
			m_stackData[m_stackPos++] = 26;
		}
		else {
			GetString(buffer);
			LOGICSTACK* value = &((LOGICSTACK*) m_stack.m_data)[m_stack.m_n - 1];
			value->m_str = buffer;
			value->m_type |= 8;
			((LOGICSTACK*) m_stack.m_data)[m_stack.m_n - 1].m_type &= ~0x40;
		}
	}
	SetNoElement(count);
}

// STUB: ALIEN 0x420d50
void LOGIC::mnog()
{
	STRING name;
	int operation;
	int unary;
	unsigned int indexCodeSize;
	int variable;
	bool done = false;

	while (!done) {
		unary = 0;
		operation = 36;
		if (m_pos[1] != '-' && Word("-")) {
			unary = 3;
		}
		else if (
			Word(
				// STRING: ALIEN 0x483420
				"~"
			)
		) {
			unary = 4;
		}
		else if (
			Word(
				// STRING: ALIEN 0x48341c
				"!"
			)
		) {
			unary = 5;
		}

		if (Word(
				// STRING: ALIEN 0x483418
				"--"
			)) {
			operation = 35;
		}
		else if (
			Word(
				// STRING: ALIEN 0x483414
				"++"
			)
		) {
			operation = 34;
		}
		else if (Word("&")) {
			operation = 37;
		}

		if (isdigit(*m_pos)) {
			GetName(&name);
			sscanf(name.m_str, "%i", &operation);
			switch (unary) {
			case 3:
				unary = 0;
				operation = -operation;
				break;
			case 4:
				unary = 0;
				operation = ~operation;
				break;
			case 5:
				unary = 0;
				operation = operation == 0;
				break;
			}
			m_stackData[m_stackPos++] = 1;
			LOGIC_BYTECODE::WriteInt32(m_stackData + m_stackPos, operation);
			m_stackPos += 4;
			done = true;
		}
		else if (*m_pos == '"') {
			m_stackData[m_stackPos++] = 2;
			m_stackPos += GetString(m_stackData + m_stackPos);
			done = true;
		}
		else if (*m_pos == '\'') {
			m_stackData[m_stackPos++] = 1;
			++m_pos;
			LOGIC_BYTECODE::WriteInt32(m_stackData + m_stackPos, *m_pos);
			m_stackPos += 4;
			++m_pos;
			if (*m_pos != '\'') {
				Error(
					13,
					// STRING: ALIEN 0x4833e0
					"second '",
					0
				);
				exit(1);
			}
			++m_pos;
			done = true;
		}
		else if (
			Word(
				// STRING: ALIEN 0x48340c
				"sizeof"
			)
		) {
			if (!Word("(")) {
				Error(
					13,
					// STRING: ALIEN 0x4833d0
					"'(' for sizeof",
					0
				);
				exit(1);
			}
			m_stackData[m_stackPos++] = 1;
			if (Word("int") || Word("string")) {
				LOGIC_BYTECODE::WriteInt32(m_stackData + m_stackPos, 4);
			}
			else {
				GetName(&name);
				variable = m_variables.Location(name);
				if (variable < 0 || m_variables.m_data[variable].m_var.m_flag != 1) {
					Error(
						4,
						// STRING: ALIEN 0x4833bc
						"sizeof parameter",
						0
					);
					exit(1);
				}
				LOGIC_BYTECODE::WriteInt32(m_stackData + m_stackPos, 4 * m_variables.m_data[variable].m_var.m_extra);
			}
			m_stackPos += 4;
			WordEnd(")");
			done = true;
		}
		else if (Word("static")) {
			m_declStatic = 1;
			if (Word("int")) {
				do {
					IntVar();
				} while (Word(","));
			}
			else {
				if (!Word("string")) {
					Error(4, "static variable", 0);
					exit(1);
				}
				do {
					StringVar();
				} while (Word(","));
			}
			m_declStatic = 0;
			done = true;
		}
		else if (Word("int")) {
			do {
				IntVar();
			} while (Word(","));
			done = true;
		}
		else if (Word("string")) {
			do {
				StringVar();
			} while (Word(","));
			done = true;
		}
		else if (
			Word(
				// STRING: ALIEN 0x4833f0
				"return"
			)
		) {
			vyrag();
			m_stackData[m_stackPos++] = 31;
			done = true;
		}
		else if (Word("(")) {
			vyrag();
			WordEnd(")");
			done = true;
		}
		else if (isalpha(*m_pos)) {
			GetName(&name);
			variable = m_variables.Location(name);

			if (variable >= 0) {
				NAMED_LIST_STRUCT_LOGICVAR& entry = m_variables.m_data[variable];
				switch (entry.m_var.m_flag) {
				case 1: {
					indexCodeSize = 0;
					char* indexCode = 0;
					if (Word("[")) {
						int indexStart = m_stackPos;
						LOGICSTACK* item = &((LOGICSTACK*) m_stack.m_data)[entry.m_var.m_a];
						if (!(item->m_type & 0x25)) {
							Error(
								10,
								// STRING: ALIEN 0x483320
								"[] for not array",
								0
							);
							exit(1);
						}
						vyrag();
						m_stackData[m_stackPos++] = 40;
						indexCodeSize = m_stackPos - indexStart;
						indexCode = (char*) operator new(indexCodeSize);
						if (!indexCode) {
							Error(2, name.m_str, 0);
							exit(1);
						}
						m_stackPos = indexStart;
						memcpy(indexCode, m_stackData + indexStart, indexCodeSize);
						WordEnd("]");
					}

					int command;
					if (m_pos[1] == '=' || !Word("=")) {
						if (Word(
								// STRING: ALIEN 0x48331c
								"+="
							)) {
							vyrag();
							command = 39;
							operation = 8;
						}
						else if (
							Word(
								// STRING: ALIEN 0x483318
								"-="
							)
						) {
							vyrag();
							command = 39;
							operation = 9;
						}
						else if (
							Word(
								// STRING: ALIEN 0x483314
								"/="
							)
						) {
							vyrag();
							command = 39;
							operation = 6;
						}
						else if (
							Word(
								// STRING: ALIEN 0x483310
								"*="
							)
						) {
							vyrag();
							command = 39;
							operation = 19;
						}
						else if (
							Word(
								// STRING: ALIEN 0x48330c
								"%="
							)
						) {
							vyrag();
							command = 39;
							operation = 7;
						}
						else if (
							Word(
								// STRING: ALIEN 0x483308
								"&="
							)
						) {
							vyrag();
							command = 39;
							operation = 12;
						}
						else if (
							Word(
								// STRING: ALIEN 0x483304
								"|="
							)
						) {
							vyrag();
							command = 39;
							operation = 11;
						}
						else if (
							Word(
								// STRING: ALIEN 0x483300
								"^="
							)
						) {
							vyrag();
							command = 39;
							operation = 10;
						}
						else if (
							Word(
								// STRING: ALIEN 0x4832fc
								"<<="
							)
						) {
							vyrag();
							command = 39;
							operation = 23;
						}
						else if (
							Word(
								// STRING: ALIEN 0x4832f8
								">>="
							)
						) {
							vyrag();
							command = 39;
							operation = 22;
						}
						else if (Word("++")) {
							command = 32;
						}
						else if (Word("--")) {
							command = 33;
						}
						else {
							command = operation;
						}
					}
					else {
						vyrag();
						command = 38;
					}

					if (indexCodeSize) {
						memcpy(m_stackData + m_stackPos, indexCode, indexCodeSize);
						m_stackPos += indexCodeSize;
						operator delete(indexCode);
					}
					else {
						LOGICSTACK* item = &((LOGICSTACK*) m_stack.m_data)[entry.m_var.m_a];
						if (item->m_type & 0x24) {
							if (command == 32 || command == 33 || command == 34 || command == 35) {
								Error(
									10,
									// STRING: ALIEN 0x4832c0
									"Increment or decrement for array",
									0
								);
								exit(1);
							}
							if (command != 36) {
								Error(
									4,
									// STRING: ALIEN 0x4832e4
									"operation for array",
									command
								);
								exit(1);
							}
						}
					}

					m_stackData[m_stackPos++] = command;
					LOGIC_BYTECODE::WriteInt32(m_stackData + m_stackPos, entry.m_var.m_a);
					m_stackPos += 4;
					if (command == 39) {
						m_stackData[m_stackPos++] = operation;
					}
					done = true;
					break;
				}
				case 2: {
					WordEnd("(");
					int count = 0;
					while (!Word(")")) {
						vyrag();
						Word(",");
						++count;
					}
					while (count < entry.m_var.m_extra) {
						int argument = count + entry.m_var.m_type;
						LOGICSTACK* item = &((LOGICSTACK*) m_stack.m_data)[argument];
						if (!(item->m_type & 8)) {
							break;
						}
						m_stackData[m_stackPos++] = 36;
						LOGIC_BYTECODE::WriteInt32(m_stackData + m_stackPos, argument);
						m_stackPos += 4;
						++count;
					}
					if (count != entry.m_var.m_extra) {
						Error(
							4,
							// STRING: ALIEN 0x483350
							"extern function parameters number",
							0
						);
						exit(1);
					}
					m_stackData[m_stackPos++] = entry.m_var.m_a;
					done = true;
					break;
				}
				case 3: {
					WordEnd("(");
					int count = 0;
					while (!Word(")")) {
						vyrag();
						Word(",");
						m_stackData[m_stackPos++] = 38;
						LOGIC_BYTECODE::WriteInt32(m_stackData + m_stackPos, count + entry.m_var.m_type);
						m_stackPos += 4;
						m_stackData[m_stackPos++] = 26;
						++count;
					}
					while (count < entry.m_var.m_extra) {
						int argument = count + entry.m_var.m_type;
						LOGICSTACK* item = &((LOGICSTACK*) m_stack.m_data)[argument];
						if (!(item->m_type & 8)) {
							break;
						}
						if (item->m_type & 1) {
							m_stackData[m_stackPos++] = 2;
							unsigned int len = strlen(item->m_str.m_str) + 1;
							memcpy(m_stackData + m_stackPos, item->m_str.m_str, len);
							m_stackPos += len;
						}
						else {
							m_stackData[m_stackPos++] = 1;
							LOGIC_BYTECODE::WriteInt32(m_stackData + m_stackPos, (int) item->m_num);
							m_stackPos += 4;
						}
						m_stackData[m_stackPos++] = 38;
						LOGIC_BYTECODE::WriteInt32(m_stackData + m_stackPos, argument);
						m_stackPos += 4;
						m_stackData[m_stackPos++] = 26;
						++count;
					}
					if (count != entry.m_var.m_extra) {
						Error(
							4,
							// STRING: ALIEN 0x483334
							"function parameters number",
							0
						);
						exit(1);
					}
					m_stackData[m_stackPos++] = 30;
					if (entry.m_var.m_a == -1) {
						m_protoFixups.push_back({m_stackPos, variable});
					}
					LOGIC_BYTECODE::WriteInt32(m_stackData + m_stackPos, entry.m_var.m_a);
					m_stackPos += 4;
					done = true;
					break;
				}
				case 4: {
					m_stackData[m_stackPos++] = 2;
					unsigned int len = strlen(entry.m_var.m_value.m_str) + 1;
					memcpy(m_stackData + m_stackPos, entry.m_var.m_value.m_str, len);
					m_stackPos += len;
					done = true;
					break;
				}
				case 5:
					m_stackData[m_stackPos++] = 1;
					LOGIC_BYTECODE::WriteInt32(m_stackData + m_stackPos, entry.m_var.m_a);
					m_stackPos += 4;
					done = true;
					break;
				case 7: {
					STRING message;
					if (*m_pos == ':') {
						Error(
							10,
							Printf(
								// STRING: ALIEN 0x483374
								"Label redefinition '%s'",
								name.m_str
							)
								.m_str,
							0
						);
					}
					else {
						Error(10, Printf("Incorrect use label '%s'", name.m_str).m_str, 0);
					}
					exit(1);
					break;
				}
				case 8:
					if (*m_pos != ':') {
						Error(
							10,
							Printf(
								// STRING: ALIEN 0x48338c
								"Incorrect use label '%s'",
								name.m_str
							)
								.m_str,
							0
						);
						exit(1);
					}
					++m_pos;
					entry.m_var.m_flag = 7;
					m_stackData[entry.m_var.m_a] = m_stackPos - entry.m_var.m_a;
					entry.m_var.m_a = m_stackPos;
					break;
				default:
					done = true;
					break;
				}
			}
			else {
				if (*m_pos != ':') {
					Error(
						10,
						Printf(
							// STRING: ALIEN 0x4832a4
							"Undeclared identifier '%s'",
							name.m_str
						)
							.m_str,
						0
					);
					exit(1);
				}
				++m_pos;
				m_variables.Insert(name, LOGICVAR(7, m_stackPos));
			}
		}
		else {
			if (unary) {
				Error(
					10,
					// STRING: ALIEN 0x483294
					"error symbol",
					0
				);
				exit(1);
			}
			if (strchr(
					// STRING: ALIEN 0x48328c
					".$#@`",
					*m_pos
				)) {
				Error(10, "error symbol", 0);
				exit(1);
			}
			done = true;
		}

		if (done && unary) {
			m_stackData[m_stackPos++] = unary;
		}
	}
}

// FUNCTION: ALIEN 0x421b70
void LOGIC::SetOperation(int p_pos, int p_op)
{
	if (m_stackData[p_pos] == 1 && m_stackData[p_pos + 5] == 1 && m_stackPos - p_pos == 10) {
		int left = LOGIC_BYTECODE::ReadInt32(m_stackData + p_pos + 1);
		const int right = LOGIC_BYTECODE::ReadInt32(m_stackData + p_pos + 6);
		switch (p_op) {
		case 19:
			left *= right;
			break;
		case 6:
			left /= right;
			break;
		case 7:
			left %= right;
			break;
		case 8:
			left += right;
			break;
		case 9:
			left -= right;
			break;
		case 22:
			left >>= (unsigned int) right & 31;
			break;
		case 23:
			left = (int) ((unsigned int) left << ((unsigned int) right & 31));
			break;
		case 10:
			left ^= right;
			break;
		case 12:
			left &= right;
			break;
		case 11:
			left |= right;
			break;
		default:
			break;
		}
		LOGIC_BYTECODE::WriteInt32(m_stackData + p_pos + 1, left);
		m_stackPos -= 5;
	}
	else {
		m_stackData[m_stackPos] = p_op;
		++m_stackPos;
	}
}

// FUNCTION: ALIEN 0x421d30
void LOGIC::slag()
{
	int pos = m_stackPos;
	mnog();
	while (1) {
		// STRING: ALIEN 0x48342c
		if (Word("*")) {
			mnog();
			SetOperation(pos, 19);
			continue;
		}
		// STRING: ALIEN 0x483428
		if (Word("/")) {
			mnog();
			SetOperation(pos, 6);
			continue;
		}
		if (Word("%")) {
			mnog();
			SetOperation(pos, 7);
			continue;
		}
		break;
	}
}

// FUNCTION: ALIEN 0x421db0
void LOGIC::cmpslag()
{
	int pos = m_stackPos;
	slag();
	while (1) {
		// STRING: ALIEN 0x4819fc
		if (Word("+")) {
			slag();
			SetOperation(pos, 8);
			continue;
		}
		// STRING: ALIEN 0x483424
		if (Word("-")) {
			slag();
			SetOperation(pos, 9);
			continue;
		}
		break;
	}
}

// FUNCTION: ALIEN 0x421e10
void LOGIC::logicslag()
{
	int pos = m_stackPos;
	cmpslag();
	while (1) {
		// STRING: ALIEN 0x48344c
		if (Word(">=")) {
			cmpslag();
			m_stackData[m_stackPos] = 0x11;
			++m_stackPos;
			continue;
		}
		// STRING: ALIEN 0x483448
		if (Word(">>")) {
			cmpslag();
			SetOperation(pos, 0x16);
			continue;
		}
		// STRING: ALIEN 0x483444
		if (Word(">")) {
			cmpslag();
			m_stackData[m_stackPos] = 0x0f;
			++m_stackPos;
			continue;
		}
		// STRING: ALIEN 0x483440
		if (Word("<=")) {
			cmpslag();
			m_stackData[m_stackPos] = 0x12;
			++m_stackPos;
			continue;
		}
		// STRING: ALIEN 0x48343c
		if (Word("<<")) {
			cmpslag();
			SetOperation(pos, 0x17);
			continue;
		}
		// STRING: ALIEN 0x483438
		if (Word("<")) {
			cmpslag();
			m_stackData[m_stackPos] = 0x10;
			++m_stackPos;
			continue;
		}
		// STRING: ALIEN 0x483434
		if (Word("==")) {
			cmpslag();
			m_stackData[m_stackPos] = 0x0d;
			++m_stackPos;
			continue;
		}
		// STRING: ALIEN 0x483430
		if (Word("!=")) {
			cmpslag();
			m_stackData[m_stackPos] = 0x14;
			++m_stackPos;
			continue;
		}
		break;
	}
}

// FUNCTION: ALIEN 0x421f60
void LOGIC::vyragAnd()
{
	int pos = m_stackPos;
	logicslag();
	while (1) {
		skipempty();
		if (*m_pos != '&' || m_pos[1] == '&') {
			break;
		}
		Word("&");
		logicslag();
		SetOperation(pos, 12);
	}
}

void LOGIC::vyragXor()
{
	int pos = m_stackPos;
	vyragAnd();
	while (Word("^")) {
		vyragAnd();
		SetOperation(pos, 10);
	}
}

void LOGIC::vyragOr()
{
	int pos = m_stackPos;
	vyragXor();
	while (1) {
		skipempty();
		if (*m_pos != '|' || m_pos[1] == '|') {
			break;
		}
		Word("|");
		vyragXor();
		SetOperation(pos, 11);
	}
}

void LOGIC::vyragCmpAnd()
{
	vyragOr();
	while (Word("&&")) {
		m_stackData[m_stackPos] = 44;
		int skip = m_stackPos + 1;
		m_stackPos = skip + 4;
		vyragOr();
		m_stackData[m_stackPos] = 0x15;
		++m_stackPos;
		LOGIC_BYTECODE::WriteInt32(m_stackData + skip, m_stackPos - skip);
	}
}

void LOGIC::vyrag()
{
	if (GameDesc->m_scriptDialect == GAME_SCRIPT_ZS1) {
		vyragCmpAnd();
		while (Word("||")) {
			m_stackData[m_stackPos] = 45;
			int skip = m_stackPos + 1;
			m_stackPos = skip + 4;
			vyragCmpAnd();
			m_stackData[m_stackPos] = 0x0e;
			++m_stackPos;
			LOGIC_BYTECODE::WriteInt32(m_stackData + skip, m_stackPos - skip);
		}
		return;
	}

	int pos = m_stackPos;
	logicslag();
	while (1) {
		// STRING: ALIEN 0x483458
		if (Word("^")) {
			logicslag();
			SetOperation(pos, 10);
			continue;
		}
		// STRING: ALIEN 0x483454
		if (Word("&&")) {
			logicslag();
			m_stackData[m_stackPos] = 0x15;
			++m_stackPos;
			continue;
		}
		if (Word("&")) {
			logicslag();
			SetOperation(pos, 12);
			continue;
		}
		// STRING: ALIEN 0x483450
		if (Word("||")) {
			logicslag();
			m_stackData[m_stackPos] = 0x0e;
			++m_stackPos;
			continue;
		}
		// STRING: ALIEN 0x4819f8
		if (Word("|")) {
			logicslag();
			SetOperation(pos, 11);
			continue;
		}
		break;
	}
}

// FUNCTION: ALIEN 0x422030
int LOGIC::vyrag_oper()
{
	int result;
	do {
		vyrag();
		m_stackData[m_stackPos] = 25;
		LOGIC_BYTECODE::WriteInt32(m_stackData + (m_stackPos = m_stackPos + 1), m_line);
		m_stackPos += 4;
		// STRING: ALIEN 0x483268
		result = Word(",");
	} while (result);
	return result;
}

// script: Branch offsets are relative to their operand; loop fixups retain break targets until loop end.
// script: Labels share the variable table so forward goto references resolve when parsed.
// STUB: ALIEN 0x422080
void LOGIC::oper(int* p_breakFixups)
{
	int inverted = Word(
		// STRING: ALIEN 0x4834e8
		"iff"
	);
	if (inverted || Word(
						// STRING: ALIEN 0x4834e4
						"if"
					)) {
		WordEnd(
			// STRING: ALIEN 0x4833ec
			"("
		);
		vyrag();
		WordEnd(
			// STRING: ALIEN 0x4833b8
			")"
		);
		m_stackData[m_stackPos] = inverted ? 29 : 24;
		int branch = m_stackPos + 1;
		m_stackPos = branch + 4;
		oper(p_breakFixups);
		LOGIC_BYTECODE::WriteInt32(m_stackData + branch, m_stackPos - branch);
		if (Word(
				// STRING: ALIEN 0x48345c
				"else"
			)) {
			LOGIC_BYTECODE::WriteInt32(m_stackData + branch, LOGIC_BYTECODE::ReadInt32(m_stackData + branch) + 5);
			m_stackData[m_stackPos] = 28;
			int endBranch = m_stackPos + 1;
			m_stackPos = endBranch + 4;
			oper(p_breakFixups);
			LOGIC_BYTECODE::WriteInt32(m_stackData + endBranch, m_stackPos - endBranch);
		}
	}
	else if (
		Word(
			// STRING: ALIEN 0x4834dc
			"while"
		)
	) {
		int breakFixups[128] = {0};
		WordEnd("(");
		int loopStart = m_stackPos;
		vyrag();
		WordEnd(")");
		m_stackData[m_stackPos] = 24;
		int loopEnd = ++m_stackPos;
		m_stackPos += 4;
		oper(breakFixups);
		m_stackData[m_stackPos] = 28;
		int loopBack = ++m_stackPos;
		LOGIC_BYTECODE::WriteInt32(m_stackData + loopBack, loopStart - loopBack);
		m_stackPos += 4;
		LOGIC_BYTECODE::WriteInt32(m_stackData + loopEnd, m_stackPos - loopEnd);
		for (int* fixup = breakFixups; *fixup; ++fixup) {
			LOGIC_BYTECODE::WriteInt32(m_stackData + *fixup, m_stackPos - *fixup);
		}
	}
	else if (
		Word(
			// STRING: ALIEN 0x4834d8
			"do"
		)
	) {
		int breakFixups[128] = {0};
		int loopStart = m_stackPos;
		oper(breakFixups);
		WordEnd("while");
		WordEnd("(");
		vyrag();
		WordEnd(")");
		m_stackData[m_stackPos++] = 5;
		m_stackData[m_stackPos++] = 24;
		int loopBack = m_stackPos;
		LOGIC_BYTECODE::WriteInt32(m_stackData + m_stackPos, loopStart - loopBack);
		m_stackPos += 4;
		for (int* fixup = breakFixups; *fixup; ++fixup) {
			LOGIC_BYTECODE::WriteInt32(m_stackData + *fixup, m_stackPos - *fixup);
		}
	}
	else if (
		Word(
			// STRING: ALIEN 0x4834d4
			"for"
		)
	) {
		int breakFixups[128] = {0};
		WordEnd("(");
		vyrag_oper();
		WordEnd(";");
		int condition = m_stackPos;
		vyrag();
		WordEnd(";");
		m_stackData[m_stackPos] = 24;
		int loopEnd = ++m_stackPos;
		m_stackPos += 4;
		m_stackData[m_stackPos] = 28;
		int bodyBranch = m_stackPos + 1;
		int increment = m_stackPos + 5;
		m_stackPos += 5;
		vyrag_oper();
		WordEnd(")");
		m_stackData[m_stackPos++] = 28;
		int conditionBack = m_stackPos;
		LOGIC_BYTECODE::WriteInt32(m_stackData + conditionBack, condition - conditionBack);
		m_stackPos += 4;
		LOGIC_BYTECODE::WriteInt32(m_stackData + bodyBranch, m_stackPos - bodyBranch);
		oper(breakFixups);
		m_stackData[m_stackPos++] = 28;
		int incrementBack = m_stackPos;
		LOGIC_BYTECODE::WriteInt32(m_stackData + m_stackPos, increment - incrementBack);
		m_stackPos += 4;
		LOGIC_BYTECODE::WriteInt32(m_stackData + loopEnd, m_stackPos - loopEnd);
		for (int* fixup = breakFixups; *fixup; ++fixup) {
			LOGIC_BYTECODE::WriteInt32(m_stackData + *fixup, m_stackPos - *fixup);
		}
	}
	else if (
		Word(
			// STRING: ALIEN 0x4834cc
			"break"
		)
	) {
		WordEnd(";");
		if (!p_breakFixups) {
			Error(
				10,
				// STRING: ALIEN 0x4834b4
				"'break' without loop",
				0
			);
			exit(1);
		}
		int count = 0;
		while (p_breakFixups[count]) {
			if (count >= 128) {
				Error(
					10,
					// STRING: ALIEN 0x4834a0
					"Too many 'break'",
					0
				);
				exit(1);
			}
			++count;
		}
		m_stackData[m_stackPos++] = 28;
		p_breakFixups[count] = m_stackPos;
		m_stackPos += 4;
		p_breakFixups[count + 1] = 0;
	}
	else if (
		Word(
			// STRING: ALIEN 0x483498
			"goto"
		)
	) {
		STRING name;
		GetName(&name);
		int label = m_variables.Location(name);
		if (label < 0) {
			m_variables.Insert(name, LOGICVAR(8, m_stackPos + 1));
			label = m_variables.m_n - 1;
		}
		else {
			if (m_variables.m_data[label].m_var.m_flag == 8) {
				Error(
					10,
					Printf(
						// STRING: ALIEN 0x483478
						"second use undefined label '%s'",
						name.m_str
					),
					0
				);
				exit(1);
			}
			if (m_variables.m_data[label].m_var.m_flag != 7) {
				Error(
					10,
					Printf(
						// STRING: ALIEN 0x483464
						"'%s' is not label",
						name.m_str
					),
					0
				);
				exit(1);
			}
		}
		m_stackData[m_stackPos++] = 28;
		LOGIC_BYTECODE::WriteInt32(m_stackData + m_stackPos, m_variables.m_data[label].m_var.m_a - m_stackPos);
		m_stackPos += 4;
	}
	else if (Word("{")) {
		int variableCount = m_variables.m_n;
		while (!Word("}")) {
			oper(p_breakFixups);
		}
		if (variableCount <= 0) {
			m_variables.m_max = 0;
			m_variables.m_n = 0;
			delete[] m_variables.m_data;
			m_variables.m_data = 0;
		}
		else if (variableCount < m_variables.m_n) {
			m_variables.m_n = variableCount;
		}
	}
	else {
		vyrag_oper();
		WordEnd(";");
	}
}

// STUB: ALIEN 0x422760
int LOGIC::func()
{
	STRING name;
	char fileName[1024];

	if (skipempty2()) {
		return 1;
	}

	if (*m_pos == '#' && m_pos[1] == 'd' && m_pos[2] == 'e' && m_pos[3] == 'f' && m_pos[4] == 'i' && m_pos[5] == 'n' &&
		m_pos[6] == 'e') {
		m_pos += 7;
		m_unk0x54 = 1;
		int defineLine = m_line;
		GetName(&name);
		m_unk0x54 = 0;
		STRING value;
		if (m_line == defineLine) {
			GetLine(&value);
		}
		if (!strcmp(name.m_str, "GETSPRITE_VID")) {
			value = "0x2000";
		}
		int define;
		if ((define = m_strings.Location(name)) < 0) {
			m_strings.Insert(name, STRING(value.m_str));
		}
		else {
			m_strings.m_data[define].m_value = value;
		}
	}
	else if (
		*m_pos == '#' && m_pos[1] == 'u' && m_pos[2] == 'n' && m_pos[3] == 'd' && m_pos[4] == 'e' && m_pos[5] == 'f'
	) {
		m_pos += 6;
		m_unk0x54 = 1;
		GetName(&name);
		m_unk0x54 = 0;
		int define;
		if ((define = m_strings.Location(name)) < 0) {
			Error(
				4,
				// STRING: ALIEN 0x48354c
				"#undef parameters",
				0
			);
			exit(1);
		}
		if (define >= 0 && define < m_strings.m_n) {
			--m_strings.m_n;
			for (int i = define; i < m_strings.m_n; ++i) {
				NAMED_LIST_STRUCT_STRING& next = m_strings.m_data[i + 1];
				NAMED_LIST_STRUCT_STRING& entry = m_strings.m_data[i];
				entry.m_name = next.m_name;
				entry.m_value = next.m_value;
			}
		}
		if (!m_strings.m_n) {
			m_strings.m_max = 0;
			m_strings.m_n = 0;
			if (m_strings.m_data) {
				delete[] m_strings.m_data;
			}
			m_strings.m_data = 0;
		}
	}
	else if (
		Word(
			// STRING: ALIEN 0x483540
			"#include"
		)
	) {
		STRING oldName(m_name.m_str);
		if (*m_pos != '"' && *m_pos != '<') {
			Error(
				13,
				// STRING: ALIEN 0x48352c
				"include file name",
				0
			);
			exit(1);
		}
		++m_pos;
		int length = 0;
		while (*m_pos != '"' && *m_pos != '>') {
			if (m_pos >= m_end) {
				Error(10, "End of file", 0);
				exit(1);
			}
			fileName[length++] = *m_pos++;
		}
		fileName[length] = 0;
		++m_pos;

		FILE* file = Platform_FOpen(fileName, "rb");
		if (!file) {
			Error(7, fileName, 0);
			exit(1);
		}
		int size = compat_filelength(file);
		char* oldPos = m_pos;
		char* oldEnd = m_end;
		void* oldBuffer = m_unk0x44;
		int oldLine = m_line;
		int oldConditionalDepth = m_unk0x4c;

		m_unk0x44 = new char[size + 4096];
		if (!m_unk0x44) {
			Error(
				2,
				// STRING: ALIEN 0x483524
				"include",
				0
			);
			exit(1);
		}
		m_pos = (char*) m_unk0x44 + 4066;
		m_end = m_pos + size;
		fread(m_pos, size, 1, file);
		m_line = 0;
		m_name = fileName;
		m_unk0x4c = 0;
		while (!func()) {
		}

		fclose(file);
		delete[] (char*) m_unk0x44;
		m_pos = oldPos;
		m_end = oldEnd;
		m_unk0x44 = oldBuffer;
		m_line = oldLine;
		m_unk0x4c = oldConditionalDepth;
		m_name = oldName;
	}
	else if (
		Word(
			// STRING: ALIEN 0x48351c
			"extern"
		)
	) {
		int variableCount = m_variables.m_n;
		GetName(&name);
		if (m_variables.Location(name) >= 0) {
			Error(
				10,
				// STRING: ALIEN 0x483504
				"function redefinition",
				0
			);
			exit(1);
		}

		int stackStart = m_stack.m_n;
		WordEnd("(");
		do {
			if (Word(
					// STRING: ALIEN 0x483400
					"int"
				)) {
				IntVar();
			}
			else if (
				Word(
					// STRING: ALIEN 0x4833f8
					"string"
				)
			) {
				StringVar();
			}
		} while (Word(","));
		WordEnd(")");
		int parameterCount = m_stack.m_n - stackStart;
		int code = GetInt();
		if (!isdigit(m_pos[-1])) {
			Error(
				13,
				// STRING: ALIEN 0x4834ec
				"extern function code",
				0
			);
			exit(1);
		}

		if (variableCount <= 0) {
			m_variables.m_max = 0;
			m_variables.m_n = 0;
			delete[] m_variables.m_data;
			m_variables.m_data = 0;
		}
		else if (variableCount < m_variables.m_n) {
			m_variables.m_n = variableCount;
		}
		LOGICVAR function(2, code);
		InsertLocalLogicVar(&m_variables, name, &function, stackStart, parameterCount);
		WordEnd(";");
	}
	else if (
		Word(
			// STRING: ALIEN 0x483404
			"static"
		)
	) {
		if (Word("int")) {
			do {
				IntVar();
			} while (Word(","));
			WordEnd(";");
		}
		else if (Word("string")) {
			do {
				StringVar();
			} while (Word(","));
			WordEnd(";");
		}
		else {
			Error(
				4,
				// STRING: ALIEN 0x4833a8
				"static variable",
				0
			);
			exit(1);
		}
	}
	else if (Word("int")) {
		do {
			IntVar();
		} while (Word(","));
		WordEnd(";");
	}
	else if (Word("string")) {
		do {
			StringVar();
		} while (Word(","));
		WordEnd(";");
	}
	else {
		int variableCount = m_variables.m_n;
		GetName(&name);
		int declared = m_variables.Location(name);
		if (declared >= 0 &&
			!(m_variables.m_data[declared].m_var.m_flag == 3 && m_variables.m_data[declared].m_var.m_a == -1)) {
			Error(10, "function redefinition", 0);
			exit(1);
		}

		int stackStart = m_stack.m_n;
		int code = m_stackPos;
		WordEnd("(");
		do {
			if (Word("int")) {
				IntVar();
			}
			else if (Word("string")) {
				StringVar();
			}
		} while (Word(","));
		WordEnd(")");
		int parameterCount = m_stack.m_n - stackStart;

		if (Word(";")) {
			if (declared < 0) {
				if (variableCount <= 0) {
					m_variables.m_max = 0;
					m_variables.m_n = 0;
					delete[] m_variables.m_data;
					m_variables.m_data = 0;
				}
				else if (variableCount < m_variables.m_n) {
					m_variables.m_n = variableCount;
				}
				LOGICVAR function(3, -1);
				function.m_type = stackStart;
				function.m_extra = parameterCount;
				m_variables.Insert(name, function);
			}
			else if (variableCount < m_variables.m_n) {
				m_variables.m_n = variableCount;
			}
			return skipempty2();
		}

		if (declared >= 0) {
			LOGICVAR* proto = &m_variables.m_data[declared].m_var;
			if (parameterCount != proto->m_extra) {
				Error(4, "function parameters number", 0);
				exit(1);
			}
			int protoBase = proto->m_type;
			LOGICSTACK* slots = (LOGICSTACK*) m_stack.m_data;
			for (int p = 0; p < parameterCount; ++p) {
				if (slots[stackStart + p].m_type & 8) {
					slots[protoBase + p] = slots[stackStart + p];
				}
			}
			for (int v = variableCount; v < m_variables.m_n; ++v) {
				LOGICVAR* var = &m_variables.m_data[v].m_var;
				if (var->m_flag == 1 && var->m_a >= stackStart && var->m_a < stackStart + parameterCount) {
					var->m_a = protoBase + (var->m_a - stackStart);
				}
			}
		}

		WordEnd("{");
		m_inBody = 1;
		while (!Word("}")) {
			oper(0);
		}
		m_inBody = 0;
		m_stackData[m_stackPos++] = 31;

		if (variableCount <= 0) {
			m_variables.m_max = 0;
			m_variables.m_n = 0;
			delete[] m_variables.m_data;
			m_variables.m_data = 0;
		}
		else if (variableCount < m_variables.m_n) {
			m_variables.m_n = variableCount;
		}
		if (declared >= 0) {
			LOGICVAR* proto = &m_variables.m_data[declared].m_var;
			proto->m_a = code;
			for (size_t f = 0; f < m_protoFixups.size();) {
				if (m_protoFixups[f].second == declared) {
					LOGIC_BYTECODE::WriteInt32(m_stackData + m_protoFixups[f].first, code);
					m_protoFixups[f] = m_protoFixups.back();
					m_protoFixups.pop_back();
				}
				else {
					++f;
				}
			}
			if (!strcmp(name.m_str, "main")) {
				m_main = declared;
			}
		}
		else {
			LOGICVAR function(3, code);
			function.m_type = stackStart;
			function.m_extra = parameterCount;
			m_variables.Insert(name, function);
			if (!strcmp(name.m_str, "main")) {
				m_main = m_variables.m_n - 1;
			}
		}
	}

	return skipempty2();
}

// FUNCTION: ALIEN 0x4231c0
char** LOGIC::GetVariableStr(char** p_out, const STRING& p_name)
{
	char* variableName;
	p_name.Before(&variableName, "[");
	STRING variableText(variableName);
	int variable = m_variables.Location(variableText);
	if (variableName != STRING::EMPTY) {
		operator delete(variableName);
	}
	if (variable < 0) {
		MYERROR::Log(
			::Error,
			// STRING: ALIEN 0x483560
			"!!!ERROR!!! SCRIPT Can't find variable '%s' in GetVariableString",
			p_name.m_str
		);
		const char* str = empty_str;
		if (str && *str) {
			unsigned int len = strlen(str);
			*p_out = (char*) operator new((len & 0xfffffff0) + 16);
			memcpy(*p_out, str, len);
			(*p_out)[len] = 0;
		}
		else {
			*p_out = STRING::EMPTY;
		}
	}
	else {
		char* indexText;
		p_name.After(&indexText, "[");
		int index = STRING(indexText).Int();
		if (indexText != STRING::EMPTY) {
			operator delete(indexText);
		}

		LOGICSTACK* value = &((LOGICSTACK*) m_stack.m_data)[m_variables.m_data[variable].m_var.m_a + index];
		STRING* valueString = value->String();
		const char* str = valueString->m_str;
		if (*str) {
			unsigned int len = strlen(str);
			*p_out = (char*) operator new((len & 0xfffffff0) + 16);
			memcpy(*p_out, str, len);
			(*p_out)[len] = 0;
		}
		else {
			*p_out = STRING::EMPTY;
		}
	}
	return p_out;
}
