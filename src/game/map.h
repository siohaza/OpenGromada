#ifndef MAP_H
#define MAP_H

#include "game/input_as.h"
#include "game/man.h"
#include "game/player.h"
#include "logic/logic.h"
#include "logic/relation.h"
#include "platform/platform_types.h"
#include "sprite/list_sprite.h"
#include "ui/menu.h"
#include "ui/mousetips.h"
#include "util/angle.h"
#include "util/decomp.h"
#include "util/resource.h"
#include "util/sprite_ids.h"
#include "world/groups.h"

class SETTINGS;
class TERRAIN_CAMERA;
union SDL_Event;

class VID;
extern VID* EmptyVid;

// VTABLE: ALIEN 0x47a308

class MAP {
public:
	static constexpr int MAX_VIDS = 8192;
	static constexpr int MAX_LAYERS = 21;
	static constexpr int GETSPRITE_VID_LEGACY = 0x800;
	static constexpr int GETSPRITE_VID = 0x2000;
	static constexpr int GETSPRITE_VID_INDEX = 0x1fff;

	static constexpr bool ValidVidIndex(int p_idx) { return (unsigned int) p_idx < MAX_VIDS; }

	static constexpr int MakeVidQuery(int p_idx)
	{
		if (!ValidVidIndex(p_idx)) {
			return 0;
		}
		return GETSPRITE_VID + p_idx;
	}

	static constexpr bool IsVidQuery(int p_query)
	{
		return (p_query & GETSPRITE_VID) != 0 || (p_query & 0x2800) == GETSPRITE_VID_LEGACY;
	}

	static constexpr int VidFromQuery(int p_query)
	{
		if (!IsVidQuery(p_query)) {
			return -1;
		}
		if (p_query & GETSPRITE_VID) {
			return p_query & GETSPRITE_VID_INDEX;
		}
		return p_query & (GETSPRITE_VID_LEGACY - 1);
	}

	static bool IsVidQueryFor(int p_query);
	static int VidFromQueryFor(int p_query);

	static int LayerCount();
	static int LayerWalkCount();

	MAP(STRING& p_argv, SETTINGS* p_settings);
	virtual ~MAP(); // 0x00
	void DiscardScriptFiles();

	// Returns non-zero when the event was consumed.
	virtual int ProcessEvent(const union SDL_Event& p_event); // 0x04
	virtual void DeletePointerToSprite(SPRITE* p_sprite);     // 0x08
	virtual int Tact();                                       // 0x0c
	virtual void DrawSecondaryInfo();                         // 0x10
	virtual void Release();                                   // 0x14
	virtual int Load(STRING p_name);                          // 0x18
	virtual int SaveMap(STRING p_name);                       // 0x1c
	virtual SPRITE* CreateSprite(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir,
								 SPRITE* p_parent); // 0x20
	int m_fps;                                      // 0x04
	int m_fpsCnt;                                   // 0x08
	unsigned int m_flag;                            // 0x0c
	undefined m_unk0x10[0x4];                       // 0x10
	float m_speed;                                  // 0x14
	STRING m_title;                                 // 0x18
	STRING m_mapName;                               // 0x1c
	STRING m_scriptName;                            // 0x20
	STRING m_prevMap;                               // 0x24
	STRING m_resName;                               // 0x28
	int m_noTact;                                   // 0x2c
	unsigned int m_unk0x30;                         // 0x30
	float m_w;                                      // 0x34
	float m_h;                                      // 0x38
	unsigned int m_shiftFlag;                       // 0x3c
	float m_shiftX1;                                // 0x40
	float m_shiftX2;                                // 0x44
	float m_shiftY1;                                // 0x48
	float m_shiftY2;                                // 0x4c
	float m_shiftX;                                 // 0x50
	float m_shiftY;                                 // 0x54
	SPRITE_LIST m_layers[MAX_LAYERS];
	LOGIC m_logic;                                  // 0x168
	RESOURCE m_resource;                            // 0x1c0
	RELATION m_relation;                            // 0x200
	short* m_groundz;                               // 0x220
	short* m_tempGroundz;                           // 0x224
	int m_groundW;                                  // 0x228
	int m_groundH;                                  // 0x22c
	void* m_window;                                 // 0x234

