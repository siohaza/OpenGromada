#include "util/resource.h"

#include "platform/paths.h"
#include "util/filter.h"
#include "util/myerror.h"

namespace
{

struct FOURCC_TEXT {
	char m_text[5];

	explicit FOURCC_TEXT(unsigned int p_type)
	{
		for (int i = 0; i < 4; ++i) {
			char c = (char) ((p_type >> (8 * i)) & 0xff);
			m_text[i] = (c >= 32 && c < 127) ? c : '?';
		}
		m_text[4] = 0;
	}
};

} // namespace

#include <stdlib.h>

// FUNCTION: ALIEN 0x4077c0
RESOURCE::RESOURCE()
{
	m_file = 0;
	m_resPos = 0;
	m_resSize = 0;
	m_begin = 0;
	m_end = 0;
	m_type = 0;
	m_signature = 0;
	m_flag = 0;
	m_state = 2;
}

// FUNCTION: ALIEN 0x407800
STRING* RESOURCE::Close()
{
	FILE* file = m_file;
	if (file) {
		if (m_flag & 1) {
			m_state = 2;
			fseek(file, m_begin + 4, 0);
			m_end += -8 - m_begin;
			Write(&m_end, 4);
		}
		fclose(m_file);
	}
	m_file = 0;
	m_end = 0;
	// STRING: ALIEN 0x48190c
	return &(m_name = "Not opened");
}

// FUNCTION: ALIEN 0x407870
int RESOURCE::Open(FILE* p_file, unsigned int p_type)
{
	unsigned int type;
	if (m_file) {
		Close();
	}
	m_file = p_file;
	if (!p_file) {
		if (::Error) {
			MYERROR::Error(
				::Error,
				"RES '%s' '%.4s'",
				7,
				// STRING: ALIEN 0x48196c
				"file is NULL",
				0,
				m_name.m_str,
				FOURCC_TEXT(m_type).m_text
			);
		}
		return 1;
	}
	m_begin = ftell(p_file);
	if (!Read(&m_signature, 4)) {
		if (m_signature == 0x20534552 || m_signature == 0x46464952) {
			Read(&m_end, 4);
			m_end += m_begin + 8;
			if (compat_filelength(m_file) < m_end) {
				int difference = compat_filelength(m_file) - m_end;
				if (::Error) {
					MYERROR::Error(
						::Error,
						"RES '%s' '%.4s'",
						10,
						// STRING: ALIEN 0x481934
						"Invalid filelength",
						difference,
						m_name.m_str,
						FOURCC_TEXT(m_type).m_text
					);
				}
			}
			Read(&type, 4);
			if (type == p_type || p_type == 0x20594e41) {
				GoBegin(0x20594e41);
				return 0;
			}
			m_type = type;
			if (::Error) {
				MYERROR::Error(
					::Error,
					"RES '%s' '%.4s'",
					4,
					// STRING: ALIEN 0x481924
					"resource type",
					0,
					m_name.m_str,
					FOURCC_TEXT(m_type).m_text
				);
			}
			Close();
			return 3;
		}
		if (::Error) {
			MYERROR::Error(
				::Error,
				"RES '%s' '%.4s'",
				4,
				// STRING: ALIEN 0x481948
				"resource signature",
				0,
				m_name.m_str,
				FOURCC_TEXT(m_type).m_text
			);
		}
		Close();
		return 2;
	}
	if (::Error) {
		MYERROR::Error(
			::Error,
			"RES '%s' '%.4s'",
			5,
			// STRING: ALIEN 0x481918
			"empty file",
			0,
			m_name.m_str,
			FOURCC_TEXT(m_type).m_text
		);
	}
	Close();
	return 4;
}

// FUNCTION: ALIEN 0x407a40
int RESOURCE::OpenForRead(const STRING& p_name, unsigned int p_type)
{
	if (m_file) {
		Close();
	}
	FILE* file = *p_name.m_str ? Platform_FOpen(p_name.m_str, "rb") : 0;
	if (!file) {
		if (::Error) {
			MYERROR::Error(::Error, "RES '%s' '%.4s'", 7, p_name.m_str, 0, m_name.m_str, FOURCC_TEXT(m_type).m_text);
		}
		return 1;
	}
	m_name = p_name;
	return Open(file, p_type);
}

// FUNCTION: ALIEN 0x407ac0
int RESOURCE::OpenForWrite(const STRING& p_name, unsigned int p_type)
{
	int headerSize = 4;
	if (m_file) {
		Close();
	}
	m_file = *p_name.m_str ? Platform_FOpen(
								 p_name.m_str,
								 // STRING: ALIEN 0x48197c
								 "w+b"
							 )
						   : 0;
	if (!m_file) {
		if (::Error) {
			MYERROR::Error(::Error, "RES '%s' '%.4s'", 3, p_name.m_str, 0, m_name.m_str, FOURCC_TEXT(m_type).m_text);
		}
		return 1;
	}
	m_begin = 0;
	m_end = 12;
	m_type = 0;
	m_name = p_name;
	m_signature = 0x20534552;
	Write(&m_signature, 4);
	Write(&headerSize, 4);
	Write(&p_type, 4);
	GoBegin(0x20594e41);
	return 0;
}

