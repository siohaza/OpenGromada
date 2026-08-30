#include "ui/menu.h"

#include "game/input_as.h"
#include "game/map.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "platform/paths.h"
#include "sprite/list_sprite.h"
#include "sprite/sprite.h"
#include "ui/menu_path.h"
#include "ui/ui_scaling.h"
#include "util/myerror.h"
#include "util/resource.h"
#include "video/vid.h"

#include <math.h>
#include <string>

// GLOBAL: ALIEN 0x47a818
double g_dbl47A818 = 0.001;

namespace
{

struct MENU_HEADER {
	int m_version;
	int m_width;
	int m_height;
	int m_originX;
	int m_originY;
};

struct MENU_LAYOUT {
	int m_gamebarWidth;
	int m_gamebarHeight;
};

static bool MenuFileExists(const std::string& p_name)
{
	FILE* file = Platform_FOpen(p_name.c_str(), "rb");
	if (!file) {
		return false;
	}
	fclose(file);
	return true;
}

static std::string ResolveMenuName(const STRING& p_name)
{
	std::string requested(p_name.m_str);
	if (MenuFileExists(requested) || !MENU_PATH::IsGamebarVariant(p_name.m_str)) {
		return requested;
	}

	GRAPH_CORE* graph = (GRAPH_CORE*) Graph;
	int preferred = MENU_PATH::PreferredGamebarWidth(graph ? (int) graph->m_width : 640, graph ? graph->m_uiScale : 1);
	std::string candidate = MENU_PATH::CanonicalGamebar(p_name.m_str, preferred);
	if (MenuFileExists(candidate)) {
		return candidate;
	}

	const int widths[] = {1024, 800, 640};
	for (int width : widths) {
		if (width == preferred) {
			continue;
		}
		candidate = MENU_PATH::CanonicalGamebar(p_name.m_str, width);
		if (MenuFileExists(candidate)) {
			return candidate;
		}
	}
	return requested;
}

static void ReadMenuHeader(RESOURCE* p_resource, MENU_HEADER* p_header)
{
	p_resource->Read(&p_header->m_version, 4);
	p_resource->Read(&p_header->m_width, 4);
	p_resource->Read(&p_header->m_height, 4);
	p_resource->Read(&p_header->m_originX, 4);
	p_resource->Read(&p_header->m_originY, 4);
}

static MENU_LAYOUT ResolveMenuLayout(const std::string& p_name)
{
	int width = MENU_PATH::GamebarVariantWidth(p_name.c_str());
	MENU_LAYOUT result = {width, MENU_PATH::GamebarVariantHeight(width)};
	return result;
}

static UI_SCALING::MENU_POINT MenuPoint(
	float p_x,
	float p_y,
	float p_z,
	int p_nvid,
	const MENU_HEADER& p_header,
	const MENU_LAYOUT& p_layout
)
{
	GRAPH_CORE* graph = (GRAPH_CORE*) Graph;
	float drawScale = (float) graph->m_uiScale * graph->m_uiPresentationScale;
	if (p_layout.m_gamebarWidth) {
		return UI_SCALING::TransformGamebarPoint(
			p_x,
			p_y,
			p_z,
			p_header.m_originX,
			p_header.m_originY,
			p_header.m_width,
			p_header.m_height,
			(float) p_layout.m_gamebarWidth,
			(float) p_layout.m_gamebarHeight,
			graph->m_width,
			graph->m_height,
			drawScale,
			p_nvid
		);
	}
	return UI_SCALING::TransformCenteredMenuPoint(
		p_x,
		p_y,
		p_z,
		p_header.m_originX,
		p_header.m_originY,
		p_header.m_width,
		p_header.m_height,
		graph->m_width,
		graph->m_height,
		drawScale
	);
}

static void PlaceLoadedMenuSprite(SPRITE* p_sprite, const MENU_HEADER& p_header, const MENU_LAYOUT& p_layout)
{
	float shiftX = Map->m_shiftX;
	float shiftY = Map->m_shiftY;
	UI_SCALING::MENU_POINT point = MenuPoint(
		p_sprite->X() - shiftX,
		p_sprite->Y() - shiftY,
		p_sprite->Z(),
		p_sprite->m_vid->m_idx,
		p_header,
		p_layout
	);
	p_sprite->ChangeCoor(point.m_x + shiftX, point.m_y + shiftY, point.m_z);
	p_sprite->SetUIScriptLayout(((GRAPH_CORE*) Graph)->m_uiScale, point.m_anchorX, point.m_anchorY);
}

static void AlignGamebarBackingWithInventory(MENU* p_menu, int p_firstLoaded, const MENU_LAYOUT& p_layout)
{
	if (!p_layout.m_gamebarWidth || !Map || !Graph) {
		return;
	}

	SPRITE* backing = 0;
	bool haveRightInventory = false;
	for (int i = p_firstLoaded; i < p_menu->m_n; ++i) {
		SPRITE* sprite = (SPRITE*) p_menu->m_data[i];
		if (!sprite || !sprite->m_vid) {
			continue;
		}
		int nvid = sprite->m_vid->m_idx;
		if (nvid == 742 && !(sprite->m_flag & SPRITE_FLAG_INVISIBLE)) {
			backing = sprite;
		}
		else if (nvid == 747 && sprite->UIAnchorX() == UI_SCALING::ANCHOR_MAX_EDGE) {
			haveRightInventory = true;
		}
	}
	if (!backing || !haveRightInventory || backing->m_vid->m_unk0x2f6 != 640) {
		return;
	}

	GRAPH_CORE* graph = (GRAPH_CORE*) Graph;
	int gap = UI_SCALING::GamebarInventoryOffset(
		graph->m_width,
		graph->m_height,
		backing->UIDrawScale(),
		backing->m_vid->m_unk0x2f6
	);
	if (gap > 0) {
		backing->SetUIHorizontalGap(gap);
	}
}

static bool ReadMenuSpriteIdentity(
	RESOURCE* p_resource,
	int p_version,
	bool p_flatRecords,
	int* p_nvid,
	float* p_x,
	float* p_y,
	float* p_z
)
{
	int pointerToken;
	p_resource->Read(&pointerToken, 4);
	if (pointerToken == -1) {
		return false;
	}
	p_resource->Read(p_nvid, 4);
	if (p_version > 9) {
		p_resource->Read(p_x, 4);
		p_resource->Read(p_y, 4);
		p_resource->Read(p_z, 4);
	}
	else {
		int coordinate;
		p_resource->Read(&coordinate, 4);
		*p_x = (float) coordinate;
		p_resource->Read(&coordinate, 4);
		*p_y = (float) coordinate;
		p_resource->Read(&coordinate, 4);
		*p_z = (float) coordinate;
	}
	if (p_flatRecords) {
		int ignored;
		p_resource->Read(&ignored, 4); // direction
		p_resource->Read(&ignored, 4); // army
	}
	return true;
}

static void DeleteMatchingMenuSprites(
	MENU* p_menu,
	int p_nvid,
	float p_x,
	float p_y,
	float p_z,
	const MENU_HEADER& p_header,
	const MENU_LAYOUT& p_layout
)
{
	UI_SCALING::MENU_POINT point = MenuPoint(p_x, p_y, p_z, p_nvid, p_header, p_layout);
	float worldX = point.m_x + Map->m_shiftX;
	float worldY = point.m_y + Map->m_shiftY;
	for (int i = 0; i < p_menu->m_n; ++i) {
		SPRITE* sprite = (SPRITE*) p_menu->m_data[i];
		if (sprite->m_vid->m_idx == p_nvid && fabs(sprite->X() - worldX) < g_dbl47A818 &&
			fabs(sprite->Y() - worldY) < g_dbl47A818 && fabs(sprite->Z() - point.m_z) < g_dbl47A818) {
			p_menu->DeleteSpriteNumber(i);
			--i;
		}
	}
}

} // namespace

