#include "audio/sound.h"
#include "game/builded_terrain.h"
#include "game/const.h"
#include "game/constant.h"
#include "game/engine.h"
#include "game/gametime.h"
#include "game/man.h"
#include "game/map.h"
#include "game/player_arcade.h"
#include "game/region.h"
#include "game/settings.h"
#include "game/unit.h"
#include "game/viewport_math.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "gfx/texture.h"
#include "logic/relation.h"
#include "platform/cursor.h"
#include "platform/paths.h"
#include "platform/portable_config.h"
#include "platform/render.h"
#include "platform/timing.h"
#include "sprite/cannon.h"
#include "sprite/frame.h"
#include "sprite/linker.h"
#include "sprite/plane.h"
#include "sprite/primitive.h"
#include "sprite/r_map.h"
#include "sprite/sprite.h"
#include "sprite/stext.h"
#include "ui/mouse.h"
#include "util/game_random.h"
#include "util/myerror.h"
#include "util/profile.h"
#include "util/registry.h"
#include "util/stream.h"
#include "video/vid.h"
#include "world/hash_map.h"

#include <SDL3/SDL.h>
#include <bit>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

// GLOBAL: ALIEN 0x48208c
static const char* EventFunctionName[] = {

	"main",
	// STRING: ALIEN 0x4822d0
	"TrainNotAmmo",
	// STRING: ALIEN 0x4822c0
	"TrainNotPower",
	// STRING: ALIEN 0x4822b4
	"TrainDamage",
	// STRING: ALIEN 0x4822a4
	"TrainCreated",
	// STRING: ALIEN 0x482298
	"TrainSplit",
	// STRING: ALIEN 0x482288
	"TrainDestroy",
	// STRING: ALIEN 0x482274
	"TrainDestroyPower",
	// STRING: ALIEN 0x482268
	"TrainArrive",
	// STRING: ALIEN 0x482254
	"???TrainNotArrive",
	// STRING: ALIEN 0x482244
	"TrainAttacked",
	// STRING: ALIEN 0x482238
	"DepoDestroy",
	// STRING: ALIEN 0x48222c
	"DepoBirth",
	// STRING: ALIEN 0x48221c
	"DepoAttacked",
	// STRING: ALIEN 0x482210
	"DepoFree",
	// STRING: ALIEN 0x482200
	"BuildingCapture",
	// STRING: ALIEN 0x4821f0
	"MasterDestroy",
	// STRING: ALIEN 0x4821ec
	"???",
	// STRING: ALIEN 0x4821d4
	"???SuperWeaponWounded",
	// STRING: ALIEN 0x4821c8
	"MineBlast",
	// STRING: ALIEN 0x4821bc
	"MineRemove",
	// STRING: ALIEN 0x4821b0
	"EnemyLinked",
	// STRING: ALIEN 0x4821a4
	"TrainClash",
	// STRING: ALIEN 0x482198
	"UnitCreated",
	// STRING: ALIEN 0x48218c
	"UnitDestroy",
	empty_str
};

static bool IsConfigArgument(const STRING& p_argument)
{
	const size_t length = strlen(p_argument.m_str);
	return length >= 4 && SDL_strcasecmp(p_argument.m_str + length - 4, ".cfg") == 0;
}

static bool IsGameplayMapName(const STRING& p_name, float p_width, float p_height)
{
	const char* leaf = p_name.m_str;
	for (const char* p = p_name.m_str; *p; ++p) {
		if (*p == '/' || *p == '\\') {
			leaf = p + 1;
		}
	}
	if (SDL_strncasecmp(leaf, "Level_", 6) != 0) {
		return SDL_strncasecmp(leaf, "survive_", 8) == 0;
	}
	const bool level06 = SDL_strcasecmp(leaf, "Level_06") == 0 || SDL_strcasecmp(leaf, "Level_06.map") == 0;
	if ((p_width > 640.0f || p_height > 480.0f) || !level06) {
		return true;
	}

	const char* parent = p_name.m_str;
	const char* separator = leaf > p_name.m_str ? leaf - 1 : leaf;
	for (const char* p = p_name.m_str; p < separator; ++p) {
		if (*p == '/' || *p == '\\') {
			parent = p + 1;
		}
	}
	const size_t parentLength = (size_t) (separator - parent);
	return !(
		(parentLength == 5 && SDL_strncasecmp(parent, "ADDON", 5) == 0) ||
		(parentLength == 6 && SDL_strncasecmp(parent, "ADDON2", 6) == 0)
	);
}

static void RefreshPointerAfterFrameResize(MAP* p_map)
{
	float x = 0.0f;
	float y = 0.0f;
	SDL_GetMouseState(&x, &y);
	Platform_RenderWindowToFrame(&x, &y);
	SDL_Event event = {};
	event.type = SDL_EVENT_MOUSE_MOTION;
	event.motion.x = x;
	event.motion.y = y;
	p_map->m_input.ProcessEvent(event);
}

