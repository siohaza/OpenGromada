#ifndef MENU_PATH_H
#define MENU_PATH_H

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <string>

namespace MENU_PATH
{

inline int PreferredGamebarWidth(int p_logicalWidth, int p_uiScale)
{
	if (p_uiScale < 1) {
		p_uiScale = 1;
	}
	else if (p_uiScale > 3) {
		p_uiScale = 3;
	}
	if (p_logicalWidth <= 0) {
		p_logicalWidth = 640 * p_uiScale;
	}

	const int widths[] = {640, 800, 1024};
	int best = widths[0];
	long long bestDistance = (long long) p_logicalWidth - (long long) best * p_uiScale;
	if (bestDistance < 0) {
		bestDistance = -bestDistance;
	}
	for (size_t i = 1; i < sizeof(widths) / sizeof(widths[0]); ++i) {
		long long distance = (long long) p_logicalWidth - (long long) widths[i] * p_uiScale;
		if (distance < 0) {
			distance = -distance;
		}
		if (distance < bestDistance) {
			best = widths[i];
			bestDistance = distance;
		}
	}
	return best;
}

inline bool IsGamebarVariant(const char* p_path)
{
	if (!p_path || !*p_path) {
		return false;
	}
	const char* name = p_path;
	for (const char* c = p_path; *c; ++c) {
		if (*c == '/' || *c == '\\') {
			name = c + 1;
		}
	}
	const char prefix[] = "gamebar";
	for (size_t i = 0; i < sizeof(prefix) - 1; ++i) {
		if (!name[i] || tolower((unsigned char) name[i]) != prefix[i]) {
			return false;
		}
	}
	const char* number = name + sizeof(prefix) - 1;
	if (!isdigit((unsigned char) *number)) {
		return false;
	}
	while (isdigit((unsigned char) *number)) {
		++number;
	}
	return strlen(number) == 4 && number[0] == '.' && tolower((unsigned char) number[1]) == 'm' &&
		   tolower((unsigned char) number[2]) == 'e' && tolower((unsigned char) number[3]) == 'n' && number[4] == 0;
}

inline int GamebarVariantWidth(const char* p_path)
{
	if (!IsGamebarVariant(p_path)) {
		return 0;
	}
	const char* name = p_path;
	for (const char* c = p_path; *c; ++c) {
		if (*c == '/' || *c == '\\') {
			name = c + 1;
		}
	}
	name += 7; // "gamebar"
	int width = 0;
	while (isdigit((unsigned char) *name)) {
		if (width > 100000) {
			return 0;
		}
		width = width * 10 + (*name++ - '0');
	}
	return width;
}

inline int GamebarVariantHeight(int p_width)
{
	return p_width > 0 ? p_width * 3 / 4 : 0;
}

inline std::string CanonicalGamebar(const char* p_path, int p_width)
{
	std::string path = p_path ? p_path : "";
	size_t slash = path.find_last_of("/\\");
	std::string directory = slash == std::string::npos ? std::string() : path.substr(0, slash + 1);
	char suffix[32];
	snprintf(suffix, sizeof(suffix), "gamebar%d.men", p_width);
	return directory + suffix;
}

} // namespace MENU_PATH

#endif
