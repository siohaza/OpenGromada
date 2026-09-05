

#include "game/region.h"

#include "game/game_descriptor.h"
#include "game/gametime.h"
#include "game/map.h"
#include "gfx/gpu_backend.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "gfx/texture.h"
#include "video/vid.h"

// FUNCTION: ALIEN 0x449640
REGION::REGION(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent)
	: SPRITE(p_vid, p_x, p_y, p_z, p_dir, p_parent)
{
	m_unk0x98 = EmptyVid;
	m_unk0x8c = 0;
	m_flag = 0;
	m_h = 0;
	m_w = 0;
	m_fogZ1 = 0;
	m_fogZ2 = 0;
	m_fogColor = 0xff000000;
	m_fogTable = 0;
	m_unk0x70 = 0;
	m_unk0x74 = 0;
	VID** p = m_vidFrom;
	int n = 6;
	do {
		p[6] = 0;
		p[0] = 0;
		p += 1;
		--n;
	} while (n);
}

// FUNCTION: ALIEN 0x4496e0
void* REGION::ScalarDeletingDestructor(unsigned int p_flags)
{
	REGION* result = this;
	this->~REGION();
	if (p_flags & 1) {
		operator delete(result);
	}
	return result;
}

// FUNCTION: ALIEN 0x449700
REGION::~REGION()
{
	Map->DeletePointerToSprite(this);
	if (m_fogTable) {
		GPU_RENDER::Forget(m_fogTable);
		operator delete(m_fogTable);
	}
}

// FUNCTION: ALIEN 0x449730
void REGION::DrawSecondaryInfo()
{
	Graph->Box((float) (X1Scr() - 1.0f),
			   (float) (Y1Scr() - 1.0f),
			   (float) (X2Scr() + 1.0f),
			   (float) (Y2Scr() + 1.0f),
			   COLOR(255, 255, 255));
}

// FUNCTION: ALIEN 0x449790
float REGION::X1Scr() const
{

	double v = m_x;
	float w = m_w;
	v -= w * 0.5f;
	return v - Map->m_shiftX;
}

// FUNCTION: ALIEN 0x4497b0
float REGION::Y1Scr() const
{

	float h = m_h;
	return m_y - m_z - h * 0.5f - Map->m_shiftY;
}

// FUNCTION: ALIEN 0x4497d0
float REGION::X2Scr() const
{
	float w = m_w;
	return w * 0.5f + m_x - Map->m_shiftX;
}

// FUNCTION: ALIEN 0x4497f0
float REGION::Y2Scr() const
{

	float h = m_h;
	return m_y - m_z + h * 0.5f - Map->m_shiftY;
}

// FUNCTION: ALIEN 0x449810
void REGION::Draw()
{
	int savedCadr = m_noCadr;
	float savedX = m_x;
	float savedY = m_y;
	VID* vid = m_vid;
	if (vid != EmptyVid) {
		GRAPH_CORE* graph = (GRAPH_CORE*) Graph;
		float savedViewXMin = graph->m_viewXMin;
		float savedViewXMax = graph->m_viewXMax;
		float savedViewYMin = graph->m_viewYMin;
		float savedViewYMax = graph->m_viewYMax;
		if (!(m_flag & 8)) {
			float halfH = m_y - m_z;
			float baseY = m_h;
			baseY *= 0.5f;
			int yMax = (int) (halfH + baseY - Map->m_shiftY);
			int xMax = (int) ((double) m_x + m_w * 0.5f - Map->m_shiftX);
			int yMin = (int) (halfH - baseY - Map->m_shiftY);
			int xMin = (int) (m_x - m_w * 0.5f - Map->m_shiftX);
			graph->SetViewPort((float) xMin, (float) yMin, (float) xMax, (float) yMax);
		}
		int tile = 0;
		float rowY = savedY - m_h * 0.5f;
		while (m_h * 0.5f + savedY > rowY) {
			float colX = savedX - m_w * 0.5f;
			while (m_w * 0.5f + savedX > colX) {
				VID* v = m_vid;
				if (!(v->m_flag & 0x1000000)) {
					m_noCadr += 2 * tile++;
					m_noCadr = m_noCadr % v->m_dotFrameCount;
				}
				m_x = v->m_unk0x2f6 / 2 + colX;
				m_y = v->m_messageLineHeight / 2 + rowY;
				v->Draw(this);
				v = m_vid;
				colX += v->m_unk0x2f6;
			}
			rowY += m_vid->m_messageLineHeight;
		}
		if (!(m_flag & 8)) {
			graph->SetViewPort(savedViewXMin, savedViewYMin, savedViewXMax, savedViewYMax);
		}
	}
	m_x = savedX;
	m_y = savedY;
	m_noCadr = savedCadr;

	int fogZ2 = m_fogZ2;
	if (m_fogZ1 < fogZ2) {
		int flag = m_flag;
		if (flag & 1) {
			if ((CurrentTime & 7) < m_unk0x74) {
				int target = 8 * fogZ2;
				if (m_unk0x70 < target) {
					m_unk0x70 += 2;
				}
				else if (m_unk0x70 > target) {
					m_unk0x70 = 0;
				}
			}
			m_unk0x74 = CurrentTime & 7;
		}
		else {
			m_unk0x70 = 8 * fogZ2;
		}

		if (flag & 8) {
			Graph->DrawFog(((GRAPH_CORE*) Graph)->GetViewXMin(),
						   ((GRAPH_CORE*) Graph)->GetViewYMin(),
						   ((GRAPH_CORE*) Graph)->GetViewXMax(),
						   ((GRAPH_CORE*) Graph)->GetViewYMax(),
						   m_fogZ1,
						   m_fogZ2,
						   m_fogColor,
						   m_fogTable,
						   m_unk0x70,
						   flag & 2);
		}
		else {
			Graph->DrawFog(X1Scr(),
						   Y1Scr(),
						   X2Scr(),
						   Y2Scr(),
						   m_fogZ1,
						   m_fogZ2,
						   m_fogColor,
						   m_fogTable,
						   m_unk0x70,
						   flag & 2);
		}
	}
	else {
		m_unk0x70 = 0;
	}
}

