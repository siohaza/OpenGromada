#include "util/resource.h"

#include "game/game_descriptor.h"
#include "platform/paths.h"
#include "util/filter.h"
#include "util/myerror.h"

#include <climits>
#include <array>
#include <cstdint>
#include <cstring>
#include <string>

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

bool ReadHeaderWord(FILE* file, unsigned int& value)
{
	unsigned char bytes[4];
	if (fread(bytes, 1, sizeof(bytes), file) != sizeof(bytes)) {
		return false;
	}
	value = (unsigned int) bytes[0] | ((unsigned int) bytes[1] << 8) |
		((unsigned int) bytes[2] << 16) | ((unsigned int) bytes[3] << 24);
	return true;
}

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
	m_readOnly = false;
	m_failed = false;
	m_readBegin = 0;
	m_readEnd = -1;
	m_recordIndex = -1;
	m_fileEnd = 0;
	m_subFlags = 0;
	m_noSubRes = 0;
	m_subPos = 0;
	m_subSize = 0;
	m_packedPos = 0;
}

bool RESOURCE::Fail(const char* p_reason)
{
	if (!m_failed) {
		MYERROR::Log(::Error, "Resource error: profile=%s file='%s' section='%.4s' record=%i offset=%ld: %s",
			GameDesc ? GameDesc->m_profileId : "unknown", m_name.m_str, FOURCC_TEXT(m_type).m_text,
			m_recordIndex, m_file ? ftell(m_file) : -1L, p_reason);
	}
	m_failed = true;
	return false;
}

int RESOURCE::Remaining() const
{
	if (!m_file || m_failed || m_readEnd < 0) {
		return 0;
	}
	long pos = ftell(m_file);
	return pos >= m_readBegin && pos <= m_readEnd ? m_readEnd - (int) pos : 0;
}

int RESOURCE::ReadWords(void* p_buf, int p_size, int p_wordSize)
{
	if ((p_wordSize != 2 && p_wordSize != 4) || p_size < 0 || p_size % p_wordSize) {
		Fail("invalid scalar read size");
		return p_size > 0 ? p_size : 1;
	}
	int missing = Read(p_buf, p_size);
	if (missing) {
		return missing;
	}
	unsigned char* bytes = static_cast<unsigned char*>(p_buf);
	for (int i = 0; i < p_size; i += p_wordSize) {
		if (p_wordSize == 2) {
			uint16_t value = uint16_t(bytes[i]) | (uint16_t(bytes[i + 1]) << 8);
			memcpy(bytes + i, &value, 2);
		}
		else {
			uint32_t value = uint32_t(bytes[i]) | (uint32_t(bytes[i + 1]) << 8) |
				(uint32_t(bytes[i + 2]) << 16) | (uint32_t(bytes[i + 3]) << 24);
			memcpy(bytes + i, &value, 4);
		}
	}
	return 0;
}

bool RESOURCE::ReadString(STRING& p_string)
{
	std::string text;
	while (Remaining() > 0) {
		unsigned char c = 0;
		if (Read(&c, 1)) {
			return false;
		}
		if (c == 0 || c == '\n') {
			p_string = text.c_str();
			return true;
		}
		if (c != '\r') {
			text.push_back(static_cast<char>(c));
		}
	}
	return Fail("unterminated string in record");
}

bool RESOURCE::Skip(int p_size)
{
	if (p_size < 0 || p_size > Remaining()) {
		return Fail("skip exceeds record boundary");
	}
	m_state = 2;
	return fseek(m_file, p_size, SEEK_CUR) == 0 || Fail("record seek failed");
}

bool RESOURCE::RequireEnd()
{
	return Good() && (Remaining() == 0 || Fail("unexpected trailing bytes in record"));
}

