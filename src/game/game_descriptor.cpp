#include "game/game_descriptor.h"

#include "platform/ini.h"
#include "platform/paths.h"
#include "video/movie_player.h"

#include <SDL3/SDL.h>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace
{




const GAME_DESCRIPTOR g_games[] = {
	{GAME_AS1, "AlienShooter", "Alien Shooter", "com.sigmateam.alienshooter", 612, 0x800, 640, 480, 0, 0,
	 "as1", "AlienShooter.cfg", "objects.res", 10, GAME_OBJ_AS1, GAME_SFX_AS1, GAME_SCRIPT_AS1, GAME_HUD_AS1, GAME_LAYERS_AS1, "experimental", true},
	{GAME_ZS1, "ZombieShooter", "Zombie Shooter", "com.sigmateam.zombieshooter", 640, 0x1000, 1024, 768, 1280, 720,


	 "zs1", "ZombieShooter.cfg", "objects.res", 15, GAME_OBJ_ZS1, GAME_SFX_ZS1, GAME_SCRIPT_ZS1, GAME_HUD_ZS1, GAME_LAYERS_ZS1, "experimental", true, 1024, 768, GAME_MENU_ZS1, 20},
	{GAME_THESEUS, "Theseus", "Theseus - Return of the Hero", "com.sigmateam.theseus", 612, 0x800, 640, 480, 0, 0,
	 "theseus", "Theseus.cfg", "objects.res", 11, GAME_OBJ_AS1, GAME_SFX_THESEUS, GAME_SCRIPT_THESEUS, GAME_HUD_AS1, GAME_LAYERS_AS1, "experimental", true, 0, 0, GAME_MENU_AS1, 17},
	{GAME_CRAZY_LUNCH, "CrazyLunch", "Crazy Lunch", "com.sigmateam.crazylunch", 632, 0x800, 800, 600, 0, 0,
	 "crazy-lunch", "CrazyLunch.cfg", "objects.res", 12, GAME_OBJ_AS1, GAME_SFX_AS1, GAME_SCRIPT_CRAZY_LUNCH, GAME_HUD_AS1, GAME_LAYERS_AS1, "experimental", true, 0, 0, GAME_MENU_AS1, 0, false},
	{GAME_LAST_HOPE, "AlienShooterLastHope", "Alien Shooter - Last Hope", "com.sigmateam.alienshooterlasthope", 612, 0x800, 640, 480, 0, 0,
	 "last-hope", "LastHope.cfg", "objects.res", 10, GAME_OBJ_AS1, GAME_SFX_AS1, GAME_SCRIPT_AS1, GAME_HUD_AS1, GAME_LAYERS_AS1, "experimental", false},
	{GAME_CHACKS_TEMPLE, "ChacksTemple", "Chak's Temple", "com.sigmateam.chackstemple", 612, 0x800, 1024, 768, 0, 0,
	 "chacks-temple", "ChakTemple.cfg", "objects.res", 0, GAME_OBJ_AS1, GAME_SFX_ZS1, GAME_SCRIPT_AS1, GAME_HUD_AS1, GAME_LAYERS_AS1, "experimental", false, 0, 0, GAME_MENU_AS1, 0, true, "ChacksTemple.cfg"},
	{GAME_LOCOLAND, "Locoland", "Locoland / Steamland", "com.sigmateam.locoland", 68, 0x800, 640, 480, 0, 0,
	 "locoland", "Locoland.cfg", "objects.res", 0, GAME_OBJ_LOCOLAND, GAME_SFX_AS1, GAME_SCRIPT_LOCOLAND, GAME_HUD_AS1, GAME_LAYERS_LOCOLAND, "experimental", true, 0, 0, GAME_MENU_AS1, 0, true, "steam.cfg", true, true, true, false, true, 10, true, true, GAME_ENEMY_SEARCH_HASH, true, true},
};

int g_cliOverride = -1;
bool g_probeJson = false;
bool g_detected = false;
std::string g_configOverride;
std::string g_config;
std::string g_resource = "objects.res";
std::string g_startMap = "maps\\logo.map";
std::string g_edition = "unspecified";
std::string g_error;
std::vector<const GAME_DESCRIPTOR*> g_candidates;

struct RECORD {
	size_t m_offset;
	size_t m_size;
};
struct SECTION {
	std::vector<RECORD> m_records;
};
struct RESOURCE_VIEW {
	std::vector<unsigned char> m_bytes;
	std::map<std::string, SECTION> m_sections;
};
RESOURCE_VIEW g_resourceView;

uint32_t Little32(const std::vector<unsigned char>& p_bytes, size_t p_offset)
{
	return uint32_t(p_bytes[p_offset]) | (uint32_t(p_bytes[p_offset + 1]) << 8) |
		(uint32_t(p_bytes[p_offset + 2]) << 16) | (uint32_t(p_bytes[p_offset + 3]) << 24);
}

bool Fail(const std::string& p_reason, const std::string& p_section = "DATA", size_t p_record = 0, size_t p_offset = 0)
{
	char position[128];
	snprintf(position, sizeof(position), " section '%s', record %zu, byte %zu: ", p_section.c_str(), p_record, p_offset);
	g_error = "Resource '" + g_resource + "'" + position + p_reason;
	return false;
}

bool ReadResource()
{
	g_resourceView = {};

	std::string path = Platform_IsAbsolutePath(g_resource.c_str()) ? g_resource : std::string(Platform_BasePath()) + g_resource;
	FILE* file = Platform_FOpen(path.c_str(), "rb");
	if (!file) return Fail("file not found under the selected data directory");
	if (fseek(file, 0, SEEK_END) != 0) {
		fclose(file);
		return Fail("cannot determine file size");
	}
	long size = ftell(file);


	if (size < 12 || size > INT_MAX || size > 256 * 1024 * 1024) {
		fclose(file);
		return Fail("invalid or unsupported resource size");
	}
	rewind(file);
	g_resourceView.m_bytes.resize((size_t) size);
	bool read = fread(g_resourceView.m_bytes.data(), 1, (size_t) size, file) == (size_t) size;
	fclose(file);
	if (!read) return Fail("short read");
	const auto& bytes = g_resourceView.m_bytes;
	if (memcmp(bytes.data(), "RES ", 4) != 0) {
		return Fail(memcmp(bytes.data(), "RIFF", 4) == 0
			? "RIFF DATA has no established legacy table adapter" : "expected RES container");
	}
	if (memcmp(bytes.data() + 8, "DATA", 4) != 0) return Fail("expected DATA container", "DATA", 0, 8);
	size_t declaredEnd = size_t(Little32(bytes, 4)) + 8;
	if (declaredEnd < 12 || declaredEnd > bytes.size()) return Fail("declared container exceeds file bounds", "DATA", 0, 4);



	size_t end = bytes.size();
	size_t padding = 0;
	size_t totalRecords = 0;
	for (size_t offset = 12; offset < end;) {
		if (g_resourceView.m_sections.size() >= 1024) return Fail("too many sections for the legacy table reader", "DATA", 0, offset);
		if (end - offset < 8) return Fail("truncated section header", "DATA", 0, offset);
		std::string type((const char*) bytes.data() + offset, 4);
		size_t length = Little32(bytes, offset + 4);
		size_t begin = offset + 8;
		if (length > end - begin) return Fail("section exceeds container bounds", type, 0, offset + 4);
		size_t sectionEnd = begin + length;
		if (g_resourceView.m_sections.count(type)) return Fail("duplicate resource section", type, 0, offset);
		SECTION& section = g_resourceView.m_sections[type];
		if (length < 4) return Fail("truncated section options/count", type, 0, begin);
		uint32_t options = Little32(bytes, begin);
		uint32_t count = options;
		begin += 4;
		if (options & 0x80000000u) {
			if (sectionEnd - begin < 8) return Fail("truncated extended section header", type, 0, begin);
			if ((options & 0x100u) || Little32(bytes, begin) != 0) {
				return Fail("compressed tables require an established decoder", type, 0, begin - 4);
			}
			if ((options & 0xffu) != 0) return Fail("unsupported section version", type, 0, begin - 4);
			count = Little32(bytes, begin + 4);
			begin += 8;
		}
		if (count > (sectionEnd - begin) / 4) return Fail("record count exceeds section bounds", type, 0, begin);
		if (count > 65536 - totalRecords) return Fail("too many records for the legacy table reader", type, 0, begin);
		totalRecords += count;
		for (uint32_t index = 0; index < count; ++index) {
			if (sectionEnd - begin < 4) return Fail("truncated record length", type, index, begin);
			size_t recordLength = Little32(bytes, begin);
			begin += 4;
			if (recordLength > sectionEnd - begin) return Fail("record exceeds section bounds", type, index, begin - 4);
			section.m_records.push_back({begin, recordLength});
			begin += recordLength;
		}
		if (begin != sectionEnd) return Fail("unaccounted bytes after the declared records", type, count, begin);
		offset = sectionEnd;
		if ((length & 1) && offset < end) { ++offset; ++padding; }
	}
	if (declaredEnd != end && declaredEnd + padding != end) return Fail("outer size disagrees with sections and their alignment", "DATA", 0, 4);
	for (const char* type : {"OBJ ", "WEAP", "CNST", "SFX "}) {
		auto it = g_resourceView.m_sections.find(type);
		if (it == g_resourceView.m_sections.end() || it->second.m_records.empty()) return Fail("required nonempty section is missing", type);
	}
	return true;
}

bool SkipString(const std::vector<unsigned char>& p_bytes, size_t& p_cursor, size_t p_end)
{
	while (p_cursor < p_end) if (p_bytes[p_cursor++] == 0) return true;
	return false;
}

bool MatchSchema(const GAME_DESCRIPTOR& p_game, bool p_explain)
{
	const auto& bytes = g_resourceView.m_bytes;
	for (const char* type : {"OBJ ", "WEAP", "CNST", "SFX "}) {
		const auto& records = g_resourceView.m_sections.at(type).m_records;
		for (size_t index = 0; index < records.size(); ++index) {
			const RECORD& record = records[index];
			size_t cursor = record.m_offset;
			size_t end = cursor + record.m_size;
			bool valid = true;
			if (!strcmp(type, "OBJ ")) {

				size_t fixed = p_game.m_objSchema == GAME_OBJ_ZS1 ? 700 : p_game.m_objSchema == GAME_OBJ_LOCOLAND ? 624 : 692;
				valid = record.m_size >= fixed + 2;
				if (valid) {
					cursor += 4;
					valid = SkipString(bytes, cursor, end) && fixed - 4 <= end - cursor;
					if (valid) {
						cursor += fixed - 4;
						valid = SkipString(bytes, cursor, end) && cursor == end;
					}
				}
			}
			else if (!strcmp(type, "WEAP")) valid = record.m_size == (size_t) p_game.m_weapRecordBytes;
			else if (!strcmp(type, "CNST")) valid = records.size() == 1 && record.m_size == 104;
			else {
				size_t prefix = p_game.m_sfxSchema == GAME_SFX_ZS1 ? 9 : p_game.m_sfxSchema == GAME_SFX_THESEUS ? 5 : 1;
				int strings = p_game.m_sfxSchema == GAME_SFX_AS1 ? 8 : 16;
				valid = record.m_size >= prefix;


				if (valid) valid = bytes[cursor + (prefix == 1 ? 0 : 4)] <= 100;
				if (valid && prefix == 9) {
					int32_t volume = (int32_t) Little32(bytes, cursor + 5);
					valid = volume >= -1000 && volume <= 1000;
				}
				if (valid) {
					cursor += prefix;
					for (int i = 0; i < strings && valid; ++i) valid = SkipString(bytes, cursor, end);
					valid = valid && cursor == end;
				}
			}
			if (!valid) return p_explain ? Fail(std::string("does not match profile '") + p_game.m_profileId + "'", type, index, record.m_offset) : false;
		}
	}
	return true;
}

std::string Normalize(const char* p_name)
{
	std::string result;
	for (const unsigned char* p = (const unsigned char*) p_name; p && *p; ++p) {
		if ((*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9')) result += (char) *p;
		else if (*p >= 'A' && *p <= 'Z') result += (char) (*p + ('a' - 'A'));
	}
	return result;
}

const GAME_DESCRIPTOR* ByName(const char* p_name)
{
	std::string name = Normalize(p_name);
	for (const auto& game : g_games) {
		if (name == Normalize(game.m_profileId) || name == Normalize(game.m_className) || name == Normalize(game.m_title)) return &game;
	}
	if (name == "steamland") return &g_games[6];
	return nullptr;
}

bool ConfigExists(const std::string& p_name)
{
	std::string path = Platform_IsAbsolutePath(p_name.c_str()) ? p_name : std::string(Platform_BasePath()) + p_name;
	FILE* file = Platform_FOpen(path.c_str(), "rb");
	if (!file) return false;
	fclose(file);
	return true;
}

std::string Json(const std::string& p_value)
{
	std::string result = "\"";
	for (unsigned char c : p_value) {
		if (c == '"' || c == '\\') { result += '\\'; result += (char) c; }
		else if (c < 32) {
			char escaped[7];
			snprintf(escaped, sizeof(escaped), "\\u%04x", c);
			result += escaped;
		}
		else result += (char) c;
	}
	return result + '"';
}

}

const GAME_DESCRIPTOR* GameDesc = &g_games[0];

void Game_SetCliOverride(GAME_ID p_id) { g_cliOverride = p_id; }
bool Game_SetCliOverride(const char* p_name)
{
	const GAME_DESCRIPTOR* game = ByName(p_name);
	if (!game) return false;
	g_cliOverride = game->m_id;
	return true;
}
void Game_SetConfigOverride(const char* p_name) { g_configOverride = p_name ? p_name : ""; }
void Game_SetProbeJson(bool p_enabled) { g_probeJson = p_enabled; }
bool Game_WantsProbeJson() { return g_probeJson; }
const char* Game_ConfigName() { return g_config.c_str(); }
const char* Game_ResourceName() { return g_resource.c_str(); }
const char* Game_StartMap() { return g_startMap.c_str(); }
const char* Game_Edition() { return g_edition.c_str(); }

bool Game_Detect()
{
	g_error.clear();
	g_candidates.clear();
	g_detected = false;
	g_resourceView = {};
	g_config.clear();
	g_resource = "objects.res";
	g_startMap = "maps\\logo.map";
	g_edition = "unspecified";
	const GAME_DESCRIPTOR* explicitGame = nullptr;
	for (const auto& game : g_games) if (game.m_id == g_cliOverride) explicitGame = &game;
	const GAME_DESCRIPTOR* configGame = nullptr;
	if (!g_configOverride.empty()) {
		g_config = g_configOverride;
		if (!ConfigExists(g_config)) g_error = "Explicit configuration '" + g_config + "' was not found.";
	}
	else if (ConfigExists("game.cfg")) g_config = "game.cfg";
	else {
		size_t configCount = 0;
		auto findConfigs = [&](const GAME_DESCRIPTOR& game) {
			bool candidate = false;
			for (const char* name : {game.m_configName, game.m_configAlias}) {
				if (name && ConfigExists(name)) {
					++configCount;
					g_config = name;
					candidate = true;
				}
			}
			if (candidate) g_candidates.push_back(&game);
		};
		if (explicitGame) findConfigs(*explicitGame);
		if (!configCount) for (const auto& game : g_games) findConfigs(game);


		if (configCount > 1) {
			g_error = "Multiple title configurations found; select one with --config.";
			g_config.clear();
		}
	}
	if (g_error.empty()) {
		g_candidates.clear();
		INI_FILE config;
		if (!g_config.empty()) {
			std::string path = Platform_IsAbsolutePath(g_config.c_str()) ? g_config : std::string(Platform_BasePath()) + g_config;
			config.Load(path.c_str());
			const char* value = config.Get("game", "Resource");
			if (value && *value) g_resource = value;
			value = config.Get("game", "StartMap");
			if (value && *value) g_startMap = value;
			configGame = ByName(config.Get("common", "Title"));
			if (!configGame) {
				std::string leaf = g_config.substr(g_config.find_last_of("/\\") + 1);
				for (const auto& game : g_games) {
					if (!SDL_strcasecmp(leaf.c_str(), game.m_configName) ||
						(game.m_configAlias && !SDL_strcasecmp(leaf.c_str(), game.m_configAlias))) configGame = &game;
				}
			}
			if (config.Get("common", "Steam")) g_edition = "steam";
		}
		if (ReadResource()) {
			for (const auto& game : g_games) if (MatchSchema(game, false)) g_candidates.push_back(&game);
			const GAME_DESCRIPTOR* pick = explicitGame ? explicitGame : configGame;
			if (pick) {
				if (!MatchSchema(*pick, true)) pick = nullptr;
			}
			else if (g_candidates.size() == 1) pick = g_candidates[0];
			else if (g_candidates.empty()) g_error = "No implemented AS1-family resource schema matches these tables. AS2-family games are outside this runtime.";
			else g_error = "Ambiguous game data; select a title with --game or supply its configuration with --config.";
			if (pick) {
				int engine = config.GetInt("game", "EngineVersion", config.GetInt("common", "EngineVersion", config.GetInt("", "EngineVersion", 0)));
				if (engine && engine != pick->m_engineVersion) {
					g_error = "Configuration EngineVersion=" + std::to_string(engine) + " contradicts profile '" + pick->m_profileId + "'.";
				}
				else {
					GameDesc = pick;
					g_detected = true;
					Platform_SetPrefApp(pick->m_className);
				}
			}
		}
	}
	if (!g_detected && !g_probeJson) {
		std::string message = g_error;
		if (!g_candidates.empty()) {
			message += "\nCandidates:";
			for (const auto* game : g_candidates) message += std::string(" ") + game->m_profileId;
		}
		fprintf(stderr, "%s\n", message.c_str());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Game Data Detection", message.c_str(), nullptr);
	}
	return g_detected;
}

bool Game_RuntimeAvailable()
{
	return GameDesc && GameDesc->m_runtimeEnabled &&
		(!GameDesc->m_nativeMoviePlayback || MoviePlayer::Available());
}

void Game_PrintProbeJson()
{
	printf("{\"detected\":%s,\"profile\":%s,\"edition\":%s,\"engineVersion\":%d,\"status\":%s,\"runtimeEnabled\":%s,\"dataDirectory\":%s,\"config\":%s,\"resource\":%s,\"startMap\":%s,\"candidates\":[",
		g_detected ? "true" : "false", g_detected ? Json(GameDesc->m_profileId).c_str() : "null", Json(g_edition).c_str(),
		g_detected ? GameDesc->m_engineVersion : 0, g_detected ? Json(GameDesc->m_status).c_str() : "null",
		g_detected && Game_RuntimeAvailable() ? "true" : "false", Json(Platform_BasePath()).c_str(), Json(g_config).c_str(),
		Json(g_resource).c_str(), Json(g_startMap).c_str());
	for (size_t i = 0; i < g_candidates.size(); ++i) printf("%s%s", i ? "," : "", Json(g_candidates[i]->m_profileId).c_str());
	printf("],\"sections\":{");
	bool first = true;
	for (const auto& item : g_resourceView.m_sections) {
		printf("%s%s:{\"records\":%zu}", first ? "" : ",", Json(item.first).c_str(), item.second.m_records.size());
		first = false;
	}
	printf("},\"moviePlaybackAvailable\":%s,\"error\":%s}\n", MoviePlayer::Available() ? "true" : "false",
		g_error.empty() ? "null" : Json(g_error).c_str());
}
