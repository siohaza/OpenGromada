#include "video/mesh.h"

#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "util/myerror.h"

// FUNCTION: ALIEN 0x41ef60
MESH::MESH(int p_nVerts, int p_nIndices)
{
	m_vertexBuffer = 0;
	m_indexBuffer = 0;
	m_nVerts = p_nVerts;
	m_nIndices = p_nIndices;
	int hr = ((GRAPH_CORE*) Graph)->m_device->CreateVertexBuffer(20 * p_nVerts,
		D3DUSAGE_WRITEONLY, D3DFVF_XYZ | D3DFVF_TEX1, D3DPOOL_MANAGED, &m_vertexBuffer);
	if (hr)
		MYERROR::Error(::Error,
			// STRING: ALIEN 0x483064
			"MESH", 3,
			// STRING: ALIEN 0x48306c
			"VertexBuffer", hr);
	if (p_nIndices) {
		int hrIdx = ((GRAPH_CORE*) Graph)->m_device->CreateIndexBuffer(2 * m_nIndices,
			D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED, &m_indexBuffer);
		if (hrIdx)
			MYERROR::Error(::Error,
				"MESH", 3,
				// STRING: ALIEN 0x483058
				"IndexBuffer", hrIdx);
	}
}

// FUNCTION: ALIEN 0x41f040
MESH::~MESH()
{
	((GRAPH_CORE*) Graph)->m_device->SetIndices(0, 0);
	((GRAPH_CORE*) Graph)->m_device->SetStreamSource(0, 0, 0x14);
	if (m_indexBuffer) {
		int released = m_indexBuffer->Release();
		if (released)
			MYERROR::Error(::Error,
				"MESH", 10,
				// STRING: ALIEN 0x483098
				"Index release count !=0", released);
	}
	if (m_vertexBuffer) {
		int released = m_vertexBuffer->Release();
		if (released)
			MYERROR::Error(::Error,
				"MESH", 10,
				// STRING: ALIEN 0x48307c
				"Vertex release count !=0", released);
	}
}

// FUNCTION: ALIEN 0x41f120
void MESH::Draw(SPRITE*)
{
	((GRAPH_CORE*) Graph)->SetRenderState(D3DRS_ZENABLE, 1);
	((GRAPH_CORE*) Graph)->SetRenderState(D3DRS_ZFUNC, D3DCMP_GREATER);
	((GRAPH_CORE*) Graph)->SetRenderState(D3DRS_ZWRITEENABLE, 1);
	((GRAPH_CORE*) Graph)->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	((GRAPH_CORE*) Graph)->SetRenderState(D3DRS_ALPHATESTENABLE, 1);
	((GRAPH_CORE*) Graph)->SetRenderState(D3DRS_ALPHAREF, 0x20);
	((GRAPH_CORE*) Graph)->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
	int hr;
	if (m_nIndices) {
		hr = ((GRAPH_CORE*) Graph)->m_device->SetIndices(m_indexBuffer, 0);
		if (hr < 0)
			MYERROR::Error(::Error,
				"MESH", 8,
				// STRING: ALIEN 0x4830d0
				"Indices", hr);
	}
	hr = ((GRAPH_CORE*) Graph)->m_device->SetStreamSource(0, m_vertexBuffer, 20);
	if (hr < 0)
		MYERROR::Error(::Error, "MESH", 8,
			// STRING: ALIEN 0x4830c8
			"Vertex", hr);
	hr = ((GRAPH_CORE*) Graph)->m_device->SetVertexShader(D3DFVF_XYZ | D3DFVF_TEX1);
	if (hr < 0)
		MYERROR::Error(::Error, "MESH", 8,
			// STRING: ALIEN 0x4830b8
			"VertexShader", hr);
	if (m_nIndices)
		hr = ((GRAPH_CORE*) Graph)->m_device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, m_nVerts, 0,
			m_nIndices / 3);
	else
		hr = ((GRAPH_CORE*) Graph)->m_device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
	if (hr < 0)
		MYERROR::Error(::Error, "MESH", 10,
			// STRING: ALIEN 0x4830b0
			"Draw", hr);
	((GRAPH_CORE*) Graph)->SetRenderState(D3DRS_ALPHATESTENABLE, 0);
}

// FUNCTION: ALIEN 0x41f2d0
void* MESH::LockVertexBuffer()
{
	BYTE* data;
	m_vertexBuffer->Lock(0, 0, &data, D3DLOCK_DISCARD);
	return data;
}

// FUNCTION: ALIEN 0x41f2f0
void MESH::UnLockVertexBuffer()
{
	m_vertexBuffer->Unlock();
}

// FUNCTION: ALIEN 0x41f300
void* MESH::LockIndexBuffer()
{
	BYTE* data;
	m_indexBuffer->Lock(0, 0, &data, D3DLOCK_DISCARD);
	return data;
}

// FUNCTION: ALIEN 0x41f320
void MESH::UnLockIndexBuffer()
{
	m_indexBuffer->Unlock();
}