// FUNCTION: ALIEN 0x407b90
int RESOURCE::Read(void* p_buf, int p_size)
{
	FILE* file = m_file;
	int result = p_size;
	if (file && p_size) {
		if (!m_state) {
			m_state = 2;
			fseek(file, 0, SEEK_CUR);
		}
		m_state = 1;
		result = p_size - fread(p_buf, 1, p_size, m_file);
	}
	return result;
}

// FUNCTION: ALIEN 0x407bf0
int RESOURCE::Write(const void* p_buf, int p_size)
{
	FILE* file = m_file;
	int result = p_size;
	if (file && p_size) {
		if (m_state == 1) {
			m_state = 2;
			fseek(file, 0, SEEK_CUR);
		}
		m_state = 0;
		result = p_size - fwrite(p_buf, 1, p_size, m_file);
	}
	return result;
}

// FUNCTION: ALIEN 0x407c50
int RESOURCE::ReadPacked(void* p_buf, unsigned int p_size, FILTER* p_filter)
{
	FILE* file = m_file;
	if (!file || !p_size) {
		return p_size;
	}
	if (!p_filter) {
		return Read(p_buf, p_size);
	}
	return p_size - p_filter->Read(p_buf, p_size, file);
}

// FUNCTION: ALIEN 0x407ca0
int RESOURCE::WritePacked(const void* p_buf, unsigned int p_size, FILTER* p_filter)
{
	FILE* file = m_file;
	if (!file || !p_size) {
		return p_size;
	}
	if (!p_filter) {
		return Write(p_buf, p_size);
	}
	m_packedPos += p_size - p_filter->Write(p_buf, p_size, file);
	return 0;
}

// FUNCTION: ALIEN 0x407cf0
int RESOURCE::GoBegin(int p_type)
{
	if (!m_file) {
		return 1;
	}
	m_resPos = m_begin + 4;
	m_resSize = 0;
	return GoNext(p_type);
}

// FUNCTION: ALIEN 0x407d20
int RESOURCE::GoNext(int p_type)
{
	if (!m_file) {
		return 1;
	}
	while (1) {
		m_resPos += ((m_resSize + 1) & ~1) + 8;
		m_state = 2;
		fseek(m_file, m_resPos, 0);
		if (m_resPos >= m_end) {
			m_resPos -= ((m_resSize + 1) & ~1) + 8;
			m_state = 2;
			if (m_signature == 0x20534552) {
				fseek(m_file, m_resPos + 24, 0);
			}
			else {
				fseek(m_file, m_resPos + 8, 0);
			}
			return 2;
		}
		Read(&m_type, 4);
		Read(&m_resSize, 4);
		if (m_signature == 0x20534552) {
			Read(&m_subFlags, 4);
			if (m_subFlags & 0x80000000) {
				Read(&m_packedPos, 4);
				Read(&m_noSubRes, 4);
			}
			else {
				m_noSubRes = m_subFlags;
				m_subFlags = 0;
			}
			m_subPos = ftell(m_file);
			Read(&m_subSize, 4);
		}
		if (p_type != m_type && p_type != 0x20594e41) {
			continue;
		}
		return 0;
	}
}

// FUNCTION: ALIEN 0x407e70
int RESOURCE::GoNextSub(int p_type)
{
	FILE* file = m_file;
	if (!file) {
		return -1;
	}
	m_subPos += m_subSize + 4;
	int v5 = m_subPos;
	if (v5 >= m_resPos + m_resSize + 8) {
		return GoNext(p_type);
	}
	m_state = 2;
	fseek(file, v5, 0);
	Read(&m_subSize, 4);
	return 0;
}

// FUNCTION: ALIEN 0x407ee0
int RESOURCE::PreAppend(unsigned int p_sig, FILTER* p_filter)
{
	if (!m_file) {
		return -1;
	}
	if (p_filter) {
		m_subFlags |= 0x100;
	}
	while (!GoNext(0x20594e41))
		;
	if (m_type != p_sig) {
		m_end = m_end + 20;
		m_type = p_sig;
		m_noSubRes = 0;
		m_subFlags = 0;
		m_resPos += ((m_resSize + 1) & 0xfffffffe) + 8;
		m_resSize = 12;
	}
	int subPos = m_resPos + m_resSize + 8;
	m_subPos = subPos;
	m_subSize = 0;
	m_packedPos = 0;
	m_state = 2;
	fseek(m_file, subPos + 4, 0);
	return 0;
}

