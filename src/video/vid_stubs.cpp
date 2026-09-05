#include "audio/sound.h"
#include "game/game_descriptor.h"
#include "game/map.h"
#include "sprite/sprite.h"
#include "ui/mouse.h"
#include "util/myerror.h"
#include "util/resource.h"
#include "video/vid.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <climits>
#include <cmath>

// STUB: ALIEN 0x413a30
int VID::Error(int p_type, const char* p_msg, int p_size)
{
	int idx = m_idx;
	return MYERROR::Error(
		::Error,
		// STRING: ALIEN 0x482b0c
		"VID [%i-%s]",
		p_type,
		p_msg,
		p_size,
		idx,
		m_name
	);
}

// FUNCTION: ALIEN 0x413c30
void VID::LoadParameters(RESOURCE* p_res)
{



	const bool extended = GameDesc->m_objSchema == GAME_OBJ_ZS1;
	const bool loco = GameDesc->m_objSchema == GAME_OBJ_LOCOLAND;
	const int parameterBytes = extended ? 696 : loco ? 620 : 688;
	if (p_res->Remaining() < parameterBytes) {
		p_res->Fail("unsupported or truncated OBJ parameter schema");
		return;
	}
	m_randomSpeed = m_randomZSpeed = 0.0f;
	p_res->ReadWords(&m_unk0x0c, 4);
	p_res->ReadWords(&m_sprClass, 4);
	p_res->ReadWords(&m_flag, 4);
	p_res->ReadWords(&m_unk0x18, 4);
	p_res->ReadWords(&m_footprintWidth, 4);
	p_res->ReadWords(&m_footprintHeight, 4);
	p_res->ReadWords(&m_unk0x24, 4);
	p_res->ReadWords(&m_defaultMaxHp, 4);
	p_res->ReadWords(&m_unk0x2c, 4);
	if (extended) {
		p_res->ReadWords(&m_randomSpeed, 4);
	}
	p_res->ReadWords(&m_unk0x30, 4);
	if (extended) {
		p_res->ReadWords(&m_randomZSpeed, 4);
	}
	p_res->ReadWords(&m_unk0x34, 4);
	p_res->ReadWords(&m_unk0x38, 4);
	p_res->ReadWords(&m_unk0x3c, 4);
	p_res->ReadWords(&m_weaponIdx, 4);
	p_res->ReadWords(&m_blastRadius, 4);
	p_res->ReadWords(&m_fireDamage, 4);
	p_res->ReadWords(&m_unk0x4c, 4);
	p_res->ReadWords(&m_unk0x50, 4);
	p_res->ReadWords(&m_unk0x54, 4);
	p_res->ReadWords(&m_nLinkVid, 4);
	p_res->ReadWords(&m_unk0x60, 4);
	if (loco) {



		m_unk0x64 = m_unk0x68 = 0.0f;
		m_unk0x6c = 0;
		m_ext1Property = m_ext2Property = 0;
		p_res->Skip(28);
	}
	else {
		p_res->ReadWords(&m_unk0x64, 4);
		p_res->ReadWords(&m_unk0x68, 4);
		p_res->ReadWords(&m_unk0x6c, 4);
		p_res->ReadWords(&m_ext1Property, 4);
		p_res->ReadWords(&m_ext2Property, 4);
		if (!extended) m_ext1Property = m_ext2Property = 0;
		p_res->Skip(8);
	}
	p_res->ReadWords(&m_noDir, 4);
	p_res->ReadWords(m_noAnimCadr, 68);
	p_res->ReadWords(m_aniSfx, 68);
	if (loco) {


		for (auto& duration : m_aniDuration) duration = m_defaultAniPeriod;
	}
	else p_res->ReadWords(m_aniDuration, 68);
	p_res->ReadWords(m_unk0x140, 68);
	p_res->ReadWords(m_unk0x184, 68);
	p_res->ReadWords(m_unk0x1c8, 68);
	p_res->ReadWords(m_unk0x20c, 68);
	p_res->ReadWords(m_aniFireCount, 68);

	int start;
	int a;
	int r;
	int g;
	int b;
	p_res->ReadWords(&r, 4);
	p_res->ReadWords(&g, 4);
	p_res->ReadWords(&b, 4);
	p_res->ReadWords(&a, 4);
	start = a;
	if (start < -255) {
		start = -255;
	}
	else if (start > 255) {
		start = 255;
	}
	unsigned int neg = 0;
	unsigned int pos = 0;
	if (start < 0) {
		neg = -start << 24;
	}
	else {
		pos = start << 24;
	}
	start = r;
	if (start < -255) {
		start = -255;
	}
	else if (start > 255) {
		start = 255;
	}
	neg &= 0xff00ffff;
	pos &= 0xff00ffff;
	if (start < 0) {
		neg |= -start << 16;
	}
	else {
		pos |= start << 16;
	}
	start = g;
	if (start < -255) {
		start = -255;
	}
	else if (start > 255) {
		start = 255;
	}
	neg &= 0xffff00ff;
	pos &= 0xffff00ff;
	if (start < 0) {
		neg |= -start << 8;
	}
	else {
		pos |= start << 8;
	}
	start = b;
	if (start < -255) {
		start = -255;
	}
	else if (start > 255) {
		start = 255;
	}
	neg &= 0xffffff00;
	pos &= 0xffffff00;
	if (start < 0) {
		neg |= -start;
	}
	else {
		pos |= start;
	}
	m_colorAdd = pos;
	m_colorSub = neg;
	p_res->ReadWords(&m_gammaR, 4);
	p_res->ReadWords(&m_gammaG, 4);
	p_res->ReadWords(&m_gammaB, 4);
	if (!p_res->Good()) {
		return;
	}
	const float geometry[] = {m_footprintWidth, m_footprintHeight, m_unk0x24, m_unk0x2c, m_unk0x30,
		m_randomSpeed, m_randomZSpeed, m_unk0x34, m_unk0x38, m_unk0x3c,
		m_blastRadius, m_unk0x4c, m_unk0x50, m_unk0x54, m_unk0x60, m_unk0x64, m_unk0x68,
		m_gammaR, m_gammaG, m_gammaB};
	for (float value : geometry) {
		if (!std::isfinite(value)) {
			p_res->Fail("non-finite OBJ geometry or motion parameter");
			return;
		}
	}
	short flag2 = m_pixelFlag16;
	if ((flag2 & 4) && (flag2 & 0x20)) {
		m_gammaB = 1.0f;
		m_gammaG = 1.0f;
		m_gammaR = 1.0f;
	}
	if (m_unk0x3c == 999999.0f) {
		m_unk0x3c = 0.0f;
	}
	else if (m_unk0x3c == 0.0f) {
		m_unk0x3c = 999999.0f;
	}
	else {
		m_unk0x3c = 256.0f / m_unk0x3c;
	}
	if (m_unk0x2c != 999999.0f) {
		m_unk0x2c = m_unk0x2c * 0.001f;
	}
	if (m_unk0x30 != 999999.0f) {
		m_unk0x30 = m_unk0x30 * 0.001f;
	}
	if (m_randomSpeed != 999999.0f) {
		m_randomSpeed = m_randomSpeed * 0.001f;
	}
	if (m_randomZSpeed != 999999.0f) {
		m_randomZSpeed = m_randomZSpeed * 0.001f;
	}
	if (m_unk0x34 != 999999.0f) {
		m_unk0x34 = m_unk0x34 * 0.000001f;
	}
	if (m_unk0x38 != 999999.0f) {
		m_unk0x38 = m_unk0x38 * 0.000001f;
	}
	m_canMove = m_unk0x2c != 0.0f || m_unk0x30 != 0.0f || (m_flag & 6) != 0 || (m_flag & 0x1000) != 0;
	if (extended && (m_randomSpeed != 0.0f || m_randomZSpeed != 0.0f)) {
		m_canMove = 1;
	}
	if (!m_noDir || m_noDir > 32000) {
		p_res->Fail("OBJ direction count is outside the legacy range");
		return;
	}
	for (int count : m_noAnimCadr) {
		if (count < 0 || count > INT_MAX / (17 * (int) m_noDir)) {
			p_res->Fail("OBJ animation count overflows legacy frame indexing");
			return;
		}
	}
	int i;
	for (i = 0; i < 17; ++i) {
		if (!m_aniDuration[i]) {
			m_aniDuration[i] = m_defaultAniPeriod;
		}
	}
	m_unk0x384 = m_footprintWidth * 0.5f;
	m_unk0x388 = m_footprintHeight * 0.5f;
	m_unk0x390 = 128 / (int) m_noDir;
	short noCadr = m_dotFrameCount;
	if (!noCadr) {
		Error(
			4,
			// STRING: ALIEN 0x482b7c
			"noCadr==0",
			0
		);
	}
	else if (noCadr < (int) m_noDir) {
		Error(
			4,
			// STRING: ALIEN 0x482b6c
			"noCadr < noDir",
			0
		);
		m_noDir = m_dotFrameCount;
	}
	int total = 0;
	for (i = 0; i < 17; ++i) {
		total += m_noAnimCadr[i] * (int) m_noDir;
	}
	if (total > m_dotFrameCount) {
		Error(
			13,
			// STRING: ALIEN 0x482b4c
			"noCadr for noAnimCadr and noDir",
			0
		);
		int k = 16;
		while (k >= 0) {
			if (total - m_noAnimCadr[k] * (int) m_noDir <= m_dotFrameCount) {
				m_noAnimCadr[k] -= (total - m_dotFrameCount) / (int) m_noDir;
				break;
			}
			total -= m_noAnimCadr[k] * (int) m_noDir;
			m_noAnimCadr[k] = 0;
			--k;
		}
	}
	start = 0;
	int firstAni = -1;
	for (i = 0; i < 17; ++i) {
		if (!Sound->ValidateSFX(m_aniSfx[i]) && m_idx != -1) {
			Error(
				4,
				// STRING: ALIEN 0x482b48
				"sfx",
				m_aniSfx[i]
			);
			m_aniSfx[i] = 0;
		}
		if (m_noAnimCadr[i]) {
			m_aniBegCadr[i] = start;
			m_aniDirCadrs[i] = m_noAnimCadr[i];
			if (firstAni < 0) {
				firstAni = i;
				if (i > 0) {
					for (int m = 0; m < i; ++m) {
						if (!m_aniDirCadrs[m]) {
							m_aniDirCadrs[m] = m_aniDirCadrs[i];
						}
					}
				}
			}
		}
		else if (m_sprClass == 10 && (i & 1) && i <= 7 && m_noAnimCadr[firstAni + 1]) {
			m_aniBegCadr[i] = m_aniBegCadr[firstAni + 1];
			m_aniDirCadrs[i] = m_aniDirCadrs[firstAni + 1];
		}
		else {
			m_aniBegCadr[i] = 0;
			m_aniDirCadrs[i] = firstAni >= 0 ? m_aniDirCadrs[firstAni] : 0;
		}
		start += m_noDir * m_noAnimCadr[i];
		if (start > m_dotFrameCount) {
			Error(
				10,
				// STRING: ALIEN 0x482b28
				"noCadr and noAnimCadr and noDir",
				i
			);
			m_aniBegCadr[i] = 0;
			m_aniDirCadrs[i] = firstAni >= 0 ? m_aniDirCadrs[firstAni] : 0;
		}
	}
	if (m_sprClass == 8) {
		m_unk0x47c |= 0x20u;
	}
	if (m_sprClass == 8 && (Map->m_flag & 1)) {
		m_sprClass = 0;
	}
	if (m_flag & 8) {
		if (m_footprintWidth < 0 || m_footprintHeight < 0 ||
			m_footprintWidth >= (float) (INT_MAX - 17) || m_footprintHeight >= (float) (INT_MAX - 17)) {
			p_res->Fail("OBJ footprint exceeds grid coordinate range");
			return;
		}
		const size_t gridWidth = ((int) m_footprintWidth + 17) / 8;
		const size_t gridHeight = ((int) m_footprintHeight + 17) / 8;
		if (gridHeight && gridWidth > ((size_t) INT_MAX / (3 * sizeof(float)) - 1) / gridHeight) {
			p_res->Fail("OBJ footprint grid size overflows");
			return;
		}
		float* dots =
			new float[3 * (gridWidth * gridHeight + 1)];
		m_nLinkDots = 0;
		int yi = 0;
		if (0.0f < m_footprintHeight) {
			float y = 0.0f;
			do {
				int xi = 0;
				if (0.0f < m_footprintWidth) {
					float x = 0.0f;
					do {
						dots[3 * m_nLinkDots] = x - m_footprintWidth * 0.5f;
						dots[3 * m_nLinkDots + 1] = y - m_footprintHeight * 0.5f;
						dots[3 * m_nLinkDots + 2] = m_unk0x24;
						++m_nLinkDots;
						xi += 8;
						x = (float) xi;
					} while (x < m_footprintWidth);
				}
				dots[3 * m_nLinkDots] = m_footprintWidth * 0.5f;
				dots[3 * m_nLinkDots + 1] = y - m_footprintHeight * 0.5f;
				dots[3 * m_nLinkDots + 2] = m_unk0x24;
				++m_nLinkDots;
				yi += 8;
				y = (float) yi;
			} while (y < m_footprintHeight);
		}
		int xi = 0;
		if (0.0f < m_footprintWidth) {
			float x = 0.0f;
			do {
				dots[3 * m_nLinkDots] = x - m_footprintWidth * 0.5f;
				dots[3 * m_nLinkDots + 1] = m_footprintHeight * 0.5f;
				dots[3 * m_nLinkDots + 2] = m_unk0x24;
				++m_nLinkDots;
				xi += 8;
				x = (float) xi;
			} while (x < m_footprintWidth);
		}
		dots[3 * m_nLinkDots] = m_footprintWidth * 0.5f;
		dots[3 * m_nLinkDots + 1] = m_footprintHeight * 0.5f;
		dots[3 * m_nLinkDots + 2] = m_unk0x24;
		float* old = m_dotCoords;
		++m_nLinkDots;
		if (old) {
			operator delete(old);
		}
		m_dotCoords = static_cast<float*>(operator new(sizeof(float) * 3 * (size_t) m_nLinkDots));
		int k = 0;
		if (m_nLinkDots > 0) {
			int j = 0;
			do {
				memcpy(m_dotCoords + j, dots + j, 3 * sizeof(float));
				++k;
				j += 3;
			} while (k < m_nLinkDots);
		}
		delete[] dots;
	}
}

