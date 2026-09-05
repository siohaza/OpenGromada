#include "video/vid.h"

#include "game/const.h"
#include "game/game_descriptor.h"
#include "game/map.h"
#include "gfx/gamma.h"
#include "gfx/graph.h"
#include "sprite/sprite.h"
#include "util/string.h"
#include "video/vid_exdata.h"

#include <stdlib.h>
#include <string.h>

int VID::ViewXMin()
{
	return Graph ? (int) Graph->m_viewXMin : 0;
}
int VID::ViewXMax()
{
	return Graph ? (int) Graph->m_viewXMax : 640;
}
int VID::ViewYMin()
{
	return Graph ? (int) Graph->m_viewYMin : 0;
}
int VID::ViewYMax()
{
	return Graph ? (int) Graph->m_viewYMax : 480;
}

// GLOBAL: ALIEN 0x490740
VID* EmptyVid = new VID();

// GLOBAL: ALIEN 0x49074c
int VID::MemoryInUse;

// FUNCTION: ALIEN 0x4127d0
void VID::DrawToVid(const SPRITE*, const VID_TEXCOOR*, TEXTURE*, TEXTURE*)
{
}

// FUNCTION: ALIEN 0x413710
VID::VID() : m_name(STRING::EMPTY), m_colorSub(0), m_colorAdd(0), m_fname(STRING::EMPTY)
{
	m_gammaR = 1.0f;
	m_gammaG = 1.0f;
	m_gammaB = 1.0f;
	m_unk0x47c = 0;
	m_sprClass = 6;
	m_unk0x0c = 0;
	m_flag = 0;
	m_footprintWidth = 24.0f;
	m_footprintHeight = 16.0f;
	m_unk0x24 = 20.0f;
	m_unk0x384 = 12.0f;
	m_unk0x388 = 8.0f;
	m_weaponPtr = this;
	m_pixelFlag16 = 0;
	m_mirror = this;
	m_layer = 15;
	m_idx = -1;
	m_defaultMaxHp = 0;
	m_noDir = 1;
	m_unk0x390 = 0;
	m_defaultAniPeriod = 71;
	m_nLinkVid = 0;
	m_linkVid = 0;
	m_weapon = 0;
	m_exData = 0;
	m_nLinkDots = 0;
	m_dotCoords = 0;
	m_dotFrameStarts = 0;
	m_unk0x478 = 0;
	m_canMove = 0;
	m_prop = 0;
	m_randomSpeed = 0.0f;
	m_randomZSpeed = 0.0f;
	for (int ani = 0; ani < 17; ++ani) {
		m_aniSfx[ani] = 0;
		m_unk0x20c[ani] = 0;
		m_aniChildVid[ani] = 0;
		m_noAnimCadr[ani] = 0;
		m_aniBegCadr[ani] = 0;
		m_aniDirCadrs[ani] = 0;
		m_aniDuration[ani] = 71;
	}
	ResetSprites();
}

// FUNCTION: ALIEN 0x413860
VID* VID::CreateMirror()
{
	return new VID();
}

void VID::DrawVidToVid(const SPRITE*)
{
}

int VID::Draw(SPRITE*)
{
	return 0;
}

// FUNCTION: ALIEN 0x413880
void VID::SetLayer()
{
	m_layer = 0;
}

// FUNCTION: ALIEN 0x413890
void* VID::ScalarDeletingDestructor(unsigned int p_flags)
{
	VID* result = this;
	this->~VID();
	if (p_flags & 1) {
		operator delete(result);
	}
	return result;
}

// FUNCTION: ALIEN 0x4138b0
VID::~VID()
{
	int no_sprites = m_entitiesNumber[3] + m_entitiesNumber[2] + m_entitiesNumber[1] + m_entitiesNumber[0];
	if (m_entitiesNumber[3] + m_entitiesNumber[2] + m_entitiesNumber[1] + m_entitiesNumber[0]) {
		// STRING: ALIEN 0x482ae4
		Error(10, "Not all sprites with this VID deleted", no_sprites);
	}

	VID* weapon = m_weaponPtr;
	if (weapon != this) {
		VID* last = m_weaponPtr;
		while (last->m_weaponPtr != this) {
			last = last->m_weaponPtr;
		}
		last->m_weaponPtr = weapon;
	}

	if (m_unk0x46c) {
		operator delete(m_unk0x46c);
	}
	m_unk0x46c = 0;
	if (m_unk0x470) {
		operator delete(m_unk0x470);
	}
	memset(&m_unk0x470, 0, sizeof(m_unk0x470));
	if (m_fname != STRING::EMPTY) {
		operator delete(m_fname);
	}
	char* name = m_name;
	if (name != STRING::EMPTY) {
		operator delete(name);
	}
}

