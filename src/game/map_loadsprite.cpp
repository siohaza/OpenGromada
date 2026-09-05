#include "game/map.h"
#include "util/myerror.h"
#include "video/vid.h"

#include <cmath>

// FUNCTION: ALIEN 0x40f0a0
SPRITE* MAP::LoadSprite(RESOURCE* p_resource, int p_version)
{
	int pointerToken = -1;
	int nvid = -1;
	float x = 0;
	float y = 0;
	float z = 0;
	ANGLE angle;
	int direction;
	int army;
	SPRITE* sprite = 0;

	p_resource->ReadWords(&pointerToken, 4);
	if (pointerToken == -1) {
		return (SPRITE*) -1;
	}
	if (p_resource->Remaining() < 24) {
		p_resource->Fail("truncated legacy MAP sprite record");
		return (SPRITE*) -1;
	}
	p_resource->ReadWords(&nvid, 4);
	if (p_version > 9) {
		p_resource->ReadWords(&x, 4);
		p_resource->ReadWords(&y, 4);
		p_resource->ReadWords(&z, 4);
	}
	else {
		p_resource->ReadWords(&direction, 4);
		x = (float) direction;
		p_resource->ReadWords(&direction, 4);
		y = (float) direction;
		p_resource->ReadWords(&direction, 4);
		z = (float) direction;
	}
	p_resource->ReadWords(&direction, 4);
	angle.m_dir = (char) direction;
	{
		p_resource->ReadWords(&army, 4);
		if (!p_resource->Good() || !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) {
			p_resource->Fail("invalid legacy MAP sprite coordinates");
			return (SPRITE*) -1;
		}

		if (nvid >= 0 && nvid < m_noVid && m_vids[nvid]) {
			sprite = CreateSprite(m_vids[nvid], x, y, z, angle, 0);
		}
		else if (::Error) {
			MYERROR::Error(::Error, "MAP", 3, "sprite, this vid not exist", nvid);
		}
	}
	m_relation.Insert((void*) (decomp_intptr) pointerToken, sprite);
	if (sprite) {
		sprite->ChangeArmy(army);
	}
	return sprite;
}
