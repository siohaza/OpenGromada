#ifndef MAP_H
#define MAP_H

#include "sprite/list_sprite.h"

#include "util/decomp.h"
#include "util/angle.h"
#include "game/input_as.h"
#include "game/man.h"
#include "game/player.h"
#include "logic/logic.h"
#include "logic/relation.h"
#include "ui/menu.h"
#include "ui/mousetips.h"
#include "util/resource.h"
#include "world/groups.h"

#include <windows.h>

class SETTINGS;

class VID;
extern VID* EmptyVid;

// VTABLE: ALIEN 0x47a308

class MAP {
public:
	MAP(HINSTANCE p_instance, HINSTANCE p_prevInstance, STRING& p_argv, int p_showCmd,
		SETTINGS* p_settings);
	virtual ~MAP(); // 0x00
	virtual int WndProc(void* p_wnd, unsigned int p_msg, unsigned int p_wparam, int p_lparam); // 0x04
	virtual void DeletePointerToSprite(SPRITE* p_sprite); // 0x08
	virtual int Tact(); // 0x0c
	virtual void DrawSecondaryInfo(); // 0x10
	virtual void Release(); // 0x14
	virtual int Load(STRING p_name); // 0x18
	virtual int SaveMap(STRING p_name); // 0x1c
	virtual SPRITE* CreateSprite(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir,
								 SPRITE* p_parent); // 0x20
	int m_fps; // 0x04
	int m_fpsCnt; // 0x08
	unsigned int m_flag; // 0x0c
	undefined m_unk0x10[0x4]; // 0x10
	float m_speed; // 0x14
	STRING m_title; // 0x18
	STRING m_mapName; // 0x1c
	STRING m_scriptName; // 0x20
	STRING m_prevMap; // 0x24
	STRING m_resName; // 0x28
	int m_noTact; // 0x2c
	unsigned int m_unk0x30; // 0x30
	float m_w; // 0x34
	float m_h; // 0x38
	unsigned int m_shiftFlag; // 0x3c
	float m_shiftX1; // 0x40
	float m_shiftX2; // 0x44
	float m_shiftY1; // 0x48
	float m_shiftY2; // 0x4c
	float m_shiftX; // 0x50
	float m_shiftY; // 0x54
	SPRITE_LIST m_layers[17]; // 0x58
	LOGIC m_logic; // 0x168
	RESOURCE m_resource; // 0x1c0
	RELATION m_relation; // 0x200
	short* m_groundz; // 0x220
	short* m_tempGroundz; // 0x224
	int m_groundW; // 0x228
	int m_groundH; // 0x22c
	void* m_hInstance; // 0x230
	void* m_hWnd; // 0x234
	void* m_hAccel; // 0x238
	undefined4 m_curArmy; // 0x23c
	PLAYER* m_player[4]; // 0x240
	INPUT_AS m_input; // 0x250
	MENU m_menu; // 0x270
	GROUPS m_groups; // 0x288
	int m_noWeapon; // 0x2ac
	void* m_weapon; // 0x2b0
	int m_noVid; // 0x2b4
	VID* m_vids[0x800]; // 0x2b8
	MOUSETIPS m_mousetips; // 0x22b8

	union {
		undefined m_unk0x22c0[4]; // 0x22c0
		unsigned int m_unk0x22c0_d;
	};

