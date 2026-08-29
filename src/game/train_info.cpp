#include "game/train_info.h"

#include "game/engine.h"
#include "game/map.h"
#include "sprite/ex_sprite_data.h"
#include "video/vid.h"
#include "video/vid_exdata.h"
#include "world/hash_map.h"

#define TRAIN_INFO_UNSET 0x497423f0

// FUNCTION: ALIEN 0x43a8e0
int TRAIN_INFO::Acceleration() const
{
	if (m_accelTime != 0.0f)
		return (int) (m_speedInc / m_accelTime * 8.0f);
	return 0;
}

// GLOBAL: ALIEN 0x4b2ca8
int cur_army;

// GLOBAL: ALIEN 0x4b2cac
int cur_train;

int* GetTrainEng(int p_army, int p_train, int* p_out);

// FUNCTION: ALIEN 0x43aa10
int* FirstTrain(int p_army)
{
	if (p_army < 0)
		return 0;
	int army = p_army;
	if (army >= 4)
		army = 3;
	cur_army = army;
	cur_train = 1;
	return GetTrainEng(army, 1, &p_army);
}

// FUNCTION: ALIEN 0x43aa50
int* GetTrainEng(int p_army, int p_train, int* p_out)
{
	HASH_MAP* hash = Hash;
	int n = hash->m_list.m_n;
	int number = 0;
	int train = 0;
	if (n) {
		int idx = n - 1;
		ENGINE* engine = (ENGINE*) hash->m_list.m_data[idx];
		if (engine) {
			while (engine) {
				if (engine->m_vid->m_sprClass == 21 && !engine->m_prevEngine) {
					if (p_army == 4 || ((engine->m_flag >> 11) & 3) == p_army) {
						++number;
						if (++train == p_train) {
							*p_out = number;
							ENGINE* trainEngine = engine->GetTrain();
							if (!trainEngine || !((engine->m_flag ^ trainEngine->m_flag) & 0x1800))
								return (int*) (trainEngine ? trainEngine : engine);
							hash = Hash;
							--train;
						}
					}
					else {
						++number;
					}
				}
				if (idx > hash->m_list.m_n)
					idx = hash->m_list.m_n;
				if (--idx < 0)
					break;
				engine = (ENGINE*) hash->m_list.m_data[idx];
			}
		}
	}
	*p_out = 0;
	return 0;
}

// FUNCTION: ALIEN 0x43ab00
int* NextTrain()
{
	int eng;
	return GetTrainEng(cur_army, ++cur_train, &eng);
}

// FUNCTION: ALIEN 0x454170
TRAIN_INFO::TRAIN_INFO(const ENGINE* p_engine)
{
	m_accelTime = 0;
	m_speedInc = 0;
	m_unk0x18 = 10000;
	m_unk0x20 = 0;
	m_unk0x1c = 0;
	m_unk0x3c = 0;
	m_unk0x28 = 0;
	m_unk0x30 = 0;
	m_unk0x2c = 0;
	m_unk0x14 = 0;
	m_unk0x24 = 0;
	m_flag &= ~3u;
	m_unk0x04 = 0;
	m_unk0x34 = 0;
	m_unk0x38 = 0;
	m_unk0x08 = TRAIN_INFO_UNSET;
	const ENGINE* e;
	for (e = p_engine; e; e = e->m_nextEngine)
		AddEngine(e);
	for (e = p_engine->m_prevEngine; e; e = e->m_prevEngine)
		AddEngine(e);
	if (m_unk0x30)
		m_unk0x3c /= m_unk0x30;
	else
		m_unk0x3c = 100;
	if (m_unk0x08 == TRAIN_INFO_UNSET)
		m_unk0x08 = 0;
	float range = m_speedInc - m_unk0x14;
	if (range != 0.0f) {
		int ratio = (int) ((range - (m_accelTime - m_unk0x14)) * m_unk0x18 / range);
		m_unk0x18 = ratio;
		if (ratio < 5)
			m_unk0x18 = 0;
	}
	if (m_unk0x18 == 10000)
		m_unk0x18 = 0;
}