// FUNCTION: ALIEN 0x407800
STRING* RESOURCE::Close()
{
	FILE* file = m_file;
	if (file) {
		if ((m_flag & 1) && !m_failed) {
			m_state = 2;
			if (fseek(file, m_begin + 4, SEEK_SET)) {
				Fail("cannot seek resource container length");
			}
			else {
				const int length = m_end - 8 - m_begin;
				Write(&length, 4);
			}
		}
		const int result = fclose(file);
		m_file = 0;
		if (result) Fail("cannot flush or close resource stream");
	}
	m_file = 0;
	m_end = 0;
	// STRING: ALIEN 0x48190c
	return &(m_name = "Not opened");
}

// FUNCTION: ALIEN 0x407870
int RESOURCE::Open(FILE* p_file, unsigned int p_type)
{
	if (m_file) {
		Close();
	}
	m_file = p_file;
	m_readOnly = true;
	m_failed = false;
	m_readEnd = -1;
	m_recordIndex = -1;
	m_flag = 0;
	m_state = 2;
	if (!p_file) {
		Fail("file is NULL");
		return 1;
	}
	long begin = ftell(p_file);
	unsigned int signature = 0, size = 0, type = 0;
	if (begin < 0 || begin > INT_MAX - 12 || !ReadHeaderWord(p_file, signature) ||
		!ReadHeaderWord(p_file, size) || !ReadHeaderWord(p_file, type)) {
		Fail("truncated container header");
		Close();
		return 4;
	}
	m_signature = signature;
	m_type = type;
	m_containerType = type;
	m_begin = (int) begin;
	if (signature != 0x20534552 && signature != 0x46464952) {
		Fail("unexpected container signature");
		Close();
		return 2;
	}
	if (type != p_type && p_type != 0x20594e41) {


		Close();
		return 3;
	}
	if (size < 4 || uint64_t(begin) + 8 + size > INT_MAX ||
		fseek(p_file, 0, SEEK_END) || uint64_t(ftell(p_file)) < uint64_t(begin) + 8 + size) {
		Fail("container length exceeds file boundary");
		Close();
		return 3;
	}
	m_end = m_begin + 8 + (int) size;
	const long fileEnd = ftell(p_file);
	if (fileEnd < 0 || fileEnd > INT_MAX) {
		Fail("resource file exceeds supported address range");
		Close();
		return 3;
	}
	m_fileEnd = (int) fileEnd;




	GoBegin(0x20594e41);
	if (m_failed) {
		Close();
		return 4;
	}
	return 0;
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
	m_readOnly = false;
	m_failed = false;
	m_readEnd = -1;
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
	if (p_size < 0 || (p_size > 0 && !p_buf)) {
		Fail("invalid read size or destination");
		return p_size > 0 ? p_size : 1;
	}
	if (m_readOnly && (m_failed || p_size > Remaining())) {
		Fail("read exceeds record boundary");
		return p_size;
	}
	FILE* file = m_file;
	int result = p_size;
	if (file && p_size) {
		if (!m_state) {
			m_state = 2;
			fseek(file, 0, SEEK_CUR);
		}
		m_state = 1;
		result = p_size - fread(p_buf, 1, p_size, m_file);
		if (result && m_readOnly) {
			Fail("truncated record payload");
		}
	}
	return result;
}

// FUNCTION: ALIEN 0x407bf0
int RESOURCE::Write(const void* p_buf, int p_size)
{
	if (m_failed || m_readOnly || !m_file || p_size < 0 || (p_size && !p_buf)) {
		Fail("invalid resource write");
		return p_size > 0 ? p_size : 1;
	}
	FILE* file = m_file;
	int result = p_size;
	if (file && p_size) {
		if (m_state == 1) {
			m_state = 2;
			if (fseek(file, 0, SEEK_CUR)) {
				Fail("cannot synchronize resource write position");
				return p_size;
			}
		}
		m_state = 0;
		result = p_size - fwrite(p_buf, 1, p_size, m_file);
		if (result) Fail("short resource write");
	}
	return result;
}

