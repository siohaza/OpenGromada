#include "video/vid_mesh.h"

#include <new>
#include <string.h>

#include <dxsdk/d3d8.h>

#include "game/map.h"
#include "gfx/gamma.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "gfx/texture.h"
#include "sprite/ex_sprite_data.h"
#include "sprite/sprite.h"
#include "util/angle.h"
#include "util/resource.h"
#include "video/mesh.h"

static inline float MeshSinY(unsigned char p_direction)
{
	return ANGLE::SinTable2[p_direction];
}

static inline float MeshCos(unsigned char p_direction)
{
	return ANGLE::CosTable[p_direction];
}

static inline float MeshSin(unsigned char p_direction)
{
	return ANGLE::SinTable[p_direction];
}

static inline float MeshCosY(unsigned char p_direction)
{
	return ANGLE::CosTable2[p_direction];
}

static __forceinline float MeshAnimationValue(
	EX_SPRITE_DATA* p_ex, float* p_table, int p_base)
{
	float frame = *(float*) ((char*) p_ex + 28);
	int fi = (int) frame;
	return (fi >= 7) ? p_table[p_base + 7]
						 : (p_table[fi + p_base + 1] - p_table[fi + p_base])
							 * (frame - fi)
							 + p_table[fi + p_base];
}

static __forceinline int MeshDepthOccluded(float p_z, int p_depth)
{
	return p_depth > 8 * (int) p_z + 1024;
}

static __forceinline void MeshAddScreenGamma(
	GAMMA* p_total, GAMMA* p_merged, GRAPH_CORE* p_graph)
{
	p_total->GAMMA::GAMMA(*p_merged->Add(*p_total,
		GAMMA(GAMMA::RAW_COPY, p_graph->m_gammaSet.m_a,
			p_graph->m_gammaSet.m_b)));
}

// FUNCTION: ALIEN 0x4128a0
VID* VID_MESH::CreateMirror()
{
	return new VID_MESH(*this);
}

// FUNCTION: ALIEN 0x4128d0
void* VID_MESH::ScalarDeletingDestructor(unsigned int p_flags)
{
	VID_MESH* result = this;
	this->~VID_MESH();
	if (p_flags & 1)
		operator delete(result);
	return result;
}

// FUNCTION: ALIEN 0x41b8f0
VID_MESH::VID_MESH(VID_MESH& p_other)
{
	m_weaponPtr = p_other.m_weaponPtr;
	p_other.m_weaponPtr = this;
	m_layer = p_other.m_layer;
	*(unsigned short*) &m_pixelFlag = *(unsigned short*) &p_other.m_pixelFlag;
	*(unsigned short*) &m_unk0x2f2[2] = *(unsigned short*) &p_other.m_unk0x2f2[2];
	*(unsigned short*) &m_unk0x2f2[0] = *(unsigned short*) &p_other.m_unk0x2f2[0];
	*(unsigned short*) &m_unk0x2f2[4] = *(unsigned short*) &p_other.m_unk0x2f2[4];
	*(unsigned short*) &m_unk0x2f2[6] = *(unsigned short*) &p_other.m_unk0x2f2[6];
	m_textures = p_other.m_textures;
	m_meshes = p_other.m_meshes;
}

// FUNCTION: ALIEN 0x41b990
VID_MESH::~VID_MESH()
{
	if (m_weaponPtr == this) {
		int frames = m_dotFrameCount;
		if (m_textures) {
			for (int a = 0; a < m_dotFrameCount; ++a) {
				for (int b = a + 1; b < m_dotFrameCount; ++b) {
					if (m_textures[a] == m_textures[b])
						m_textures[b] = 0;
				}
			}
			for (int i = 0; i < m_dotFrameCount; ++i) {
				if (m_textures[i])
					delete m_textures[i];
			}
			operator delete(m_textures);
			m_textures = 0;
		}
		if (m_meshes) {
			for (int a = 0; a < m_dotFrameCount; ++a) {
				for (int b = a + 1; b < m_dotFrameCount; ++b) {
					if (m_meshes[a] == m_meshes[b])
						m_meshes[b] = 0;
				}
			}
			for (int i = 0; i < m_dotFrameCount; ++i) {
				if (m_meshes[i])
					delete m_meshes[i];
			}
			operator delete(m_meshes);
			m_meshes = 0;
		}
	}
}

