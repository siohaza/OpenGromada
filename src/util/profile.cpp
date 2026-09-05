#include "util/profile.h"

#include "game/game_descriptor.h"
#include "platform/ini.h"
#include "util/myerror.h"

#include <algorithm>
#include <ctype.h>
#include <map>
#include <set>
#include <string.h>
#include <string>

namespace
{

INI_FILE& ProfileFor(const STRING& p_name)
{
	static std::map<std::string, INI_FILE> cache;

	const char* path = p_name.m_str;
	auto it = cache.find(path);
	if (it == cache.end()) {
		it = cache.emplace(path, INI_FILE()).first;
		it->second.Load(path);
	}
	return it->second;
}

bool IsStringsProfile(const STRING& p_name)
{
	const char* base = p_name.m_str;
	for (const char* p = base; *p; ++p) {
		if (*p == '/' || *p == '\\') {
			base = p + 1;
		}
	}

	static const char expected[] = "strings.ini";
	for (size_t i = 0;; ++i) {
		const unsigned char a = (unsigned char) base[i];
		const unsigned char b = (unsigned char) expected[i];
		if (tolower(a) != tolower(b)) {
			return false;
		}
		if (!a) {
			return true;
		}
	}
}

void AppendUtf8(std::string& p_out, unsigned int p_codepoint)
{
	if (p_codepoint < 0x80) {
		p_out += (char) p_codepoint;
	}
	else if (p_codepoint < 0x800) {
		p_out += (char) (0xc0 | (p_codepoint >> 6));
		p_out += (char) (0x80 | (p_codepoint & 0x3f));
	}
	else {
		p_out += (char) (0xe0 | (p_codepoint >> 12));
		p_out += (char) (0x80 | ((p_codepoint >> 6) & 0x3f));
		p_out += (char) (0x80 | (p_codepoint & 0x3f));
	}
}

std::string Cp1251ToUtf8(const char* p_value)
{
	static const unsigned short special[64] = {
		0x0402, 0x0403, 0x201a, 0x0453, 0x201e, 0x2026, 0x2020, 0x2021, 0x20ac, 0x2030, 0x0409, 0x2039, 0x040a,
		0x040c, 0x040b, 0x040f, 0x0452, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, 0xfffd, 0x2122,
		0x0459, 0x203a, 0x045a, 0x045c, 0x045b, 0x045f, 0x00a0, 0x040e, 0x045e, 0x0408, 0x00a4, 0x0490, 0x00a6,
		0x00a7, 0x0401, 0x00a9, 0x0404, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x0407, 0x00b0, 0x00b1, 0x0406, 0x0456,
		0x0491, 0x00b5, 0x00b6, 0x00b7, 0x0451, 0x2116, 0x0454, 0x00bb, 0x0458, 0x0405, 0x0455, 0x0457
	};

	std::string result;
	result.reserve(strlen(p_value) * 2);
	for (const unsigned char* p = (const unsigned char*) p_value; *p; ++p) {
		unsigned int codepoint = *p;
		if (*p >= 0xc0) {
			codepoint = 0x0410 + (*p - 0xc0);
		}
		else if (*p >= 0x80) {
			codepoint = special[*p - 0x80];
		}
		AppendUtf8(result, codepoint);
	}
	return result;
}

void ReportExpansionLimit(const STRING& p_name, const STRING& p_app, const STRING& p_key, const char* p_reason)
{
	fprintf(stderr, "PROFILE[%s] %s [%s] %s: reference expansion stopped (%s)\n",
		GameDesc->m_profileId, p_name.m_str, p_app.m_str, p_key.m_str, p_reason);
	if (Error) {
		MYERROR::Log(Error, "PROFILE %s [%s] %s: reference expansion stopped (%s)\n",
			p_name.m_str, p_app.m_str, p_key.m_str, p_reason);
	}
}

std::string ExpandProfileReferences(
	INI_FILE& p_ini, const STRING& p_name, const STRING& p_app, const STRING& p_key,
	const char* p_value, const STRING& p_default)
{





	constexpr size_t maxBytes = 4095;
	constexpr size_t maxSubstitutions = 128;
	size_t inputSize = 0;
	while (inputSize <= maxBytes && p_value[inputSize]) ++inputSize;
	std::string value(p_value, std::min(inputSize, maxBytes));
	if (inputSize > maxBytes) {
		ReportExpansionLimit(p_name, p_app, p_key, "4095-byte input limit");
	}
	std::set<std::string> seen;
	seen.insert(value);
	for (size_t count = 0; count < maxSubstitutions; ++count) {
		const size_t begin = value.find('{');
		if (begin == std::string::npos) return value;
		const size_t colon = value.find(':', begin + 1);
		if (colon == std::string::npos) return value;
		const size_t end = value.find('}', colon + 1);
		if (end == std::string::npos) return value;
		const std::string section = value.substr(begin + 1, colon - begin - 1);
		const std::string key = value.substr(colon + 1, end - colon - 1);
		const char* replacement = p_ini.Get(section.c_str(), key.c_str());
		if (!replacement) replacement = p_default.m_str;
		if (!*replacement) return value;
		size_t replacementSize = 0;
		while (replacementSize < maxBytes && replacement[replacementSize]) ++replacementSize;
		const size_t remaining = value.size() - (end - begin + 1);
		if (replacementSize > maxBytes - remaining) {
			ReportExpansionLimit(p_name, p_app, p_key, "4095-byte output limit");
			return value;
		}
		std::string next = value.substr(0, begin);
		next.append(replacement, replacementSize);
		next.append(value, end + 1, std::string::npos);
		if (!seen.insert(next).second) {
			ReportExpansionLimit(p_name, p_app, p_key, "cyclic reference");
			return value;
		}
		value.swap(next);
	}
	if (value.find('{') != std::string::npos) {
		ReportExpansionLimit(p_name, p_app, p_key, "128-substitution limit");
	}
	return value;
}

} // namespace

// FUNCTION: ALIEN 0x407660
int PROFILE::Load(const STRING& p_name)
{
	m_name = empty_str;
	m_name = p_name;
	return 0;
}

// FUNCTION: ALIEN 0x407680
unsigned int PROFILE::GetInt(const STRING& p_app, const STRING& p_key, int p_default)
{
	return (unsigned int) ProfileFor(m_name).GetInt(p_app.m_str, p_key.m_str, p_default);
}

// GLOBAL: ALIEN 0x490734
PROFILE* Strings;

// FUNCTION: ALIEN 0x4076a0
STRING PROFILE::GetString(const STRING& p_app, const STRING& p_key, const STRING& p_default)
{
	INI_FILE& ini = ProfileFor(m_name);
	const char* value = ini.Get(p_app.m_str, p_key.m_str);
	std::string expanded;
	if (GameDesc->m_expandProfileReferences) {
		expanded = ExpandProfileReferences(ini, m_name, p_app, p_key, value ? value : p_default.m_str, p_default);
		if (!value) {


			return STRING(expanded.c_str());
		}
		value = expanded.c_str();
	}
	if (!value) {
		if (*p_default.m_str) {
			return STRING(p_default.m_str);
		}
		return STRING();
	}
	if (*value) {
		if (IsStringsProfile(m_name)) {
			const std::string utf8 = Cp1251ToUtf8(value);
			return STRING(utf8.c_str());
		}
		return STRING(value);
	}
	return STRING();
}