	STRING GetMapFileName(int p_save, const char* p_mask);
	int DemoTact();
	VID* Vid(int p_idx) const;
	int VidExist(int p_idx) const;
	void SetFlagman(int p_army, SPRITE* p_sprite) const;
	MAN* Flagman(int p_idx) const;
	SPRITE* SpriteUnderCursor() const;
	int ScriptRun(int p_fn, const SPRITE* p_a, const SPRITE* p_b, int p_c);
	VID** ExecFunc(int p_cmd);
	int PopInt();
	VID* PopVid(const char* p_context);
	VID* ReadVid(STREAM* p_stream) const;
	void WriteVid(STREAM* p_stream, const VID* p_vid) const;
	int VidExists(int p_n)
	{
		return p_n >= 0 && p_n < m_noVid && m_vids[p_n] != 0;
	}
	VID* GetVid(int p_n)
	{
		if (VidExists(p_n))
			return m_vids[p_n];
		return EmptyVid;
	}
	void ExchangeVid(VID* p_vid1, VID* p_vid2);
	int LoadWeapon(RESOURCE* p_res);
	void DeleteExtraVid();
	float GetGroundZScr(float p_x, float p_y);
	char* LoadVid(RESOURCE* p_res);
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
	int PopObject();
	float AbsX(float p_x);
	float AbsY(float p_y);
	float ScrX(float p_x);
	float ScrY(float p_y);
	char* Error(int p_type, const char* p_msg, int p_size);
	void SetShiftCoor(float p_x, float p_y, int p_flag);
	int ResetGroundZ();
	PLAYER* Player(int p_army);
	char** GetVariableStr(char** p_out, STRING p_name);
	int PauseOn();
	void PauseOff();
	void RestoreVidSurfaces();
	void CreateEmptyHardwareGround();
	void ReleaseVidSurfaces();
	SPRITE* ReadPointer(STREAM* p_stream);
	SPRITE* LoadSprite(RESOURCE* p_resource, int p_version);
	SPRITE* OldLoadSprite(RESOURCE* p_resource);
	int RemoveSpriteFromLayer(SPRITE* p_sprite);
	char* InsertSpriteToLayer(SPRITE* p_sprite);
};

DECOMP_SIZE_ASSERT(MAP, 0x22c4)

// SYNTHETIC: ALIEN 0x40b140
// MAP::`scalar deleting destructor'

extern MAP* Map;

extern int EvFunctionNumber[64];

#ifdef DECOMP_INLINE_MAP_NEXTSPRITE
inline SPRITE* MAP::NextSprite(int p_layer, int* p_iter) const
{

#ifdef DECOMP_INLINE_MAP_NEXTSPRITE_BYVALUE
	--*p_iter;
	if (*p_iter >= 0) {
		while (*p_iter >= 0) {
			if (m_layers[p_layer].m_data[*p_iter])
				return m_layers[p_layer].m_data[*p_iter];
			--*p_iter;
		}
	}
#elif defined(DECOMP_INLINE_MAP_NEXTSPRITE_CURSOR)
	int i;
	--*p_iter;
	if (*p_iter >= 0) {
		SPRITE** const* data = &m_layers[p_layer].m_data;
		while (*p_iter >= 0) {
			if ((*data)[*p_iter])
				return (*data)[*p_iter];
			--*p_iter;
		}
	}
#else
	int i = *p_iter - 1;
	*p_iter = i;
	if (i >= 0) {
		SPRITE** const* data = &m_layers[p_layer].m_data;
		while (i >= 0) {
			if ((*data)[i])
				return (*data)[*p_iter];
			*p_iter = --i;
		}
	}
#endif
	return 0;
}
#endif

#ifdef DECOMP_INLINE_MAP_VIDIO
#include "util/stream.h"
#include "video/vid.h"

inline VID* MAP::ReadVid(STREAM* p_stream) const
{
	int idx;
	VID* vid;
	p_stream->Read(&idx, 4);
	if (idx < 0 || idx >= m_noVid || (vid = m_vids[idx]) == 0)
		vid = 0;
	return vid;
}

inline void MAP::WriteVid(STREAM* p_stream, const VID* p_vid) const
{
	int noVid = -1;
	p_stream->Write(p_vid ? &p_vid->m_idx : &noVid, 4);
}
#endif

#ifdef DECOMP_INLINE_MAP_FIRSTSPRITE
inline SPRITE* MAP::FirstSprite(int p_layer, int* p_iter) const
{
	SPRITE* sprite;
	*p_iter = m_layers[p_layer].m_n - 1;
	SPRITE** const* data = &m_layers[p_layer].m_data;
	for (;;) {
		if (*p_iter < 0) {
			sprite = 0;
			break;
		}
		if ((*data)[*p_iter]) {
			sprite = (*data)[*p_iter];
			break;
		}
		--*p_iter;
	}
	return sprite;
}
#endif

#endif
