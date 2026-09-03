
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace
{

struct SectionInfo {
	char tag[5];
	uint32_t count;
	uint32_t firstRecord;
};

uint32_t ReadU32(const std::vector<unsigned char>& data, size_t offset)
{
	uint32_t value;
	memcpy(&value, data.data() + offset, 4);
	return value;
}

const char* GuessEngine(uint32_t weapFirst, uint32_t sfxFirst, uint32_t objCount, uint32_t weapCount, uint32_t sfxCount)
{
	if (weapFirst == 68) {
		return "Locoland";
	}
	if (weapFirst == 632) {
		return "CrazyLunch";
	}
	if (weapFirst == 612) {
		if (sfxFirst == 21) {
			return "AS1";
		}
		if (sfxFirst == 37) {
			return "Theseus";
		}
		if (sfxFirst == 41) {
			return "ChacksTemple";
		}
		return "AS1-family?";
	}
	if (weapFirst == 640) {
		if (sfxFirst == 37) {
			return "AS2-Original";
		}
		if (sfxFirst == 57) {
			return "ObjectsExtended";
		}
		if (sfxFirst == 41) {
			if (objCount >= 1000 || weapCount >= 80 || sfxCount >= 99) {
				return "ZS1Mobile/AS2/ZS2";
			}
			return "ZS1/AS2/ZS2";
		}
		return "AS2-family?";
	}
	return "unknown";
}

int ProbeFile(const char* path)
{
	FILE* file = fopen(path, "rb");
	if (!file) {
		printf("%s: ERROR cannot open\n", path);
		return 1;
	}
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);
	std::vector<unsigned char> data(size > 0 ? (size_t) size : 0);
	if (size <= 0 || fread(data.data(), 1, data.size(), file) != data.size()) {
		fclose(file);
		printf("%s: ERROR cannot read\n", path);
		return 1;
	}
	fclose(file);

	const char* base = strrchr(path, '/');
	base = base ? base + 1 : path;

	if (data.size() < 12 || memcmp(data.data(), "RES ", 4) != 0) {
		printf("%s: ERROR no RES signature\n", base);
		return 1;
	}
	uint32_t totalSize = ReadU32(data, 4);
	char containerTag[5] = {};
	memcpy(containerTag, data.data() + 8, 4);
	for (int i = 0; i < 4; ++i) {
		if (!containerTag[i]) {
			containerTag[i] = '.';
		}
	}

	int errors = 0;
	std::vector<SectionInfo> sections;
	size_t offset = 12;
	while (offset + 8 <= data.size()) {
		size_t zeros = 0;
		while (offset < data.size() && data[offset] == 0 && zeros < 4) {
			++offset;
			++zeros;
		}
		if (offset + 20 > data.size()) {
			break;
		}

		SectionInfo info = {};
		memcpy(info.tag, data.data() + offset, 4);
		uint32_t dataSize = ReadU32(data, offset + 4);
		uint32_t magic = ReadU32(data, offset + 8);
		uint32_t unk0 = ReadU32(data, offset + 12);
		info.count = ReadU32(data, offset + 16);
		size_t sectionEnd = offset + 8 + dataSize;

		if (magic != 0x80000000u || unk0 != 0 || sectionEnd > data.size()) {
			printf("%s: ERROR bad section header '%s' at 0x%zx (magic=%#x unk=%u end=%zu size=%zu)\n",
				   base, info.tag, offset, magic, unk0, sectionEnd, data.size());
			return 1;
		}

		size_t cursor = offset + 20;
		bool isCnst = memcmp(info.tag, "CNST", 4) == 0;
		if (isCnst) {
			uint32_t blockSize = ReadU32(data, cursor);
			info.firstRecord = blockSize;
			cursor += 4 + blockSize;
			if (info.count != 1) {
				printf("%s: ERROR CNST count=%u\n", base, info.count);
				++errors;
			}
		}
		else {
			for (uint32_t i = 0; i < info.count; ++i) {
				if (cursor + 4 > sectionEnd) {
					printf("%s: ERROR %s record %u truncated at 0x%zx\n", base, info.tag, i, cursor);
					++errors;
					break;
				}
				uint32_t bytesLarge = ReadU32(data, cursor);
				if (i == 0) {
					info.firstRecord = bytesLarge;
				}
				cursor += 4 + bytesLarge;
			}
		}
		if (cursor != sectionEnd) {
			printf("%s: ERROR %s reconcile: cursor=0x%zx sectionEnd=0x%zx (delta %ld)\n",
				   base, info.tag, cursor, sectionEnd, (long) sectionEnd - (long) cursor);
			++errors;
		}
		sections.push_back(info);
		offset = sectionEnd;
	}

	uint32_t objCount = 0, weapCount = 0, sfxCount = 0, weapFirst = 0, sfxFirst = 0;
	std::string layout;
	char buffer[64];
	for (const SectionInfo& info : sections) {
		snprintf(buffer, sizeof(buffer), "%s%.4s n=%u first=%u", layout.empty() ? "" : " ", info.tag, info.count,
				 info.firstRecord);
		layout += buffer;
		if (!memcmp(info.tag, "OBJ ", 4)) {
			objCount = info.count;
		}
		else if (!memcmp(info.tag, "WEAP", 4)) {
			weapCount = info.count;
			weapFirst = info.firstRecord;
		}
		else if (!memcmp(info.tag, "SFX ", 4)) {
			sfxCount = info.count;
			sfxFirst = info.firstRecord;
		}
	}

	printf("%s: tag=%s total=%u | %s | guess=%s | %s\n", base, containerTag, totalSize, layout.c_str(),
		   GuessEngine(weapFirst, sfxFirst, objCount, weapCount, sfxCount), errors ? "ERRORS" : "OK");
	return errors ? 1 : 0;
}

}

int main(int argc, char** argv)
{
	if (argc < 2) {
		fprintf(stderr, "usage: res_probe <file.res> [more...]\n");
		return 2;
	}
	int failures = 0;
	for (int i = 1; i < argc; ++i) {
		failures += ProbeFile(argv[i]);
	}
	return failures ? 1 : 0;
}
