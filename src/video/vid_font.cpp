#include "video/vid_font.h"

#include "game/map.h"
#include "gfx/debugfont.h"
#include "gfx/gamma.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
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
	if (p_flags & 1) {
		operator delete(result);
	}
	return result;
}

// FUNCTION: ALIEN 0x415070
VID_FONT::VID_FONT(VID_FONT& p_other)
{
	m_weaponPtr = p_other.m_weaponPtr;
	p_other.m_weaponPtr = this;
	m_layer = p_other.m_layer;
	m_pixelFlag16 = p_other.m_pixelFlag16;
	m_defaultAniPeriod = p_other.m_defaultAniPeriod;
	m_dotFrameCount = p_other.m_dotFrameCount;
	m_unk0x2f6 = p_other.m_unk0x2f6;
	m_messageLineHeight = p_other.m_messageLineHeight;
	m_font = p_other.m_font;
}

// FUNCTION: ALIEN 0x415100
VID_FONT::~VID_FONT()
{
	if (m_weaponPtr == this) {
		if (m_font) {
			delete m_font;
		}
		m_font = 0;
	}
}

// FUNCTION: ALIEN 0x415140
void VID_FONT::Load(RESOURCE* p_res)
{
	m_defaultAniPeriod = 71;
	m_dotFrameCount = 256;
	m_unk0x2f6 = (short) m_footprintWidth;
	m_messageLineHeight = (short) m_footprintHeight;
	m_pixelFlag16 = 0x4000;
	m_font = new DEBUG_FONT(GetFileName(), (int) m_footprintHeight, 8);
}

// FUNCTION: ALIEN 0x4151f0
void VID_FONT::SetLayer()
{
	m_layer = 0xe;
}

// FUNCTION: ALIEN 0x415200
int VID_FONT::Draw(SPRITE* p_sprite)
{
	if (!m_font) {
		return 0;
	}
	int color = ~p_sprite->GetGamma().m_a;
	return m_font->DrawDebugText(
		p_sprite->m_x - Map->m_shiftX,
		p_sprite->m_y - p_sprite->m_z - Map->m_shiftY,
		color,
		STRING::EMPTY,
		0
	);
}
