
#include "gfx/d3dfont.h"

#include <string.h>

#define D3DFONT_BOLD        0x0001
#define D3DFONT_ITALIC      0x0002
#define D3DFONT_ZENABLE     0x0004

#define D3DFONT_FILTERED    0x0004

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) { if (p) { (p)->Release(); (p) = 0; } }
#endif

#define MAX_NUM_VERTICES (50 * 6)
#define D3DFVF_FONT2DVERTEX (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)

struct FONT2DVERTEX {
	float x, y, z, rhw;
	unsigned int color;
	float tu, tv;
};

static FONT2DVERTEX InitFont2DVertex(float x, float y, float z, float rhw,
									 unsigned int color, float tu, float tv)
{
	FONT2DVERTEX v;
	v.x = x;
	v.y = y;
	v.z = z;
	v.rhw = rhw;
	v.color = color;
	v.tu = tu;
	v.tv = tv;
	return v;
}

// STUB: ALIEN 0x434940
CD3DFont::CD3DFont(const char* p_name, int p_width, int p_height, int p_flags)
{
	strcpy(m_strFontName, p_name);
	m_dwFontHeight = p_height;
	m_dwFontWidth = p_width;
	m_pd3dDevice = 0;
	m_pTexture = 0;
	m_pVB = 0;
	m_dwSavedStateBlock = 0;
	m_dwDrawTextStateBlock = 0;
	m_dwFontFlags = p_flags;
}

// STUB: ALIEN 0x4349a0
CD3DFont::~CD3DFont()
{
	InvalidateDeviceObjects();
	DeleteDeviceObjects();
}

// STUB: ALIEN 0x4349c0
int CD3DFont::InitDeviceObjects(IDirect3DDevice8* p_device)
{
	HRESULT hr;

	m_pd3dDevice = p_device;
	m_fTextScale = 1.0f;

	if (m_dwFontHeight > 40)
		m_dwTexWidth = m_dwTexHeight = 1024;
	else if (m_dwFontHeight > 20)
		m_dwTexWidth = m_dwTexHeight = 512;
	else
		m_dwTexWidth = m_dwTexHeight = 256;

	D3DCAPS8 d3dCaps;
	m_pd3dDevice->GetDeviceCaps(&d3dCaps);
	if (m_dwTexWidth > d3dCaps.MaxTextureWidth) {
		m_fTextScale = (float) d3dCaps.MaxTextureWidth / (float) m_dwTexWidth;
		m_dwTexWidth = m_dwTexHeight = d3dCaps.MaxTextureWidth;
	}

	hr = m_pd3dDevice->CreateTexture(m_dwTexWidth, m_dwTexHeight, 1, 0,
									 D3DFMT_A4R4G4B4, D3DPOOL_MANAGED, &m_pTexture);
	if (FAILED(hr))
		return hr;

	unsigned long* pBitmapBits;
	BITMAPINFO bmi;
	memset(&bmi.bmiHeader, 0, sizeof(BITMAPINFOHEADER));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = (int) m_dwTexWidth;
	bmi.bmiHeader.biHeight = -(int) m_dwTexHeight;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biBitCount = 32;

	HDC hDC = CreateCompatibleDC(NULL);
	HBITMAP hbmBitmap = CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS,
										 (void**) &pBitmapBits, NULL, 0);
	SetMapMode(hDC, MM_TEXT);

	int nHeight = -MulDiv(m_dwFontHeight, (int) (GetDeviceCaps(hDC, LOGPIXELSY) * m_fTextScale), 72);
	int nWidth = MulDiv(m_dwFontWidth, (int) (GetDeviceCaps(hDC, LOGPIXELSX) * m_fTextScale), 72);
	unsigned long dwBold = (m_dwFontFlags & D3DFONT_BOLD) ? FW_BOLD : FW_NORMAL;
	unsigned long dwItalic = (m_dwFontFlags & D3DFONT_ITALIC) ? TRUE : FALSE;
	HFONT hFont = CreateFontA(nHeight, -nWidth, 0, 0, dwBold, dwItalic, FALSE, FALSE,
							  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
							  ANTIALIASED_QUALITY, (BYTE) (2 - ((m_dwFontFlags & 8) != 0)),
							  m_strFontName);
	if (hFont == NULL)
		return E_FAIL;

	SelectObject(hDC, hbmBitmap);
	SelectObject(hDC, hFont);
	SetTextColor(hDC, RGB(255, 255, 255));
	SetBkColor(hDC, 0x00000000);
	SetTextAlign(hDC, TA_TOP);

	unsigned int x = 0;
	unsigned int y = 0;
	char str[2];
	str[1] = 0;
	SIZE size;
	for (int c = 0; c < 256; c++) {
		str[0] = (char) c;
		GetTextExtentPoint32A(hDC, str, 1, &size);

		if ((unsigned int) (size.cx + x + 1) > m_dwTexWidth) {
			x = 0;
			y += size.cy + 1;
		}

		ExtTextOutA(hDC, x, y, ETO_OPAQUE, NULL, str, 1, NULL);

		m_fTexCoords[c][0] = ((float) (x)) / m_dwTexWidth;
		m_fTexCoords[c][1] = ((float) (y)) / m_dwTexHeight;
		m_fTexCoords[c][2] = ((float) (x + size.cx)) / m_dwTexWidth;
		m_fTexCoords[c][3] = ((float) (y + size.cy)) / m_dwTexHeight;

		x += size.cx + 1;
	}

	D3DLOCKED_RECT d3dlr;
	m_pTexture->LockRect(0, &d3dlr, 0, 0);
	unsigned char* pDstRow = (unsigned char*) d3dlr.pBits;
	for (y = 0; y < m_dwTexHeight; y++) {
		unsigned short* pDst16 = (unsigned short*) pDstRow;
		for (x = 0; x < m_dwTexWidth; x++) {
			unsigned char bAlpha = (unsigned char) ((pBitmapBits[m_dwTexWidth * y + x] & 0xff) >> 4);
			if (bAlpha > 0)
				*pDst16++ = (unsigned short) ((bAlpha << 12) | 0x0fff);
			else
				*pDst16++ = 0x0000;
		}
		pDstRow += d3dlr.Pitch;
	}
	m_pTexture->UnlockRect(0);

	DeleteObject(hbmBitmap);
	DeleteDC(hDC);
	DeleteObject(hFont);
	return S_OK;
}