// STUB: ALIEN 0x408ff0
MAP::MAP(STRING& p_argv, SETTINGS* p_settings)
{
	RealCurrentTime = Platform_Ticks();
	PrevRealCurrentTime = RealCurrentTime - 10;
	for (int i = 0; i < 256; ++i) {
		ANGLE::SinTable2[i] = (ANGLE::SinTable[i] * 4096.0f) * 0.00017262212f;
		ANGLE::CosTable2[i] = (ANGLE::CosTable[i] * 4096.0f) * 0.00017262212f;
	}

	m_flag = (p_settings->m_flag & 1) | 0x111080;
	m_groundz = 0;
	m_tempGroundz = 0;
	m_w = 640.0f;
	m_h = 480.0f;
	m_curArmy = 0;
	m_shiftX = 0.0f;
	m_weapon = 0;
	m_noWeapon = 0;
	m_noVid = 0;
	m_noTact = 0;
	m_shiftY = 1.0f;
	m_speed = 1.0f;
	m_unk0x30 = RealCurrentTime;
	m_fps = 0;
	m_fpsCnt = 0;
	m_shiftFlag = 1;
	memset(m_vids, 0, sizeof(m_vids));
	m_player[0] = 0;
	m_player[1] = 0;
	m_player[2] = 0;
	m_player[3] = 0;
	m_window = 0;
	m_quit = 0;
	m_terrainCamera = 0;
	ResetGroundZ();

	::Error = new MYERROR(1);

	// Keep the profile identity stable if the executable is renamed.
	STRING className("AlienShooter");

	STRING base(Platform_BasePath());
	PROFILE profile;
	profile.Load(
		base + className +
		// STRING: ALIEN 0x4824e0
		".cfg"
	);
	if (IsConfigArgument(p_argv)) {
		STRING configPath = Platform_IsAbsolutePath(p_argv.m_str) ? p_argv : base + p_argv;
		profile.Load(configPath);
	}

	Strings = new PROFILE;
	if (Strings) {
		Strings->Load(
			base +
			// STRING: ALIEN 0x4824c8
			"Strings.ini"
		);
	}

	m_title = profile.GetString(
		// STRING: ALIEN 0x4824b8
		STRING("common"),
		// STRING: ALIEN 0x4824c0
		STRING("Title"),
		STRING("Alien Shooter")
	);

	Registry = new REGISTRY;
	if (Registry) {
		Registry->m_path = profile.GetString(
			STRING("common"),
			// STRING: ALIEN 0x4824b0
			STRING("RegPath"),
			// STRING: ALIEN 0x48249c
			STRING("SOFTWARE\\Gromada\\") + className
		);
	}

	if (profile.GetInt(
			// STRING: ALIEN 0x48248c
			STRING("graph"),
			// STRING: ALIEN 0x482494
			STRING("VSync"),
			1
		)) {
		p_settings->m_flag |= 2;
	}
	if (profile.GetInt(
			// STRING: ALIEN 0x482470
			STRING("game"),
			// STRING: ALIEN 0x482478
			STRING("StartDialogIsFull"),
			0
		)) {
		p_settings->m_flag |= 4;
	}
	const int configVersion = PortableConfig_GetInt("meta", "ConfigVersion", 0);
	const bool importLegacyConfig = configVersion < PORTABLE_CONFIG_VERSION;
	STRING legacyDisplayPath = Registry->m_path;
	if (importLegacyConfig && !SDL_strcasecmp(Registry->m_path.m_str, "SOFTWARE\\Gromada\\AlienShooter")) {
		static const char* const oldPaths[] = {
			"SOFTWARE\\Gromada\\AlienShooter-portable",
			"SOFTWARE\\Gromada\\",
		};
		int newestPolicy = Registry_GetIntExact(legacyDisplayPath, STRING("DisplayPolicyVersion"), -1);
		for (const char* oldPath : oldPaths) {
			int policy = Registry_GetIntExact(STRING(oldPath), STRING("DisplayPolicyVersion"), -1);
			if (policy > newestPolicy) {
				legacyDisplayPath = oldPath;
				newestPolicy = policy;
			}
		}
	}
	auto displaySetting = [importLegacyConfig, &legacyDisplayPath](const char* p_name, int p_default) {
		int fallback =
			importLegacyConfig ? Registry_GetIntExact(legacyDisplayPath, STRING(p_name), p_default) : p_default;
		return PortableConfig_GetInt("display", p_name, fallback);
	};
	p_settings->m_device = displaySetting(
		// STRING: ALIEN 0x482468
		"Device",
		p_settings->m_device
	);
	p_settings->m_screenX = displaySetting(
		// STRING: ALIEN 0x482460
		"ScreenX",
		p_settings->m_screenX
	);
	p_settings->m_screenY = displaySetting(
		// STRING: ALIEN 0x482458
		"ScreenY",
		p_settings->m_screenY
	);
	p_settings->m_screenBpp = displaySetting(
		// STRING: ALIEN 0x482454
		"BPP",
		p_settings->m_screenBpp
	);
	p_settings->m_fullscreen = displaySetting(
		// STRING: ALIEN 0x482448
		"FullScreen",
		p_settings->m_fullscreen
	);
	p_settings->m_renderWidth = displaySetting("RenderWidth", p_settings->m_renderWidth);
	p_settings->m_nativeResolution = displaySetting("NativeResolution", p_settings->m_nativeResolution);
	p_settings->m_uiScale = displaySetting("UIScale", p_settings->m_uiScale);
	p_settings->m_desktopResolution = displaySetting("AutomaticResolution", p_settings->m_desktopResolution);
	p_settings->m_displayPolicyVersion = displaySetting("DisplayPolicyVersion", p_settings->m_displayPolicyVersion);
	if (displaySetting("VSync", (p_settings->m_flag & 2) != 0)) {
		p_settings->m_flag |= 2;
	}
	else {
		p_settings->m_flag &= ~2u;
	}
	int legacyWindowX =
		importLegacyConfig ? Registry_GetIntExact(legacyDisplayPath, STRING("WindowPositionX"), SDL_WINDOWPOS_UNDEFINED)
						   : SDL_WINDOWPOS_UNDEFINED;
	int legacyWindowY =
		importLegacyConfig ? Registry_GetIntExact(legacyDisplayPath, STRING("WindowPositionY"), SDL_WINDOWPOS_UNDEFINED)
						   : SDL_WINDOWPOS_UNDEFINED;
	int windowX = PortableConfig_GetInt("window", "PositionX", legacyWindowX);
	int windowY = PortableConfig_GetInt("window", "PositionY", legacyWindowY);
	if (importLegacyConfig) {
		// GRAPH::Init writes the schema marker even in fullscreen, so carry the
		// old window position into the same first flush before it becomes inert.
		PortableConfig_SetInt("window", "PositionX", windowX);
		PortableConfig_SetInt("window", "PositionY", windowY);
	}
	Settings_MigrateLegacyDisplay(p_settings);
	Settings_ApplyCommandLine(p_settings);
	strcpy(p_settings->m_appName, m_title);

	GRAPH* graph = new GRAPH(p_settings);
	Graph = graph;

	Map = this;
	if (Graph->Init()) {
		return;
	}
	m_window = graph->m_window;

	if (SDL_GetWindowFlags((SDL_Window*) m_window) & SDL_WINDOW_INPUT_FOCUS) {
		m_flag |= 8;
	}

	Platform_SetCursor(0);

	if (!(graph->m_flags & 0x80)) {
		Platform_RenderRestoreWindowPosition(windowX, windowY);
	}
	SDL_SetWindowTitle((SDL_Window*) m_window, m_title);

	int fontSizeY = profile.GetInt(
		STRING("graph"),
		// STRING: ALIEN 0x4823e4
		STRING("FontSizeY"),
		8
	);
	int fontSizeX = profile.GetInt(
		STRING("graph"),
		// STRING: ALIEN 0x4823d8
		STRING("FontSizeX"),
		0
	);
	graph->CreateDebugFont(
		profile.GetString(
			STRING("graph"),
			// STRING: ALIEN 0x4823c8
			STRING("Font"),
			// STRING: ALIEN 0x4823d0
			STRING("Arial")
		),
		fontSizeX,
		fontSizeY
	);

	RESOURCE resource;
	m_resName = profile.GetString(
		STRING("game"),
		// STRING: ALIEN 0x4823b0
		STRING("Resource"),
		// STRING: ALIEN 0x4823bc
		STRING("objects.res")
	);
	if (resource.OpenForRead(m_resName, 0x41544144 /* 'DATA' */)) {
		if (::Error) {
			MYERROR::Error(::Error, "MAP", 7, "resource file", 0);
		}
		return;
	}

	Sound = new SOUND(
		&resource,
		Registry->GetInt(
			// STRING: ALIEN 0x47f714
			STRING("SoundHighQuality"),
			0
		)
	);

	Const = new CONSTANT(&resource);
	Const->m_debugMode = profile.GetInt(
		STRING("game"),
		// STRING: ALIEN 0x482394
		STRING("DebugMode"),
		0
	);
	m_flag = (m_flag & 0xfffdffff) | ((profile.GetInt(
										   STRING("game"),
										   // STRING: ALIEN 0x48238c
										   STRING("DrawFPS"),
										   0
									   ) &
									   1)
									  << 17);
	m_flag = (m_flag & 0xfffbffff) | ((profile.GetInt(
										   STRING("game"),
										   // STRING: ALIEN 0x482378
										   STRING("DrawPresentation"),
										   1
									   ) &
									   1)
									  << 18);

	LoadVid(&resource);

	EmptyVid->m_exData = (VID_EXDATA*) m_weapon;
	Hash = new HASH_MAP(m_w, m_h, m_vids, m_noVid);
	resource.Close();

	Mouse = new MOUSE(EmptyVid, graph->GetWidth() * 0.5f, graph->GetHeight() * 0.5f, 0.0f, ANGLE((unsigned char) 0), 0);
	Mouse->Enable();

	auto controlKey = [](const char* p_name, const char* p_default) {
		const char* value = PortableConfig_GetString("control", p_name);
		return INPUT_AS::GetKeyByName(STRING(value && *value ? value : p_default));
	};
	g_keyScrollLeft = controlKey("Left", "A");
	g_keyScrollRight = controlKey("Right", "D");
	g_keyScrollUp = controlKey("Up", "W");
	g_keyScrollDown = controlKey("Down", "S");
	g_relativeControl = PortableConfig_GetInt("control", "Relative", 0);
	INPUT_AS::firstKey1 = controlKey("First", "LBUTTON");
	INPUT_AS::secondKey1 = controlKey("Second", "RBUTTON");
	INPUT_AS::prevKey1 = controlKey("Prev", "Q");
	INPUT_AS::nextKey1 = controlKey("Next", "E");

	m_player[0] = new PLAYER_ARCADE(1, 0);
	m_player[2] = new PLAYER_ARCADE(0, 2);
	m_player[1] = new PLAYER_ARCADE(2, 1);
	m_player[3] = new PLAYER_ARCADE(0, 3);

	m_shiftX2 = m_w;
	m_shiftX1 = 0.0f;
	m_shiftY1 = 0.0f;
	m_shiftY2 = m_h;

	if (!strcmp(p_argv, empty_str) || IsConfigArgument(p_argv)) {
		m_scriptName = profile.GetString(
			STRING("game"),
			// STRING: ALIEN 0x4822e8
			STRING("StartMap"),
			// STRING: ALIEN 0x4822f4
			STRING("maps\\logo.map")
		);
	}
	else {
		m_scriptName = p_argv;
	}
	m_flag |= 4;
}