static void AssignVidString(char*& p_destination, const char* p_source)
{
	STRING replacement(p_source);
	if (p_destination != STRING::EMPTY) {
		operator delete(p_destination);
	}
	p_destination = replacement.m_str;
	replacement.m_str = STRING::EMPTY;
}

void VID::SetName(const char* p_name)
{
	AssignVidString(m_name, p_name);
}

void VID::SetFileName(const char* p_name)
{
	AssignVidString(m_fname, p_name);
}

// FUNCTION: ALIEN 0x413980
unsigned int VID::ResetSprites()
{
	for (int i = 0; i < 20; ++i) {
		m_unk0x408[i] = -1;
	}
	int hp = m_defaultMaxHp;
	m_unk0x394[4] = -1;
	m_unk0x394[3] = -1;
	m_unk0x394[2] = -1;
	m_unk0x394[1] = -1;
	m_unk0x394[0] = -1;
	unsigned int r = m_unk0x47c & 0xffffffef;
	m_recolors[3] = 0;
	m_recolors[2] = 0;
	m_recolors[1] = 0;
	m_recolors[0] = 0;
	m_deaths[3] = 0;
	m_deaths[2] = 0;
	m_deaths[1] = 0;
	m_deaths[0] = 0;
	m_entitiesNumber[3] = 0;
	m_entitiesNumber[2] = 0;
	m_entitiesNumber[1] = 0;
	m_entitiesNumber[0] = 0;
	m_maxHp[3] = hp;
	m_maxHp[2] = hp;
	m_maxHp[1] = hp;
	m_maxHp[0] = hp;
	m_unk0x458 = 0;
	m_unk0x47c = r;
	return r;
}

int VID::DefaultLifetime() const
{
	return GameDesc->m_lifetimeInWeapon ? (m_exData ? m_exData->m_legacyLifeTime : 0) : m_unk0x6c;
}

// FUNCTION: ALIEN 0x413a60
void VID::SetChildAndLink()
{
	int nLink = m_nLinkVid;
	if (nLink) {
		if (Map->VidExists(m_nLinkVid)) {
			m_linkVid = Map->GetVid(m_nLinkVid)->m_mirror;
		}
		else {
			Error(
				4,
				// STRING: ALIEN 0x482b20
				"LinkVid",
				nLink
			);
		}
	}
	if (GameDesc->m_weaponHasKeyframes) {
		int* p = m_exData->m_unk0x84;
		int n = 8;
		do {
			if (p[-8] || p[0] || p[8] || p[16]) {
				m_unk0x47c |= 1;
			}
			if (p[24] != 0x3f800000 || p[32] != 0x3f800000 || p[40] != 0x3f800000) {
				m_unk0x47c |= 2;
			}
			if (((float*) p)[48] != 0.0f || ((float*) p)[56] != 0.0f || ((float*) p)[64] != 0.0f) {
				m_unk0x47c |= 4;
			}
			if (p[72] || p[80] || p[88]) {
				m_unk0x47c |= 8;
			}
			++p;
			--n;
		} while (n);
	}
	else {


		m_unk0x47c &= ~0xFu;
		m_unk0x478 = 0;
	}
	if (DefaultLifetime() != 999999 || (m_flag & 0x200000) != 0) {
		m_unk0x478 = 1;
	}
	else if (GameDesc->m_weaponHasKeyframes && ((m_unk0x47c & 0xF) != 0 || (m_flag & 0x400) != 0)) {
		m_unk0x478 = 1;
	}
	// Resolve animation child VIDs without crossing array bounds.
	for (int i = 0; i < 17; ++i) {
		int c = m_unk0x20c[i];
		if (c) {
			if (Map->VidExists(abs(c))) {
				VID* m = Map->GetVid(abs(c))->m_mirror;
				m_aniChildVid[i] = m;
				if (m->m_flag & 0x80) {
					m_unk0x478 = 1;
				}
			}
			else {
				Error(
					4,
					// STRING: ALIEN 0x482b18
					"child",
					c
				);
			}
		}
	}
}

