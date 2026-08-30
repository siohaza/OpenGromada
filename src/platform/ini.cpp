#include "platform/ini.h"

#include "platform/paths.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace
{

bool IEquals(const std::string& p_a, const char* p_b)
{
	size_t i = 0;
	for (; i < p_a.size(); ++i) {
		unsigned char a = (unsigned char) p_a[i];
		unsigned char b = (unsigned char) p_b[i];
		if (!b) {
			return false;
		}
		if (tolower(a) != tolower(b)) {
			return false;
		}
	}
	return p_b[i] == 0;
}

std::string Trim(const char* p_begin, const char* p_end)
{
	while (p_begin < p_end && (unsigned char) *p_begin <= ' ') {
		++p_begin;
	}
	while (p_end > p_begin && (unsigned char) p_end[-1] <= ' ') {
		--p_end;
	}
	return std::string(p_begin, (size_t) (p_end - p_begin));
}

} // namespace

INI_FILE::Section* INI_FILE::Find(const char* p_section)
{
	for (Section& s : m_sections) {
		if (IEquals(s.m_name, p_section)) {
			return &s;
		}
	}
	return nullptr;
}

const INI_FILE::Section* INI_FILE::Find(const char* p_section) const
{
	return const_cast<INI_FILE*>(this)->Find(p_section);
}

bool INI_FILE::Load(const char* p_path)
{
	m_sections.clear();
	m_dirty = false;
	m_path = p_path ? p_path : "";

	FILE* file = p_path ? Platform_FOpen(p_path, "rb") : nullptr;
	if (!file) {
		return false;
	}

	m_sections.push_back(Section{std::string(), {}});

	char line[4096];
	while (fgets(line, sizeof(line), file)) {
		char* end = line + strlen(line);
		while (end > line && (end[-1] == '\n' || end[-1] == '\r')) {
			--end;
		}

		const char* p = line;
		while (p < end && (unsigned char) *p <= ' ') {
			++p;
		}
		if (p == end || *p == ';' || *p == '#') {
			continue;
		}

		if (*p == '[') {
			const char* close = (const char*) memchr(p, ']', (size_t) (end - p));
			if (close) {
				m_sections.push_back(Section{Trim(p + 1, close), {}});
			}
			continue;
		}

		const char* eq = (const char*) memchr(p, '=', (size_t) (end - p));
		if (!eq) {
			continue;
		}

		m_sections.back().m_entries.push_back(Entry{Trim(p, eq), Trim(eq + 1, end)});
	}

	fclose(file);
	return true;
}

bool INI_FILE::Save()
{
	if (m_path.empty()) {
		return false;
	}

	FILE* file = Platform_FOpen(m_path.c_str(), "wb");
	if (!file) {
		return false;
	}

	for (const Section& s : m_sections) {
		if (s.m_entries.empty()) {
			continue;
		}
		if (!s.m_name.empty()) {
			fprintf(file, "[%s]\n", s.m_name.c_str());
		}
		for (const Entry& e : s.m_entries) {
			fprintf(file, "%s=%s\n", e.m_key.c_str(), e.m_value.c_str());
		}
		fprintf(file, "\n");
	}

	if (fclose(file)) {
		return false;
	}
	m_dirty = false;
	return true;
}

void INI_FILE::MergeMissing(const INI_FILE& p_source)
{
	for (const Section& source : p_source.m_sections) {
		Section* target = Find(source.m_name.c_str());
		if (!target) {
			m_sections.push_back(Section{source.m_name, {}});
			target = &m_sections.back();
		}
		for (const Entry& entry : source.m_entries) {
			bool found = false;
			for (const Entry& current : target->m_entries) {
				if (IEquals(current.m_key, entry.m_key.c_str())) {
					found = true;
					break;
				}
			}
			if (!found) {
				target->m_entries.push_back(entry);
				m_dirty = true;
			}
		}
	}
}

const char* INI_FILE::Get(const char* p_section, const char* p_key) const
{
	const Section* section = Find(p_section ? p_section : "");
	if (!section) {
		return nullptr;
	}
	for (const Entry& e : section->m_entries) {
		if (IEquals(e.m_key, p_key ? p_key : "")) {
			return e.m_value.c_str();
		}
	}
	return nullptr;
}

int INI_FILE::GetInt(const char* p_section, const char* p_key, int p_default) const
{
	const char* value = Get(p_section, p_key);
	return value ? atoi(value) : p_default;
}

void INI_FILE::Set(const char* p_section, const char* p_key, const char* p_value)
{
	const char* name = p_section ? p_section : "";
	Section* section = Find(name);
	if (!section) {
		m_sections.push_back(Section{name, {}});
		section = &m_sections.back();
	}

	for (Entry& e : section->m_entries) {
		if (IEquals(e.m_key, p_key ? p_key : "")) {
			if (e.m_value != p_value) {
				m_dirty = true;
			}
			e.m_value = p_value ? p_value : "";
			return;
		}
	}

	section->m_entries.push_back(Entry{p_key ? p_key : "", p_value ? p_value : ""});
	m_dirty = true;
}

void INI_FILE::SetInt(const char* p_section, const char* p_key, int p_value)
{
	char buffer[32];
	snprintf(buffer, sizeof(buffer), "%d", p_value);
	Set(p_section, p_key, buffer);
}

void INI_FILE::Erase(const char* p_section, const char* p_key)
{
	Section* section = Find(p_section ? p_section : "");
	if (!section) {
		return;
	}
	for (size_t i = 0; i < section->m_entries.size(); ++i) {
		if (IEquals(section->m_entries[i].m_key, p_key ? p_key : "")) {
			section->m_entries.erase(section->m_entries.begin() + (long) i);
			m_dirty = true;
			return;
		}
	}
}
