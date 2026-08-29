#include "gfx/texture.h"

#include <dxsdk/d3d8.h>
#include <dxsdk/d3dx8tex.h>
#include "gfx/graph.h"
#include "gfx/gamma.h"
#include "gfx/graph_core.h"
#include "util/myerror.h"
#include "util/stream.h"
#include "util/string.h"

// GLOBAL: ALIEN 0x4905c8
int TextureMemoryInUse;

// GLOBAL: ALIEN 0x4905cc
int g_textureSquare;
// GLOBAL: ALIEN 0x4905d0
int g_textureDefaultPool;
// GLOBAL: ALIEN 0x4905d4
int g_texturePalette;
// GLOBAL: ALIEN 0x4905d8
int g_textureDxt;
// GLOBAL: ALIEN 0x4905dc
int g_textureAlphaPalette;
// GLOBAL: ALIEN 0x4905e0
int g_textureCondNonPow2;
// GLOBAL: ALIEN 0x47f080
int g_texturePowerOfTwo = 1;
// GLOBAL: ALIEN 0x47f084
int g_textureMaxWidth = 256;
// GLOBAL: ALIEN 0x47f088
int g_textureMaxHeight = 256;

// FUNCTION: ALIEN 0x403290
TEXTURE::TEXTURE(int p_width, int p_height, int p_format, unsigned int p_flags)
{
	Create(p_width, p_height, p_format, p_flags);
}

static inline int TextureFormatBits(int p_format)
{
	switch (p_format) {
	case 21:
	case 22:
		return 32;
	case 23:
	case 24:
	case 25:
	case 26:
		return 16;
	case 20:
		return 24;
	default:
		return 0;
	case 41:
	case 0x33545844 /* 'DXT3' */ :
	case 0x35545844 /* 'DXT5' */ :
		return 8;
	case 0x31545844 /* 'DXT1' */ :
		return 4;
	}
}

class TEXTURE_LOAD_GUARD {
public:
	~TEXTURE_LOAD_GUARD() {}
};

// STUB: ALIEN 0x4032e0
TEXTURE::TEXTURE(int p_width, int p_height, int p_format, unsigned int p_flags,
	const void* p_palette, STREAM* p_stream)
{
	static TEXTURE_LOAD_GUARD guard;
	Create(p_width, p_height, p_format, p_flags);

	char* pixels;
	IDirect3DSurface8* surface;
	PALETTEENTRY palette[256];
	PALETTEENTRY* pal = 0;
	if (p_palette) {
		const unsigned int* src = (const unsigned int*) p_palette;
		for (int i = 0; i < 256; ++i) {
			palette[i].peFlags = (unsigned char) (src[i] >> 24);
			palette[i].peRed = (unsigned char) (src[i] >> 16);
			palette[i].peGreen = (unsigned char) (src[i] >> 8);
			palette[i].peBlue = (unsigned char) src[i];
		}
		pal = palette;
	}
	if (m_format == 41)
		SetPaletteEntries(pal);

	pixels = (char*) operator new(TextureFormatBits(p_format) * p_width * p_height / 8);
	p_stream->Read(pixels, TextureFormatBits(p_format) * p_width * p_height / 8);

	m_texture->GetSurfaceLevel(0, &surface);
	PALETTEENTRY* destPalette = m_format != 41 ? 0 : pal;
	RECT src = { 0, 0, p_width, p_height };
	RECT dst = { 0, 0, p_width, p_height };
	unsigned int pitch = TextureFormatBits(p_format) * p_width;
	if (p_format == 0x31545844 || p_format == 0x33545844 || p_format == 0x35545844)
		pitch >>= 1;
	else
		pitch >>= 3;
	D3DXLoadSurfaceFromMemory(surface, destPalette, &dst, pixels, (D3DFORMAT) p_format,
		pitch, pal, &src, 1, (m_flags & 8) ? 0 : 0xff000000);
	surface->Release();
	operator delete(pixels);
}

// FUNCTION: ALIEN 0x403620
TEXTURE::~TEXTURE()
{
	if (m_texture) {
		((GRAPH_CORE*) Graph)->m_device->SetTexture(0, 0);
		int refs = m_texture->Release();
		if (refs) {
			if (Error)
				MYERROR::Error(Error,
					// STRING: ALIEN 0x47f3f4
					"TEXTURE", 10,
					// STRING: ALIEN 0x47f3fc
					"Release count !=0", refs);
		} else {
			TextureMemoryInUse -= m_width * m_height * ((m_format != 41) + 1);
		}
	}
	if (m_data)
		operator delete(m_data);
}

