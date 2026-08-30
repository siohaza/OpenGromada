#include "game/builded_terrain.h"

#include "game/map.h"
#include "video/vid.h"

// FUNCTION: ALIEN 0x40edf0
BUILDED_TERRAIN::BUILDED_TERRAIN(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent)
	: SPRITE(p_vid, p_x, p_y, p_z, p_dir, p_parent)
{
	if (Map->GetVid(1024) == EmptyVid) {
		Map->CreateEmptyHardwareGround();
	}

	VID* ground = Map->GetVid(1024);
	if (ground != EmptyVid && Map->GetVid(1024)->m_noDir == 1) {
		Map->GetVid(1024)->DrawVidToVid(this);
		if (!(m_vid->m_flag & 0x40)) {
			ChangeAnimation(15);
		}
	}
}

// FUNCTION: ALIEN 0x40eed0
void BUILDED_TERRAIN::Draw()
{
	int noVid = Map->m_noVid;
	VID* v2;
	if (noVid <= 1024 || (v2 = Map->m_vids[1024]) == 0) {
		v2 = EmptyVid;
	}
	if (v2 != EmptyVid) {
		VID* result;
		if (noVid <= 1024 || (result = Map->m_vids[1024]) == 0) {
			result = EmptyVid;
		}
		if (result->m_noDir == 1) {
			return;
		}
	}
	m_vid->Draw(this);
}