// FUNCTION: ALIEN 0x4145b0
void VID::SetGridZ(SPRITE* p_sprite)
{
	if (p_sprite == Mouse) {
		return;
	}
	int begin;
	int end;
	if (m_dotFrameStarts) {
		begin = p_sprite->m_noCadr < m_dotFrameCount ? m_dotFrameStarts[p_sprite->m_noCadr] : 0;
		end = p_sprite->m_noCadr < m_dotFrameCount - 1 ? m_dotFrameStarts[p_sprite->m_noCadr + 1] : m_nLinkDots;
	}
	else {
		begin = 0;
		end = m_nLinkDots;
	}
	if (!(Map->m_flag & 1) && p_sprite->m_vid->m_sprClass == 8) {
		for (int i = begin; i < end; ++i) {
			float* dot = m_dotCoords + 3 * i;
			Map->SetGroundZ(
				p_sprite->X() + m_dotCoords[3 * i],
				p_sprite->Y() + m_dotCoords[3 * i + 1],
				p_sprite->Z() + m_dotCoords[3 * i + 2]
			);
		}
	}
	else {
		for (int i = begin; i < end; ++i) {
			float* dot = m_dotCoords + 3 * i;
			Map->SetTempGroundZ(
				p_sprite->X() + m_dotCoords[3 * i],
				p_sprite->Y() + m_dotCoords[3 * i + 1],
				p_sprite->Z() + m_dotCoords[3 * i + 2]
			);
		}
	}
}

// FUNCTION: ALIEN 0x4146b0
void VID::ResetGridZ(SPRITE* p_sprite)
{
	if (p_sprite == Mouse || (!(Map->m_flag & 1) && p_sprite->m_vid->m_sprClass == 8)) {
		return;
	}
	int begin;
	int end;
	if (m_dotFrameStarts) {
		begin = p_sprite->m_noCadr < m_dotFrameCount ? m_dotFrameStarts[p_sprite->m_noCadr] : 0;
		end = p_sprite->m_noCadr < m_dotFrameCount - 1 ? m_dotFrameStarts[p_sprite->m_noCadr + 1] : m_nLinkDots;
	}
	else {
		begin = 0;
		end = m_nLinkDots;
	}
	for (int i = begin; i < end; ++i) {
		float* dot = m_dotCoords + 3 * i;
		Map->ClearTempGroundZ(
			p_sprite->X() + m_dotCoords[3 * i],
			p_sprite->Y() + m_dotCoords[3 * i + 1],
			p_sprite->Z() + m_dotCoords[3 * i + 2]
		);
	}
}