// STUB: ALIEN 0x434d90
int CD3DFont::RestoreDeviceObjects()
{
	HRESULT hr = m_pd3dDevice->CreateVertexBuffer(MAX_NUM_VERTICES * sizeof(FONT2DVERTEX),
												  D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, 0,
												  D3DPOOL_DEFAULT, &m_pVB);
	if (FAILED(hr))
		return hr;

	for (unsigned int which = 0; which < 2; which++) {
		m_pd3dDevice->BeginStateBlock();
		m_pd3dDevice->SetTexture(0, m_pTexture);

		if (m_dwFontFlags & D3DFONT_ZENABLE)
			m_pd3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
		else
			m_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);

		m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		m_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		m_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		m_pd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
		m_pd3dDevice->SetRenderState(D3DRS_ALPHAREF, 0x08);
		m_pd3dDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
		m_pd3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
		m_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
		m_pd3dDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);
		m_pd3dDevice->SetRenderState(D3DRS_CLIPPING, TRUE);
		m_pd3dDevice->SetRenderState(D3DRS_EDGEANTIALIAS, FALSE);
		m_pd3dDevice->SetRenderState(D3DRS_CLIPPLANEENABLE, FALSE);
		m_pd3dDevice->SetRenderState(D3DRS_VERTEXBLEND, FALSE);
		m_pd3dDevice->SetRenderState(D3DRS_INDEXEDVERTEXBLENDENABLE, FALSE);
		m_pd3dDevice->SetRenderState(D3DRS_FOGENABLE, FALSE);
		m_pd3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		m_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		m_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
		m_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
		m_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		m_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
		m_pd3dDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_POINT);
		m_pd3dDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_POINT);
		m_pd3dDevice->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTEXF_NONE);
		m_pd3dDevice->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
		m_pd3dDevice->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
		m_pd3dDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
		m_pd3dDevice->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

		if (which == 0)
			m_pd3dDevice->EndStateBlock(&m_dwSavedStateBlock);
		else
			m_pd3dDevice->EndStateBlock(&m_dwDrawTextStateBlock);
	}

	return S_OK;
}

