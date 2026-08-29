#include "video/vid_hardware_z.h"

#include <string.h>

#include "game/gametime.h"
#include "game/map.h"
#include "gfx/asmdraw.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "gfx/texture.h"
#include "sprite/sprite.h"

extern float FSin[256];

static inline int DrawBoxInViewPort(int p_x0, int p_y0, int p_x1, int p_y1)
{
	if (p_x1 < VID::viewXMin)
		return 0;
	if (p_x0 >= VID::viewXMax)
		return 0;
	if (p_y1 < VID::viewYMin)
		return 0;
	if (p_y0 >= VID::viewYMax)
		return 0;
	return 1;
}

// FUNCTION: ALIEN 0x4127a0
VID* VID_HARDWARE_Z::CreateMirror()
{
	return new VID_HARDWARE_Z((STREAM*) this);
}

// FUNCTION: ALIEN 0x4127e0
int VID_HARDWARE_Z::SetGamma(const GAMMA& p_gamma, unsigned int p_flags)
{
	return VID::SetGamma(p_gamma, p_flags);
}

// STUB: ALIEN 0x41b0e0
int VID_HARDWARE_Z::Draw(SPRITE* p_sprite)
{
	if (!(m_unk0x47c & 0x40)) {
		int width = m_unk0x2f6;
		int x0 = (int) (p_sprite->m_x - Map->m_shiftX - width / 2);
		int y0 = (int) (p_sprite->m_y - p_sprite->m_z - Map->m_shiftY - m_messageLineHeight / 2);
		if (DrawBoxInViewPort(x0, y0, x0 + width, y0 + m_messageLineHeight)) {

			int z = (int) (p_sprite->m_z * 8.0f);
			if ((m_flag & 0x8000) && z < 0x3fff) {
				z += 0x3fff;
			} else if (m_flag & 0x10000) {
				int bob = (int) (FSin[(CurrentTime >> 3) & 0xff] * m_unk0x60 * 8.0f);
				z += bob;
				y0 += bob / -8;
			}

			short* frame = (short*) ((char*) m_unk0x48c
				+ ((int*) m_unk0x484)[p_sprite->m_noCadr]);
			frame += 3 * frame[0] + 1;
			int yTop = y0 + *frame++;
			int yEnd = yTop + *frame++;

			unsigned char* rle = (unsigned char*) frame;
			if (!(yTop >= viewYMax || yEnd < viewYMin)) {
				if (yEnd > viewYMax)
					yEnd = viewYMax;
				if (yTop < viewYMin) {
					int skip = viewYMin - yTop;
					yTop += skip;
					do {
						if (*(short*) rle) {
							int count;
							do {
								count = rle[1];
								rle += 4 * count + 2;
							} while (*(short*) rle);
						}
						rle += 2;
					} while (--skip);
				}

				RECT screen;
				screen.left = x0;
				screen.top = yTop;
				screen.right = x0 + width;
				screen.bottom = yEnd;
				RECT source;
				source.left = 0;
				source.top = 0;
				source.right = width;
				yEnd -= yTop;
				source.bottom = yEnd;

				int zpitch = ((GRAPH_CORE*) Graph)->m_unk0x250;
				frame = (short*) ((GRAPH_CORE*) Graph)->m_zbuffer;
				char* pix = (char*) ((GRAPH_CORE*) Graph)->m_texE14->Lock(&width, &source);
				width /= 2;
				*(short*) &AsmDrawData[0] = (short) z;
				char* pixEnd = pix + 2 * width * (yEnd + 1);
				short* zrow = frame + yTop * zpitch;

				unsigned short flag2 = m_pixelFlag16;
				if ((flag2 & 2) && (flag2 & 1)) {
					if (x0 >= viewXMin && x0 + m_unk0x2f6 <= viewXMax) {
						while (pix < pixEnd) {
							memset(pix, 0, 2 * width);
							int x = x0;
							while (*(short*) rle) {
								x += *rle++;
								int count = *rle++;
								short* run = (short*) rle;
								AsmDrawAlphaWithZ(run, run + count, zrow + x,
									(short*) pix + (x - x0), count);
								rle = (unsigned char*) (run + 2 * count);
								x += count;
							}
							rle += 2;
							pix += 2 * width;
							zrow += zpitch;
						}
					} else {
						while (pix < pixEnd) {
							memset(pix, 0, 2 * width);
							int x = x0;
							while (*(short*) rle) {
								x += *rle++;
								int count = *rle++;
								short* run = (short*) rle;
								int xEnd;
								if (x < viewXMin) {
									xEnd = x + count;
									if (xEnd > viewXMax)
										AsmDrawAlphaWithZ(run + (viewXMin - x),
											run + (viewXMin - x) + count, zrow + viewXMin,
											(short*) pix + (viewXMin - x0), viewXMax - viewXMin);
									else if (xEnd > viewXMin)
										AsmDrawAlphaWithZ(run + (viewXMin - x),
											run + (viewXMin - x) + count, zrow + viewXMin,
											(short*) pix + (viewXMin - x0), xEnd - viewXMin);
								} else {
									xEnd = x + count;
									if (xEnd > viewXMax) {
										if (x < viewXMax)
											AsmDrawAlphaWithZ(run, run + count, zrow + x,
												(short*) pix + (x - x0), viewXMax - x);
									} else {
										AsmDrawAlphaWithZ(run, run + count, zrow + x,
											(short*) pix + (x - x0), count);
									}
								}
								rle = (unsigned char*) (run + 2 * count);
								x = xEnd;
							}
							rle += 2;
							pix += 2 * width;
							zrow += zpitch;
						}
					}
					((GRAPH_CORE*) Graph)->SetAlphaBlend(5, 6);
				} else {
					if (x0 >= viewXMin && x0 + m_unk0x2f6 <= viewXMax) {
						while (pix < pixEnd) {
							memset(pix, 0, 2 * width);
							int x = x0;
							while (*(short*) rle) {
								x += *rle++;
								int count = *rle++;
								short* run = (short*) rle;
								AsmDrawLightWithZ(run, run + count, zrow + x,
									(short*) pix + (x - x0), count);
								rle = (unsigned char*) (run + 2 * count);
								x += count;
							}
							rle += 2;
							pix += 2 * width;
							zrow += zpitch;
						}
					} else {
						while (pix < pixEnd) {
							memset(pix, 0, 2 * width);
							int x = x0;
							while (*(short*) rle) {
								x += *rle++;
								int count = *rle++;
								short* run = (short*) rle;
								int xEnd;
								if (x < viewXMin) {
									xEnd = x + count;
									if (xEnd > viewXMax)
										AsmDrawLightWithZ(run + (viewXMin - x),
											run + (viewXMin - x) + count, zrow + viewXMin,
											(short*) pix + (viewXMin - x0), viewXMax - viewXMin);
									else if (xEnd > viewXMin)
										AsmDrawLightWithZ(run + (viewXMin - x),
											run + (viewXMin - x) + count, zrow + viewXMin,
											(short*) pix + (viewXMin - x0), xEnd - viewXMin);
								} else {
									xEnd = x + count;
									if (xEnd > viewXMax) {
										if (x < viewXMax)
											AsmDrawLightWithZ(run, run + count, zrow + x,
												(short*) pix + (x - x0), viewXMax - x);
									} else {
										AsmDrawLightWithZ(run, run + count, zrow + x,
											(short*) pix + (x - x0), count);
									}
								}
								rle = (unsigned char*) (run + 2 * count);
								x = xEnd;
							}
							rle += 2;
							pix += 2 * width;
							zrow += zpitch;
						}
					}
					((GRAPH_CORE*) Graph)->SetAlphaBlend(5, 2);
				}

				IDirect3DTexture8* texture = ((GRAPH_CORE*) Graph)->m_texE14->m_texture;
				if (texture)
					texture->UnlockRect(0);
				((GRAPH_CORE*) Graph)->SetRenderState(D3DRS_ZFUNC, 8);

				if (m_flag & 0x800) {
					GAMMA total;
					total.Add(*(GAMMA*) &m_colorSub, GAMMA(GAMMA::RAW_COPY, p_sprite->GetGamma()));
					((GRAPH_CORE*) Graph)->m_texE14->Draw(&screen, &source, &total);
				} else {
					int graphNeg = ((GRAPH_CORE*) Graph)->m_gammaSet.m_a;
					int graphPos = ((GRAPH_CORE*) Graph)->m_gammaSet.m_b;
					GAMMA total;
					total.Add(*(GAMMA*) &m_colorSub, GAMMA(GAMMA::RAW_COPY, p_sprite->GetGamma()));
					GAMMA final;
					final.Add(total, GAMMA(GAMMA::RAW_COPY, graphNeg, graphPos));
					((GRAPH_CORE*) Graph)->m_texE14->Draw(&screen, &source, &final);
				}
			}
		}
	}
	if (0)
		return 0;
}

// FUNCTION: ALIEN 0x41b880
void VID_HARDWARE_Z::SetLayer()
{
	if (m_flag & 0x40000000) {
		m_layer = 4;
		return;
	}
	if (m_unk0x0c == 0x40) {
		m_layer = 0xa;
		return;
	}
	unsigned short pf = *(unsigned short*) &m_pixelFlag;
	if (pf & 4) {
		if (pf & 2)
			m_layer = 0xc;
		else
			m_layer = 8;
	}
	else if (pf & 2) {
		if (m_flag & 0x10000)
			m_layer = 9;
		else
			m_layer = 0xc;
	}
	else
		m_layer = 8;
}
