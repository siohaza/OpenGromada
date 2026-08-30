#include "platform/paths.h"
#include "util/string.h"

#include <string.h>
#include <sys/stat.h>
#include <time.h>

// FUNCTION: ALIEN 0x406900
STRING FFileTime(const STRING& p_name)
{
	STRING result;

	struct stat st;
	memset(&st, 0, sizeof(st));
	if (stat(Platform_ResolvePath(p_name.m_str), &st) != 0) {
		return result;
	}

	const time_t times[3] = {st.st_ctime, st.st_atime, st.st_mtime};
	static const char* const labels[3] = {
		// STRING: ALIEN 0x481858
		"Cr-",
		// STRING: ALIEN 0x481838
		" La-",
		// STRING: ALIEN 0x481830
		" Lw-",
	};

	for (int i = 0; i < 3; ++i) {
		char text[80];
		struct tm local;
#if defined(_WIN32)
		localtime_s(&local, &times[i]);
#else
		localtime_r(&times[i], &local);
#endif
		result += labels[i];
		// STRING: ALIEN 0x48184c
		strftime(text, sizeof(text), "%Y-%m-%d", &local);
		result += text;
		result += " ";
		// STRING: ALIEN 0x481840
		strftime(text, sizeof(text), "%H:%M:%S", &local);
		result += text;
	}

	return result;
}