// FUNCTION: ALIEN 0x43e120
MENU::MENU()
{
	m_data = 0;
	m_n = 0;
	m_max = 0;
	m_underCursor = 0;
	m_state = 0;
}

// FUNCTION: ALIEN 0x43e140
int MENU::Control(INPUT_AS* p_input)
{
	m_underCursor = 0;
	m_state = 0;
	for (int i = 0; i < m_n; ++i) {
		SPRITE* sprite = (SPRITE*) m_data[i];
		if (sprite) {
			VID* vid = sprite->m_vid;
			if (vid->m_unk0x18) {
				int ani = sprite->m_ani;
				if (ani != 14 && ani < 15 && ani != 7 && ani != 6) {
					if (!sprite->IsInside(p_input->m_worldX, p_input->m_worldY) ||
						(m_underCursor && sprite->Z() <= m_underCursor->Z())) {
						sprite->ChangeAnimation(sprite->m_ani & 1);
					}
					else {
						m_underCursor = sprite;
					}
				}
			}
		}
	}
	if (m_underCursor) {
		if (p_input->m_button & 1) {
			m_state |= 1;
			p_input->ClearLClick();
			m_underCursor->ChangeAnimation((m_underCursor->m_ani & 1) | 4);
			return 1;
		}
		if (p_input->m_button & 4) {
			m_state |= 2;
			p_input->ClearRClick();
		}
		SPRITE* sprite = m_underCursor;
		int ani = sprite->m_ani;
		if ((ani & 0xfffffffe) != 4) {
			sprite->ChangeAnimation((ani & 1) | 2);
		}
	}
	return 0;
}

