#include "video/vid.h"

// FUNCTION: ALIEN 0x43a260
unsigned int VID::GetEntitiesNumberTotal()
{
	return m_entitiesNumber[3] + m_entitiesNumber[2] + m_entitiesNumber[1]
		+ m_entitiesNumber[0];
}
