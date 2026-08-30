#include "video/vid_light.h"

#include "gfx/color.h"
#include "gfx/gamma.h"
#include "gfx/graph.h"
#include "sprite/sprite.h"
#include "util/resource.h"

#include <bit>

// FUNCTION: ALIEN 0x412800
VID* VID_LIGHT::CreateMirror()
{
	return new VID_LIGHT(*this);
}

// FUNCTION: ALIEN 0x412830
void* VID_LIGHT::ScalarDeletingDestructor(unsigned int p_flags)
{
	VID_LIGHT* result = this;
	this->~VID_LIGHT();
	if (p_flags & 1) {
		operator delete(result);
	}
	return result;
}

// FUNCTION: ALIEN 0x414b70
VID_LIGHT::VID_LIGHT(VID_LIGHT& p_other)
{
	m_weaponPtr = p_other.m_weaponPtr;
	p_other.m_weaponPtr = this;
	m_layer = p_other.m_layer;
	m_pixelFlag16 = p_other.m_pixelFlag16;
	m_defaultAniPeriod = p_other.m_defaultAniPeriod;
	m_dotFrameCount = p_other.m_dotFrameCount;
	m_unk0x2f6 = p_other.m_unk0x2f6;
	m_messageLineHeight = p_other.m_messageLineHeight;
	m_unk0x488 = p_other.m_unk0x488;
	m_unk0x484 = p_other.m_unk0x484;
}

// FUNCTION: ALIEN 0x414c10
VID_LIGHT::~VID_LIGHT()
{
	if (m_weaponPtr == this) {
		if (m_unk0x488) {
			operator delete(m_unk0x488);
		}
		m_unk0x488 = 0;
		VID::MemoryInUse -= m_unk0x484;
		m_unk0x484 = 0;
	}
}

// FUNCTION: ALIEN 0x414c60
void VID_LIGHT::Load(RESOURCE* p_res)
{
	if (p_res->GoNext(0x41544144)) {
		Error(5, "DATA", 0);
	}
	m_unk0x2f6 = (short) m_footprintWidth;
	m_messageLineHeight = (short) m_footprintHeight;
	m_unk0x484 = p_res->SubLoad((void**) &m_unk0x488, 0);
	if (!m_unk0x484) {
		Error(5, "cadr", 0);
	}
	VID::MemoryInUse = VID::MemoryInUse + m_unk0x484;
}

// FUNCTION: ALIEN 0x414d00
void VID_LIGHT::SetLayer()
{
	m_layer = 0xb;
}

inline static unsigned int BlendLightColor(unsigned int color, const GAMMA& sum)
{
	if (sum.m_a || sum.m_b) {
		unsigned int inv = ~sum.m_a;
		int red = ((color >> 16 & 0xff) * (((inv >> 16) & 0xff) + 1) >> 8) + ((sum.m_b >> 16) & 0xff);
		int green = ((color >> 8 & 0xff) * (((inv >> 8) & 0xff) + 1) >> 8) + ((sum.m_b >> 8) & 0xff);
		int blue = ((color & 0xff) * ((inv & 0xff) + 1) >> 8) + (sum.m_b & 0xff);
		color = COLOR(red, green, blue).m_value;
	}
	return color;
}

// STUB: ALIEN 0x414d10
int VID_LIGHT::Draw(SPRITE* p_sprite)
{
	unsigned int color = m_unk0x488[p_sprite->m_noCadr];
	if (!(m_unk0x47c & 0x40) && color && color != 0xff000000) {
		if (m_flag & 0x800000) {
			Graph->SetTextureStageState(D3DTSS_COLOROP, D3DTOP_MODULATE2X);
		}
		GAMMA sum;
		if (m_flag & 0x800) {
			sum.Add(GAMMA(GAMMA::RAW_COPY, m_colorSub, m_colorAdd), GAMMA(p_sprite->GetGamma()));
			color = BlendLightColor(color, sum);
		}
		else {
			GAMMA graphGamma = Graph->m_gammaSet;
			GAMMA gamma;
			gamma.Add(GAMMA(GAMMA::RAW_COPY, m_colorSub, m_colorAdd), GAMMA(p_sprite->GetGamma()));
			sum.Add(gamma, graphGamma);
			color = BlendLightColor(color, sum);
		}
		Graph->DrawLight(
			(float) p_sprite->ScreenX(),
			(float) p_sprite->ScreenY(),
			(float) p_sprite->GetZ(),
			std::bit_cast<int>(m_footprintWidth),
			std::bit_cast<int>(m_footprintHeight),
			color
		);
		if (m_flag & 0x800000) {
			Graph->SetTextureStageState(D3DTSS_COLOROP, D3DTOP_MODULATE);
		}
	}
	return 0;
}