// FUNCTION: ALIEN 0x40b4b0
void MAP::DeletePointerToSprite(SPRITE* p_sprite)
{
	for (int i = 0; i < 4; ++i) {
		m_player[i]->DeletePointerToSprite(p_sprite);
	}
	m_logic.m_stack.DeletePointerToObject(p_sprite);
	m_groups.DeletePointerToSprite(p_sprite);

	if (p_sprite->m_noRef > 1) {
		if (Hash->m_list.m_n) {
			SPRITE* sprite = (SPRITE*) Hash->m_list.m_data[Hash->m_list.m_n - 1];
			for (int idx = Hash->m_list.m_n - 1; sprite; sprite = (SPRITE*) Hash->m_list.m_data[idx]) {
				sprite->DeletePointerToSprite(p_sprite);
				if (idx > Hash->m_list.m_n) {
					idx = Hash->m_list.m_n;
				}
				if (--idx < 0) {
					break;
				}
			}
		}
	}

	for (int layer = 0; layer < 17; ++layer) {
		if (p_sprite->m_noRef > 1) {
			int idx;
			SPRITE* sprite = FirstSprite(layer, &idx);
			while (sprite) {
				sprite->DeletePointerToSprite(p_sprite);
				sprite = 0;
				for (--idx; idx >= 0; --idx) {
					if (m_layers[layer].m_data[idx]) {
						sprite = m_layers[layer].m_data[idx];
						break;
					}
				}
			}
		}
	}
}

inline static bool InViewPort(const GRAPH_CORE* p_graph, float p_x, float p_y)
{
	return p_x >= p_graph->m_viewXMin && p_x < p_graph->m_viewXMax && p_y >= p_graph->m_viewYMin &&
		   p_y < p_graph->m_viewYMax;
}

inline static float ViewXMin(const GRAPH_CORE* p_graph)
{
	return p_graph->m_viewXMin;
}
inline static float ViewXMax(const GRAPH_CORE* p_graph)
{
	return p_graph->m_viewXMax;
}
inline static float ViewYMin(const GRAPH_CORE* p_graph)
{
	return p_graph->m_viewYMin;
}

// FUNCTION: ALIEN 0x40b590
void MAP::DrawSecondaryInfo()
{
	if (m_flag & 0x20000) {
		GRAPH_CORE::PrintfXY(Graph, ViewXMin(Graph), ViewYMin(Graph) + 1.0f, "%i", m_fps);
	}
	if (!Const->m_debugMode) {
		return;
	}
	if (m_flag & 0x10000) {
		GRAPH_CORE::PrintfXY(
			Graph,
			ViewXMax(Graph) - 20.0f,
			ViewYMin(Graph) + 1.0f,
			// STRING: ALIEN 0x482504
			"%2i",
			Sound->GetNoPlayed()
		);
	}
	if ((m_flag & 0x800) && m_groundz) {
		for (int gy = 1; gy < m_groundH; ++gy) {
			for (int gx = 1; gx < m_groundW; ++gx) {
				int z1 = m_groundz[gx + gy * m_groundW] > m_tempGroundz[gx + gy * m_groundW]
							 ? m_groundz[gx + gy * m_groundW]
							 : m_tempGroundz[gx + gy * m_groundW];
				int z0 = m_groundz[gx + gy * m_groundW - 1] > m_tempGroundz[gx + gy * m_groundW - 1]
							 ? m_groundz[gx + gy * m_groundW - 1]
							 : m_tempGroundz[gx + gy * m_groundW - 1];
				if (InViewPort(Graph, (float) (8 * gx + 4) - m_shiftX, (float) (8 * gy + 4 - z1) - m_shiftY) ||
					InViewPort(Graph, (float) (8 * gx - 4) - m_shiftX, (float) (8 * gy + 4 - z0) - m_shiftY)) {
					Graph->Line(
						(float) (8 * gx + 4) - m_shiftX,
						(float) (8 * gy + 4 - z1) - m_shiftY,
						(float) (8 * gx - 4) - m_shiftX,
						(float) (8 * gy + 4 - z0) - m_shiftY,
						GRAPH_CORE::GRAY
					);
				}
			}
		}
	}
	if (m_flag & 0x8000) {
		for (int layer = 0; layer < 16; ++layer) {
			int i;
			SPRITE* s = FirstSprite(layer, &i);
			while (s) {
				s->DrawRectangle();
				s = 0;
				for (--i; i >= 0; --i) {
					if (m_layers[layer].m_data[i]) {
						s = m_layers[layer].m_data[i];
						break;
					}
				}
			}
		}
	}
	if (m_flag & 0x1000) {
		if (Flagman(m_curArmy)) {
			((SPRITE*) Flagman(m_curArmy))->DrawSecondaryInfo();
		}
		else if (SpriteUnderCursor()) {
			SpriteUnderCursor()->DrawSecondaryInfo();
		}
		else if (m_menu.m_underCursor) {
			m_menu.m_underCursor->DrawSecondaryInfo();
		}
	}
	if (m_flag & 0x4000) {
		m_groups.DrawNumber();
	}
	if (m_flag & 0x2000) {
		RailMap.DebugDraw();
	}
}

