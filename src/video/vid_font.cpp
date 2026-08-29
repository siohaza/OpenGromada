#include "video/vid_font.h"

#include "gfx/d3dfont.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "game/map.h"
#include "gfx/gamma.h"
#include "sprite/sprite.h"
#include "util/string.h"

// FUNCTION: ALIEN 0x412850
VID* VID_FONT::CreateMirror()
{
	return new VID_FONT(*this);
}

// FUNCTION: ALIEN 0x412880
void* VID_FONT::ScalarDeletingDestructor(unsigned int p_flags)
{
	VID_FONT* result = this;
	this->~VID_FONT();
	if (p_flags & 1)
		operator delete(result);
	return result;
}

// FUNCTION: ALIEN 0x415070
VID_FONT::VID_FONT(VID_FONT& p_other)
{
	m_weaponPtr = p_other.m_weaponPtr;
	p_other.m_weaponPtr = this;
	m_layer = p_other.m_layer;
	*(unsigned short*) &m_pixelFlag = *(unsigned short*) &p_other.m_pixelFlag;
	*(unsigned short*) &m_unk0x2f2[2] = *(unsigned short*) &p_other.m_unk0x2f2[2];
	*(unsigned short*) &m_unk0x2f2[0] = *(unsigned short*) &p_other.m_unk0x2f2[0];
	*(unsigned short*) &m_unk0x2f2[4] = *(unsigned short*) &p_other.m_unk0x2f2[4];
	*(unsigned short*) &m_unk0x2f2[6] = *(unsigned short*) &p_other.m_unk0x2f2[6];
	m_font = p_other.m_font;
}

// FUNCTION: ALIEN 0x415100
VID_FONT::~VID_FONT()
{
	if (m_weaponPtr == this) {
		if (m_font)
			delete m_font;
		m_font = 0;
	}
}

// FUNCTION: ALIEN 0x415140
void VID_FONT::Load(RESOURCE* p_res)
{
	*(short*) &m_unk0x2f2[0] = 71;
	m_dotFrameCount = 256;
	*(short*) &m_unk0x2f2[4] = (int) m_footprintWidth;
	m_messageLineHeight = (int) m_footprintHeight;
	*(short*) &m_pixelFlag = 0x4000;
	(m_font = new CD3DFont(GetFileName(), (int) m_footprintWidth, (int) m_footprintHeight, 8))
		->InitDeviceObjects(((GRAPH_CORE*) Graph)->m_device);
	m_font->RestoreDeviceObjects();
}

// FUNCTION: ALIEN 0x4151f0
void VID_FONT::SetLayer()
{
	m_layer = 0xe;
}

// FUNCTION: ALIEN 0x415200
int VID_FONT::Draw(SPRITE* p_sprite)
{
	int result = (int) m_font;
	if (m_font) {
		int color = ~p_sprite->GetGamma().m_a;
		return m_font->DrawText(p_sprite->m_x - Map->m_shiftX,
								p_sprite->m_y - p_sprite->m_z - Map->m_shiftY,
								color, STRING::EMPTY, 0);
	}
	return result;
}

// FUNCTION: ALIEN 0x415260
void VID_FONT::RestoreFont()
{
	if (m_font)
		m_font->RestoreDeviceObjects();
}

// FUNCTION: ALIEN 0x415270
void VID_FONT::ReleaseFont()
{
	CD3DFont* font = *(CD3DFont**) ((char*) this + 0x484);
	if (font)
		font->InvalidateDeviceObjects();
}
