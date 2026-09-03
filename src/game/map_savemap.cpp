
#include "game/map.h"

#include "game/gametime.h"
#include "game/player.h"
#include "gfx/graph.h"
#include "platform/paths.h"
#include "sprite/sprite.h"
#include "util/myerror.h"
#include "util/resource.h"
#include "video/vid.h"
#include "world/groups.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// STUB: ALIEN 0x40ded0
int MAP::SaveMap(STRING p_name)
{
	int version = 12;
	RESOURCE out;
	if (!strcmp(p_name.m_str, empty_str)) {
		return 0;
	}
	if (!strcmp(p_name.m_str, m_mapName)) {
		// Read from the temporary only when the writable source map was renamed;
		// shipped maps remain readable in place.
		STRING temp(
			// STRING: ALIEN 0x4828a0
			"tmp_del!.map"
		);
		if (Platform_Rename(m_mapName, temp.m_str) == 0) {
			m_mapName = temp.m_str;
		}
	}
	if (out.OpenForWrite(p_name, 0x2050414d /* 'MAP ' */)) {
		MYERROR::Window(
			::Error,
			// STRING: ALIEN 0x48288c
			"Can't open file %s",
			p_name.m_str
		);
		return 0;
	}
	int needData = 0;
	for (int v = 0; needData < m_noVid; ++needData) {
		if (m_vids[needData] && (m_vids[needData]->m_pixelFlag16 & 0x200)) {
			break;
		}
	}
	if (needData < m_noVid) {
		RESOURCE src;
		if (!src.OpenForRead(m_mapName, 0x2050414d)) {
			out.Append(&src, 0x50414557 /* 'WEAP' */);
			out.Append(&src, 0x204a424f /* 'OBJ ' */);
			src.Close();
		}
		else {
			MYERROR::Window(
				::Error,
				// STRING: ALIEN 0x482860
				"Can't open file '%s', needed for save map",
				m_mapName.m_str
			);
		}
	}
	out.PreAppend(0x48505247 /* 'GRPH' */, 0);
	Graph->SaveParameters(&out);
	out.PostAppend();
	out.PreAppend(0x44414548 /* 'HEAD' */, 0);
	out.Write(&m_w, 4);
	out.Write(&m_h, 4);
	out.Write(&m_shiftX, 4);
	out.Write(&m_shiftY, 4);
	out.Write(&CurrentTime, 4);
	out.Write(&version, 4);
	out.PostAppend();
	int hasGround = 0;
	for (int gy = 0; gy < m_groundH; ++gy) {
		for (int gx = 0; gx < m_groundW; ++gx) {
			if (m_groundz[gx + gy * m_groundW]) {
				hasGround = 1;
				break;
			}
		}
		if (hasGround) {
			break;
		}
	}
	if (hasGround) {
		out.PreAppend(0x44495247 /* 'GRID' */, 0);
		out.Write(m_groundz, 2 * m_groundW * m_groundH);
		out.PostAppend();
	}
	out.PreAppend(0x20525053 /* 'SPR ' */, 0);
	SPRITE* buffer = 0;
	for (int layer = 0; layer < MAP::LayerCount(); ++layer) {
		int i = m_layers[layer].m_n;
		buffer = NextSprite(layer, &i);
		while (buffer) {
			if (!buffer->m_parent) {
				int inMenu = -1;
				for (int k = m_menu.m_n; k;) {
					if ((SPRITE*) m_menu.m_data[--k] == buffer) {
						inMenu = k;
						break;
					}
				}
				if (inMenu < 0) {
					buffer->Write(&out);
				}
			}
			buffer = NextSprite(layer, &i);
		}
	}
	// The sprite identity token is 32 bits in the file; writing the pointer
	// itself only agreed with that by little-endian accident.
	int token = -1;
	out.Write(&token, 4);
	out.PostAppend();
	for (int layer2 = 0; layer2 < MAP::LayerCount(); ++layer2) {
		int i = m_layers[layer2].m_n;
		for (buffer = NextSprite(layer2, &i); buffer; buffer = NextSprite(layer2, &i)) {
			if (buffer->m_parent) {
				continue;
			}
			int inMenu = -1;
			for (int k = m_menu.m_n; k;) {
				if ((SPRITE*) m_menu.m_data[--k] == buffer) {
					inMenu = k;
					break;
				}
			}
			if (inMenu >= 0) {
				continue;
			}
			int mark = ftell(out.m_file);
			out.PreAppend(0x44525053 /* 'SPRD' */, 0);
			token = (int) (decomp_intptr) buffer;
			out.Write(&token, 4);
			buffer->Action(80, (decomp_intptr) &out, 0, 0);
			if (ftell(out.m_file) > mark + 5) {
				out.PostAppend();
			}
		}
	}
	out.PreAppend(0x44525053, 0);
	token = -1;
	out.Write(&token, 4);
	out.PostAppend();
	out.PreAppend(0x59414c50 /* 'PLAY' */, 0);
	for (int p = 0; p < 4; ++p) {
		m_player[p]->Save(&out);
	}
	out.PostAppend();
	out.PreAppend(0x554f5247 /* 'GROU' */, 0);
	m_groups.Save(&out);
	out.PostAppend();
	out.Close();
	if (!strcmp(m_mapName, "tmp_del!.map")) {
		{
			STRING temp("tmp_del!.map");
			Platform_Remove(temp.m_str);
		}
		m_mapName = p_name;
	}
	return 0;
}