	int m_quit;
	undefined4 m_curArmy; // 0x23c
	PLAYER* m_player[4];  // 0x240
	INPUT_AS m_input;     // 0x250
	MENU m_menu;          // 0x270
	GROUPS m_groups;      // 0x288
	int m_noWeapon;       // 0x2ac
	void* m_weapon;       // 0x2b0
	int m_noVid;          // 0x2b4
	VID* m_vids[MAX_VIDS];
	MOUSETIPS m_mousetips;

	union {
		undefined m_unk0x22c0[4];
		unsigned int m_unk0x22c0_d;
	};

	TERRAIN_CAMERA* m_terrainCamera;
	int m_menuFrameActive;
	int m_menuFrameSavedW;
	int m_menuFrameSavedH;
	SPRITE_SAVE_IDS m_saveSpriteIds;
	bool m_gameplayMap = false;

	int DemoTact();
	VID* Vid(int p_idx) const;
	int VidExist(int p_idx) const;
	void SetFlagman(int p_army, SPRITE* p_sprite) const;
	MAN* Flagman(int p_idx) const;
	void GetAudioListener(float* p_x, float* p_y) const;
	SPRITE* SpriteUnderCursor() const;
	int ScriptRun(int p_fn, const SPRITE* p_a, const SPRITE* p_b, int p_c);
	VID** ExecFunc(int p_cmd);
	int PopInt();
	VID* PopVid(const char* p_context);
	VID* ReadVid(STREAM* p_stream) const;
	void WriteVid(STREAM* p_stream, const VID* p_vid) const;
	int VidExists(int p_n) { return ValidVidIndex(p_n) && p_n < m_noVid && m_vids[p_n] != 0; }
	VID* GetVid(int p_n)
	{
		if (VidExists(p_n)) {
			return m_vids[p_n];
		}
		return EmptyVid;
	}
	void ExchangeVid(VID* p_vid1, VID* p_vid2);
	int LoadWeapon(RESOURCE* p_res);
	void DeleteExtraVid();
	float GetGroundZScr(float p_x, float p_y);
	void LoadVid(RESOURCE* p_res);
	VID* CreateVid(RESOURCE* p_res, int p_idx);
	STRING* PopStr();
	float GetGroundZ_ff(float p_x, float p_y);
	float GetGroundZ_vid(VID* p_vid, float p_x, float p_y);
	void SetGroundZ(float p_x, float p_y, float p_z);
	void SetTempGroundZ(float p_x, float p_y, float p_z);
	void ClearTempGroundZ(float p_x, float p_y, float p_z);
	SPRITE* FirstSprite(int p_layer, int* p_iter) const;
	int StartTact();
	void ReloadVid();
	void ControlShiftCoor();
	void DrawLayer(int p_layer);
	STRING GetMouseTipsString() const;
	SPRITE* NextSprite(int p_layer, int* p_iter) const;
	SPRITE* NextSpriteByType(int p_type, int* p_iter, int p_flag);
	SPRITE* GetSprite(int p_type, float p_x, float p_y, SPRITE* p_prev);
	SPRITE* FindNearestSprite(int p_type, float p_x, float p_y, float p_radius, SPRITE* p_prev);
	SPRITE* GetSpriteScr(int p_type, float p_scrX, float p_scrY);
	void PushInt(int p_value);
	void PushStr(const STRING& p_value);
	void PushObject(const void* p_object);
	decomp_intptr PopObject();
	float AbsX(float p_x);
	float AbsY(float p_y);
	float ScrX(float p_x);
	float ScrY(float p_y);
	int Error(int p_type, const char* p_msg, int p_size);
	void SetShiftCoor(float p_x, float p_y, int p_flag);
	int ResetGroundZ();
	PLAYER* Player(int p_army);
	char** GetVariableStr(char** p_out, STRING p_name);
	int PauseOn();
	void PauseOff();
	void CreateEmptyHardwareGround();
	void FinalizeTerrainCamera(int p_gameplay);
	void EnterFullscreenMenuFrame();
	void LeaveFullscreenMenuFrame();
	void ClearTerrainCamera();
	bool CurrentTerrainViewSafe() const;
	SPRITE* ReadPointer(STREAM* p_stream);
	SPRITE* LoadSprite(RESOURCE* p_resource, int p_version);
	SPRITE* OldLoadSprite(RESOURCE* p_resource);
	int RemoveSpriteFromLayer(SPRITE* p_sprite);
	void InsertSpriteToLayer(SPRITE* p_sprite);
};