// FUNCTION: ALIEN 0x40c570
void MAP::Release()
{
	ClearTerrainCamera();
	for (int i = 0; i < m_noVid; ++i) {
		if (VidExists(i) && m_vids[i]->GetEntitiesNumberTotal() != 0) {
			MYERROR::Log(
				::Error,
				// STRING: ALIEN 0x4825f0
				"NoVid[%3i]=%i %i %i %i %s Layer=%i %s",
				i,
				m_vids[i]->m_entitiesNumber[0],
				m_vids[i]->m_entitiesNumber[1],
				m_vids[i]->m_entitiesNumber[2],
				m_vids[i]->m_entitiesNumber[3],
				m_vids[i]->m_name,
				m_vids[i]->m_layer,
				m_vids[i]->m_fname
			);
		}
	}
	m_relation.Release();
	m_speed = 1.0f;
	m_fps = 0;
	m_fpsCnt = 0;
	((GRAPH*) Graph)->SetWind(25, ANGLE(200));
	((GRAPH*) Graph)->SetEnvironment(-1);
	if (m_flag & 0x200) {
		Mouse->Enable();
		m_resource.Close();
	}
	if (m_flag & 0x100) {
		int end = -1;
		m_resource.Write(&end, 4);
		m_resource.PostAppend();
		m_resource.Close();
	}
	m_flag &= 0xfffffcff;
	m_shiftFlag = 1;
	MYERROR::Log(
		::Error,
		// STRING: ALIEN 0x4825e0
		"Player release"
	);
	for (int p = 0; p < 4; ++p) {
		if (m_player[p]) {
			m_player[p]->Release();
		}
	}
	MYERROR::Log(
		::Error,
		// STRING: ALIEN 0x4825d0
		"Sprite release"
	);
	ENGINE::globaldeleting = 1;
	for (int verifyLayer = 0; verifyLayer < 17; ++verifyLayer) {
		int n;
		SPRITE* sprite = FirstSprite(verifyLayer, &n);
		while (sprite) {
			sprite->ScalarDeletingDestructor(1);
			sprite = 0;
			for (--n; n >= 0; --n) {
				if (m_layers[verifyLayer].m_data[n]) {
					sprite = m_layers[verifyLayer].m_data[n];
					break;
				}
			}
		}
	}
	ENGINE::globaldeleting = 0;
	for (int layer = 0; layer < 17; ++layer) {
		int idx;
		if (FirstSprite(layer, &idx)) {
			SPRITE* sprite = FirstSprite(layer, &idx);
			MYERROR::Error(
				::Error,
				"SPRITE %i",
				10,
				// STRING: ALIEN 0x4825b4
				"Sprite exist after delete",
				idx,
				sprite->m_vid ? sprite->m_vid->m_idx : -1
			);
		}
	}
	if (m_groups.First() && ::Error) {
		MYERROR::Error(
			::Error,
			"MAP",
			10,
			// STRING: ALIEN 0x482580
			"Incorrect delete groups in DeleteAll()",
			0
		);
	}
	MYERROR::Log(
		::Error,
		// STRING: ALIEN 0x482570
		"Menu   release"
	);
	if (m_menu.m_n) {
		SPRITE* first = (SPRITE*) m_menu.m_data[0];
		MYERROR::Error(
			::Error,
			"SPRITE %i",
			10,
			// STRING: ALIEN 0x482550
			"Menu sprite exist after delete",
			0,
			first->m_vid ? first->m_vid->m_idx : -1
		);
		m_menu.DeleteAll();
	}
	unsigned int fps = CurrentTime - m_unk0x30 ? 1000 * m_noTact / (CurrentTime - m_unk0x30) : 0;
	MYERROR::Log(
		::Error,
		// STRING: ALIEN 0x482540
		"Average fps=%i",
		fps
	);
	MYERROR::Log(
		::Error,
		// STRING: ALIEN 0x482530
		"Script release"
	);
	m_logic.Release();
	m_noTact = 0;
	DeleteExtraVid();
	for (int v = 0; v < m_noVid; ++v) {
		if (m_vids[v]) {
			m_vids[v]->ResetSprites();
		}
	}
	for (unsigned int e = 0; e < 64; ++e) {
		EvFunctionNumber[e] = e + 1000000;
	}
}