// FUNCTION: ALIEN 0x454260
void TRAIN_INFO::AddEngine(const ENGINE* p_engine)
{
	VID* vid = p_engine->m_vid;

	if (p_engine->m_vid->m_exData->m_unk0x10 != 0.0f) {
		float speed = p_engine->m_exData ? p_engine->m_exData->m_unk0x20
										 : p_engine->m_vid->m_unk0x2c;
		if ((p_engine->m_exData ? p_engine->m_exData->m_unk0x20
									 : p_engine->m_vid->m_unk0x2c) * 1000.0f
			< (float) m_unk0x18)
			m_unk0x18 = (int) ((p_engine->m_exData ? p_engine->m_exData->m_unk0x20
												 : p_engine->m_vid->m_unk0x2c) * 1000.0f);
	}
	m_speedInc += p_engine->m_vid->m_exData->m_unk0x10;
	m_accelTime += p_engine->m_vid->m_exData->m_unk0x0c;
	if (p_engine->m_vid->m_exData->m_unk0x10 > 0.0f)
		m_unk0x14 += p_engine->m_vid->m_exData->m_unk0x0c;

	if (p_engine->m_vid->m_idx != 45)
		m_flag |= 1;
	else
		m_flag |= 2;

	m_unk0x20 += p_engine->m_unk0x54;

	int shots = p_engine->m_ammo / 64;
	int maxAmmo = p_engine->m_vid->GetMaxAmmo();
	if (shots > 0) {

		float range;
		SPRITE* child = p_engine->m_child;
		if (child && child->m_vid == p_engine->m_vid->m_linkVid
			&& child->m_vid->m_weaponVid && child->m_vid->m_weapon
			&& p_engine->m_vid->m_exData->m_unk0x18 == 0.0f)
			range = child->m_vid->m_exData->m_unk0x18;
		else
			range = p_engine->m_vid->m_exData->m_unk0x18;
		if (range > m_maxWeaponRange)
			m_maxWeaponRange = range;
		if (range != 0.0f && range < m_minWeaponRange)
			m_minWeaponRange = range;
	}
	if (maxAmmo && maxAmmo != 999999 && p_engine->m_vid->m_idx != 85) {
		++m_unk0x30;
		m_unk0x34 += shots;
		m_unk0x38 += maxAmmo;
		m_unk0x3c += 100 * shots / maxAmmo;
	}

	m_unk0x24 += p_engine->m_vid->m_maxHp[(p_engine->m_flag >> 11) & 3];
	m_unk0x2c += p_engine->m_vid->GetBuildTime();

	int fireDamage;
	if (shots) {
		if (p_engine->m_vid->m_idx == 82) {
			VID* shell = Map->m_noVid > 70 && Map->m_vids[70] ? Map->m_vids[70]
														  : EmptyVid;
			fireDamage = shell->GetFireDamage();
		} else {
			fireDamage = ((SPRITE*) p_engine)->GetFireDamage();
		}
	} else
		fireDamage = 0;
	m_unk0x28 += fireDamage;

	if (p_engine->m_vid->m_linkVid) {
		m_unk0x24 += p_engine->m_vid->m_linkVid->m_maxHp[(p_engine->m_flag >> 11) & 3];
		SPRITE* child = p_engine->m_child;
		if (!child || child->m_vid != p_engine->m_vid->m_linkVid) {
			if (p_engine->m_vid->m_linkVid->m_entitiesNumber[(p_engine->m_flag >> 11) & 3]
				>= p_engine->m_vid->m_entitiesNumber[(p_engine->m_flag >> 11) & 3])
				m_unk0x20 += p_engine->m_vid->m_linkVid->m_maxHp[(p_engine->m_flag >> 11) & 3];
		}
	}
	if (p_engine->m_child && p_engine->m_child->m_vid == p_engine->m_vid->m_linkVid) {
		m_accelTime += p_engine->m_child->m_vid->m_exData->m_unk0x0c;
		if (p_engine->m_child->m_vid->m_sprClass != 9)
			m_unk0x20 += p_engine->m_child->m_unk0x54;
		if (p_engine->m_vid->m_exData->m_unk0x10 > 0.0f)
			m_unk0x14 += p_engine->m_child->m_vid->m_exData->m_unk0x0c;
	}
	++m_unk0x1c;
}
