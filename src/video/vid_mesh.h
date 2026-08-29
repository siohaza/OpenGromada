#ifndef VID_MESH_H
#define VID_MESH_H

#include "video/vid.h"

class STREAM;

// VTABLE: ALIEN 0x47a4e0

class VID_MESH : public VID {
public:

	VID_MESH()
	{
		m_textures = 0;
		m_meshes = 0;
	}
	VID_MESH(VID_MESH& p_other);
	virtual void* ScalarDeletingDestructor(unsigned int p_flags);
	~VID_MESH();
	int Draw(SPRITE* p_sprite);
	void Load(RESOURCE* p_res);

	VID* CreateMirror();
	void SetLayer();

	undefined m_unk0x484[0x28]; // 0x484
	class MESH** m_meshes; // 0x4ac
	class TEXTURE** m_textures; // 0x4b0
	int m_format; // 0x4b4
};

DECOMP_SIZE_ASSERT(VID_MESH, 0x4b8)

#endif