// FUNCTION: ALIEN 0x449be0
VID* REGION::ConvertVid(VID* p_vid, float p_x, float p_y, float p_z)
{
	int iter;

	const int layer = GameDesc->m_layerRules == GAME_LAYERS_LOCOLAND ? 7 : 10;
	for (SPRITE* sprite = Map->FirstSprite(layer, &iter); sprite; sprite = Map->NextSprite(layer, &iter)) {
		REGION* region = (REGION*) sprite;
		if (region->m_vid->m_sprClass == 23 && region->m_z + 25.0f > p_z &&
			((region->m_flag & 8) || region->IsInsideXY(p_x, p_y))) {
			for (int i = 0; i < 6; ++i) {
				if (region->m_vidFrom[i] == p_vid) {
					return region->m_vidFrom[i + 6];
				}
			}
		}
	}
	return p_vid;
}

// STUB: ALIEN 0x449d00
void REGION::SetFogParameters(int p_z1, int p_z2, COLOR p_color)
{
	m_fogZ2 = p_z2;
	m_fogZ1 = p_z1;
	m_fogColor = p_color;
	if (m_fogTable) {
		GPU_RENDER::Forget(m_fogTable);
		operator delete(m_fogTable);
	}
	if (m_fogZ1 >= m_fogZ2) {
		return;
	}

	unsigned short* table = (unsigned short*) operator new(16 * (m_fogZ2 - m_fogZ1) + 2);
	m_fogTable = table;
	if (!table) {
		MYERROR::LogExit(::Error,
						 // STRING: ALIEN 0x4847f4
						 "Enough memory for DrawFog %i",
						 8 * (m_fogZ2 - m_fogZ1) + 1);
	}

	int count = 8 * (m_fogZ2 - m_fogZ1);
	if (count < 0) {
		return;
	}
	int num = 255 * count;
	for (int i = count; count >= 0; --count, num -= 255) {
		int value;
		if (((GRAPH_CORE*) Graph)->m_texE0C->m_format != 41) {
			((unsigned short*) m_fogTable)[count] = ((GRAPH_CORE*) Graph)->m_snowRamp[num / (m_fogZ2 - m_fogZ1) / 8];
		}
		else { // D3DFMT_P8
			((unsigned short*) m_fogTable)[count] = (unsigned short) (num / (m_fogZ2 - m_fogZ1) / 8);
		}
	}
}

// FUNCTION: ALIEN 0x449e20
decomp_intptr REGION::Action(int p_action, decomp_intptr p_a, decomp_intptr p_b, decomp_intptr p_c)
{
	int fogZ2;
	int fogZ1;
	int fogColor;
	switch (p_action) {
	case 81:
	case 200: {
		STREAM* stream = (STREAM*) p_a;
		SPRITE::Action(p_action, p_a, p_b, p_c);
		stream->Read(&m_flag, 4);
		stream->Read(&fogZ2, 4);
		stream->Read(&fogZ1, 4);
		stream->Read(&fogColor, 4);
		SetFogParameters(fogZ1, fogZ2, fogColor);
		if (p_b > 9) {
			stream->Read(&m_w, 4);
			stream->Read(&m_h, 4);
		}
		else {
			stream->Read(&p_c, 4);
			m_w = (float) p_c;
			stream->Read(&p_c, 4);
			m_h = (float) p_c;
		}
		stream->Read(&m_unk0x9c, 4);
		VID* vid = Map->ReadVid(stream);
		m_unk0x98 = vid;
		if (!vid) {
			m_unk0x98 = EmptyVid;
		}
		for (p_c = 0; p_c < 6; ++p_c) {
			m_vidFrom[p_c] = Map->ReadVid(stream);
			m_vidFrom[p_c + 6] = Map->ReadVid(stream);
			if (m_vidFrom[p_c]) {
				m_vidFrom[p_c]->m_unk0x47c |= 0x10;
			}
		}
		return 0;
	}
	case 80: {
		STREAM* stream = (STREAM*) p_a;
		SPRITE::Action(p_action, p_a, p_b, p_c);
		stream->Write(&m_flag, 4);
		stream->Write(&m_fogZ2, 4);
		stream->Write(&m_fogZ1, 4);
		stream->Write(&m_fogColor, 4);
		stream->Write(&m_w, 4);
		stream->Write(&m_h, 4);
		stream->Write(&m_unk0x9c, 4);
		Map->WriteVid(stream, m_unk0x98);
		for (p_c = 0; p_c < 6; ++p_c) {
			Map->WriteVid(stream, m_vidFrom[p_c]);
			Map->WriteVid(stream, m_vidFrom[p_c + 6]);
		}
		return 0;
	}
	}
	return SPRITE::Action(p_action, p_a, p_b, p_c);
}