// STUB: ALIEN 0x435010
int CD3DFont::InvalidateDeviceObjects()
{
	SAFE_RELEASE(m_pVB);

	if (m_pd3dDevice) {
		if (m_dwSavedStateBlock)
			m_pd3dDevice->DeleteStateBlock(m_dwSavedStateBlock);
		if (m_dwDrawTextStateBlock)
			m_pd3dDevice->DeleteStateBlock(m_dwDrawTextStateBlock);
	}

	m_dwSavedStateBlock = 0;
	m_dwDrawTextStateBlock = 0;
	return S_OK;
}

// STUB: ALIEN 0x435080
int CD3DFont::DeleteDeviceObjects()
{
	SAFE_RELEASE(m_pTexture);
	m_pd3dDevice = 0;
	return S_OK;
}

// STUB: ALIEN 0x4350b0
int CD3DFont::DrawText(float p_x, float p_y, unsigned int p_color, const char* p_text, int p_flags)
{
	if (m_pd3dDevice == NULL)
		return E_FAIL;

	float sx = p_x;
	float sy = p_y;

	m_pd3dDevice->CaptureStateBlock(m_dwSavedStateBlock);
	m_pd3dDevice->ApplyStateBlock(m_dwDrawTextStateBlock);
	m_pd3dDevice->SetVertexShader(D3DFVF_FONT2DVERTEX);
	m_pd3dDevice->SetStreamSource(0, m_pVB, sizeof(FONT2DVERTEX));

	if (p_flags & D3DFONT_FILTERED) {
		m_pd3dDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
		m_pd3dDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
	}

	float fStartX = sx;

	FONT2DVERTEX* pVertices = NULL;
	unsigned int dwNumTriangles = 0;
	m_pVB->Lock(0, 0, (BYTE**) &pVertices, D3DLOCK_DISCARD);

	unsigned char c;
	while ((c = (unsigned char) *p_text++) != 0) {
		if (c == '\n') {
			sx = fStartX;
			sy += (m_fTexCoords[0][3] - m_fTexCoords[0][1]) * m_dwTexHeight;
		}

		float tx1 = m_fTexCoords[c][0];
		float ty1 = m_fTexCoords[c][1];
		float tx2 = m_fTexCoords[c][2];
		float ty2 = m_fTexCoords[c][3];

		float w = (tx2 - tx1) * m_dwTexWidth / m_fTextScale;
		float h = (ty2 - ty1) * m_dwTexHeight / m_fTextScale;

		if (c != ' ') {
			*pVertices++ = InitFont2DVertex(sx + 0 - 0.5f, sy + h - 0.5f, 0.9f, 1.0f, p_color, tx1, ty2);
			*pVertices++ = InitFont2DVertex(sx + 0 - 0.5f, sy + 0 - 0.5f, 0.9f, 1.0f, p_color, tx1, ty1);
			*pVertices++ = InitFont2DVertex(sx + w - 0.5f, sy + h - 0.5f, 0.9f, 1.0f, p_color, tx2, ty2);
			*pVertices++ = InitFont2DVertex(sx + w - 0.5f, sy + 0 - 0.5f, 0.9f, 1.0f, p_color, tx2, ty1);
			*pVertices++ = InitFont2DVertex(sx + w - 0.5f, sy + h - 0.5f, 0.9f, 1.0f, p_color, tx2, ty2);
			*pVertices++ = InitFont2DVertex(sx + 0 - 0.5f, sy + 0 - 0.5f, 0.9f, 1.0f, p_color, tx1, ty1);
			dwNumTriangles += 2;

			if (dwNumTriangles * 3 > (MAX_NUM_VERTICES - 6)) {

				m_pVB->Unlock();
				m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, dwNumTriangles);
				pVertices = NULL;
				m_pVB->Lock(0, 0, (BYTE**) &pVertices, D3DLOCK_DISCARD);
				dwNumTriangles = 0;
			}
		}

		sx += w;
	}

	m_pVB->Unlock();
	if (dwNumTriangles > 0)
		m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, dwNumTriangles);

	m_pd3dDevice->ApplyStateBlock(m_dwSavedStateBlock);
	return S_OK;
}