// FUNCTION: ALIEN 0x414570
int VID::SetGamma(const GAMMA& p_gamma, unsigned int p_idx)
{
	int result = p_idx;
	if (p_idx < 4) {
		m_gamma[p_idx].m_a = p_gamma.m_a;
		m_gamma[p_idx].m_b = p_gamma.m_b;
	}
	else if (p_idx != 4) {
		// STRING: ALIEN 0x482b94
		result = Error(4, "n_gamma in VID::SetGamma", p_idx);
	}
	return result;
}

// FUNCTION: ALIEN 0x414760
int VID::GetFireDamage()
{
	int fd;
	if (m_unk0x5c) {
		fd = m_unk0x5c->GetFireDamage();
	}
	else {
		fd = 0;
	}
	int v5;
	if (m_aniChildVid[15]) {
		v5 = m_aniFireCount[15] * (m_aniChildVid[15])->GetFireDamage();
	}
	else {
		v5 = 0;
	}
	int v7;
	if (m_aniChildVid[14]) {
		v7 = m_aniFireCount[14] * (m_aniChildVid[14])->GetFireDamage();
	}
	else {
		v7 = 0;
	}
	int wterm;
	if (m_weaponVid) {
		wterm = m_aniFireCount[8] * m_weaponVid->GetFireDamage();
	}
	else {
		wterm = 0;
	}
	return m_fireDamage + wterm + v7 + v5 + fd;
}

// FUNCTION: ALIEN 0x4147f0
int VID::GetBuildTime()
{
	VID* vid = m_unk0x5c;
	if (vid && vid->m_unk0x40) {
		return vid->m_exData->m_buildTime / 1000;
	}
	return m_exData->m_buildTime / 1000;
}

// FUNCTION: ALIEN 0x414840
VID* VID::SetPropHide(int p_hide)
{
	if (GameDesc->m_layerRules == GAME_LAYERS_LOCOLAND) {


		VID* vid = this;
		do {
			vid->m_flag = (vid->m_flag & ~0x400u) | (p_hide ? 0x400u : 0u);
			vid = vid->m_unk0x5c;
		} while (vid);
		return vid;
	}
	int bit = ((p_hide != 0) & 1) << 6;
	m_unk0x47c = (m_unk0x47c & 0xffffffbf) | bit;
	VID* p = m_unk0x5c;
	while (p) {
		p->m_unk0x47c = (p->m_unk0x47c & 0xffffffbf) | bit;
		p = p->m_unk0x5c;
	}
	return p;
}

// FUNCTION: ALIEN 0x414890
void VID::SetHpCoeff(int p_army, int p_coeff)
{
	p_army &= 3;
	int oldMax = m_maxHp[p_army];
	if (p_coeff >= 0) {
		m_maxHp[p_army] = p_coeff * m_defaultMaxHp / 100;
	}
	if (m_defaultMaxHp) {
		int iter;
		SPRITE* sprite = Map->FirstSprite(m_layer, &iter);
		while (sprite) {
			if (sprite->m_vid == this && ((sprite->m_flag >> 11) & 3) == p_army) {
				sprite->ChangeHp(16 * m_maxHp[p_army] * sprite->m_unk0x54 / oldMax / 16);
			}
			sprite = Map->NextSprite(m_layer, &iter);
		}
	}
	if (m_unk0x5c) {
		m_unk0x5c->SetHpCoeff(p_army, p_coeff);
	}
}

// FUNCTION: ALIEN 0x414990
VID* VID::SetMaxHp(int p_army, int p_maxHp)
{
	p_army &= 3;
	int oldMax = m_maxHp[p_army];
	if (p_maxHp >= 0) {
		m_maxHp[p_army] = p_maxHp;
	}
	if (m_defaultMaxHp) {
		int iter;
		SPRITE* sprite = Map->FirstSprite(m_layer, &iter);
		while (sprite) {
			if (sprite->m_vid == this && ((sprite->m_flag >> 11) & 3) == p_army) {
				sprite->ChangeHp(((sprite->m_unk0x54 * m_maxHp[p_army]) << 8) / oldMax / 256);
			}
			sprite = Map->NextSprite(m_layer, &iter);
		}
	}
	if (m_unk0x5c) {
		return m_unk0x5c->SetMaxHp(p_army, p_maxHp);
	}
	return this;
}

