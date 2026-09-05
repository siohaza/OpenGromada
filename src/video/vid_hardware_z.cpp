#include "video/vid_hardware_z.h"

#include "game/game_descriptor.h"
#include "game/gametime.h"
#include "game/map.h"
#include "gfx/asmdraw.h"
#include "gfx/gpu_backend.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "gfx/texture.h"
#include "sprite/sprite.h"
#include "util/packed.h"

#include <cmath>
#include <limits>
#include <string.h>
#include <vector>

extern float FSin[256];

inline static int DrawBoxInViewPort(int p_x0, int p_y0, int p_x1, int p_y1)
{
	if (p_x1 < VID::ViewXMin()) {
		return 0;
	}
	if (p_x0 >= VID::ViewXMax()) {
		return 0;
	}
	if (p_y1 < VID::ViewYMin()) {
		return 0;
	}
	if (p_y0 >= VID::ViewYMax()) {
		return 0;
	}
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
	if (!p_sprite || !Map) {
		return 0;
	}
	return DrawFrame(p_sprite->m_noCadr,
					 p_sprite->m_x,
					 p_sprite->m_y,
					 p_sprite->m_z,
					 Map->m_shiftX,
					 Map->m_shiftY,
					 p_sprite->GetGamma());
}

int VID_HARDWARE_Z::DrawFrame(int p_frame,
							  float p_x,
							  float p_y,
							  float p_z,
							  float p_shiftX,
							  float p_shiftY,
							  const GAMMA& p_gamma)
{
	GRAPH_CORE* graph = (GRAPH_CORE*) Graph;
	if (!graph || !m_unk0x48c || !m_unk0x484 || p_frame < 0 || p_frame >= m_dotFrameCount || m_unk0x2f6 <= 0 ||
		m_messageLineHeight <= 0) {
		return 0;
	}

	if (!PropHide()) {
		int width = m_unk0x2f6;
		float drawX = p_x - p_shiftX - width / 2;
		float drawY = p_y - p_z - p_shiftY - m_messageLineHeight / 2;
		float drawZ = p_z * 8.0f;
		const double limit = std::numeric_limits<int>::max() - 65536.0;
		if (!std::isfinite(drawX) || !std::isfinite(drawY) || !std::isfinite(drawZ) ||
			std::abs(double(drawX)) >= limit || std::abs(double(drawY)) >= limit || std::abs(double(drawZ)) >= limit) {
			return 0;
		}
		int x0 = (int) drawX;
		int y0 = (int) drawY;
		if (DrawBoxInViewPort(x0, y0, x0 + width, y0 + m_messageLineHeight)) {

			int z = (int) drawZ;
			if ((m_flag & 0x8000) && z < 0x3fff) {
				z += 0x3fff;
			}
			else if (m_flag & 0x10000) {
				float displacement = FSin[(CurrentTime >> 3) & 0xff] * m_unk0x60 * 8.0f;
				if (!std::isfinite(displacement) || std::abs(double(displacement)) >= limit ||
					std::abs(double(z) + displacement) >= limit || std::abs(double(y0) - displacement / 8) >= limit) {
					return 0;
				}
				int bob = (int) displacement;
				z += bob;
				y0 += bob / -8;
			}

			int frameOffset = m_unk0x484[p_frame];
			if (frameOffset < 0 || frameOffset > m_unk0x488 - 6) {
				return 0;
			}
			unsigned char* frame = (unsigned char*) m_unk0x48c + frameOffset;
			short contourCount = PackedRead<short>(frame);
			if (contourCount < 0 || 6 * size_t(contourCount) + 6 > size_t(m_unk0x488 - frameOffset)) {
				if (GPU_RENDER::Active()) {
					GPU_RENDER::Fail("Invalid soft-Z sprite header");
				}
				return 0;
			}
			unsigned char* header = frame + 6 * contourCount + 2;
			int yTop = y0 + PackedRead<short>(header);
			int yEnd = yTop + PackedRead<short>(header + 2);

			unsigned char* rle = header + 4;
			if (GPU_RENDER::Active()) {
				const unsigned char* end = (const unsigned char*) m_unk0x48c + m_unk0x488;
				const unsigned char* cursor = rle;
				for (int row = yTop; row < yEnd; ++row) {
					unsigned x = 0;
					for (;;) {
						if (size_t(end - cursor) < 2) {
							GPU_RENDER::Fail("Truncated soft-Z sprite row");
							return 0;
						}
						unsigned skip = cursor[0], count = cursor[1];
						cursor += 2;
						if (!skip && !count) {
							break;
						}
						x += skip + count;
						if (x > unsigned(width) || size_t(end - cursor) < 4 * count) {
							GPU_RENDER::Fail("Invalid soft-Z sprite span");
							return 0;
						}
						cursor += 4 * count;
					}
				}
			}
			if (!(yTop >= ViewYMax() || yEnd <= ViewYMin())) {
				if (yEnd > ViewYMax()) {
					yEnd = ViewYMax();
				}
				if (yTop < ViewYMin()) {
					int skip = ViewYMin() - yTop;
					yTop += skip;
					do {
						if (PackedRleRun(rle)) {
							int count;
							do {
								count = rle[1];
								rle += 4 * count + 2;
							} while (PackedRleRun(rle));
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

				if (GPU_RENDER::Active()) {
					if (!graph->m_texE14 || source.right > graph->m_texE14->m_width ||
						source.bottom > graph->m_texE14->m_height) {
						return 0;
					}
					static std::vector<uint32_t> rows;
					rows.clear();
					const unsigned char* base = (const unsigned char*) m_unk0x48c;
					const unsigned char* end = base + m_unk0x488;
					for (int row = 0; row < yEnd; ++row) {
						rows.push_back(uint32_t(rle - base));
						while (rle + 2 <= end && PackedRleRun(rle)) {
							const size_t bytes = 4 * size_t(rle[1]) + 2;
							if (bytes > size_t(end - rle)) {
								GPU_RENDER::Fail("Truncated soft-Z sprite run");
								return 0;
							}
							rle += bytes;
						}
						if (rle + 2 > end) {
							GPU_RENDER::Fail("Truncated soft-Z sprite row");
							return 0;
						}
						rle += 2;
					}
					GPU_RENDER::Command command;
					command.op = 500;
					command.right = graph->m_texE14->m_width;
					command.bottom = yEnd;
					command.p[0] = GPU_RENDER::Upload(m_unk0x48c, m_unk0x48c, m_unk0x488);
					command.p[1] = GPU_RENDER::Upload(&rows, rows.data(), rows.size() * sizeof(uint32_t));
					command.p[2] = graph->m_texE14->GpuData();
					command.p[3] = graph->m_texE14->m_width;
					command.p[4] = x0;
					command.p[5] = yTop;
					command.p[6] = z;
					command.p[7] = (m_pixelFlag16 & 3) == 3;
					command.p[8] = ViewXMin();
					command.p[9] = ViewXMax();
					command.p[10] = m_unk0x488;
					GPU_RENDER::Submit(command, true);
					graph->m_texE14->GpuWritten();
					graph->SetAlphaBlend(5, command.p[7] ? 6 : 2);
				}
				else {
					if (!graph->m_zbuffer || graph->m_zpitch < (int) graph->m_width || !graph->m_texE14 ||
						source.right > graph->m_texE14->m_width || source.bottom > graph->m_texE14->m_height) {
						return 0;
					}
					int zpitch = graph->m_zpitch;
					short* zbuffer = (short*) graph->m_zbuffer;
					char* pix = (char*) graph->m_texE14->Lock(&width, &source);
					if (!pix || width < 2 * source.right) {
						return 0;
					}
					width /= 2;
					char* pixEnd = pix + 2 * width * yEnd;
					short* zrow = zbuffer + yTop * zpitch;

					unsigned short flag2 = m_pixelFlag16;
					if ((flag2 & 2) && (flag2 & 1)) {
						if (x0 >= ViewXMin() && x0 + m_unk0x2f6 <= ViewXMax()) {
							while (pix < pixEnd) {
								memset(pix, 0, 2 * width);
								int x = x0;
								while (PackedRleRun(rle)) {
									x += *rle++;
									int count = *rle++;
									unsigned char* run = rle;
									AsmDrawAlphaWithZ(run,
													  run + 2 * count,
													  zrow + x,
													  (short*) pix + (x - x0),
													  count,
													  z);
									rle = run + 4 * count;
									x += count;
								}
								rle += 2;
								pix += 2 * width;
								zrow += zpitch;
							}
						}
						else {
							while (pix < pixEnd) {
								memset(pix, 0, 2 * width);
								int x = x0;
								while (PackedRleRun(rle)) {
									x += *rle++;
									int count = *rle++;
									unsigned char* run = rle;
									int xEnd;
									if (x < ViewXMin()) {
										xEnd = x + count;
										if (xEnd > ViewXMax()) {
											AsmDrawAlphaWithZ(run + 2 * (ViewXMin() - x),
															  run + 2 * (ViewXMin() - x + count),
															  zrow + ViewXMin(),
															  (short*) pix + (ViewXMin() - x0),
															  ViewXMax() - ViewXMin(),
															  z);
										}
										else if (xEnd > ViewXMin()) {
											AsmDrawAlphaWithZ(run + 2 * (ViewXMin() - x),
															  run + 2 * (ViewXMin() - x + count),
															  zrow + ViewXMin(),
															  (short*) pix + (ViewXMin() - x0),
															  xEnd - ViewXMin(),
															  z);
										}
									}
									else {
										xEnd = x + count;
										if (xEnd > ViewXMax()) {
											if (x < ViewXMax()) {
												AsmDrawAlphaWithZ(run,
																  run + 2 * count,
																  zrow + x,
																  (short*) pix + (x - x0),
																  ViewXMax() - x,
																  z);
											}
										}
										else {
											AsmDrawAlphaWithZ(run,
															  run + 2 * count,
															  zrow + x,
															  (short*) pix + (x - x0),
															  count,
															  z);
										}
									}
									rle = run + 4 * count;
									x = xEnd;
								}
								rle += 2;
								pix += 2 * width;
								zrow += zpitch;
							}
						}
						graph->SetAlphaBlend(5, 6);
					}
					else {
						if (x0 >= ViewXMin() && x0 + m_unk0x2f6 <= ViewXMax()) {
							while (pix < pixEnd) {
								memset(pix, 0, 2 * width);
								int x = x0;
								while (PackedRleRun(rle)) {
									x += *rle++;
									int count = *rle++;
									unsigned char* run = rle;
									AsmDrawLightWithZ(run,
													  run + 2 * count,
													  zrow + x,
													  (short*) pix + (x - x0),
													  count,
													  z);
									rle = run + 4 * count;
									x += count;
								}
								rle += 2;
								pix += 2 * width;
								zrow += zpitch;
							}
						}
						else {
							while (pix < pixEnd) {
								memset(pix, 0, 2 * width);
								int x = x0;
								while (PackedRleRun(rle)) {
									x += *rle++;
									int count = *rle++;
									unsigned char* run = rle;
									int xEnd;
									if (x < ViewXMin()) {
										xEnd = x + count;
										if (xEnd > ViewXMax()) {
											AsmDrawLightWithZ(run + 2 * (ViewXMin() - x),
															  run + 2 * (ViewXMin() - x + count),
															  zrow + ViewXMin(),
															  (short*) pix + (ViewXMin() - x0),
															  ViewXMax() - ViewXMin(),
															  z);
										}
										else if (xEnd > ViewXMin()) {
											AsmDrawLightWithZ(run + 2 * (ViewXMin() - x),
															  run + 2 * (ViewXMin() - x + count),
															  zrow + ViewXMin(),
															  (short*) pix + (ViewXMin() - x0),
															  xEnd - ViewXMin(),
															  z);
										}
									}
									else {
										xEnd = x + count;
										if (xEnd > ViewXMax()) {
											if (x < ViewXMax()) {
												AsmDrawLightWithZ(run,
																  run + 2 * count,
																  zrow + x,
																  (short*) pix + (x - x0),
																  ViewXMax() - x,
																  z);
											}
										}
										else {
											AsmDrawLightWithZ(run,
															  run + 2 * count,
															  zrow + x,
															  (short*) pix + (x - x0),
															  count,
															  z);
										}
									}
									rle = run + 4 * count;
									x = xEnd;
								}
								rle += 2;
								pix += 2 * width;
								zrow += zpitch;
							}
						}
						graph->SetAlphaBlend(5, 2);
					}
				}
				graph->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);

				if (m_flag & 0x800) {
					GAMMA total;
					total.Add(GAMMA(GAMMA::RAW_COPY, m_colorSub, m_colorAdd), GAMMA(GAMMA::RAW_COPY, p_gamma));
					graph->m_texE14->Draw(&screen, &source, &total);
				}
				else {
					int graphNeg = graph->m_gammaSet.m_a;
					int graphPos = graph->m_gammaSet.m_b;
					GAMMA total;
					total.Add(GAMMA(GAMMA::RAW_COPY, m_colorSub, m_colorAdd), GAMMA(GAMMA::RAW_COPY, p_gamma));
					GAMMA final;
					final.Add(total, GAMMA(GAMMA::RAW_COPY, graphNeg, graphPos));
					graph->m_texE14->Draw(&screen, &source, &final);
				}
			}
		}
	}
	return 0;
}

// FUNCTION: ALIEN 0x41b880
void VID_HARDWARE_Z::SetLayer()
{
	if (GameDesc->m_layerRules == GAME_LAYERS_LOCOLAND) {

		if (m_flag & 0x40000000) {
			m_layer = 1;
		}
		else if (m_unk0x0c == 64) {
			m_layer = 7;
		}
		else if (!(m_pixelFlag16 & 2)) {
			m_layer = 5;
		}
		else {
			m_layer = (!(m_pixelFlag16 & 4) && (m_flag & 0x10000)) ? 6 : 9;
		}
		return;
	}
	if (m_flag & 0x40000000) {
		m_layer = 4;
		return;
	}
	if (m_unk0x0c == 0x40) {
		m_layer = 0xa;
		return;
	}
	unsigned short pf = m_pixelFlag16;
	if (pf & 4) {
		if (pf & 2) {
			m_layer = 0xc;
		}
		else {
			m_layer = 8;
		}
	}
	else if (pf & 2) {
		if (m_flag & 0x10000) {
			m_layer = 9;
		}
		else {
			m_layer = 0xc;
		}
	}
	else {
		m_layer = 8;
	}
}