// FUNCTION: ALIEN 0x407c50
int RESOURCE::ReadPacked(void* p_buf, unsigned int p_size, FILTER* p_filter)
{
	FILE* file = m_file;
	if (!file || !p_size || p_size > INT_MAX || m_failed) {
		return p_size;
	}
	if (!p_filter) {
		return Read(p_buf, p_size);
	}
	long start = ftell(file);
	if (m_readOnly && (start < m_readBegin || start > m_readEnd)) {
		Fail("packed stream begins outside its record");
		return p_size;
	}
	int decoded = p_filter->Read(p_buf, p_size, file);
	if (decoded < 0 || (unsigned int) decoded > p_size ||
		(m_readOnly && ftell(file) > m_readEnd)) {
		Fail("packed stream exceeds its record");
		return p_size;
	}
	if ((unsigned int) decoded != p_size) {
		Fail("truncated packed stream");
	}
	return p_size - decoded;
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
	if (!m_file || m_failed) {
		return 1;
	}
	while (1) {
		int64_t next = int64_t(m_resPos) + ((int64_t(m_resSize) + 1) & ~int64_t(1)) + 8;
		if (next >= m_end) {
			return 2;
		}
		const int fileEnd = m_readOnly ? m_fileEnd : m_end;
		if (next < m_begin + 12 || next + 8 > fileEnd) {
			Fail("truncated section header");
			return 1;
		}
		m_resPos = (int) next;
		m_state = 2;
		unsigned int type = 0, size = 0;
		if (fseek(m_file, m_resPos, SEEK_SET) || !ReadHeaderWord(m_file, type) ||
			!ReadHeaderWord(m_file, size) || uint64_t(m_resPos) + 8 + size > (unsigned int) fileEnd) {
			Fail("section length exceeds physical file boundary");
			return 1;
		}
		if (m_readOnly && uint64_t(m_resPos) + 8 + size > (unsigned int) m_end + 1u) {
			MYERROR::Log(::Error, "Resource compatibility: profile=%s file='%s' section='%.4s' offset=%i: "
				"complete section extends past original container length", GameDesc->m_profileId,
				m_name.m_str, FOURCC_TEXT(type).m_text, m_resPos);
		}
		m_type = (int) type;
		m_resSize = (int) size;
		m_recordIndex = -1;
		m_readBegin = m_resPos + 8;
		m_readEnd = m_readBegin + m_resSize;
		m_noSubRes = 0;
		if (m_signature == 0x20534552) {
			unsigned int options = 0, packed = 0, count = 0;
			if (size < 4 || !ReadHeaderWord(m_file, options)) {
				Fail("truncated resource section metadata");
				return 1;
			}
			if (options & 0x80000000) {
				if (size < 12 || !ReadHeaderWord(m_file, packed) || !ReadHeaderWord(m_file, count)) {
					Fail("truncated extended section metadata");
					return 1;
				}
			}
			else {
				count = options;
				options = 0;
			}
			m_subFlags = (int) options;
			m_packedPos = (int) packed;
			m_subPos = (int) ftell(m_file);
			if (count > (unsigned int) (m_readEnd - m_subPos) / 4) {
				Fail("record count exceeds section length");
				return 1;
			}
			m_noSubRes = (int) count;

			int cursor = m_subPos;
			for (unsigned int i = 0; i < count; ++i) {
				unsigned int recordSize = 0;
				m_recordIndex = (int) i;
				if (cursor > m_readEnd - 4 || fseek(m_file, cursor, SEEK_SET) ||
					!ReadHeaderWord(m_file, recordSize) || recordSize > (unsigned int) (m_readEnd - cursor - 4)) {
					Fail("record length exceeds section boundary");
					return 1;
				}
				if (i == 0) {
					m_subSize = (int) recordSize;
				}
				cursor += 4 + (int) recordSize;
			}
			if (cursor != m_readEnd) {
				Fail("section record count does not cover its payload");
				return 1;
			}
			if (!count) {
				continue;
			}
			m_recordIndex = 0;
			m_readBegin = m_subPos + 4;
			m_readEnd = m_readBegin + m_subSize;
			fseek(m_file, m_readBegin, SEEK_SET);
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
	if (!file || m_failed) {
		return -1;
	}
	if (m_signature != 0x20534552 || m_recordIndex + 1 >= m_noSubRes) {
		return GoNext(p_type);
	}
	int64_t next = int64_t(m_subPos) + m_subSize + 4;
	int sectionEnd = m_resPos + m_resSize + 8;
	if (next + 4 > sectionEnd) {
		Fail("truncated next record");
		return 1;
	}
	m_subPos = (int) next;
	++m_recordIndex;
	m_state = 2;
	unsigned int size = 0;
	if (fseek(file, m_subPos, SEEK_SET) || !ReadHeaderWord(file, size) ||
		size > (unsigned int) (sectionEnd - m_subPos - 4)) {
		Fail("record length exceeds section boundary");
		return 1;
	}
	m_subSize = (int) size;
	m_readBegin = m_subPos + 4;
	m_readEnd = m_readBegin + m_subSize;
	return 0;
}

// FUNCTION: ALIEN 0x407ee0
int RESOURCE::PreAppend(unsigned int p_sig, FILTER* p_filter)
{
	if (!m_file || m_failed || m_readOnly) {
		Fail("invalid resource append state");
		return -1;
	}
	while (!GoNext(0x20594e41))
		;
	if (m_failed) return -1;


	if (m_type != (int) p_sig || !(m_subFlags & 0x80000000u) || (m_subFlags & 0x100) || p_filter) {
		const int64_t sectionPos = int64_t(m_resPos) + ((int64_t(m_resSize) + 1) & ~int64_t(1)) + 8;
		if (sectionPos < 12 || sectionPos + 24 > INT_MAX) {
			Fail("appended section exceeds resource limits");
			return -1;
		}
		m_end = (int) sectionPos + 20;
		m_type = p_sig;
		m_noSubRes = 0;
		m_subFlags = 0;
		m_resPos = (int) sectionPos;
		m_resSize = 12;
	}
	if (p_filter) m_subFlags |= 0x100;
	const int64_t recordPos = int64_t(m_resPos) + m_resSize + 8;
	if (recordPos + 4 > INT_MAX) {
		Fail("appended record exceeds resource limits");
		return -1;
	}
	int subPos = (int) recordPos;
	m_subPos = subPos;
	m_subSize = 0;
	m_packedPos = 0;
	m_state = 2;
	if (fseek(m_file, subPos + 4, SEEK_SET)) {
		Fail("cannot seek appended record");
		return -1;
	}
	return 0;
}

// FUNCTION: ALIEN 0x407f90
int RESOURCE::PostAppend()
{
	if (!m_file || m_failed || m_readOnly) {
		Fail("invalid completed resource record");
		return 1;
	}
	const long end = ftell(m_file);
	if (end < int64_t(m_subPos) + 4 || end > INT_MAX ||
		int64_t(m_resSize) + end - m_subPos > INT_MAX) {
		Fail("invalid completed resource record length");
		return 1;
	}
	m_flag |= 1;
	m_subFlags |= 0x80000000;
	m_noSubRes = m_noSubRes + 1;
	int size = (int) end - 4 - m_subPos;
	m_subSize = size;
	m_resSize += size + 4;


	m_end = m_subPos + 4 + size;
	m_state = 2;
	if (fseek(m_file, m_subPos, SEEK_SET)) {
		Fail("cannot seek resource record length");
		return 1;
	}
	m_subPos += m_subSize + 4;
	if (Write(&m_subSize, 4)) return 1;
	m_state = 2;
	if (fseek(m_file, m_resPos, SEEK_SET)) {
		Fail("cannot seek resource section header");
		return 1;
	}
	if (Write(&m_type, 4) || Write(&m_resSize, 4) || Write(&m_subFlags, 4) || Write(&m_packedPos, 4)) {
		return 1;
	}
	int result = Write(&m_noSubRes, 4);
	return result;
}

// FUNCTION: ALIEN 0x408080
int RESOURCE::SubLoad(void** p_out, FILTER* p_filter)
{
	if (!p_out || *p_out || m_failed || m_subSize <= 0 || m_subSize > Remaining()) {
		Fail("invalid subresource allocation or size");
		return 0;
	}
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
	if (Read(v4, m_subSize)) {
		operator delete(v4);
		*p_out = 0;
		return 0;
	}
	return m_subSize;
}

// FUNCTION: ALIEN 0x408150
int RESOURCE::Load(unsigned int p_type, void** p_out, int p_size)
{
	if (!m_file || m_failed || !p_out || *p_out || p_size <= 0) {
		Fail("invalid fixed-record table destination");
		return 0;
	}
	int count = GetNoSubRes(p_type);
	if (count <= 0 || (size_t) count > (size_t) m_fileEnd / p_size || GoBegin(p_type)) {
		Fail("invalid fixed-record table size");
		return 0;
	}
	char* data = static_cast<char*>(operator new((size_t) count * p_size));
	for (int i = 0; i < count; ++i) {
		if (m_subSize != p_size || Read(data + (size_t) i * p_size, p_size) ||
			(i + 1 < count && GoNextSub(p_type))) {
			Fail("fixed-record table length mismatch");
			operator delete(data);
			return 0;
		}
	}
	*p_out = data;
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
	return m_failed ? 0 : total;
}

// FUNCTION: ALIEN 0x4082c0
int RESOURCE::Append(RESOURCE* p_src, unsigned int p_type)
{
	if (!m_file || m_readOnly || m_failed || !p_src || !p_src->m_file || !p_src->Good() || p_src == this) {
		Fail("invalid resource append source or destination");
		return 1;
	}




	if (p_src->m_signature != m_signature) {
		Fail("cannot append sections with a different resource framing");
		return 1;
	}
	std::array<unsigned char, 16384> bytes;
	for (int found = p_src->GoBegin(p_type); !found; found = p_src->GoNext(p_type)) {
		const int64_t destination = (int64_t(m_end) + 1) & ~int64_t(1);
		const int64_t length = int64_t(p_src->m_resSize) + 8;
		if (length < 8 || destination + length > INT_MAX ||
			int64_t(p_src->m_resPos) + length > p_src->m_fileEnd) {
			Fail("appended section exceeds resource limits");
			return 1;
		}
		m_state = p_src->m_state = 2;
		if (fseek(m_file, m_end, SEEK_SET) ||
			(destination != m_end && fputc(0, m_file) == EOF) ||
			fseek(p_src->m_file, p_src->m_resPos, SEEK_SET)) {
			Fail("cannot seek resource append stream");
			return 1;
		}
		for (int64_t left = length; left;) {
			const size_t count = left < int64_t(bytes.size()) ? (size_t) left : bytes.size();
			if (fread(bytes.data(), 1, count, p_src->m_file) != count) {
				p_src->Fail("truncated appended section");
				Fail("cannot read appended section");
				return 1;
			}
			if (Write(bytes.data(), (int) count)) {
				Fail("cannot write appended section");
				return 1;
			}
			left -= count;
		}
		m_resPos = (int) destination;
		m_resSize = p_src->m_resSize;
		m_end = (int) (destination + length);
		m_type = p_src->m_type;
		m_subFlags = p_src->m_subFlags;
		m_noSubRes = p_src->m_noSubRes;
		m_packedPos = p_src->m_packedPos;
		m_flag |= 1;
	}
	if (!p_src->Good()) {
		Fail("invalid source section while appending resources");
		return 1;
	}
	return 0;
}