// STUB: ALIEN 0x40c960
int MAP::Load(STRING p_name)
{
	RESOURCE res;
	m_flag |= 0x20;
	if (!strcmp(p_name, empty_str)) {
		return 0;
	}

	if (m_w != 0.0f || m_h != 0.0f) {
		if (Const->m_debugMode) {
			((GRAPH*) Graph)
				->DrawDebugText(
					// STRING: ALIEN 0x482848
					"Release previous map"
				);
		}
		Release();
	}

	if (!m_resource.m_file && !m_resource.OpenForRead(p_name, 0x4f4d4544 /* 'DEMO' */)) {
		p_name.Read_res(&m_resource);
		m_flag |= 0x200;
	}

	if (res.OpenForRead(p_name, 0x2050414d /* 'MAP ' */)) {
		MYERROR::Window(
			::Error,
			// STRING: ALIEN 0x482820 (approx; the invalid-map message)
			"!!!ERROR!!!LOAD: Invalid map file %s",
			p_name.m_str
		);
		return 0;
	}

	Mouse->Disable();
	m_prevMap = m_mapName;
	m_mapName = p_name;
	unsigned int seed;
	if (m_flag & 0x200) {
		m_resource.Read(&seed, 4);
	}
	else {
		seed = Platform_Ticks();
	}
	GameSrand(seed);

	if (Const->m_debugMode) {
		((GRAPH*) Graph)
			->DrawDebugText(
				// STRING: ALIEN 0x482810
				"Load extra vid"
			);
	}
	LoadVid(&res);

	int buf = 0;
	bool loaded = false;
	if (res.GoBegin(0x48505247 /* 'GRPH' */)) {

		if (res.GoBegin(0x44414548 /* 'HEAD' */)) {
			if (::Error) {
				MYERROR::Error(::Error, "MAP", 11, "HEAD", 0);
			}
			return 0;
		}
		int dim;
		res.Read(&dim, 4);
		m_w = (float) dim;
		res.Read(&dim, 4);
		m_h = (float) dim;
		short s;
		res.Read(&s, 2);
		m_shiftX = s;
		res.Read(&s, 2);
		m_shiftY = s;
		res.Read(&CurrentTime, 4);
		m_unk0x30 = CurrentTime;
		PrevCurrentTime = CurrentTime;
		res.Read(&buf, 4);
		((GRAPH*) Graph)->OldLoadParameters(&res);
		if (Hash) {
			Hash->~HASH_MAP();
			operator delete(Hash);
		}
		Hash = new HASH_MAP(m_w, m_h, m_vids, m_noVid);
		m_shiftX2 = m_w;
		m_shiftX1 = 0.0f;
		m_shiftY2 = m_h;
		m_shiftY1 = 0.0f;
		int frameChanged = ((GRAPH_CORE*) Graph)->ConfigureFrameForMap(m_w, m_h, IsGameplayMapName(p_name, m_w, m_h));
		for (int player = 0; player < 4; ++player) {
			if (m_player[player]) {
				((PLAYER_ARCADE*) m_player[player])->RefreshUILayout();
			}
		}
		if (frameChanged > 0) {
			SetShiftCoor(Graph->m_width * 0.5f + m_shiftX, Graph->m_height * 0.5f + m_shiftY, 0);
			RefreshPointerAfterFrameResize(this);
		}
		ResetGroundZ();
		if (res.GoNext(0x44495247 /* 'GRID' */)) {
			res.GoBegin(0x20594e41 /* 'ANY ' */);
		}
		else {
			if (m_groundz) {
				operator delete(m_groundz);
			}
			m_groundz = 0;
			int loaded = res.SubLoad((void**) &m_groundz, 0);
			int expected = 2 * ((int) (m_w + 7.0f) / 8) * ((int) (m_h + 7.0f) / 8);
			if (loaded != expected) {
				if (::Error) {
					MYERROR::Error(
						::Error,
						"MAP",
						4,
						// STRING: ALIEN 0x482800
						"grid",
						loaded
					);
				}
				ResetGroundZ();
			}
		}
		if (!res.GoNext(0x20525053 /* 'SPR ' */)) {
			while (OldLoadSprite(&res) != (SPRITE*) -1)
				;
			RailMap.CreateAdditionalDots();
			if (!res.GoNext(0x44525053 /* 'SPRD' */)) {
				for (SPRITE* s2 = ReadPointer(&res); s2 != (SPRITE*) -1; s2 = ReadPointer(&res)) {
					if (s2) {
						s2->Action(200, (decomp_intptr) &res, buf, 0);
					}
					res.GoNextSub(0x44525053 /* 'SPRD' */);
				}
				loaded = true;
			}
			else if (::Error) {
				MYERROR::Error(
					::Error,
					"MAP",
					11,
					// STRING: ALIEN 0x4826f4
					"SPRD",
					0
				);
			}
		}
		else if (::Error) {
			MYERROR::Error(
				::Error,
				"MAP",
				11,
				// STRING: ALIEN 0x48277c
				"SPR ",
				0
			);
		}
	}
	else {

		if (Const->m_debugMode) {
			((GRAPH*) Graph)
				->DrawDebugText(
					// STRING: ALIEN 0x4827e8
					"Load graph parameters"
				);
		}
		((GRAPH*) Graph)->LoadParameters(&res);
		if (res.GoNext(0x44414548 /* 'HEAD' */)) {
			if (::Error) {
				MYERROR::Error(
					::Error,
					"MAP",
					11,
					// STRING: ALIEN 0x482808
					"HEAD",
					0
				);
			}
			return 0;
		}
		res.Read(&m_w, 4);
		res.Read(&m_h, 4);
		res.Read(&m_shiftX, 4);
		res.Read(&m_shiftY, 4);
		res.Read(&CurrentTime, 4);
		PrevCurrentTime = 1;
		CurrentTime = 10;
		if (!(m_flag & 0x200) && m_resource.m_file) {
			m_flag |= 0x100;
			m_resource.PreAppend(0x4f4d4544 /* 'DEMO' */, 0);
			m_resource.Write(p_name.m_str, strlen(p_name.m_str) + 1);
			m_resource.Write(&seed, 4);
			m_resource.Write(&CurrentTime, 4);
		}
		if (m_flag & 0x200) {
			m_resource.Read(&CurrentTime, 4);
		}
		m_unk0x30 = CurrentTime;
		res.Read(&buf, 4);
		if (buf <= 9) {
			m_w = (float) std::bit_cast<int>(m_w);
			m_h = (float) std::bit_cast<int>(m_h);
			m_shiftX = (float) std::bit_cast<int>(m_shiftX);
			m_shiftY = (float) std::bit_cast<int>(m_shiftY);
		}
		MYERROR::Log(
			::Error,
			// STRING: ALIEN 0x4827a8
			"CurrentTime   =%-15u   sizeof(SPRITE)=%-8i Map version   =%i",
			CurrentTime,
			112,
			buf
		);

		if (Const->m_debugMode) {
			((GRAPH*) Graph)
				->DrawDebugText(
					// STRING: ALIEN 0x482790
					"Create new hash table"
				);
		}
		if (Hash) {
			Hash->~HASH_MAP();
			operator delete(Hash);
		}
		Hash = new HASH_MAP(m_w, m_h, m_vids, m_noVid);
		m_shiftX1 = 0.0f;
		m_shiftX2 = m_w;
		m_shiftY1 = 0.0f;
		m_shiftY2 = m_h;
		int frameChanged = ((GRAPH_CORE*) Graph)->ConfigureFrameForMap(m_w, m_h, IsGameplayMapName(p_name, m_w, m_h));
		for (int player = 0; player < 4; ++player) {
			if (m_player[player]) {
				((PLAYER_ARCADE*) m_player[player])->RefreshUILayout();
			}
		}
		SetShiftCoor(Graph->m_width * 0.5f + m_shiftX, Graph->m_height * 0.5f + m_shiftY, 0);
		if (frameChanged > 0) {
			RefreshPointerAfterFrameResize(this);
		}

		if (Const->m_debugMode) {
			((GRAPH*) Graph)
				->DrawDebugText(
					// STRING: ALIEN 0x482784
					"Load gridZ"
				);
		}
		ResetGroundZ();
		if (res.GoNext(0x44495247 /* 'GRID' */)) {
			res.GoBegin(0x20594e41 /* 'ANY ' */);
		}
		else {
			if (m_groundz) {
				operator delete(m_groundz);
			}
			m_groundz = 0;
			int loaded = res.SubLoad((void**) &m_groundz, 0);
			if (loaded != 2 * m_groundW * m_groundH) {
				if (::Error) {
					MYERROR::Error(::Error, "MAP", 4, "grid", loaded);
				}
				ResetGroundZ();
			}
		}

		if (res.GoNext(0x20525053 /* 'SPR ' */)) {
			if (::Error) {
				MYERROR::Error(::Error, "MAP", 11, "SPR ", 0);
			}
			return 0;
		}
		if (Const->m_debugMode) {
			((GRAPH*) Graph)
				->DrawDebugText(
					// STRING: ALIEN 0x482764
					"Load hardware terrain"
				);
		}
		int loadedLayer = 0;
		SPRITE* s2 = LoadSprite(&res, buf);
		while (s2 != (SPRITE*) -1) {
			if (Const->m_debugMode && loadedLayer != s2->m_vid->m_layer) {
				loadedLayer = s2->m_vid->m_layer;
				if (!loadedLayer) {
					((GRAPH*) Graph)->DrawDebugText("Load hardware terrain");
				}
				else if (loadedLayer == 1) {
					((GRAPH*) Graph)
						->DrawDebugText(
							// STRING: ALIEN 0x482748
							"Build sprites in terrain"
						);
				}
				else if (loadedLayer == 2) {
					((GRAPH*) Graph)
						->DrawDebugText(
							// STRING: ALIEN 0x482724
							"Build sprites with alpha in terrain"
						);
				}
				else {
					((GRAPH*) Graph)
						->DrawDebugText(
							// STRING: ALIEN 0x482714
							"Load sprites"
						);
				}
			}
			((GRAPH*) Graph)->DrawLoadBar(m_vids[0]);
			s2 = LoadSprite(&res, buf);
		}
		RailMap.CreateAdditionalDots();

		if (Const->m_debugMode) {
			((GRAPH*) Graph)
				->DrawDebugText(
					// STRING: ALIEN 0x4826fc
					"Load data for sprite"
				);
		}
		if (res.GoNext(0x44525053 /* 'SPRD' */)) {
			if (::Error) {
				MYERROR::Error(::Error, "MAP", 11, "SPRD", 0);
			}
			return 0;
		}
		for (SPRITE* sd = ReadPointer(&res); sd != (SPRITE*) -1; sd = ReadPointer(&res)) {
			((GRAPH*) Graph)->DrawLoadBar(m_vids[0]);
			if (sd) {
				sd->Action(81, (decomp_intptr) &res, buf, 0);
			}
			res.GoNextSub(0x44525053 /* 'SPRD' */);
		}

		if (Const->m_debugMode) {
			((GRAPH*) Graph)
				->DrawDebugText(
					// STRING: ALIEN 0x4826e0
					"Load players info"
				);
		}
		if (res.GoNext(0x59414c50 /* 'PLAY' */)) {
			if (::Error) {
				MYERROR::Error(
					::Error,
					"MAP",
					11,
					// STRING: ALIEN 0x4826d8
					"PLAY",
					0
				);
			}
			return 0;
		}
		for (int i = 0; i < 4; ++i) {
			m_player[i]->Load(&res);
		}

		if (Const->m_debugMode) {
			((GRAPH*) Graph)
				->DrawDebugText(
					// STRING: ALIEN 0x4826c4
					"Load groups info"
				);
		}
		if (res.GoNext(0x554f5247 /* 'GROU' */)) {
			if (::Error) {
				MYERROR::Error(
					::Error,
					"MAP",
					11,
					// STRING: ALIEN 0x4826bc
					"GROU",
					0
				);
			}
			return 0;
		}
		m_groups.Load(&res);
		loaded = true;
	}

	if (!loaded) {
		return 0;
	}

	FinalizeTerrainCamera(IsGameplayMapName(p_name, m_w, m_h) ? 1 : 0);

	m_flag &= ~0x20;
	res.Close();
	m_relation.Release();
	MYERROR::Log(::Error, "Vid    release %i %i", TextureMemoryInUse, VID::MemoryInUse);

	if (Const->m_debugMode) {
		((GRAPH*) Graph)
			->DrawDebugText(
				// STRING: ALIEN 0x4826a8
				"Load script file"
			);
	}

	STRING base;
	p_name.BeforeLast((char**) &base, ".");
	STRING lgcName = base +
					 // STRING: ALIEN 0x4826a0
					 ".lgc";
	bool haveLgc = false;
	if (*lgcName.m_str) {
		FILE* f = Platform_FOpen(lgcName.m_str, "rb");
		if (f) {
			fclose(f);
			haveLgc = true;
		}
	}
	if (haveLgc) {
		m_logic.Load(lgcName);
	}
	else {
		STRING lgdName = base +
						 // STRING: ALIEN 0x482698
						 ".lgd";
		bool haveLgd = false;
		if (*lgdName.m_str) {
			FILE* f = Platform_FOpen(lgdName.m_str, "rb");
			if (f) {
				fclose(f);
				haveLgd = true;
			}
		}
		m_logic.Load(haveLgd ? lgdName : lgcName);
	}

	if (Const->m_debugMode) {
		((GRAPH*) Graph)
			->DrawDebugText(
				// STRING: ALIEN 0x482674
				"Connect script function with VID"
			);
	}
	for (int function = 0; function < m_logic.m_variables.m_n; ++function) {
		NAMED_LIST_STRUCT_LOGICVAR& entry = m_logic.m_variables.m_data[function];
		if (entry.m_var.m_flag != 3) {
			continue;
		}
		STRING name(entry.m_name.m_str);
		for (int event = 0; *EventFunctionName[event]; ++event) {
			if (!strcmp(name.m_str, EventFunctionName[event])) {
				EvFunctionNumber[event] = function;
			}
		}
		char* text = name.m_str;
		if (text[0] != 'F' || !isdigit(text[1]) || !isdigit(text[2]) || !isdigit(text[3])) {
			continue;
		}

		if (text[4] == '_') {
			int vid = 100 * (text[1] - '0') + 10 * (text[2] - '0') + text[3] - '0';
			if (!strncmp(
					text + 5,
					// STRING: ALIEN 0x48266c
					"DAMAGE",
					7
				)) {
				if (entry.m_var.m_extra != 3) {
					STRING part(
						// STRING: ALIEN 0x48264c
						"no parameters in functions '",
						text
					);
					STRING message(part.m_str, "'");
					if (::Error) {
						MYERROR::Error(::Error, "MAP", 4, message.m_str, entry.m_var.m_extra);
					}
				}
				else if (vid >= 0 && vid < m_noVid && m_vids[vid]) {
					m_vids[vid]->m_unk0x408[18] = function;
				}
			}
			else if (!strncmp(
						 text + 5,
						 // STRING: ALIEN 0x482644
						 "DESTROY",
						 7
					 )) {
				if (entry.m_var.m_extra != 1) {
					STRING part("no parameters in functions '", text);
					STRING message(part.m_str, "'");
					if (::Error) {
						MYERROR::Error(::Error, "MAP", 4, message.m_str, entry.m_var.m_extra);
					}
				}
				else if (vid >= 0 && vid < m_noVid && m_vids[vid]) {
					m_vids[vid]->m_unk0x408[17] = function;
				}
			}
			else if (!strncmp(
						 text + 5,
						 // STRING: ALIEN 0x482638
						 "COLLISION",
						 9
					 )) {
				if (entry.m_var.m_extra != 2) {
					STRING part("no parameters in functions '", text);
					STRING message(part.m_str, "'");
					if (::Error) {
						MYERROR::Error(::Error, "MAP", 4, message.m_str, entry.m_var.m_extra);
					}
				}
				else if (vid >= 0 && vid < m_noVid && m_vids[vid]) {
					m_vids[vid]->m_unk0x408[19] = function;
				}
			}
			else {
				int animation = text[6] ? 10 * (text[5] - '0') + text[6] - '0' : text[5] - '0';
				if (entry.m_var.m_extra != 1) {
					STRING part("no parameters in functions '", text);
					STRING message(part.m_str, "'");
					if (::Error) {
						MYERROR::Error(::Error, "MAP", 4, message.m_str, entry.m_var.m_extra);
					}
				}
				else if (vid >= 0 && vid < m_noVid && m_vids[vid] && animation < 17) {
					m_vids[vid]->m_unk0x408[animation] = function;
				}
			}
		}
		else if (isdigit(text[4]) && text[5] == '_') {
			int vid = 1000 * (text[1] - '0') + 100 * (text[2] - '0') + 10 * (text[3] - '0') + text[4] - '0';
			int animation = text[7] ? 10 * (text[6] - '0') + text[7] - '0' : text[6] - '0';
			if (entry.m_var.m_extra != 1) {
				STRING part("no parameters in functions '", text);
				STRING message(part.m_str, "'");
				if (::Error) {
					MYERROR::Error(::Error, "MAP", 4, message.m_str, entry.m_var.m_extra);
				}
			}
			else if (vid >= 0 && vid < m_noVid && m_vids[vid] && animation < 17) {
				m_vids[vid]->m_unk0x408[animation] = function;
			}
		}
	}

	if (m_flag & 0x200) {
		m_logic.LoadVar(&m_resource);
	}
	else if (m_flag & 0x100) {
		m_logic.SaveVar(&m_resource);
	}
	RealCurrentTime = Platform_Ticks();

	if (Const->m_debugMode) {
		((GRAPH*) Graph)
			->DrawDebugText(
				// STRING: ALIEN 0x482618
				"Run scripts for create sprites"
			);
	}
	if (!(m_flag & 1)) {
		for (int layer = 0; layer < 17; ++layer) {
			int iter = m_layers[layer].m_n;
			for (SPRITE* sprite = NextSprite(layer, &iter); sprite; sprite = NextSprite(layer, &iter)) {
				int fn = sprite->m_vid->m_unk0x408[14];
				if (fn >= 0) {
					ScriptRun(fn, sprite, 0, 0);
				}
			}
		}
	}
	for (int layer = 0; layer < 17; ++layer) {
		int iter = m_layers[layer].m_n;
		for (SPRITE* sprite = NextSprite(layer, &iter); sprite; sprite = NextSprite(layer, &iter)) {
			if (sprite->m_vid->m_sprClass == 21) {
				((ENGINE*) sprite)->CheckPrevNextEngine();
			}
		}
	}

	if (!(m_flag & 0x200)) {
		Mouse->Enable();
	}
	if (Const->m_debugMode) {
		((GRAPH*) Graph)->DrawDebugText("");
	}
	return 0;
}

