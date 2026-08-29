#ifndef MESH_H
#define MESH_H

#include "util/decomp.h"
#include <dxsdk/d3d8.h>

class SPRITE;

// VTABLE: ALIEN 0x47a6c0

class MESH_BASE {
public:
	virtual ~MESH_BASE() {} // vtable+0x00
	virtual void* LockVertexBuffer() { return 0; } // vtable+0x04
	virtual void UnLockVertexBuffer() {} // vtable+0x08
	virtual void* LockIndexBuffer() { return 0; } // vtable+0x0c
	virtual void UnLockIndexBuffer() {} // vtable+0x10
	virtual void Draw(SPRITE* p_sprite) = 0; // vtable+0x14
};

// VTABLE: ALIEN 0x47a6a8

class MESH : public MESH_BASE {
public:
	MESH(int p_nVerts, int p_nIndices);
	virtual ~MESH();

	int m_nVerts; // 0x04
	int m_nIndices; // 0x08
	IDirect3DIndexBuffer8* m_indexBuffer; // 0x0c
	IDirect3DVertexBuffer8* m_vertexBuffer; // 0x10

	virtual void* LockVertexBuffer();
	virtual void UnLockVertexBuffer();
	virtual void* LockIndexBuffer();
	virtual void UnLockIndexBuffer();
	virtual void Draw(SPRITE* p_sprite);
};

DECOMP_SIZE_ASSERT(MESH, 0x14)

// SYNTHETIC: ALIEN 0x41f020
// MESH::`scalar deleting destructor'

// SYNTHETIC: ALIEN 0x41f100
// MESH_BASE::`scalar deleting destructor'

#endif
