#include "game/map.h"
#include "util/myerror.h"
#include "video/vid.h"

// FUNCTION: ALIEN 0x40f0a0
SPRITE* MAP::LoadSprite(RESOURCE* p_resource, int p_version)
{
	int pointerToken;
	int nvid;
	float x;
	float y;
	float z;
	ANGLE angle;
	int direction;
	int army;
	SPRITE* sprite = 0;

	p_resource->Read(&pointerToken, 4);
	if (pointerToken == -1) {
		return (SPRITE*) -1;
	}
	p_resource->Read(&nvid, 4);
	if (p_version > 9) {
		p_resource->Read(&x, 4);
		p_resource->Read(&y, 4);
		p_resource->Read(&z, 4);
	}
	else {
		int coordinate;
		p_resource->Read(&direction, 4);
		x = (float) direction;
		p_resource->Read(&direction, 4);
		y = (float) direction;
		p_resource->Read(&direction, 4);
		z = (float) direction;
	}
	p_resource->Read(&direction, 4);
	angle.m_dir = (char) direction;
	{
		p_resource->Read(&army, 4);

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