// FUNCTION: ALIEN 0x4036b0
void TEXTURE::Create(int p_width, int p_height, int p_format, unsigned int p_flags)
{
	m_texture = 0;
	m_data = 0;
	int result = 0;
	if (!g_textureDefaultPool)
		p_flags &= ~1;

	if (g_textureSquare) {
		if (p_width > p_height)
			p_height = p_width;
		else if (p_width < p_height)
			p_width = p_height;
	}
	if (g_texturePowerOfTwo) {
		int newWidth;
		int newHeight;
		for (newWidth = 1; p_width > newWidth; newWidth *= 2) {
		}
		for (newHeight = 1; p_height > newHeight; newHeight *= 2) {
		}
		p_width = newWidth;
		p_height = newHeight;
	}
	if (p_width > g_textureMaxWidth || p_width <= 0) {
		if (Error) {
			MYERROR::Error(Error,
				"TEXTURE", 4,
				// STRING: ALIEN 0x47f410
				"initial sizeX", p_width);
		}
		return;
	}
	if (p_height > g_textureMaxHeight || p_height <= 0) {
		if (Error) {
			MYERROR::Error(Error,
				"TEXTURE", 4,
				// STRING: ALIEN 0x47f420
				"initial sizeY", p_height);
		}
		return;
	}

	unsigned int format = p_format;
	m_width = p_width;
	m_height = p_height;
	m_flags = p_flags;
	if (p_format == 21 || p_format == 26 || p_format == 25 || p_format == 0x33545844 || p_format == 0x35545844)
		m_flags = p_flags | 8;
	if (p_format == 41 && (m_flags & 8) && !g_textureAlphaPalette)
		format = 21;
	if (p_flags & 0x10) {
		if (format == 41) {
			if (!(m_flags & 8))
				format = 25;
		} else if (format == 23) {
			format = 25;
		} else if (format == 20) {
			format = 21;
		}
	}
	if (format == 41 && !g_texturePalette) {
		format = 23;
	}

	if ((p_flags & 2) && format == 80) {
		m_data = operator new(2 * p_height * p_width);
	} else {
		unsigned int usage = 0;
		if (p_flags & 4)
			usage = 1;
		int default_pool = p_flags & 1;
		if (default_pool)
			usage |= 0x200;
		while (1) {
			if (default_pool)
				result = ((GRAPH_CORE*) Graph)->m_device->CreateTexture(p_width, p_height, 1, usage,
					(D3DFORMAT) format, D3DPOOL_DEFAULT, &m_texture);
			else
				result = ((GRAPH_CORE*) Graph)->m_device->CreateTexture(p_width, p_height, 1, usage,
					(D3DFORMAT) format, D3DPOOL_MANAGED, &m_texture);
			if (!result)
				break;
			if (format == 41) {
				format = ~m_flags;
				if ((char) format & 8)
					format = 23;
				else
					format = 21;
			}
			else if (format == 0x31545844)
				format = 23;
			else if (format == 0x33545844)
				format = 26;
			else if (format == 23)
				format = 24;
			else if (format == 24)
				format = 25;
			else if (format == 20)
				format = 22;
			else if (format == 22)
				format = 21;
			else
				break;
		}
	}
	m_format = format;
	if (result) {
		if (Error)
			MYERROR::Error(Error,
				"TEXTURE", 3, empty_str, result);
	} else if (m_texture) {
		TextureMemoryInUse += m_width * m_height * ((format != 41) + 1);
	}
}

// FUNCTION: ALIEN 0x403970
int TEXTURE::CopyFromSurface(IDirect3DSurface8* p_surface, const RECT* p_rect, const POINT* p_point)
{
	IDirect3DSurface8* surface;
	int hr = m_texture->GetSurfaceLevel(0, &surface);
	if (!hr) {
		RECT dst;
		dst.left = p_point->x;
		dst.top = p_point->y;
		dst.right = p_point->x + p_rect->right - p_rect->left;
		dst.bottom = p_point->y + p_rect->bottom - p_rect->top;
		int result = D3DXLoadSurfaceFromSurface(surface, 0, &dst, p_surface, 0, p_rect, 0xffffffff, 0);
		if (result && ::Error)
			MYERROR::Error(::Error, "TEXTURE", 1,
				// STRING: ALIEN 0x47f444
				"from surface", result);
		surface->Release();
		return result;
	}
	if (::Error)
		MYERROR::Error(::Error,
			"TEXTURE", 9,
			// STRING: ALIEN 0x47f430
			"surface for copy", hr);
	return hr;
}

// FUNCTION: ALIEN 0x403a40
int TEXTURE::Lock(int* p_pitch, const RECT* p_rect)
{
	if (m_flags & 2) {
		*p_pitch = 2 * m_width;
		if (p_rect)
			return (int) m_data + 2 * (p_rect->left + m_width * p_rect->top);
		return (int) m_data;
	}
	int rect[2];
	int hr = m_texture->LockRect(0, (D3DLOCKED_RECT*) rect, p_rect, 0);
	if (hr) {
		if (::Error)
			MYERROR::Error(::Error, "TEXTURE", 0, empty_str, hr);
		return 0;
	}
	*p_pitch = rect[0];
	return rect[1];
}