// FUNCTION: ALIEN 0x40e870
SPRITE* MAP::CreateSprite(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent)
{
	if (!p_vid) {
		return 0;
	}
	VID* vid = p_vid;
	if (p_vid->m_unk0x47c & 0x10) {
		vid = REGION::ConvertVid(p_vid, p_x, p_y, p_z);
	}

	SPRITE* sprite;

	switch (vid->m_sprClass) {
	case 5: // B_CANNON
		sprite = new CANNON(vid, p_x, p_y, p_z, p_dir, p_parent);
		break;
	case 9: // B_SPRITE
		sprite = new SPRITE(vid, p_x, p_y, p_z, p_dir, p_parent);
		break;
	case 2: // B_UNIT
		sprite = new UNIT(vid, p_x, p_y, p_z, p_dir, p_parent);
		break;
	case 4: // B_AVIA
		sprite = new PLANE(vid, p_x, p_y, p_z, p_dir, p_parent);
		break;
	case 10: // B_FRAME
		sprite = new FRAME(vid, p_x, p_y, p_z, p_dir, p_parent);
		break;
	case 12: // B_LINKER
		sprite = new LINKER(vid, p_x, p_y, p_z, p_dir, p_parent);
		break;
	case 19: // B_TEXT
		sprite = new STEXT(vid, p_x, p_y, p_z, p_dir, p_parent);
		break;
	case 6: // B_PRIMITIVE
		sprite = new PRIMITIVE(vid, p_x, p_y, p_z, p_dir, p_parent);
		break;
	case 23: // B_REGION
		sprite = new REGION(vid, p_x, p_y, p_z, p_dir, p_parent);
		break;
	case 7: // B_PLAYER__
		sprite = new MAN(vid, p_x, p_y, p_z, p_dir, p_parent);
		break;
	case 8: // B_UNK_MB_OBJ__
		sprite = new BUILDED_TERRAIN(vid, p_x, p_y, p_z, p_dir, p_parent);
		break;
	default:
		if (::Error) {
			MYERROR::Error(
				::Error,
				"MAP",
				3,
				// STRING: ALIEN 0x4828b0
				"sprite - Behave is invalidate",
				vid->m_sprClass
			);
		}
		return 0;
	}

	if (!(m_flag & 0x21) && sprite) {
		int fn = sprite->m_vid->m_unk0x408[14];
		if (fn >= 0) {
			ScriptRun(fn, sprite, 0, 0);
		}
	}
	return sprite;
}

