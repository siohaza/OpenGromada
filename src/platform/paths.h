#ifndef PLATFORM_PATHS_H
#define PLATFORM_PATHS_H

#include <stdio.h>

// Resolves Windows-style, inconsistently cased game paths.

// Executable data directory with a trailing separator. The environment override
// is ALIEN_SHOOTER_DATA_PATH.
const char* Platform_BasePath();

// Writable user directory with a trailing separator; ALIEN_SHOOTER_PREF_PATH overrides it.
const char* Platform_PrefPath();

// Executable path, or the base path when unavailable.
const char* Platform_ExecutablePath();

// True for POSIX-rooted, Windows drive-rooted, and UNC-style paths.
bool Platform_IsAbsolutePath(const char* p_path);

// Normalizes separators and resolves components case-insensitively. The returned
// pointer is valid until the next call on the same thread.
const char* Platform_ResolvePath(const char* p_path);

// Preference-path rename() and remove() wrappers.
int Platform_Rename(const char* p_from, const char* p_to);
int Platform_Remove(const char* p_path);

// Relative reads try data then preferences; relative writes target preferences
// and create parent directories. Absolute paths remain absolute.
FILE* Platform_FOpen(const char* p_path, const char* p_mode);

// Drops cached directory listings after writes.
void Platform_InvalidatePathCache();

#endif
