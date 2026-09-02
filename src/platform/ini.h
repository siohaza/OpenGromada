#ifndef PLATFORM_INI_H
#define PLATFORM_INI_H

#include <string>
#include <vector>

class INI_FILE {
public:
	bool Load(const char* p_path);
	bool Save();
	void MergeMissing(const INI_FILE& p_source);

	const char* Get(const char* p_section, const char* p_key) const;
	int GetInt(const char* p_section, const char* p_key, int p_default) const;

	void Set(const char* p_section, const char* p_key, const char* p_value);
	void SetInt(const char* p_section, const char* p_key, int p_value);
	void Erase(const char* p_section, const char* p_key);
	void EraseSection(const char* p_section);
	const char* KeyAt(const char* p_section, int p_index) const;

	const std::string& Path() const { return m_path; }
	bool Dirty() const { return m_dirty; }

private:
	struct Entry {
		std::string m_key;
		std::string m_value;
	};
	struct Section {
		std::string m_name;
		std::vector<Entry> m_entries;
	};

	Section* Find(const char* p_section);
	const Section* Find(const char* p_section) const;

	std::vector<Section> m_sections;
	std::string m_path;
	bool m_dirty = false;
};

#endif