// SYNTHETIC: ALIEN 0x40b140
// MAP::`scalar deleting destructor'

extern MAP* Map;

extern int EvFunctionNumber[64];

static_assert(MAP::ValidVidIndex(2047));
static_assert(MAP::ValidVidIndex(2048));
static_assert(MAP::ValidVidIndex(4095));
static_assert(MAP::ValidVidIndex(4096));
static_assert(MAP::ValidVidIndex(8191));
static_assert(!MAP::ValidVidIndex(8192));

static_assert(!MAP::ValidVidIndex(-1));

static_assert(sizeof(((MAP*) 0)->m_vids) / sizeof(VID*) == MAP::MAX_VIDS);
static_assert(MAP::VidFromQuery(MAP::MakeVidQuery(2047)) == 2047);
static_assert(MAP::VidFromQuery(MAP::MakeVidQuery(2048)) == 2048);
static_assert(MAP::VidFromQuery(MAP::MakeVidQuery(4095)) == 4095);
static_assert(MAP::VidFromQuery(MAP::MakeVidQuery(4096)) == 4096);
static_assert(MAP::VidFromQuery(MAP::MakeVidQuery(8191)) == 8191);
static_assert(MAP::MakeVidQuery(4096) == 0x3000);
static_assert(MAP::MakeVidQuery(8191) == 0x3fff);

static_assert(MAP::MakeVidQuery(0) == 0x2000);
static_assert(MAP::MakeVidQuery(2047) == 0x27ff);
static_assert(MAP::MakeVidQuery(2048) == 0x2800);
static_assert(MAP::MakeVidQuery(4095) == 0x2fff);
static_assert(MAP::VidFromQuery(0x800) == 0);
static_assert(MAP::VidFromQuery(0xfff) == 2047);
static_assert(MAP::VidFromQuery(0x1800) == 0);
static_assert(MAP::VidFromQuery(0x1fff) == 2047);
static_assert(!MAP::IsVidQuery(0x1000));
static_assert(!MAP::IsVidQuery(0x1015));
static_assert(!MAP::IsVidQuery(0x9015));
static_assert(MAP::VidFromQuery(0x9015) == -1);
static_assert(MAP::VidFromQuery(MAP::MakeVidQuery(0) | 0x18000) == 0);
static_assert(MAP::VidFromQuery(MAP::MakeVidQuery(4095) | 0x18000) == 4095);
static_assert(MAP::MakeVidQuery(-1) == 0);
static_assert(MAP::MakeVidQuery(8192) == 0);
static_assert(MAP::IsVidQuery(MAP::GETSPRITE_VID));
static_assert(MAP::VidFromQuery(MAP::GETSPRITE_VID) == 0);
static_assert(MAP::IsVidQuery(0x3000));
static_assert(MAP::IsVidQuery(0x3fff));
static_assert(MAP::VidFromQuery(0x3000) == 4096);
static_assert(MAP::VidFromQuery(0x3fff) == 8191);
static_assert(!MAP::IsVidQuery(0x400000));
static_assert(!MAP::IsVidQuery(0x1000 | 10));
static_assert(MAP::VidFromQuery(MAP::MakeVidQuery(5000) | 0x80000000) == 5000);
static_assert(MAP::VidFromQuery(MAP::MakeVidQuery(5000) | 0xf8000) == 5000);

#endif