// FUNCTION: ALIEN 0x40ef30
SPRITE* MAP::ReadPointer(STREAM* p_stream)
{
	// The stream holds a 32-bit identity token, not a live pointer. Reading it
	// straight into a pointer variable left the top four bytes uninitialised.
	int token = 0;
	p_stream->Read(&token, 4);
	if (token == -1) {
		return (SPRITE*) -1;
	}
	return (SPRITE*) m_relation.Decode((const void*) (decomp_intptr) token);
}

// FUNCTION: ALIEN 0x40ef70
SPRITE* MAP::OldLoadSprite(RESOURCE* p_resource)
{
	SPRITE* sprite = 0;
	int token;
	p_resource->Read(&token, 4);
	if (token == -1) {
		return (SPRITE*) -1;
	}
	short nvid;
	short x;
	short y;
	short z;
	unsigned char direction;
	unsigned char unused;
	p_resource->Read(&nvid, 2);
	p_resource->Read(&x, 2);
	p_resource->Read(&y, 2);
	p_resource->Read(&z, 2);
	p_resource->Read(&direction, 1);
	p_resource->Read(&unused, 1);
	if (VidExists(nvid)) {
		sprite = CreateSprite(m_vids[nvid], (float) x, (float) y, (float) z, ANGLE((char) direction), 0);
	}
	else if (::Error) {
		MYERROR::Error(
			::Error,
			"MAP",
			3,
			// STRING: ALIEN 0x4828d0
			"sprite, this vid not exist",
			nvid
		);
	}
	m_relation.Insert((void*) (decomp_intptr) token, sprite);
	return sprite;
}

