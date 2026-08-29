#define DECOMP_UNINITIALIZED_STRING_DEFAULT_CTOR
#define DECOMP_INLINE_STRING_CHARP_CTOR
#define DECOMP_INLINE_STRING_COPY_CTOR
#define DECOMP_INLINE_STRING_DTOR
#include "util/registry.h"

#include <stdlib.h>
#include <windows.h>

// GLOBAL: ALIEN 0x492768
char g_registryBuf[512];

// GLOBAL: ALIEN 0x492968
char g_registryValueBuf[512];

// GLOBAL: ALIEN 0x492b68
REGISTRY* Registry;

// FUNCTION: ALIEN 0x4074c0
char** REGISTRY::Path(char** p_subkey, void** p_hkey) const
{
	const char* path;
	int length;
	if (!strncmp(m_path.m_str,
		// STRING: ALIEN 0x481900
		"HKEY_USERS\\", strlen("HKEY_USERS\\"))) {
		*p_hkey = HKEY_USERS;
		goto suffix;
	}
	if (!strncmp(m_path.m_str,
		// STRING: ALIEN 0x4818ec
		"HKEY_CURRENT_USER\\", strlen("HKEY_CURRENT_USER\\"))) {
		*p_hkey = HKEY_CURRENT_USER;
		goto suffix;
	}
	if (!strncmp(m_path.m_str,
		// STRING: ALIEN 0x4818d8
		"HKEY_CLASSES_ROOT\\", strlen("HKEY_CLASSES_ROOT\\"))) {
		*p_hkey = HKEY_CLASSES_ROOT;
		goto suffix;
	}
	if (!strncmp(m_path.m_str,
		// STRING: ALIEN 0x4818c0
		"HKEY_CURRENT_CONFIG\\", strlen("HKEY_CURRENT_CONFIG\\"))) {
		*p_hkey = HKEY_CURRENT_CONFIG;
		goto suffix;
	}
	if (!strncmp(m_path.m_str,
		// STRING: ALIEN 0x4818ac
		"HKEY_LOCAL_MACHINE\\", strlen("HKEY_LOCAL_MACHINE\\"))) {
		*p_hkey = HKEY_LOCAL_MACHINE;
		goto suffix;
	}
	*p_hkey = HKEY_LOCAL_MACHINE;
	path = m_path.m_str;
	if (*path) {
		length = strlen(path);
		*p_subkey = (char*) operator new((length & 0xfffffff0) + 16);
		memcpy(*p_subkey, path, length);
		(*p_subkey)[length] = 0;
	}
	else {
		*p_subkey = STRING::EMPTY;
	}
	return p_subkey;

suffix:
	m_path.After(p_subkey, "\\");
	return p_subkey;
}

// FUNCTION: ALIEN 0x42cf70
STRING REGISTRY::GetString(const STRING& p_name, const STRING& p_default)
{
	STRING subKey;
	HKEY hKey;
	Path((char**) &subKey, (void**) &hKey);
	if (!RegOpenKeyExA(hKey, subKey.m_str, 0, KEY_QUERY_VALUE, &hKey)) {
		DWORD type = 0;
		DWORD cbData = 511;
		RegQueryValueExA(hKey, p_name.m_str, 0, &type, (BYTE*) g_registryValueBuf, &cbData);
		RegCloseKey(hKey);
		if (type == REG_DWORD || type == REG_BINARY)
			_itoa(*(int*) g_registryValueBuf, g_registryValueBuf, 10);
		else if (type != REG_SZ)
			return p_default;
		return STRING(g_registryValueBuf);
	}
	return p_default;
}

// FUNCTION: ALIEN 0x42d130
int REGISTRY::GetInt(const STRING& p_name, int p_default)
{
	HKEY hKey;
	DWORD type;
	char* lpSubKey;
	DWORD cbData;
	Path(&lpSubKey, (void**) &hKey);
	int result = p_default;
	if (!RegOpenKeyExA(hKey, lpSubKey, 0, KEY_QUERY_VALUE, &hKey)) {
		type = 0;
		cbData = 511;
		RegQueryValueExA(hKey, p_name.m_str, 0, &type, (BYTE*) g_registryBuf, &cbData);
		if (type == REG_SZ)
			result = atoi(g_registryBuf);
		else if (type == REG_DWORD || type == REG_BINARY)
			result = *(int*) g_registryBuf;
		RegCloseKey(hKey);
	}
	if (lpSubKey != STRING::EMPTY)
		operator delete(lpSubKey);
	return result;
}

// FUNCTION: ALIEN 0x42d1f0
void REGISTRY::SetString(const STRING& p_name, const STRING& p_value)
{
	HKEY hKey;
	char* lpSubKey;
	DWORD dwDisposition;
	Path(&lpSubKey, (void**) &hKey);
	if (!RegCreateKeyExA(hKey, lpSubKey, 0, empty_str, 0, 0xf003f, 0, &hKey, &dwDisposition)) {
		RegSetValueExA(hKey, p_name.m_str, 0, REG_SZ, (const BYTE*) p_value.m_str, strlen(p_value.m_str) + 1);
		RegCloseKey(hKey);
	}
	if (lpSubKey != STRING::EMPTY)
		operator delete(lpSubKey);
}

// FUNCTION: ALIEN 0x42d280
void REGISTRY::SetInt(const STRING& p_name, int p_data)
{
	HKEY hKey;
	char* lpSubKey;
	DWORD dwDisposition;
	Path(&lpSubKey, (void**) &hKey);
	if (!RegCreateKeyExA(hKey, lpSubKey, 0, empty_str, 0, 0xf003f, 0, &hKey, &dwDisposition)) {
		RegSetValueExA(hKey, p_name.m_str, 0, REG_DWORD, (const BYTE*) &p_data, 4);
		RegCloseKey(hKey);
	}
	if (lpSubKey != STRING::EMPTY)
		operator delete(lpSubKey);
}

// FUNCTION: ALIEN 0x42d310
void REGISTRY::Delete(const STRING& p_name)
{
	HKEY hKey;
	char* lpSubKey;
	Path(&lpSubKey, (void**) &hKey);
	if (!RegOpenKeyExA(hKey, lpSubKey, 0, 0xf003f, &hKey)) {
		RegDeleteValueA(hKey, p_name.m_str);
		RegCloseKey(hKey);
	}
	if (lpSubKey != STRING::EMPTY)
		operator delete(lpSubKey);
}