// FUNCTION: ALIEN 0x414a80
float VID::CalculateZSpeed(float p_dz, float p_dist)
{
	float speed;
	if (m_flag & 2) {
		speed = p_dist * Const->m_unk0x08 / m_unk0x2c * 0.5f + p_dz * m_unk0x2c / p_dist;
		speed *= speed > 0.0f ? 1.1f : 0.9f;
	}
	else if (m_flag & 4) {
		speed = p_dist * Const->m_unk0x0c / m_unk0x2c * 0.5f + p_dz * m_unk0x2c / p_dist;
		speed *= speed > 0.0f ? 1.1f : 0.9f;
	}
	else if (m_flag & 0x8000000) {
		speed = m_unk0x30;
	}
	else if (m_unk0x60 == 0.0f) {
		speed = p_dz * m_unk0x2c / p_dist;
	}
	else {
		speed = 0.0f;
	}
	if (speed > m_unk0x30) {
		return m_unk0x30;
	}
	if (speed < -m_unk0x30) {
		return -m_unk0x30;
	}
	return speed;
}

// FUNCTION: ALIEN 0x43a1b0
unsigned int VID::PropHide()
{
	if (GameDesc->m_layerRules == GAME_LAYERS_LOCOLAND) {
		return (m_flag >> 10) & 1;
	}
	return (m_unk0x47c >> 6) & 1;
}

// FUNCTION: ALIEN 0x43a1c0
unsigned int VID::RealDirection(ANGLE p_dir)
{
	return (((m_unk0x390 + p_dir.m_dir) & 0xff) * m_noDir) >> 8;
}

// FUNCTION: ALIEN 0x43a1e0
int VID::HaveWeapon()
{
	return m_weaponVid && m_unk0x40;
}

// FUNCTION: ALIEN 0x43a200
int VID::GetMaxAmmo()
{
	VID* linkVid = m_unk0x5c;
	if (linkVid && linkVid->m_weaponVid && linkVid->m_unk0x40) {
		return linkVid->m_exData->m_maxAmmo;
	}
	return m_exData->m_maxAmmo;
}

// FUNCTION: ALIEN 0x43a230
int VID::GetMaxHp(int army)
{
	return m_maxHp[army & 3];
}

// FUNCTION: ALIEN 0x43a250
unsigned int VID::GetEntitiesNumber(int a2)
{
	return m_entitiesNumber[a2];
}

// FUNCTION: ALIEN 0x43a280
int VID::GetDeathsNumber(int army)
{
	return m_deaths[army & 3];
}

// FUNCTION: ALIEN 0x43a2a0
unsigned int VID::GetDeathsNumberTotal()
{
	return m_deaths[3] + m_deaths[2] + m_deaths[1] + m_deaths[0];
}

// FUNCTION: ALIEN 0x43a2c0
int VID::GetRecolors(int army)
{
	return m_recolors[army & 3];
}

// FUNCTION: ALIEN 0x43a2e0
unsigned int VID::GetRecolorsTotal()
{
	return m_recolors[3] + m_recolors[2] + m_recolors[1] + m_recolors[0];
}

// FUNCTION: ALIEN 0x43a300
unsigned int VID::GetNotCreateAsChild()
{
	return m_prop;
}

// FUNCTION: ALIEN 0x43a310
unsigned int VID::SetNotCreateAsChild(unsigned int p_value)
{
	m_prop = p_value;
	return p_value;
}

// FUNCTION: ALIEN 0x43a320
unsigned int VID::IsLight()
{
	return m_flag & 0x80;
}

// FUNCTION: ALIEN 0x442e00
ANGLE VID::SteppedDirection(ANGLE p_dir) const
{
	return m_noDir ? ANGLE((int) (((VID*) this)->RealDirection(p_dir) << 8) / (int) m_noDir) : p_dir;
}