// FUNCTION: ALIEN 0x41baf0
void VID_MESH::Load(RESOURCE* p_res)
{
	p_res->Read(&m_format, 4);
	m_textures = (TEXTURE**) operator new(4 * m_dotFrameCount);
	m_meshes = (MESH**) operator new(4 * m_dotFrameCount);

	unsigned int palette[256];
	if (m_pixelFlag16 & 8) {
		if (!p_res->GoNext(0x204c4150 /* 'PAL ' */ ))
			p_res->Read(palette, 1024);
		else
			Error(5,
				// STRING: ALIEN 0x482be4
				"PAL ", 0);
	}
	if (p_res->GoNext(0x41544144 /* 'DATA' */ ))
		Error(5,
			"DATA", 0);
	for (int i = 0; i < m_dotFrameCount; ++i) {
		unsigned short w;
		unsigned short h;
		p_res->Read(&w, 2);
		p_res->Read(&h, 2);
		int texFlags = 16;
		if (m_pixelFlag16 & 2)
			texFlags = 24;
		m_textures[i] = new TEXTURE(w, h, m_format, texFlags, palette, (STREAM*) p_res);
		int nVerts;
		int nIndices;
		p_res->Read(&nVerts, 4);
		p_res->Read(&nIndices, 4);
		m_meshes[i] = new MESH(nVerts, nIndices);
		float* vert = (float*) m_meshes[i]->LockVertexBuffer();
		for (int v = 0; v < nVerts; ++v) {
			short x;
			short y;
			short z;
			short u;
			short t;
			p_res->Read(&x, 2);
			p_res->Read(&y, 2);
			p_res->Read(&z, 2);
			p_res->Read(&u, 2);
			p_res->Read(&t, 2);
			vert[5 * v + 0] = x;
			vert[5 * v + 1] = y * 1.4143066f;
			vert[5 * v + 2] = (z - 1024) * 0.125f;
			vert[5 * v + 3] = (u + 0.5f) / m_textures[i]->m_width;
			vert[5 * v + 4] = (t + 0.5f) / m_textures[i]->m_height;
		}
		m_meshes[i]->UnLockVertexBuffer();
		if (nIndices) {
			void* indices = m_meshes[i]->LockIndexBuffer();
			p_res->Read(indices, 2 * nIndices);
			m_meshes[i]->UnLockIndexBuffer();
		}
		p_res->GoNextSub(0x41544144 /* 'DATA' */ );
	}
}

// FUNCTION: ALIEN 0x41bde0
void VID_MESH::SetLayer()
{
	m_layer = 8;
}

// STUB: ALIEN 0x41bdf0
int VID_MESH::Draw(SPRITE* p_sprite)
{
	do {
		if (m_unk0x47c & 0x40)
			break;

		if (!(m_flag & 0x8000) && !(m_pixelFlag16 & 4)) {
			float sx = p_sprite->m_x - Map->m_shiftX;
			float sy = p_sprite->m_y - p_sprite->m_z - Map->m_shiftY;
			float z = p_sprite->m_z;
			if (sx < ((GRAPH_CORE*) Graph)->m_viewXMin || sx >= ((GRAPH_CORE*) Graph)->m_viewXMax
				|| sy < ((GRAPH_CORE*) Graph)->m_viewYMin
				|| sy >= ((GRAPH_CORE*) Graph)->m_viewYMax)
				break;
			if (MeshDepthOccluded(*(volatile float*) &z,
					((unsigned short*) ((GRAPH_CORE*) Graph)->m_zbuffer)[(int) sx
						+ (int) sy * ((GRAPH_CORE*) Graph)->m_unk0x250]))
				break;
		}

		float scaleX = m_gammaR;
		float scaleY = m_gammaG;
		float scaleZ = m_gammaB;
		float tx = p_sprite->m_x;
		float ty = p_sprite->m_y;
		float tz = p_sprite->m_z;
		if (m_unk0x47c & 2) {
			EX_SPRITE_DATA* ex = p_sprite->m_exData;
			float* tbl = (float*) m_exData;
			scaleX *= MeshAnimationValue(ex, tbl, 57);
			scaleY *= MeshAnimationValue(ex, tbl, 65);
			scaleZ *= MeshAnimationValue(ex, tbl, 73);
		}
		if (m_unk0x47c & 4) {
			EX_SPRITE_DATA* ex = p_sprite->m_exData;
			float* tbl = (float*) m_exData;
			tx += MeshAnimationValue(ex, tbl, 81);
			ty += MeshAnimationValue(ex, tbl, 89);
			tz += MeshAnimationValue(ex, tbl, 97);
		}

		float matrix[16];
		matrix[0] = MeshCos(0) * scaleX;
		matrix[1] = MeshSinY(0);
		matrix[2] = 0.0f;
		matrix[3] = 0.0f;
		matrix[4] = -MeshSin(0);
		matrix[5] = MeshCosY(0) * scaleY;
		matrix[6] = 0.0f;
		matrix[7] = 0.0f;
		matrix[8] = 0.0f;
		matrix[9] = 0.0f;
		matrix[10] = scaleZ;
		matrix[11] = 0.0f;
		matrix[12] = tx;
		matrix[13] = ty;
		matrix[14] = tz;
		matrix[15] = 1.0f;
		int hr = ((GRAPH_CORE*) Graph)->m_device->SetTransform(D3DTS_WORLD, (D3DMATRIX*) matrix);
		if (hr < 0)
			Error(8,
				// STRING: ALIEN 0x482d20
				"Transform world", hr);

		GAMMA total;
		total.Add(total, GAMMA(GAMMA::RAW_COPY, p_sprite->GetGamma()));
		if (!(m_flag & 0x800)) {
			GAMMA merged;
			MeshAddScreenGamma(&total, &merged, (GRAPH_CORE*) Graph);
		}
		if (total.m_a || total.m_b) {
			((GRAPH_CORE*) Graph)->m_device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
			((GRAPH_CORE*) Graph)->m_device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
			((GRAPH_CORE*) Graph)->SetRenderState(D3DRS_TEXTUREFACTOR, ~total.m_a);
		}
		if (m_pixelFlag16 & 2)
			((GRAPH_CORE*) Graph)->SetAlphaBlend(5, 6);
		else
			((GRAPH_CORE*) Graph)->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
		((GRAPH_CORE*) Graph)->m_device->SetTexture(0, m_textures[p_sprite->m_noCadr]->m_texture);
		m_meshes[p_sprite->m_noCadr]->Draw(p_sprite);
		if (total.m_a || total.m_b) {
			((GRAPH_CORE*) Graph)->m_device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
			((GRAPH_CORE*) Graph)->m_device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
		}
	} while (0);
	if (0)
		return 0;
}