struct DRAW_Z_VERTEX {
	float m_x;
	float m_y;
	float m_z;
	float m_rhw;
	unsigned int m_diffuse;
	unsigned int m_specular;
	float m_u;
	float m_v;
};

// FUNCTION: ALIEN 0x403ae0
int TEXTURE::Draw(const RECT* p_dst, const RECT* p_src, const GAMMA* p_gamma)
{
	float w;
	float h;
	unsigned int diffuse = ~(unsigned int) p_gamma->m_a;
	unsigned int specular = (unsigned int) p_gamma->m_b;
	const int* dst = (const int*) p_dst;
	const int* src = (const int*) p_src;

	DRAW_Z_VERTEX v[4];
	v[0].m_x = (float) p_dst->left;
	v[0].m_y = (float) p_dst->top;
	v[0].m_z = 0.99999988f;
	v[0].m_rhw = 1.0f;
	v[0].m_diffuse = diffuse;
	v[0].m_specular = specular;
	w = (float) m_width;
	v[0].m_u = (p_src->left + 0.5f) / w;
	h = (float) m_height;
	v[0].m_v = (p_src->top + 0.5f) / h;

	v[1].m_x = (float) p_dst->right;
	v[1].m_y = (float) p_dst->top;
	v[1].m_z = 0.99999988f;
	v[1].m_rhw = 1.0f;
	v[1].m_diffuse = diffuse;
	v[1].m_specular = specular;
	v[1].m_u = (p_src->right + 0.5f) / w;
	v[1].m_v = (p_src->top + 0.5f) / h;

	v[2].m_x = (float) p_dst->right;
	v[2].m_y = (float) p_dst->bottom;
	v[2].m_z = 0.99999988f;
	v[2].m_rhw = 1.0f;
	v[2].m_diffuse = diffuse;
	v[2].m_specular = specular;
	v[2].m_u = (p_src->right + 0.5f) / w;
	v[2].m_v = (p_src->bottom + 0.5f) / h;

	v[3].m_x = (float) p_dst->left;
	v[3].m_y = (float) p_dst->bottom;
	v[3].m_z = 0.99999988f;
	v[3].m_rhw = 1.0f;
	v[3].m_diffuse = diffuse;
	v[3].m_specular = specular;
	v[3].m_u = (p_src->left + 0.5f) / w;
	v[3].m_v = (p_src->bottom + 0.5f) / h;

	GRAPH_CORE* graph;
	IDirect3DDevice8* device;
	int hr = ((GRAPH_CORE*) Graph)->m_device->SetTexture(0, m_texture);
	if (hr && ::Error)
		MYERROR::Error(::Error, "TEXTURE", 8, empty_str, hr);
	if (m_format == 41) {
		int hr2 = ((GRAPH_CORE*) Graph)->m_device->SetCurrentTexturePalette(*(unsigned int*) m_unk0x18);
		if (hr2 && ::Error)
			MYERROR::Error(::Error, "TEXTURE", 8,
				// STRING: ALIEN 0x47f454
				"palette", hr2);
	}
	((GRAPH_CORE*) Graph)->SetRenderState(D3DRS_SPECULARENABLE, p_gamma->m_b != 0);
	int filter;
	if (p_dst->right - p_dst->left == p_src->right - p_src->left
		&& p_dst->bottom - p_dst->top == p_src->bottom - p_src->top) {
		((GRAPH_CORE*) Graph)->m_device->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_POINT);
		((GRAPH_CORE*) Graph)->m_device->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_POINT);
	} else {
		((GRAPH_CORE*) Graph)->m_device->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
		((GRAPH_CORE*) Graph)->m_device->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
	}
	((GRAPH_CORE*) Graph)->m_device->SetVertexShader(452);
	int result = ((GRAPH_CORE*) Graph)->m_device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, v, 32);
	if (result && ::Error)
		return (int) MYERROR::Error(::Error, "TEXTURE", 10,
			"DrawPrimitiveUP", result);
	return result;
}