// FUNCTION: ALIEN 0x43e2b0
int MENU::Load(const STRING& p_name)
{
	RESOURCE resource;
	std::string resolvedName = ResolveMenuName(p_name);
	STRING actualName(resolvedName.c_str());
	if (resource.OpenForRead(actualName, 0x554e454d)) {
		MYERROR::Error(
			::Error,
			// STRING: ALIEN 0x48436c
			"MENU",
			7,
			p_name.m_str,
			0
		);
		return 1;
	}
	if (resource.GoBegin(0x44414548)) {
		MYERROR::Error(
			::Error,
			"MENU",
			11,
			// STRING: ALIEN 0x48435c
			"'HEAD'in menu",
			0
		);
		return 1;
	}
	MENU_HEADER header;
	ReadMenuHeader(&resource, &header);
	MENU_LAYOUT layout = ResolveMenuLayout(resolvedName);
	int firstLoaded = m_n;

	if (!resource.GoNext(0x20525053)) {
		SPRITE* sprite = Map->LoadSprite(&resource, header.m_version);
		while (sprite != (SPRITE*) -1) {
			if (sprite) {
				PlaceLoadedMenuSprite(sprite, header, layout);
				sprite->Action(0x51, (decomp_intptr) &resource, header.m_version, 0);
				sprite->SetUIScriptLayout(((GRAPH_CORE*) Graph)->m_uiScale, sprite->UIAnchorX(), sprite->UIAnchorY());
			}
			resource.GoNextSub(0x20525053);
			sprite = Map->LoadSprite(&resource, header.m_version);
		}
	}
	else if (!resource.GoBegin(0x49525053)) {
		SPRITE* sprite = Map->LoadSprite(&resource, header.m_version);
		while (sprite != (SPRITE*) -1) {
			if (sprite) {
				PlaceLoadedMenuSprite(sprite, header, layout);
			}
			sprite = Map->LoadSprite(&resource, header.m_version);
		}
	}
	else {
		MYERROR::Error(
			::Error,
			"MENU",
			11,
			// STRING: ALIEN 0x484340
			"'SPR ' or 'SPRI' in menu",
			0
		);
		return 1;
	}
	AlignGamebarBackingWithInventory(this, firstLoaded, layout);
	resource.Close();
	return 0;
}

// STUB: ALIEN 0x43e5c0
int MENU::DeleteFromFile(const STRING& p_name)
{
	RESOURCE resource;
	std::string resolvedName = ResolveMenuName(p_name);
	STRING actualName(resolvedName.c_str());
	if (resource.OpenForRead(actualName, 0x554e454d)) {
		MYERROR::Error(::Error, "MENU", 7, p_name.m_str, 0);
		return 1;
	}
	if (resource.GoBegin(0x44414548)) {
		MYERROR::Error(::Error, "MENU", 11, "'HEAD'in menu", 0);
		return 1;
	}
	MENU_HEADER header;
	ReadMenuHeader(&resource, &header);
	MENU_LAYOUT layout = ResolveMenuLayout(resolvedName);
	bool flatRecords = false;
	if (resource.GoNext(0x20525053)) {
		if (resource.GoBegin(0x49525053)) {
			MYERROR::Error(::Error, "MENU", 11, "'SPR ' or 'SPRI' in MENU::DeleteFromFile", 0);
			return 1;
		}
		flatRecords = true;
	}

	for (;;) {
		int nvid;
		float x;
		float y;
		float z;
		if (!ReadMenuSpriteIdentity(&resource, header.m_version, flatRecords, &nvid, &x, &y, &z)) {
			break;
		}
		DeleteMatchingMenuSprites(this, nvid, x, y, z, header, layout);
		if (!flatRecords) {
			resource.GoNextSub(0x20525053);
		}
	}
	resource.Close();
	return 0;
}

// FUNCTION: ALIEN 0x43e880
int MENU::NVidUnderCursor() const
{
	SPRITE* underCursor = m_underCursor;
	if (underCursor) {
		return underCursor->m_vid->m_idx;
	}
	return 0;
}

// FUNCTION: ALIEN 0x43e8a0
unsigned int MENU::NDirUnderCursor() const
{
	SPRITE* underCursor = m_underCursor;
	if (underCursor) {
		return underCursor->m_vid->RealDirection(ANGLE(underCursor->m_dir));
	}
	return 0;
}
