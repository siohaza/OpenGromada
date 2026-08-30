#ifndef REGISTRY_H
#define REGISTRY_H

#include "util/string.h"

class REGISTRY {
public:
	STRING m_path; // 0x00

	char** Path(char** p_subkey, void** p_hkey) const;
	int GetInt(const STRING& p_name, int p_default);
	STRING GetString(const STRING& p_name, const STRING& p_default);
	void SetString(const STRING& p_name, const STRING& p_value);
	void SetInt(const STRING& p_name, int p_data);
	void Delete(const STRING& p_name);
};

extern REGISTRY* Registry;

int Registry_GetIntExact(const STRING& p_path, const STRING& p_name, int p_default);

#endif
