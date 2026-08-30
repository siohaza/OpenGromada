#ifndef GRAPH_H
#define GRAPH_H

#include "gfx/color.h"
#include "gfx/graph_core.h"
#include "util/angle.h"
#include "util/decomp.h"
#include "util/stream.h"
#include "util/string.h"

class VID;

class GRAPH : public GRAPH_CORE {
public:
	GRAPH(SETTINGS* p_settings) : GRAPH_CORE(p_settings) {}

	int Init();
	int ScreenShot(STRING* p_name, int p_x, int p_y, int p_w, int p_h);
	void DrawDebugText(const char* p_format, ...) DECOMP_PRINTF(2, 3);
	void DrawLoadBar(VID* p_vid);
	void Tact(int p_draw);
	void DrawSquall();
	int ShadowBar(float p_x, float p_y, float p_x1, float p_y1, unsigned int p_color);
	int LightBar(float p_x, float p_y, float p_x1, float p_y1, unsigned int p_color);
	void DrawEffect(int p_effect);
	void DrawSnow();
	void DrawRain();
	void DrawSnowflakes();
	STRING m_fontName; // 0xe2c

	void DrawLight(float p_x, float p_y, float p_z, int p_a, int p_b, unsigned int p_color);
	void PutPixel(float p_x, float p_y, COLOR p_color);
	void PutBigPixel(float p_x, float p_y, COLOR p_color);
	void Box(float p_x, float p_y, float p_x1, float p_y1, COLOR p_color);
	void DrawFog(
		float p_x0,
		float p_y0,
		float p_x1,
		float p_y1,
		int p_zTop,
		int p_zBottom,
		COLOR p_color,
		const unsigned short* p_ramp,
		int p_zBase,
		int p_blend
	);
	void DrawVid(VID* p_vid, int p_cadr, float p_x, float p_y, float p_z);
	void Line(float p_x, float p_y, float p_x1, float p_y1, COLOR p_color);
	int PlayMovie(const char* p_filename);
	int SetEnvironment(int p_env);
	void SetGamma(const GAMMA& p_gamma);
	unsigned char* OldLoadParameters(STREAM* p_stream);
	int LoadParameters(STREAM* p_stream);
	int SaveParameters(STREAM* p_stream);
	unsigned int GetEffectState(int p_effect) const;
	unsigned char* SetWind(int p_force, ANGLE p_direction);
	unsigned char* WindDirection(unsigned char* p_out);
};

extern GRAPH* Graph;

#endif