// FUNCTION: ALIEN 0x407f90
int RESOURCE::PostAppend()
{
	m_flag |= 1;
	m_subFlags |= 0x80000000;
	m_noSubRes = m_noSubRes + 1;
	int size = ftell(m_file) - 4 - m_subPos;
	m_subSize = size;
	m_resSize += size + 4;
	m_end = size + 4 + m_end;
	m_state = 2;
	fseek(m_file, m_subPos, 0);
	m_subPos += m_subSize + 4;
	Write(&m_subSize, 4);
	m_state = 2;
	fseek(m_file, m_resPos, 0);
	Write(&m_type, 4);
	Write(&m_resSize, 4);
	Write(&m_subFlags, 4);
	Write(&m_packedPos, 4);
	int result = Write(&m_noSubRes, 4);
	m_subFlags &= ~0x100u;
	return result;
}

// FUNCTION: ALIEN 0x408080
int RESOURCE::SubLoad(void** p_out, FILTER* p_filter)
{
	if (!m_file) {
		if (Error) {
			MYERROR::Error(
				Error,
				// STRING: ALIEN 0x48195c
				"RES '%s' '%.4s'",
				5,
				// STRING: ALIEN 0x4819a0
				"file not opened",
				0,
				m_name.m_str,
				FOURCC_TEXT(m_type).m_text
			);
		}
		return 0;
	}
	if (m_subSize <= 0) {
		if (Error) {
			MYERROR::Error(
				Error,
				"RES '%s' '%.4s'",
				11,
				// STRING: ALIEN 0x481998
				"SubLoad",
				0,
				m_name.m_str,
				FOURCC_TEXT(m_type).m_text
			);
		}
		return 0;
	}
	void* v4 = operator new(m_subSize);
	*p_out = v4;
	if (!v4) {
		if (Error) {
			MYERROR::Error(
				Error,
				"RES '%s' '%.4s'",
				2,
				// STRING: ALIEN 0x481988
				"Subload data",
				m_subSize,
				m_name.m_str,
				FOURCC_TEXT(m_type).m_text
			);
		}
		return 0;
	}
	if (Read(v4, m_subSize) && Error) {
		MYERROR::Error(
			Error,
			"RES '%s' '%.4s'",
			5,
			// STRING: ALIEN 0x481980
			"Subload",
			0,
			m_name.m_str,
			FOURCC_TEXT(m_type).m_text
		);
	}
	return m_subSize;
}

// FUNCTION: ALIEN 0x408150
int RESOURCE::Load(unsigned int p_type, void** p_out, int p_size)
{
	if (!m_file) {
		if (::Error) {
			MYERROR::Error(
				::Error,
				"RES '%s' '%.4s'",
				5,
				"file not opened",
				0,
				m_name.m_str,
				FOURCC_TEXT(m_type).m_text
			);
		}
		exit(1);
	}
	int count = GetNoSubRes(p_type);
	if (!count) {
		if (::Error) {
			MYERROR::Error(
				::Error,
				"RES '%s' '%.4s'",
				11,
				// STRING: ALIEN 0x4819f0
				"Load",
				p_type,
				m_name.m_str,
				FOURCC_TEXT(m_type).m_text
			);
		}
		exit(1);
	}
	GoBegin(p_type);
	if (!*p_out) {
		*p_out = operator new(p_size * count);
	}
	else {
		if (::Error) {
			MYERROR::Error(
				::Error,
				"RES '%s' '%.4s'",
				5,
				// STRING: ALIEN 0x4819e0
				"Already loaded",
				0,
				m_name.m_str,
				FOURCC_TEXT(m_type).m_text
			);
		}
	}
	if (!*p_out) {
		MYERROR::LogExit(
			::Error,
			// STRING: ALIEN 0x4819b0
			"ResLoad::type=%.4s no_sub=%i Not enough Memory",
			FOURCC_TEXT(p_type).m_text,
			count
		);
	}
	char* dst = (char*) *p_out;
	for (int i = count; i > 0; --i) {
		Read((char*) *p_out + (count - i) * p_size, p_size);
		GoNextSub(p_type);
	}
	return count;
}

// FUNCTION: ALIEN 0x408280
int RESOURCE::GetNoSubRes(int p_type)
{
	if (!m_file) {
		return 0;
	}
	int total = 0;
	if (!GoBegin(p_type)) {
		do {
			total += m_noSubRes;
		} while (!GoNext(p_type));
	}
	return total;
}

// FUNCTION: ALIEN 0x4082c0
int RESOURCE::Append(RESOURCE* p_src, unsigned int p_type)
{
	if (!m_file || !p_src->m_file) {
		return 1;
	}
	if (!p_src->GoBegin(p_type)) {
		PreAppend(p_type, 0);
		m_noSubRes = 0;
		int size = p_src->m_resSize;
		m_noSubRes = p_src->m_noSubRes;
		void* buf = operator new(size);
		if (buf) {
			while (1) {
				p_src->Read(buf, size);
				Write(buf, size);
				operator delete(buf);
				if (p_src->GoNext(p_type)) {
					break;
				}
				size = p_src->m_resSize;
				m_noSubRes += p_src->m_noSubRes;
				buf = operator new(size);
				if (!buf) {
					return 1;
				}
			}
			--m_noSubRes;
			PostAppend();
			return 0;
		}
		return 1;
	}
	return 0;
}
