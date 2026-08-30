#ifndef UTIL_PACKED_H
#define UTIL_PACKED_H

#include <cstring>
#include <type_traits>

template <typename T>
inline T PackedRead(const void* p_source)
{
	static_assert(std::is_trivially_copyable_v<T>);
	T value;
	std::memcpy(&value, p_source, sizeof(value));
	return value;
}

template <typename T>
inline void PackedWrite(void* p_destination, const T& p_value)
{
	static_assert(std::is_trivially_copyable_v<T>);
	std::memcpy(p_destination, &p_value, sizeof(p_value));
}

inline bool PackedRleRun(const unsigned char* p_run)
{
	return p_run[0] != 0 || p_run[1] != 0;
}

#endif
