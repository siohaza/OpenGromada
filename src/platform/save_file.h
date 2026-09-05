#pragma once

#include <string>



bool Platform_WriteSaveAtomic(const std::string& p_relative, const std::string& p_bytes);
bool Platform_ArchiveSave(const std::string& p_relative);





class Platform_StagedSave {
public:
	Platform_StagedSave() = default;
	~Platform_StagedSave();
	Platform_StagedSave(const Platform_StagedSave&) = delete;
	Platform_StagedSave& operator=(const Platform_StagedSave&) = delete;
	Platform_StagedSave(Platform_StagedSave&&) = delete;
	Platform_StagedSave& operator=(Platform_StagedSave&&) = delete;

	bool Begin(const std::string& p_relative);
	const std::string& Path() const { return m_temp; }
	bool Commit();

private:
	std::string m_relative;
	std::string m_target;
	std::string m_temp;
	std::string m_parent;
};