// FUNCTION: ALIEN 0x40f7f0
void MAP::DrawLayer(int p_layer)
{
	int iter;
	if (p_layer && p_layer != 10) {
		float viewWidth = Graph->m_viewXMax - Graph->m_viewXMin;
		float viewHeight = Graph->m_viewYMax - Graph->m_viewYMin;
		int cx = (int) ((Graph->m_viewXMin + Graph->m_viewXMax) * 0.5f + m_shiftX);
		int cy = (int) ((Graph->m_viewYMin + Graph->m_viewYMax) * 0.5f + m_shiftY);
		iter = m_layers[p_layer].m_n;
		SPRITE* sprite = NextSprite(p_layer, &iter);
		while (sprite) {
			if (!(sprite->m_flag & 0x10000) && VIEWPORT_MATH::CoarseSpriteVisible(
												   (int) sprite->m_x,
												   (int) (sprite->m_y - sprite->m_z),
												   (int) sprite->m_y,
												   cx,
												   cy,
												   viewWidth,
												   viewHeight
											   )) {
				sprite->Draw();
			}
			sprite = 0;
			for (--iter; iter >= 0; --iter) {
				if (m_layers[p_layer].m_data[iter]) {
					sprite = m_layers[p_layer].m_data[iter];
					break;
				}
			}
		}
	}
	else {
		SPRITE* sprite = FirstSprite(p_layer, &iter);
		while (sprite) {
			if (!(sprite->m_flag & 0x10000)) {
				sprite->Draw();
			}
			sprite = 0;
			for (--iter; iter >= 0; --iter) {
				if (m_layers[p_layer].m_data[iter]) {
					sprite = m_layers[p_layer].m_data[iter];
					break;
				}
			}
		}
	}
	MOUSE* child = Mouse;
	if (!Mouse->m_unk0x70 && !(Mouse->m_vid->m_flag & 0x8000) && Mouse) {
		do {
			if (child->m_vid->m_layer == p_layer && !(child->m_flag & 0x10000)) {
				child->Draw();
			}
			child = (MOUSE*) child->m_child;
		} while (child);
	}
}

// FUNCTION: ALIEN 0x410e60
float MAP::GetGroundZ_ff(float p_x, float p_y)
{
	float v3;
	if (p_x < 0.0f) {
		v3 = 0.0f;
	}
	else {
		float v4;
		if (p_x >= m_w) {
			v4 = m_w - 1.0f;
		}
		else {
			v4 = p_x;
		}
		v3 = v4 * 0.125f;
	}
	float v5;
	if (p_y < 0.0f) {
		v5 = 0.0f;
	}
	else {
		float v6;
		if (p_y >= m_h) {
			v6 = m_h - 1.0f;
		}
		else {
			v6 = p_y;
		}
		v5 = v6 * 0.125f;
	}
	int v7 = (int) v3 + (int) v5 * m_groundW;
	short result;
	if (m_groundz[v7] > m_tempGroundz[v7]) {
		result = m_groundz[v7];
	}
	else {
		result = m_tempGroundz[v7];
	}
	return result;
}

// FUNCTION: ALIEN 0x411050
float MAP::GetGroundZ_vid(VID* p_vid, float p_x, float p_y)
{
	if (p_vid->m_sprClass != 7) {
		return GetGroundZ_ff(p_x, p_y);
	}

	float halfWidth = p_vid->m_footprintWidth * 0.5f;
	float halfHeight = halfWidth + p_x - 3.0f;
	float left = p_vid->m_footprintHeight * 0.5f;
	float bottom = p_y + left - 3.0f;
	p_x = p_x - (halfWidth - 3.0f);
	p_y = p_y - (left - 3.0f);

	float x0;
	if (p_x < 0.0f) {
		x0 = 0.0f;
	}
	else if (p_x >= m_w) {
		x0 = (m_w - 1.0f) * 0.125f;
	}
	else {
		x0 = p_x * 0.125f;
	}
	float y0;
	if (p_y < 0.0f) {
		y0 = 0.0f;
	}
	else if (p_y >= m_h) {
		y0 = (m_h - 1.0f) * 0.125f;
	}
	else {
		y0 = p_y * 0.125f;
	}
	float x1;
	if (halfHeight < 0.0f) {
		x1 = 0.0f;
	}
	else if (halfHeight >= m_w) {
		x1 = (m_w - 1.0f) * 0.125f;
	}
	else {
		x1 = halfHeight * 0.125f;
	}
	float y1;
	if (bottom < 0.0f) {
		y1 = 0.0f;
	}
	else if (bottom >= m_h) {
		y1 = (m_h - 1.0f) * 0.125f;
	}
	else {
		y1 = bottom * 0.125f;
	}
	int height = -16383;
	while (y0 <= y1) {
		for (float x = x0; x <= x1; x = x + 1.0f) {
			if (m_groundz[(int) x + (int) y0 * m_groundW] > height) {
				height = m_groundz[(int) x + (int) y0 * m_groundW];
			}
			if (m_tempGroundz[(int) x + (int) y0 * m_groundW] > height) {
				height = m_tempGroundz[(int) x + (int) y0 * m_groundW];
			}
		}
		y0 = y0 + 1.0f;
	}
	return height;
}

// FUNCTION: ALIEN 0x411260
void MAP::SetGroundZ(float p_x, float p_y, float p_z)
{
	if (p_x >= 0.0f && p_x < m_w && p_y >= 0.0f && p_y < m_h) {
		float y = p_y;
		int idx = (int) p_x / 8 - (int) (y * -0.125f) * m_groundW;
		if (m_groundz[idx] < (int) p_z) {
			m_groundz[idx] = (short) p_z;
		}
	}
}

// FUNCTION: ALIEN 0x411300
void MAP::SetTempGroundZ(float p_x, float p_y, float p_z)
{
	if (p_x >= 0.0f && p_x < m_w && p_y >= 0.0f && p_y < m_h) {
		float y = p_y;
		int idx = (int) p_x / 8 - (int) (y * -0.125f) * m_groundW;
		if (m_tempGroundz[idx] < (int) p_z) {
			m_tempGroundz[idx] = (short) p_z;
		}
	}
}

// FUNCTION: ALIEN 0x4113a0
void MAP::ClearTempGroundZ(float p_x, float p_y, float p_z)
{
	if (p_x >= 0.0f && p_x < m_w && p_y >= 0.0f && p_y < m_h) {
		float y = p_y;
		int idx = (int) p_x / 8 - (int) (y * -0.125f) * m_groundW;
		if (m_tempGroundz[idx] == (int) p_z) {
			m_tempGroundz[idx] = 0;
		}
	}
}
