#ifndef D3DFONT_H
#define D3DFONT_H

#include <dxsdk/d3d8.h>

#ifdef DrawText
#undef DrawText
#endif

class CD3DFont {
public:
	char m_strFontName[0x50]; // 0x000
	unsigned int m_dwFontHeight; // 0x050
	unsigned int m_dwFontWidth; // 0x054
	unsigned int m_dwFontFlags; // 0x058
	IDirect3DDevice8* m_pd3dDevice; // 0x05c
	IDirect3DTexture8* m_pTexture; // 0x060
	IDirect3DVertexBuffer8* m_pVB; // 0x064
	unsigned int m_dwTexWidth; // 0x068
	unsigned int m_dwTexHeight; // 0x06c
	float m_fTextScale; // 0x070
	float m_fTexCoords[256][4]; // 0x074
	unsigned long m_dwSavedStateBlock; // 0x1074
	unsigned long m_dwDrawTextStateBlock; // 0x1078

	CD3DFont(const char* p_name, int p_width, int p_height, int p_flags);
	~CD3DFont();
	int InitDeviceObjects(IDirect3DDevice8* p_device);
	int RestoreDeviceObjects();
	int InvalidateDeviceObjects();
	int DeleteDeviceObjects();
	int DrawText(float p_x, float p_y, unsigned int p_color, const char* p_text, int p_flags);
};

#endif