// FUNCTION: ALIEN 0x403dd0
char* TEXTURE::Draw_z(float p_z1, int p_z2, const int* p_dst, const int* p_src, const GAMMA* p_gamma)
{
	float w;
	float h;
	unsigned int diffuse = ~(unsigned int) p_gamma->m_a;
	unsigned int specular = (unsigned int) p_gamma->m_b;
	float z2 = *(const float*) &p_z2;

	DRAW_Z_VERTEX v[4];
	v[0].m_x = (float) p_dst[0];
	v[0].m_y = (float) p_dst[1];
	v[0].m_z = p_z1;
	v[0].m_rhw = 1.0f;
	v[0].m_diffuse = diffuse;
	v[0].m_specular = specular;
	w = (float) m_width;
	v[0].m_u = (p_src[0] + 0.5f) / w;
	h = (float) m_height;
	v[0].m_v = (p_src[1] + 0.5f) / h;

	v[1].m_x = (float) p_dst[2];
	v[1].m_y = (float) p_dst[1];
	v[1].m_z = p_z1;
	v[1].m_rhw = 1.0f;
	v[1].m_diffuse = diffuse;
	v[1].m_specular = specular;
	v[1].m_u = (p_src[2] + 0.5f) / w;
	v[1].m_v = (p_src[1] + 0.5f) / h;

	v[2].m_x = (float) p_dst[2];
	v[2].m_y = (float) p_dst[3];
	v[2].m_z = z2;
	v[2].m_rhw = 1.0f;
	v[2].m_diffuse = diffuse;
	v[2].m_specular = specular;
	v[2].m_u = (p_src[2] + 0.5f) / w;
	v[2].m_v = (p_src[3] + 0.5f) / h;

	v[3].m_x = (float) p_dst[0];
	v[3].m_y = (float) p_dst[3];
	v[3].m_z = z2;
	v[3].m_rhw = 1.0f;
	v[3].m_diffuse = diffuse;
	v[3].m_specular = specular;
	v[3].m_u = (p_src[0] + 0.5f) / w;
	v[3].m_v = (p_src[3] + 0.5f) / h;

	GRAPH_CORE* graph;
	IDirect3DDevice8* device;
	int hr = ((GRAPH_CORE*) Graph)->m_device->SetTexture(0, m_texture);
	if (hr && ::Error)
		MYERROR::Error(::Error, "TEXTURE", 8, empty_str, hr);
	if (m_format == 41) {
		int hr2 = ((GRAPH_CORE*) Graph)->m_device->SetCurrentTexturePalette(*(unsigned int*) m_unk0x18);
		if (hr2 && ::Error)
			MYERROR::Error(::Error, "TEXTURE", 8,
				"palette", hr2);
	}
	((GRAPH_CORE*) Graph)->SetRenderState(D3DRS_SPECULARENABLE, p_gamma->m_b != 0);
	((GRAPH_CORE*) Graph)->m_device->SetVertexShader(452);
	int filter;
	if (p_dst[2] - p_dst[0] == p_src[2] - p_src[0]
		&& p_dst[3] - p_dst[1] == p_src[3] - p_src[1]) {
		((GRAPH_CORE*) Graph)->m_device->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_POINT);
		((GRAPH_CORE*) Graph)->m_device->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_POINT);
	} else {
		((GRAPH_CORE*) Graph)->m_device->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
		((GRAPH_CORE*) Graph)->m_device->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
	}
	char* result = (char*) ((GRAPH_CORE*) Graph)->m_device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, v, 32);
	if (result && ::Error)
		return MYERROR::Error(::Error, "TEXTURE", 10,
			"DrawPrimitiveUP", (int) result);
	return result;
}

// GLOBAL: ALIEN 0x4905c4
static int NextTexturePalette;

// FUNCTION: ALIEN 0x4040c0
int TEXTURE::SetPaletteEntries(void* p_entries)
{
	int result;
	if (!m_texture) {
		result = ::Error;
		if (result)
			return (int) MYERROR::Error(result, "TEXTURE", 8,
				// STRING: ALIEN 0x47f488
				"palette for non initialized texture", 0);
		goto done;
	}
	if (m_format == 41) {
		int hr = ((GRAPH_CORE*) Graph)->m_device->SetPaletteEntries(NextTexturePalette,
			(const PALETTEENTRY*) p_entries);
		if (hr < 0) {
			if (::Error)
				return (int) MYERROR::Error(::Error, "TEXTURE", 8,
					// STRING: ALIEN 0x47f47c
					"palettes", hr);
			return hr;
		}
		int palette = NextTexturePalette;
		*(unsigned int*) m_unk0x18 = palette;
		NextTexturePalette++;
		return palette;
	}
	result = ::Error;
	if (result)
		result = (int) MYERROR::Error(result, "TEXTURE", 8,
			// STRING: ALIEN 0x47f45c
			"palette for non palette texture", 0);
done:
	return result;
}

// FUNCTION: ALIEN 0x404180
int TEXTURE::SetPalette(const void* p_palette)
{
	PALETTEENTRY pal[256];
	const unsigned int* src = (const unsigned int*) p_palette;
	for (int i = 0; i < 256; i++) {
		pal[i].peFlags = (unsigned char) (src[i] >> 24);
		pal[i].peRed = (unsigned char) (src[i] >> 16);
		pal[i].peGreen = (unsigned char) (src[i] >> 8);
		pal[i].peBlue = (unsigned char) src[i];
	}
	return SetPaletteEntries(pal);
}
