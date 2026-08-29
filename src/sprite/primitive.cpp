#include "sprite/primitive.h"

// FUNCTION: ALIEN 0x40ec00
PRIMITIVE::PRIMITIVE(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent)
	: SPRITE(p_vid, p_x, p_y, p_z, p_dir, p_parent)
{
}

// FUNCTION: ALIEN 0x40ec50
void PRIMITIVE::Tact()
{
	PrimitiveTact();
}
