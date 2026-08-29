#define DECOMP_INLINE_STRING_COPY_LIFETIME
#define DECOMP_INLINE_LIST_SPRITE_SPECIAL_MEMBERS
#include "ui/menu.h"

#include "game/map.h"
#include "gfx/graph.h"
#include "sprite/list_sprite.h"
#include "sprite/sprite.h"
#include "util/myerror.h"
#include "util/resource.h"

#include <math.h>
#include "game/input_as.h"
#include "video/vid.h"

// GLOBAL: ALIEN 0x47a818
double g_dbl47A818 = 0.001;

// FUNCTION: ALIEN 0x43e120
MENU::MENU()
{
	m_data = 0;
	m_n = 0;
	m_max = 0;
	m_underCursor = 0;
	m_state &= 0xfffffffc;
}

// FUNCTION: ALIEN 0x43e140
int MENU::Control(INPUT_AS* p_input)
{
	m_underCursor = 0;
	m_state &= 0xfffffffc;
	for (int i = 0; i < m_n; ++i) {
		SPRITE* sprite = (SPRITE*) m_data[i];
		if (sprite) {
			VID* vid = sprite->m_vid;
			if (vid->m_unk0x18) {
				int ani = sprite->m_ani;
				if (ani != 14 && ani < 15 && ani != 7 && ani != 6) {
					if (!sprite->IsInside(p_input->m_worldX, p_input->m_worldY)
						|| (m_underCursor && sprite->Z() <= m_underCursor->Z()))
						sprite->ChangeAnimation(sprite->m_ani & 1);
					else
						m_underCursor = sprite;
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
		if ((ani & 0xfffffffe) != 4)
			sprite->ChangeAnimation((ani & 1) | 2);
	}
	return 0;
}

// FUNCTION: ALIEN 0x43e2b0
int MENU::Load(const STRING& p_name)
{
	RESOURCE resource;
	if (resource.OpenForRead(p_name, 0x554e454d)) {
		MYERROR::Error(::Error,
			// STRING: ALIEN 0x48436c
			"MENU", 7, p_name.m_str, 0);
		return 1;
	}
	if (resource.GoBegin(0x44414548)) {
		MYERROR::Error(::Error, "MENU", 11,
			// STRING: ALIEN 0x48435c
			"'HEAD'in menu", 0);
		return 1;
	}
	int version;
	int originY;
	int height;
	int originX;
	int width;
	resource.Read(&version, 4);
	resource.Read(&width, 4);
	resource.Read(&height, 4);
	resource.Read(&originX, 4);
	resource.Read(&originY, 4);

	if (!resource.GoNext(0x20525053)) {
		SPRITE* sprite = Map->LoadSprite(&resource, version);
		while (sprite != (SPRITE*) -1) {
			if (sprite) {
				float screenWidth = Graph->m_width;
				float screenHeight = Graph->m_height;
				sprite->ChangeCoor(
					sprite->X() - originX - width / 2 + screenWidth * 0.5f,
					sprite->Y() - originY - height / 2 + screenHeight * 0.5f,
					sprite->Z());
				sprite->Action(0x51, (int) (decomp_intptr) &resource, version, 0);
			}
			resource.GoNextSub(0x20525053);
			sprite = Map->LoadSprite(&resource, version);
		}
	}
	else if (!resource.GoBegin(0x49525053)) {
		SPRITE* sprite = Map->LoadSprite(&resource, version);
		while (sprite != (SPRITE*) -1) {
			if (sprite) {
				float screenWidth = Graph->m_width;
				float screenHeight = Graph->m_height;
				sprite->ChangeCoor(
					sprite->X() - originX - width / 2 + screenWidth * 0.5f,
					sprite->Y() - originY - height / 2 + screenHeight * 0.5f,
					sprite->Z());
			}
			sprite = Map->LoadSprite(&resource, version);
		}
	}
	else {
		MYERROR::Error(::Error, "MENU", 11,
			// STRING: ALIEN 0x484340
			"'SPR ' or 'SPRI' in menu", 0);
		return 1;
	}
	resource.Close();
	return 0;
}

// STUB: ALIEN 0x43e5c0
int MENU::DeleteFromFile(const STRING& p_name)
{
	RESOURCE resource;
	if (resource.OpenForRead(p_name, 0x554e454d)) {
		MYERROR::Error(::Error, "MENU", 7, p_name.m_str, 0);
		return 1;
	}
	if (resource.GoBegin(0x44414548)) {
		MYERROR::Error(::Error, "MENU", 11, "'HEAD'in menu", 0);
		return 1;
	}
	int nvid;
	int originX;
	float x;
	int width;
	int version;
	int originY;
	float y;
	int height;
	float z;
	int pointerToken;
	resource.Read(&version, 4);
	resource.Read(&width, 4);
	resource.Read(&height, 4);
	resource.Read(&originX, 4);
	resource.Read(&originY, 4);
	if (!resource.GoNext(0x20525053)) {
		for (;;) {
			resource.Read(&pointerToken, 4);
			if (pointerToken == -1)
				break;
			resource.Read(&nvid, 4);
			resource.Read(&x, 4);
			resource.Read(&y, 4);
			resource.Read(&z, 4);
			for (int i = 0; i < m_n; ++i) {
				SPRITE* sprite = (SPRITE*) m_data[i];
				if (sprite->m_vid->m_idx == nvid
					&& fabs(sprite->X()
						   - (x - originX - width / 2 + Graph->m_width * 0.5f))
						< g_dbl47A818
					&& fabs(sprite->Y()
						   - (y - originY - height / 2 + Graph->m_height * 0.5f))
						< g_dbl47A818
					&& fabs(sprite->Z() - z) < g_dbl47A818) {
					DeleteSpriteNumber(i);
					--i;
				}
			}
			resource.GoNextSub(0x20525053);
		}
		resource.Close();
		return 0;
	}
	MYERROR::Error(::Error, "MENU", 11,
		// STRING: ALIEN 0x484374
		"'SPR ' in MENU::DeleteFromFile", 0);
	return 1;
}

// FUNCTION: ALIEN 0x43e880
int MENU::NVidUnderCursor() const
{
	SPRITE* underCursor = m_underCursor;
	if (underCursor)
		return underCursor->m_vid->m_idx;
	return 0;
}

// FUNCTION: ALIEN 0x43e8a0
unsigned int MENU::NDirUnderCursor() const
{
	SPRITE* underCursor = m_underCursor;
	if (underCursor)
		return underCursor->m_vid->RealDirection(ANGLE(underCursor->m_dir));
	return 0;
}
