#include "gfx/texture.h"

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__x86_64__) || defined(_M_X64)
#include <emmintrin.h>
#define ALIEN_TEXTURE_SSE2 1
#elif defined(__aarch64__) || defined(__arm64__)
#include <arm_neon.h>
#define ALIEN_TEXTURE_NEON 1
#endif

#include "gfx/gamma.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "util/myerror.h"
#include "util/stream.h"
#include "util/string.h"

// GLOBAL: ALIEN 0x4905c8
int TextureMemoryInUse;

// GLOBAL: ALIEN 0x47f084
int g_textureMaxWidth = 4096;
// GLOBAL: ALIEN 0x47f088
int g_textureMaxHeight = 4096;

int TEXTURE::BitsPerPixel() const
{
	switch (m_format) {
	case D3DFMT_A8R8G8B8:
	case D3DFMT_X8R8G8B8:
		return 32;
	case D3DFMT_R8G8B8:
		return 24;
	case D3DFMT_R5G6B5:
	case D3DFMT_X1R5G5B5:
	case D3DFMT_A1R5G5B5:
	case D3DFMT_A4R4G4B4:
	case D3DFMT_D16:
		return 16;
	case D3DFMT_P8:
	case D3DFMT_A8:
		return 8;
	default:
		return 0;
	}
}

static int FormatBits(int p_format)
{
	switch (p_format) {
	case D3DFMT_A8R8G8B8:
	case D3DFMT_X8R8G8B8:
		return 32;
	case D3DFMT_R8G8B8:
		return 24;
	case D3DFMT_R5G6B5:
	case D3DFMT_X1R5G5B5:
	case D3DFMT_A1R5G5B5:
	case D3DFMT_A4R4G4B4:
	case D3DFMT_D16:
		return 16;
	case D3DFMT_P8:
	case D3DFMT_A8:
		return 8;
	default:
		return 0;
	}
}

// FUNCTION: ALIEN 0x403290
TEXTURE::TEXTURE(int p_width, int p_height, int p_format, unsigned int p_flags)
{
	Create(p_width, p_height, p_format, p_flags);
}

// STUB: ALIEN 0x4032e0
TEXTURE::TEXTURE(int p_width, int p_height, int p_format, unsigned int p_flags, const void* p_palette, STREAM* p_stream)
{
	Create(p_width, p_height, p_format, p_flags);
	if (p_palette && m_format == D3DFMT_P8) {
		SetPalette(p_palette);
	}

	// Preserve the authored format; the software renderer supports each asset format.
	int bytes = FormatBits(p_format) * p_width * p_height / 8;
	if (bytes <= 0) {
		return;
	}
	if (!m_data) {
		// Preserve stream alignment after allocation failure.
		void* discard = malloc((size_t) bytes);
		if (discard) {
			p_stream->Read(discard, bytes);
			free(discard);
		}
		return;
	}
	p_stream->Read(m_data, bytes);
}

// FUNCTION: ALIEN 0x403620
TEXTURE::~TEXTURE()
{
	if (m_data) {
		TextureMemoryInUse -= m_width * m_height * ((m_format != D3DFMT_P8) + 1);
		free(m_data);
		m_data = 0;
	}
	free(m_palette);
	m_palette = 0;
}

// FUNCTION: ALIEN 0x4036b0
void TEXTURE::Create(int p_width, int p_height, int p_format, unsigned int p_flags)
{
	m_data = 0;
	m_palette = 0;
	m_pitch = 0;
	m_width = p_width;
	m_height = p_height;
	m_format = p_format;
	m_flags = p_flags;

	if (p_width > g_textureMaxWidth || p_width <= 0) {
		if (::Error) {
			MYERROR::Error(
				::Error,
				"TEXTURE",
				4,
				// STRING: ALIEN 0x47f410
				"initial sizeX",
				p_width
			);
		}
		return;
	}
	if (p_height > g_textureMaxHeight || p_height <= 0) {
		if (::Error) {
			MYERROR::Error(
				::Error,
				"TEXTURE",
				4,
				// STRING: ALIEN 0x47f420
				"initial sizeY",
				p_height
			);
		}
		return;
	}

	if (p_format == D3DFMT_A8R8G8B8 || p_format == D3DFMT_A4R4G4B4 || p_format == D3DFMT_A1R5G5B5) {
		m_flags |= 8;
	}

	int bits = FormatBits(p_format);
	if (bits == 0) {
		if (::Error) {
			MYERROR::Error(::Error, "TEXTURE", 3, empty_str, p_format);
		}
		return;
	}

	m_pitch = bits * p_width / 8;
	m_data = calloc((size_t) m_pitch * (size_t) p_height, 1);
	if (!m_data) {
		if (::Error) {
			MYERROR::Error(::Error, "TEXTURE", 3, empty_str, 0);
		}
		return;
	}
	TextureMemoryInUse += m_width * m_height * ((m_format != D3DFMT_P8) + 1);
}

// FUNCTION: ALIEN 0x403970
int TEXTURE::CopyFromScreen(const RECT* p_rect, const POINT* p_point)
{
	GRAPH_CORE* graph = (GRAPH_CORE*) Graph;
	if (!m_data || !graph->m_color) {
		return 1;
	}

	int w = (int) (p_rect->right - p_rect->left);
	int h = (int) (p_rect->bottom - p_rect->top);
	if (w <= 0 || h <= 0) {
		return 1;
	}
	if (p_point->x + w > m_width) {
		w = m_width - (int) p_point->x;
	}
	if (p_point->y + h > m_height) {
		h = m_height - (int) p_point->y;
	}
	if (w <= 0 || h <= 0) {
		return 1;
	}

	const unsigned int* src =
		(const unsigned int*) graph->m_color + (size_t) p_rect->top * graph->m_pitch + p_rect->left;

	if (m_format == D3DFMT_A8R8G8B8 || m_format == D3DFMT_X8R8G8B8) {
		unsigned int* dst = (unsigned int*) m_data + (size_t) p_point->y * m_width + p_point->x;
		for (int y = 0; y < h; ++y) {
			memcpy(dst, src, (size_t) w * sizeof(unsigned int));
			src += graph->m_pitch;
			dst += m_width;
		}
		return 0;
	}

	if (m_format == D3DFMT_R5G6B5) {
		unsigned short* dst = (unsigned short*) m_data + (size_t) p_point->y * m_width + p_point->x;
		for (int y = 0; y < h; ++y) {
			for (int x = 0; x < w; ++x) {
				unsigned int c = src[x];
				dst[x] = (unsigned short) (((c >> 8) & 0xf800) | ((c >> 5) & 0x07e0) | ((c >> 3) & 0x001f));
			}
			src += graph->m_pitch;
			dst += m_width;
		}
		return 0;
	}

	return 1;
}

// FUNCTION: ALIEN 0x403a40
void* TEXTURE::Lock(int* p_pitch, const RECT* p_rect)
{
	if (p_pitch) {
		*p_pitch = m_pitch;
	}
	if (!m_data) {
		return 0;
	}
	if (!p_rect) {
		return m_data;
	}
	int bytes = BitsPerPixel() / 8;
	return (char*) m_data + m_pitch * p_rect->top + bytes * p_rect->left;
}

inline static unsigned int Expand5(unsigned int p_v)
{
	return (p_v << 3) | (p_v >> 2);
}

inline static unsigned int Expand6(unsigned int p_v)
{
	return (p_v << 2) | (p_v >> 4);
}

inline static unsigned int Expand4(unsigned int p_v)
{
	return (p_v << 4) | p_v;
}

inline static unsigned int Expand565(unsigned int p_v)
{
	return 0xff000000u | (Expand5(p_v >> 11) << 16) | (Expand6((p_v >> 5) & 0x3f) << 8) | Expand5(p_v & 0x1f);
}

inline static unsigned int ModulateChannel(
	unsigned int p_channel,
	unsigned int p_multiplier,
	unsigned int p_specular,
	int p_shift
)
{
	unsigned int result = (p_channel * p_multiplier >> p_shift) + p_specular;
	return result > 255 ? 255 : result;
}

static void OpaqueRgb565PointRowScalar(
	const unsigned short* p_src,
	unsigned int* p_dst,
	int p_count,
	unsigned int p_dr,
	unsigned int p_dg,
	unsigned int p_db,
	unsigned int p_sr,
	unsigned int p_sg,
	unsigned int p_sb,
	int p_shift
)
{
	for (int i = 0; i < p_count; ++i) {
		unsigned int texel = Expand565(p_src[i]);
		unsigned int r = ModulateChannel((texel >> 16) & 0xff, p_dr + 1, p_sr, p_shift);
		unsigned int g = ModulateChannel((texel >> 8) & 0xff, p_dg + 1, p_sg, p_shift);
		unsigned int b = ModulateChannel(texel & 0xff, p_db + 1, p_sb, p_shift);
		p_dst[i] = 0xff000000u | (r << 16) | (g << 8) | b;
	}
}

#if defined(ALIEN_TEXTURE_SSE2)

inline static __m128i Clamp255U16Sse2(__m128i p_value)
{
	const __m128i maximum = _mm_set1_epi16(255);
	__m128i over = _mm_cmpgt_epi16(p_value, maximum);
	return _mm_or_si128(_mm_and_si128(over, maximum), _mm_andnot_si128(over, p_value));
}

template <int Shift>
static void OpaqueRgb565PointRowSse2(
	const unsigned short* p_src,
	unsigned int* p_dst,
	int p_count,
	unsigned int p_dr,
	unsigned int p_dg,
	unsigned int p_db,
	unsigned int p_sr,
	unsigned int p_sg,
	unsigned int p_sb
)
{
	const __m128i zero = _mm_setzero_si128();
	const __m128i mask5 = _mm_set1_epi16(0x1f);
	const __m128i mask6 = _mm_set1_epi16(0x3f);
	const __m128i alpha = _mm_set1_epi32((int) 0xff000000u);
	const __m128i mr = _mm_set1_epi16((short) (p_dr + 1));
	const __m128i mg = _mm_set1_epi16((short) (p_dg + 1));
	const __m128i mb = _mm_set1_epi16((short) (p_db + 1));
	const __m128i sr = _mm_set1_epi16((short) p_sr);
	const __m128i sg = _mm_set1_epi16((short) p_sg);
	const __m128i sb = _mm_set1_epi16((short) p_sb);
	int i = 0;
	for (; i + 8 <= p_count; i += 8) {
		__m128i packed = _mm_loadu_si128((const __m128i*) (p_src + i));
		__m128i r5 = _mm_srli_epi16(packed, 11);
		__m128i g6 = _mm_and_si128(_mm_srli_epi16(packed, 5), mask6);
		__m128i b5 = _mm_and_si128(packed, mask5);
		__m128i r = _mm_or_si128(_mm_slli_epi16(r5, 3), _mm_srli_epi16(r5, 2));
		__m128i g = _mm_or_si128(_mm_slli_epi16(g6, 2), _mm_srli_epi16(g6, 4));
		__m128i b = _mm_or_si128(_mm_slli_epi16(b5, 3), _mm_srli_epi16(b5, 2));
		r = Clamp255U16Sse2(_mm_add_epi16(_mm_srli_epi16(_mm_mullo_epi16(r, mr), Shift), sr));
		g = Clamp255U16Sse2(_mm_add_epi16(_mm_srli_epi16(_mm_mullo_epi16(g, mg), Shift), sg));
		b = Clamp255U16Sse2(_mm_add_epi16(_mm_srli_epi16(_mm_mullo_epi16(b, mb), Shift), sb));

		__m128i out0 = _mm_or_si128(
			alpha,
			_mm_or_si128(
				_mm_slli_epi32(_mm_unpacklo_epi16(r, zero), 16),
				_mm_or_si128(_mm_slli_epi32(_mm_unpacklo_epi16(g, zero), 8), _mm_unpacklo_epi16(b, zero))
			)
		);
		__m128i out1 = _mm_or_si128(
			alpha,
			_mm_or_si128(
				_mm_slli_epi32(_mm_unpackhi_epi16(r, zero), 16),
				_mm_or_si128(_mm_slli_epi32(_mm_unpackhi_epi16(g, zero), 8), _mm_unpackhi_epi16(b, zero))
			)
		);
		_mm_storeu_si128((__m128i*) (p_dst + i), out0);
		_mm_storeu_si128((__m128i*) (p_dst + i + 4), out1);
	}
	OpaqueRgb565PointRowScalar(p_src + i, p_dst + i, p_count - i, p_dr, p_dg, p_db, p_sr, p_sg, p_sb, Shift);
}

#elif defined(ALIEN_TEXTURE_NEON)

template <int Shift>
static void OpaqueRgb565PointRowNeon(
	const unsigned short* p_src,
	unsigned int* p_dst,
	int p_count,
	unsigned int p_dr,
	unsigned int p_dg,
	unsigned int p_db,
	unsigned int p_sr,
	unsigned int p_sg,
	unsigned int p_sb
)
{
	const uint16x8_t mask5 = vdupq_n_u16(0x1f);
	const uint16x8_t mask6 = vdupq_n_u16(0x3f);
	const uint32x4_t alpha = vdupq_n_u32(0xff000000u);
	const uint16x8_t mr = vdupq_n_u16((unsigned short) (p_dr + 1));
	const uint16x8_t mg = vdupq_n_u16((unsigned short) (p_dg + 1));
	const uint16x8_t mb = vdupq_n_u16((unsigned short) (p_db + 1));
	const uint16x8_t sr = vdupq_n_u16((unsigned short) p_sr);
	const uint16x8_t sg = vdupq_n_u16((unsigned short) p_sg);
	const uint16x8_t sb = vdupq_n_u16((unsigned short) p_sb);
	const uint16x8_t maximum = vdupq_n_u16(255);
	int i = 0;
	for (; i + 8 <= p_count; i += 8) {
		uint16x8_t packed = vld1q_u16(p_src + i);
		uint16x8_t r5 = vshrq_n_u16(packed, 11);
		uint16x8_t g6 = vandq_u16(vshrq_n_u16(packed, 5), mask6);
		uint16x8_t b5 = vandq_u16(packed, mask5);
		uint16x8_t r = vorrq_u16(vshlq_n_u16(r5, 3), vshrq_n_u16(r5, 2));
		uint16x8_t g = vorrq_u16(vshlq_n_u16(g6, 2), vshrq_n_u16(g6, 4));
		uint16x8_t b = vorrq_u16(vshlq_n_u16(b5, 3), vshrq_n_u16(b5, 2));
		r = vminq_u16(vaddq_u16(vshrq_n_u16(vmulq_u16(r, mr), Shift), sr), maximum);
		g = vminq_u16(vaddq_u16(vshrq_n_u16(vmulq_u16(g, mg), Shift), sg), maximum);
		b = vminq_u16(vaddq_u16(vshrq_n_u16(vmulq_u16(b, mb), Shift), sb), maximum);

		uint32x4_t out0 = vorrq_u32(
			alpha,
			vorrq_u32(
				vshlq_n_u32(vmovl_u16(vget_low_u16(r)), 16),
				vorrq_u32(vshlq_n_u32(vmovl_u16(vget_low_u16(g)), 8), vmovl_u16(vget_low_u16(b)))
			)
		);
		uint32x4_t out1 = vorrq_u32(
			alpha,
			vorrq_u32(
				vshlq_n_u32(vmovl_u16(vget_high_u16(r)), 16),
				vorrq_u32(vshlq_n_u32(vmovl_u16(vget_high_u16(g)), 8), vmovl_u16(vget_high_u16(b)))
			)
		);
		vst1q_u32(p_dst + i, out0);
		vst1q_u32(p_dst + i + 4, out1);
	}
	OpaqueRgb565PointRowScalar(p_src + i, p_dst + i, p_count - i, p_dr, p_dg, p_db, p_sr, p_sg, p_sb, Shift);
}

#endif

static void OpaqueRgb565PointRow(
	const unsigned short* p_src,
	unsigned int* p_dst,
	int p_count,
	unsigned int p_dr,
	unsigned int p_dg,
	unsigned int p_db,
	unsigned int p_sr,
	unsigned int p_sg,
	unsigned int p_sb,
	int p_shift
)
{
#if defined(ALIEN_TEXTURE_SSE2)
	if (p_shift == 7) {
		OpaqueRgb565PointRowSse2<7>(p_src, p_dst, p_count, p_dr, p_dg, p_db, p_sr, p_sg, p_sb);
	}
	else {
		OpaqueRgb565PointRowSse2<8>(p_src, p_dst, p_count, p_dr, p_dg, p_db, p_sr, p_sg, p_sb);
	}
#elif defined(ALIEN_TEXTURE_NEON)
	if (p_shift == 7) {
		OpaqueRgb565PointRowNeon<7>(p_src, p_dst, p_count, p_dr, p_dg, p_db, p_sr, p_sg, p_sb);
	}
	else {
		OpaqueRgb565PointRowNeon<8>(p_src, p_dst, p_count, p_dr, p_dg, p_db, p_sr, p_sg, p_sb);
	}
#else
	OpaqueRgb565PointRowScalar(p_src, p_dst, p_count, p_dr, p_dg, p_db, p_sr, p_sg, p_sb, p_shift);
#endif
}

inline static unsigned int SampleTexel(const TEXTURE* p_tex, int p_x, int p_y)
{
	const unsigned char* row = (const unsigned char*) p_tex->m_data + (size_t) p_y * p_tex->m_pitch;

	switch (p_tex->m_format) {
	case D3DFMT_A8R8G8B8:
		return ((const unsigned int*) row)[p_x];
	case D3DFMT_X8R8G8B8:
		return 0xff000000u | ((const unsigned int*) row)[p_x];
	case D3DFMT_R8G8B8: {
		const unsigned char* px = row + 3 * p_x;
		return 0xff000000u | ((unsigned int) px[2] << 16) | ((unsigned int) px[1] << 8) | px[0];
	}
	case D3DFMT_R5G6B5: {
		unsigned int v = ((const unsigned short*) row)[p_x];
		return Expand565(v);
	}
	case D3DFMT_X1R5G5B5:
	case D3DFMT_A1R5G5B5: {
		unsigned int v = ((const unsigned short*) row)[p_x];
		unsigned int a = (p_tex->m_format == D3DFMT_A1R5G5B5 && !(v & 0x8000)) ? 0 : 0xff;
		return (a << 24) | (Expand5((v >> 10) & 0x1f) << 16) | (Expand5((v >> 5) & 0x1f) << 8) | Expand5(v & 0x1f);
	}
	case D3DFMT_A4R4G4B4: {
		unsigned int v = ((const unsigned short*) row)[p_x];
		return (Expand4(v >> 12) << 24) | (Expand4((v >> 8) & 0xf) << 16) | (Expand4((v >> 4) & 0xf) << 8) |
			   Expand4(v & 0xf);
	}
	case D3DFMT_P8:
		return p_tex->m_palette ? p_tex->m_palette[row[p_x]] : 0;
	default:
		return 0;
	}
}

inline static unsigned int Lerp8(unsigned int p_a, unsigned int p_b, unsigned int p_t)
{
	return p_a + (((int) p_b - (int) p_a) * (int) p_t >> 8);
}

inline static unsigned int LerpArgb(unsigned int p_a, unsigned int p_b, unsigned int p_t)
{
	return (Lerp8(p_a >> 24, p_b >> 24, p_t) << 24) | (Lerp8((p_a >> 16) & 0xff, (p_b >> 16) & 0xff, p_t) << 16) |
		   (Lerp8((p_a >> 8) & 0xff, (p_b >> 8) & 0xff, p_t) << 8) | Lerp8(p_a & 0xff, p_b & 0xff, p_t);
}

static unsigned int SampleBilinear(const TEXTURE* p_tex, int p_u, int p_v)
{
	int x0 = p_u >> 16;
	int y0 = p_v >> 16;
	int x1 = x0 + 1;
	int y1 = y0 + 1;
	unsigned int fx = (unsigned int) ((p_u >> 8) & 0xff);
	unsigned int fy = (unsigned int) ((p_v >> 8) & 0xff);

	if (x0 < 0) {
		x0 = 0;
	}
	if (y0 < 0) {
		y0 = 0;
	}
	if (x1 > p_tex->m_width - 1) {
		x1 = p_tex->m_width - 1;
	}
	if (y1 > p_tex->m_height - 1) {
		y1 = p_tex->m_height - 1;
	}
	if (x0 > x1) {
		x0 = x1;
	}
	if (y0 > y1) {
		y0 = y1;
	}

	unsigned int top = LerpArgb(SampleTexel(p_tex, x0, y0), SampleTexel(p_tex, x1, y0), fx);
	unsigned int bottom = LerpArgb(SampleTexel(p_tex, x0, y1), SampleTexel(p_tex, x1, y1), fx);
	return LerpArgb(top, bottom, fy);
}

template <int Format>
inline static unsigned int SampleLightTexel(const TEXTURE* p_tex, int p_x, int p_y)
{
	const unsigned char* row = (const unsigned char*) p_tex->m_data + (size_t) p_y * p_tex->m_pitch;
	if constexpr (Format == D3DFMT_P8) {
		return p_tex->m_palette ? p_tex->m_palette[row[p_x]] : 0;
	}
	else {
		return Expand565(((const unsigned short*) row)[p_x]);
	}
}

template <int Format>
static unsigned int SampleLightBilinear(const TEXTURE* p_tex, int p_u, int p_v)
{
	int x0 = p_u >> 16;
	int y0 = p_v >> 16;
	int x1 = x0 + 1;
	int y1 = y0 + 1;
	unsigned int fx = (unsigned int) ((p_u >> 8) & 0xff);
	unsigned int fy = (unsigned int) ((p_v >> 8) & 0xff);
	if (x0 < 0) {
		x0 = 0;
	}
	if (y0 < 0) {
		y0 = 0;
	}
	if (x1 > p_tex->m_width - 1) {
		x1 = p_tex->m_width - 1;
	}
	if (y1 > p_tex->m_height - 1) {
		y1 = p_tex->m_height - 1;
	}
	if (x0 > x1) {
		x0 = x1;
	}
	if (y0 > y1) {
		y0 = y1;
	}
	unsigned int top = LerpArgb(SampleLightTexel<Format>(p_tex, x0, y0), SampleLightTexel<Format>(p_tex, x1, y0), fx);
	unsigned int bottom =
		LerpArgb(SampleLightTexel<Format>(p_tex, x0, y1), SampleLightTexel<Format>(p_tex, x1, y1), fx);
	return LerpArgb(top, bottom, fy);
}

inline static unsigned int LightDestColorOneChannel(
	unsigned int p_src,
	unsigned int p_dst,
	unsigned int p_multiplier,
	int p_shift
)
{
	p_src = ModulateChannel(p_src, p_multiplier, 0, p_shift);
	unsigned int result = p_dst * (p_src + 255) / 255;
	return result > 255 ? 255 : result;
}

inline static unsigned int LightDestColorOnePixel(
	unsigned int p_texel,
	unsigned int p_dst,
	unsigned int p_dr,
	unsigned int p_dg,
	unsigned int p_db,
	int p_shift
)
{
	unsigned int r = LightDestColorOneChannel((p_texel >> 16) & 0xff, (p_dst >> 16) & 0xff, p_dr + 1, p_shift);
	unsigned int g = LightDestColorOneChannel((p_texel >> 8) & 0xff, (p_dst >> 8) & 0xff, p_dg + 1, p_shift);
	unsigned int b = LightDestColorOneChannel(p_texel & 0xff, p_dst & 0xff, p_db + 1, p_shift);
	return 0xff000000u | (r << 16) | (g << 8) | b;
}

struct LIGHT_SAMPLE_BLOCK {
	alignas(16) unsigned int m_topLeft[4];
	alignas(16) unsigned int m_topRight[4];
	alignas(16) unsigned int m_bottomLeft[4];
	alignas(16) unsigned int m_bottomRight[4];
	alignas(16) unsigned int m_fx[4];
};

template <int Format>
static void FillLightSampleBlock(
	const TEXTURE* p_tex,
	int p_u,
	int p_stepU,
	int p_y0,
	int p_y1,
	LIGHT_SAMPLE_BLOCK* p_block
)
{
	for (int lane = 0; lane < 4; ++lane, p_u += p_stepU) {
		int x0 = p_u >> 16;
		int x1 = x0 + 1;
		p_block->m_fx[lane] = (unsigned int) ((p_u >> 8) & 0xff);
		if (x0 < 0) {
			x0 = 0;
		}
		if (x1 > p_tex->m_width - 1) {
			x1 = p_tex->m_width - 1;
		}
		if (x0 > x1) {
			x0 = x1;
		}
		p_block->m_topLeft[lane] = SampleLightTexel<Format>(p_tex, x0, p_y0);
		p_block->m_topRight[lane] = SampleLightTexel<Format>(p_tex, x1, p_y0);
		p_block->m_bottomLeft[lane] = SampleLightTexel<Format>(p_tex, x0, p_y1);
		p_block->m_bottomRight[lane] = SampleLightTexel<Format>(p_tex, x1, p_y1);
	}
}

static void FillRgb565LightSampleBlock(
	const TEXTURE* p_tex,
	int p_u,
	int p_stepU,
	int p_y0,
	int p_y1,
	LIGHT_SAMPLE_BLOCK* p_block
)
{
	const unsigned short* top =
		(const unsigned short*) ((const unsigned char*) p_tex->m_data + (size_t) p_y0 * p_tex->m_pitch);
	const unsigned short* bottom =
		(const unsigned short*) ((const unsigned char*) p_tex->m_data + (size_t) p_y1 * p_tex->m_pitch);
	for (int lane = 0; lane < 4; ++lane, p_u += p_stepU) {
		int x0 = p_u >> 16;
		int x1 = x0 + 1;
		p_block->m_fx[lane] = (unsigned int) ((p_u >> 8) & 0xff);
		if (x0 < 0) {
			x0 = 0;
		}
		if (x1 > p_tex->m_width - 1) {
			x1 = p_tex->m_width - 1;
		}
		if (x0 > x1) {
			x0 = x1;
		}
		p_block->m_topLeft[lane] = top[x0];
		p_block->m_topRight[lane] = top[x1];
		p_block->m_bottomLeft[lane] = bottom[x0];
		p_block->m_bottomRight[lane] = bottom[x1];
	}
}

#if defined(ALIEN_TEXTURE_SSE2)

inline static __m128i MulLerpSse2(__m128i p_a, __m128i p_b)
{
	const __m128i zero = _mm_setzero_si128();
	__m128i a16 = _mm_packs_epi32(p_a, zero);
	__m128i b16 = _mm_packs_epi32(p_b, zero);
	return _mm_madd_epi16(_mm_unpacklo_epi16(a16, zero), _mm_unpacklo_epi16(b16, zero));
}

inline static __m128i LightChannelSse2(const LIGHT_SAMPLE_BLOCK& p_block, int p_shift, __m128i p_fy)
{
	const __m128i mask = _mm_set1_epi32(0xff);
	__m128i tl = _mm_and_si128(_mm_srli_epi32(_mm_load_si128((const __m128i*) p_block.m_topLeft), p_shift), mask);
	__m128i tr = _mm_and_si128(_mm_srli_epi32(_mm_load_si128((const __m128i*) p_block.m_topRight), p_shift), mask);
	__m128i bl = _mm_and_si128(_mm_srli_epi32(_mm_load_si128((const __m128i*) p_block.m_bottomLeft), p_shift), mask);
	__m128i br = _mm_and_si128(_mm_srli_epi32(_mm_load_si128((const __m128i*) p_block.m_bottomRight), p_shift), mask);
	__m128i fx = _mm_load_si128((const __m128i*) p_block.m_fx);
	__m128i top = _mm_add_epi32(tl, _mm_srai_epi32(MulLerpSse2(_mm_sub_epi32(tr, tl), fx), 8));
	__m128i bottom = _mm_add_epi32(bl, _mm_srai_epi32(MulLerpSse2(_mm_sub_epi32(br, bl), fx), 8));
	return _mm_add_epi32(top, _mm_srai_epi32(MulLerpSse2(_mm_sub_epi32(bottom, top), p_fy), 8));
}

inline static __m128i ExpandRgb565ChannelSse2(__m128i p_value, int p_shift, int p_mask, int p_left, int p_right)
{
	__m128i component = _mm_and_si128(_mm_srli_epi32(p_value, p_shift), _mm_set1_epi32(p_mask));
	return _mm_or_si128(_mm_slli_epi32(component, p_left), _mm_srli_epi32(component, p_right));
}

inline static __m128i Rgb565LightChannelSse2(
	const LIGHT_SAMPLE_BLOCK& p_block,
	int p_shift,
	int p_mask,
	int p_left,
	int p_right,
	__m128i p_fy
)
{
	__m128i tl =
		ExpandRgb565ChannelSse2(_mm_load_si128((const __m128i*) p_block.m_topLeft), p_shift, p_mask, p_left, p_right);
	__m128i tr =
		ExpandRgb565ChannelSse2(_mm_load_si128((const __m128i*) p_block.m_topRight), p_shift, p_mask, p_left, p_right);
	__m128i bl = ExpandRgb565ChannelSse2(
		_mm_load_si128((const __m128i*) p_block.m_bottomLeft),
		p_shift,
		p_mask,
		p_left,
		p_right
	);
	__m128i br = ExpandRgb565ChannelSse2(
		_mm_load_si128((const __m128i*) p_block.m_bottomRight),
		p_shift,
		p_mask,
		p_left,
		p_right
	);
	__m128i fx = _mm_load_si128((const __m128i*) p_block.m_fx);
	__m128i top = _mm_add_epi32(tl, _mm_srai_epi32(MulLerpSse2(_mm_sub_epi32(tr, tl), fx), 8));
	__m128i bottom = _mm_add_epi32(bl, _mm_srai_epi32(MulLerpSse2(_mm_sub_epi32(br, bl), fx), 8));
	return _mm_add_epi32(top, _mm_srai_epi32(MulLerpSse2(_mm_sub_epi32(bottom, top), p_fy), 8));
}

inline static __m128i PackChannelU16Sse2(__m128i p_value)
{
	return _mm_packs_epi32(p_value, _mm_setzero_si128());
}

template <int Shift>
inline static __m128i ModulateLightSse2(__m128i p_value, unsigned int p_multiplier)
{
	return Clamp255U16Sse2(
		_mm_srli_epi16(_mm_mullo_epi16(PackChannelU16Sse2(p_value), _mm_set1_epi16((short) p_multiplier)), Shift)
	);
}

inline static __m128i BlendLightSse2(__m128i p_src, __m128i p_dst)
{
	__m128i product = _mm_mullo_epi16(p_src, p_dst);
	__m128i adjusted = _mm_add_epi16(product, _mm_set1_epi16(1));
	__m128i quotient = _mm_srli_epi16(_mm_add_epi16(adjusted, _mm_srli_epi16(adjusted, 8)), 8);
	return Clamp255U16Sse2(_mm_add_epi16(p_dst, quotient));
}

template <int Shift>
static void LightBlockSse2(
	const LIGHT_SAMPLE_BLOCK& p_block,
	unsigned int* p_dst,
	unsigned int p_dr,
	unsigned int p_dg,
	unsigned int p_db,
	unsigned int p_fy
)
{
	__m128i fy = _mm_set1_epi32((int) p_fy);
	const __m128i zero = _mm_setzero_si128();
	__m128i dst = _mm_loadu_si128((const __m128i*) p_dst);
	__m128i dstR = PackChannelU16Sse2(_mm_and_si128(_mm_srli_epi32(dst, 16), _mm_set1_epi32(0xff)));
	__m128i dstG = PackChannelU16Sse2(_mm_and_si128(_mm_srli_epi32(dst, 8), _mm_set1_epi32(0xff)));
	__m128i dstB = PackChannelU16Sse2(_mm_and_si128(dst, _mm_set1_epi32(0xff)));
	__m128i srcR = ModulateLightSse2<Shift>(LightChannelSse2(p_block, 16, fy), p_dr + 1);
	__m128i srcG = ModulateLightSse2<Shift>(LightChannelSse2(p_block, 8, fy), p_dg + 1);
	__m128i srcB = ModulateLightSse2<Shift>(LightChannelSse2(p_block, 0, fy), p_db + 1);
	__m128i outR = _mm_unpacklo_epi16(BlendLightSse2(srcR, dstR), zero);
	__m128i outG = _mm_unpacklo_epi16(BlendLightSse2(srcG, dstG), zero);
	__m128i outB = _mm_unpacklo_epi16(BlendLightSse2(srcB, dstB), zero);
	__m128i out = _mm_or_si128(
		_mm_set1_epi32((int) 0xff000000u),
		_mm_or_si128(_mm_slli_epi32(outR, 16), _mm_or_si128(_mm_slli_epi32(outG, 8), outB))
	);
	_mm_storeu_si128((__m128i*) p_dst, out);
}

template <int Shift>
static void Rgb565LightBlockSse2(
	const LIGHT_SAMPLE_BLOCK& p_block,
	unsigned int* p_dst,
	unsigned int p_dr,
	unsigned int p_dg,
	unsigned int p_db,
	unsigned int p_fy
)
{
	__m128i fy = _mm_set1_epi32((int) p_fy);
	const __m128i zero = _mm_setzero_si128();
	__m128i dst = _mm_loadu_si128((const __m128i*) p_dst);
	__m128i dstR = PackChannelU16Sse2(_mm_and_si128(_mm_srli_epi32(dst, 16), _mm_set1_epi32(0xff)));
	__m128i dstG = PackChannelU16Sse2(_mm_and_si128(_mm_srli_epi32(dst, 8), _mm_set1_epi32(0xff)));
	__m128i dstB = PackChannelU16Sse2(_mm_and_si128(dst, _mm_set1_epi32(0xff)));
	__m128i srcR = ModulateLightSse2<Shift>(Rgb565LightChannelSse2(p_block, 11, 0x1f, 3, 2, fy), p_dr + 1);
	__m128i srcG = ModulateLightSse2<Shift>(Rgb565LightChannelSse2(p_block, 5, 0x3f, 2, 4, fy), p_dg + 1);
	__m128i srcB = ModulateLightSse2<Shift>(Rgb565LightChannelSse2(p_block, 0, 0x1f, 3, 2, fy), p_db + 1);
	__m128i outR = _mm_unpacklo_epi16(BlendLightSse2(srcR, dstR), zero);
	__m128i outG = _mm_unpacklo_epi16(BlendLightSse2(srcG, dstG), zero);
	__m128i outB = _mm_unpacklo_epi16(BlendLightSse2(srcB, dstB), zero);
	__m128i out = _mm_or_si128(
		_mm_set1_epi32((int) 0xff000000u),
		_mm_or_si128(_mm_slli_epi32(outR, 16), _mm_or_si128(_mm_slli_epi32(outG, 8), outB))
	);
	_mm_storeu_si128((__m128i*) p_dst, out);
}

#elif defined(ALIEN_TEXTURE_NEON)

inline static uint32x4_t LightChannelNeon(const LIGHT_SAMPLE_BLOCK& p_block, int p_shift, int32x4_t p_fy)
{
	const uint32x4_t mask = vdupq_n_u32(0xff);
	uint32x4_t tl = vandq_u32(vshlq_u32(vld1q_u32(p_block.m_topLeft), vdupq_n_s32(-p_shift)), mask);
	uint32x4_t tr = vandq_u32(vshlq_u32(vld1q_u32(p_block.m_topRight), vdupq_n_s32(-p_shift)), mask);
	uint32x4_t bl = vandq_u32(vshlq_u32(vld1q_u32(p_block.m_bottomLeft), vdupq_n_s32(-p_shift)), mask);
	uint32x4_t br = vandq_u32(vshlq_u32(vld1q_u32(p_block.m_bottomRight), vdupq_n_s32(-p_shift)), mask);
	int32x4_t fx = vreinterpretq_s32_u32(vld1q_u32(p_block.m_fx));
	int32x4_t top = vaddq_s32(
		vreinterpretq_s32_u32(tl),
		vshrq_n_s32(vmulq_s32(vsubq_s32(vreinterpretq_s32_u32(tr), vreinterpretq_s32_u32(tl)), fx), 8)
	);
	int32x4_t bottom = vaddq_s32(
		vreinterpretq_s32_u32(bl),
		vshrq_n_s32(vmulq_s32(vsubq_s32(vreinterpretq_s32_u32(br), vreinterpretq_s32_u32(bl)), fx), 8)
	);
	return vreinterpretq_u32_s32(vaddq_s32(top, vshrq_n_s32(vmulq_s32(vsubq_s32(bottom, top), p_fy), 8)));
}

inline static uint32x4_t ExpandRgb565ChannelNeon(
	uint32x4_t p_value,
	int p_shift,
	unsigned int p_mask,
	int p_left,
	int p_right
)
{
	uint32x4_t component = vandq_u32(vshlq_u32(p_value, vdupq_n_s32(-p_shift)), vdupq_n_u32(p_mask));
	return vorrq_u32(vshlq_u32(component, vdupq_n_s32(p_left)), vshlq_u32(component, vdupq_n_s32(-p_right)));
}

inline static uint32x4_t Rgb565LightChannelNeon(
	const LIGHT_SAMPLE_BLOCK& p_block,
	int p_shift,
	unsigned int p_mask,
	int p_left,
	int p_right,
	int32x4_t p_fy
)
{
	uint32x4_t tl = ExpandRgb565ChannelNeon(vld1q_u32(p_block.m_topLeft), p_shift, p_mask, p_left, p_right);
	uint32x4_t tr = ExpandRgb565ChannelNeon(vld1q_u32(p_block.m_topRight), p_shift, p_mask, p_left, p_right);
	uint32x4_t bl = ExpandRgb565ChannelNeon(vld1q_u32(p_block.m_bottomLeft), p_shift, p_mask, p_left, p_right);
	uint32x4_t br = ExpandRgb565ChannelNeon(vld1q_u32(p_block.m_bottomRight), p_shift, p_mask, p_left, p_right);
	int32x4_t fx = vreinterpretq_s32_u32(vld1q_u32(p_block.m_fx));
	int32x4_t top = vaddq_s32(
		vreinterpretq_s32_u32(tl),
		vshrq_n_s32(vmulq_s32(vsubq_s32(vreinterpretq_s32_u32(tr), vreinterpretq_s32_u32(tl)), fx), 8)
	);
	int32x4_t bottom = vaddq_s32(
		vreinterpretq_s32_u32(bl),
		vshrq_n_s32(vmulq_s32(vsubq_s32(vreinterpretq_s32_u32(br), vreinterpretq_s32_u32(bl)), fx), 8)
	);
	return vreinterpretq_u32_s32(vaddq_s32(top, vshrq_n_s32(vmulq_s32(vsubq_s32(bottom, top), p_fy), 8)));
}

template <int Shift>
inline static uint16x4_t ModulateLightNeon(uint32x4_t p_value, unsigned int p_multiplier)
{
	uint16x4_t narrowed = vmovn_u32(p_value);
	return vmin_u16(vshr_n_u16(vmul_n_u16(narrowed, (unsigned short) p_multiplier), Shift), vdup_n_u16(255));
}

inline static uint16x4_t BlendLightNeon(uint16x4_t p_src, uint16x4_t p_dst)
{
	uint16x4_t product = vmul_u16(p_src, p_dst);
	uint16x4_t adjusted = vadd_u16(product, vdup_n_u16(1));
	uint16x4_t quotient = vshr_n_u16(vadd_u16(adjusted, vshr_n_u16(adjusted, 8)), 8);
	return vmin_u16(vadd_u16(p_dst, quotient), vdup_n_u16(255));
}

template <int Shift>
static void LightBlockNeon(
	const LIGHT_SAMPLE_BLOCK& p_block,
	unsigned int* p_dst,
	unsigned int p_dr,
	unsigned int p_dg,
	unsigned int p_db,
	unsigned int p_fy
)
{
	int32x4_t fy = vdupq_n_s32((int) p_fy);
	uint32x4_t dst = vld1q_u32(p_dst);
	uint16x4_t dstR = vmovn_u32(vandq_u32(vshrq_n_u32(dst, 16), vdupq_n_u32(0xff)));
	uint16x4_t dstG = vmovn_u32(vandq_u32(vshrq_n_u32(dst, 8), vdupq_n_u32(0xff)));
	uint16x4_t dstB = vmovn_u32(vandq_u32(dst, vdupq_n_u32(0xff)));
	uint16x4_t srcR = ModulateLightNeon<Shift>(LightChannelNeon(p_block, 16, fy), p_dr + 1);
	uint16x4_t srcG = ModulateLightNeon<Shift>(LightChannelNeon(p_block, 8, fy), p_dg + 1);
	uint16x4_t srcB = ModulateLightNeon<Shift>(LightChannelNeon(p_block, 0, fy), p_db + 1);
	uint32x4_t outR = vmovl_u16(BlendLightNeon(srcR, dstR));
	uint32x4_t outG = vmovl_u16(BlendLightNeon(srcG, dstG));
	uint32x4_t outB = vmovl_u16(BlendLightNeon(srcB, dstB));
	uint32x4_t out =
		vorrq_u32(vdupq_n_u32(0xff000000u), vorrq_u32(vshlq_n_u32(outR, 16), vorrq_u32(vshlq_n_u32(outG, 8), outB)));
	vst1q_u32(p_dst, out);
}

template <int Shift>
static void Rgb565LightBlockNeon(
	const LIGHT_SAMPLE_BLOCK& p_block,
	unsigned int* p_dst,
	unsigned int p_dr,
	unsigned int p_dg,
	unsigned int p_db,
	unsigned int p_fy
)
{
	int32x4_t fy = vdupq_n_s32((int) p_fy);
	uint32x4_t dst = vld1q_u32(p_dst);
	uint16x4_t dstR = vmovn_u32(vandq_u32(vshrq_n_u32(dst, 16), vdupq_n_u32(0xff)));
	uint16x4_t dstG = vmovn_u32(vandq_u32(vshrq_n_u32(dst, 8), vdupq_n_u32(0xff)));
	uint16x4_t dstB = vmovn_u32(vandq_u32(dst, vdupq_n_u32(0xff)));
	uint16x4_t srcR = ModulateLightNeon<Shift>(Rgb565LightChannelNeon(p_block, 11, 0x1f, 3, 2, fy), p_dr + 1);
	uint16x4_t srcG = ModulateLightNeon<Shift>(Rgb565LightChannelNeon(p_block, 5, 0x3f, 2, 4, fy), p_dg + 1);
	uint16x4_t srcB = ModulateLightNeon<Shift>(Rgb565LightChannelNeon(p_block, 0, 0x1f, 3, 2, fy), p_db + 1);
	uint32x4_t outR = vmovl_u16(BlendLightNeon(srcR, dstR));
	uint32x4_t outG = vmovl_u16(BlendLightNeon(srcG, dstG));
	uint32x4_t outB = vmovl_u16(BlendLightNeon(srcB, dstB));
	uint32x4_t out =
		vorrq_u32(vdupq_n_u32(0xff000000u), vorrq_u32(vshlq_n_u32(outR, 16), vorrq_u32(vshlq_n_u32(outG, 8), outB)));
	vst1q_u32(p_dst, out);
}

#endif

template <int Format, int Shift>
static void BlitLightDestColorOne(
	const TEXTURE* p_tex,
	GRAPH_CORE* p_graph,
	int p_dstL,
	int p_dstT,
	int p_srcL,
	int p_srcT,
	int p_clipL,
	int p_clipT,
	int p_clipR,
	int p_clipB,
	int p_stepU,
	int p_stepV,
	unsigned int p_dr,
	unsigned int p_dg,
	unsigned int p_db
)
{
	for (int y = p_clipT; y < p_clipB; ++y) {
		int v = (int) (((long long) (y - p_dstT) * 2 + 1) * p_stepV / 2) + (p_srcT << 16) - 0x8000;
		int u = (int) (((long long) (p_clipL - p_dstL) * 2 + 1) * p_stepU / 2) + (p_srcL << 16) - 0x8000;
		unsigned int fy = (unsigned int) ((v >> 8) & 0xff);
		int y0 = v >> 16;
		int y1 = y0 + 1;
		if (y0 < 0) {
			y0 = 0;
		}
		if (y1 > p_tex->m_height - 1) {
			y1 = p_tex->m_height - 1;
		}
		if (y0 > y1) {
			y0 = y1;
		}
		unsigned int* out = (unsigned int*) p_graph->m_color + (size_t) y * p_graph->m_pitch;
		int x = p_clipL;
#if defined(ALIEN_TEXTURE_SSE2) || defined(ALIEN_TEXTURE_NEON)
		for (; x + 4 <= p_clipR; x += 4, u += 4 * p_stepU) {
			LIGHT_SAMPLE_BLOCK block;
			if constexpr (Format == D3DFMT_R5G6B5) {
				FillRgb565LightSampleBlock(p_tex, u, p_stepU, y0, y1, &block);
			}
			else {
				FillLightSampleBlock<Format>(p_tex, u, p_stepU, y0, y1, &block);
			}
#if defined(ALIEN_TEXTURE_SSE2)
			if constexpr (Format == D3DFMT_R5G6B5) {
				Rgb565LightBlockSse2<Shift>(block, out + x, p_dr, p_dg, p_db, fy);
			}
			else {
				LightBlockSse2<Shift>(block, out + x, p_dr, p_dg, p_db, fy);
			}
#else
			if constexpr (Format == D3DFMT_R5G6B5) {
				Rgb565LightBlockNeon<Shift>(block, out + x, p_dr, p_dg, p_db, fy);
			}
			else {
				LightBlockNeon<Shift>(block, out + x, p_dr, p_dg, p_db, fy);
			}
#endif
		}
#endif
		for (; x < p_clipR; ++x, u += p_stepU) {
			unsigned int texel = SampleLightBilinear<Format>(p_tex, u, v);
			out[x] = LightDestColorOnePixel(texel, out[x], p_dr, p_dg, p_db, Shift);
		}
	}
}

template <int Format>
static void DispatchLightDestColorOne(
	const TEXTURE* p_tex,
	GRAPH_CORE* p_graph,
	int p_dstL,
	int p_dstT,
	int p_srcL,
	int p_srcT,
	int p_clipL,
	int p_clipT,
	int p_clipR,
	int p_clipB,
	int p_stepU,
	int p_stepV,
	unsigned int p_dr,
	unsigned int p_dg,
	unsigned int p_db,
	int p_shift
)
{
	if (p_shift == 7) {
		BlitLightDestColorOne<Format, 7>(
			p_tex,
			p_graph,
			p_dstL,
			p_dstT,
			p_srcL,
			p_srcT,
			p_clipL,
			p_clipT,
			p_clipR,
			p_clipB,
			p_stepU,
			p_stepV,
			p_dr,
			p_dg,
			p_db
		);
	}
	else {
		BlitLightDestColorOne<Format, 8>(
			p_tex,
			p_graph,
			p_dstL,
			p_dstT,
			p_srcL,
			p_srcT,
			p_clipL,
			p_clipT,
			p_clipR,
			p_clipB,
			p_stepU,
			p_stepV,
			p_dr,
			p_dg,
			p_db
		);
	}
}

static void Argb4444AlphaBlendRow(
	const unsigned short* p_src,
	unsigned int* p_out,
	int p_count,
	int p_u0,
	int p_stepU,
	unsigned int p_dr,
	unsigned int p_dg,
	unsigned int p_db,
	unsigned int p_da,
	int p_shift
)
{
	int u = p_u0;
	for (int i = 0; i < p_count; ++i, u += p_stepU) {
		unsigned int v = p_src[u >> 16];
		unsigned int ta = (((v >> 12) * 17) * (p_da + 1)) >> 8;
		if (ta == 0) {
			continue;
		}
		unsigned int r = ((((v >> 8) & 0xf) * 17) * (p_dr + 1)) >> p_shift;
		unsigned int g = ((((v >> 4) & 0xf) * 17) * (p_dg + 1)) >> p_shift;
		unsigned int b = (((v & 0xf) * 17) * (p_db + 1)) >> p_shift;
		if (r > 255) {
			r = 255;
		}
		if (g > 255) {
			g = 255;
		}
		if (b > 255) {
			b = 255;
		}
		unsigned int d = p_out[i];
		unsigned int inv = 255 - ta;
		unsigned int outR = (r * ta + ((d >> 16) & 0xff) * inv) / 255;
		unsigned int outG = (g * ta + ((d >> 8) & 0xff) * inv) / 255;
		unsigned int outB = (b * ta + (d & 0xff) * inv) / 255;
		p_out[i] = 0xff000000u | (outR << 16) | (outG << 8) | outB;
	}
}

static void BlitQuad(TEXTURE* p_tex, const int* p_dst, const int* p_src, const GAMMA* p_gamma)
{
	GRAPH_CORE* graph = (GRAPH_CORE*) Graph;
	if (!p_tex->m_data || !graph->m_color) {
		return;
	}

	int dstL = p_dst[0];
	int dstT = p_dst[1];
	int dstR = p_dst[2];
	int dstB = p_dst[3];
	int srcL = p_src[0];
	int srcT = p_src[1];
	int srcR = p_src[2];
	int srcB = p_src[3];

	int dstW = dstR - dstL;
	int dstH = dstB - dstT;
	int srcW = srcR - srcL;
	int srcH = srcB - srcT;
	if (dstW <= 0 || dstH <= 0 || srcW <= 0 || srcH <= 0) {
		return;
	}

	// Texels per destination pixel, 16.16.
	int stepU = (int) (((long long) srcW << 16) / dstW);
	int stepV = (int) (((long long) srcH << 16) / dstH);

	// Scaling retains the current point or linear sampler state.
	int filter = (dstW > srcW || dstH > srcH) ? graph->m_state.m_magFilter : graph->m_state.m_minFilter;
	int bilinear = (dstW != srcW || dstH != srcH) && filter == D3DTEXF_LINEAR;

	int clipL = dstL < (int) graph->m_viewXMin ? (int) graph->m_viewXMin : dstL;
	int clipT = dstT < (int) graph->m_viewYMin ? (int) graph->m_viewYMin : dstT;
	int clipR = dstR > (int) graph->m_viewXMax ? (int) graph->m_viewXMax : dstR;
	int clipB = dstB > (int) graph->m_viewYMax ? (int) graph->m_viewYMax : dstB;
	if (clipL >= clipR || clipT >= clipB) {
		return;
	}

	unsigned int diffuse = ~(unsigned int) p_gamma->m_a;
	unsigned int specular = (unsigned int) p_gamma->m_b;
	unsigned int dr = (diffuse >> 16) & 0xff;
	unsigned int dg = (diffuse >> 8) & 0xff;
	unsigned int db = diffuse & 0xff;
	unsigned int da = diffuse >> 24;
	unsigned int sr = (specular >> 16) & 0xff;
	unsigned int sg = (specular >> 8) & 0xff;
	unsigned int sb = specular & 0xff;

	int modulateShift = graph->m_state.m_colorOp == D3DTOP_MODULATE2X ? 7 : 8;
	int useSpecular = graph->m_state.m_specular;
	int blend = graph->m_state.m_alphaBlend;
	int srcBlend = graph->m_state.m_srcBlend;
	int dstBlend = graph->m_state.m_dstBlend;

	// Specialize opaque unscaled RGB565 terrain with exact scalar/SIMD paths.
	int directSrcX = srcL + clipL - dstL;
	int directSrcY = srcT + clipT - dstT;
	if (p_tex->m_format == D3DFMT_R5G6B5 && !bilinear && !blend && dstW == srcW && dstH == srcH && directSrcX >= 0 &&
		directSrcY >= 0 && directSrcX + clipR - clipL <= p_tex->m_width &&
		directSrcY + clipB - clipT <= p_tex->m_height) {
		for (int y = clipT; y < clipB; ++y) {
			const unsigned short* src = (const unsigned short*) ((const unsigned char*) p_tex->m_data +
																 (size_t) (directSrcY + y - clipT) * p_tex->m_pitch) +
										directSrcX;
			unsigned int* out = (unsigned int*) graph->m_color + (size_t) y * graph->m_pitch + clipL;
			OpaqueRgb565PointRow(src, out, clipR - clipL, dr, dg, db, sr, sg, sb, modulateShift);
		}
		return;
	}

	// Specialize bilinear P8/RGB565 DrawLight while preserving blend order.
	if (bilinear && blend && srcBlend == D3DBLEND_DESTCOLOR && dstBlend == D3DBLEND_ONE && sr == 0 && sg == 0 &&
		sb == 0) {
		if (p_tex->m_format == D3DFMT_P8) {
			DispatchLightDestColorOne<D3DFMT_P8>(
				p_tex,
				graph,
				dstL,
				dstT,
				srcL,
				srcT,
				clipL,
				clipT,
				clipR,
				clipB,
				stepU,
				stepV,
				dr,
				dg,
				db,
				modulateShift
			);
			return;
		}
		if (p_tex->m_format == D3DFMT_R5G6B5) {
			DispatchLightDestColorOne<D3DFMT_R5G6B5>(
				p_tex,
				graph,
				dstL,
				dstT,
				srcL,
				srcT,
				clipL,
				clipT,
				clipR,
				clipB,
				stepU,
				stepV,
				dr,
				dg,
				db,
				modulateShift
			);
			return;
		}
	}

	if (p_tex->m_format == D3DFMT_A4R4G4B4 && !bilinear && !useSpecular && blend && srcBlend == D3DBLEND_SRCALPHA &&
		dstBlend == D3DBLEND_INVSRCALPHA) {
		int count = clipR - clipL;
		int u0 = (int) (((long long) (clipL - dstL) * 2 + 1) * stepU / 2) + (srcL << 16);
		int uLast = u0 + (count - 1) * stepU;
		if (count > 0 && (u0 >> 16) >= 0 && (uLast >> 16) < p_tex->m_width) {
			for (int y = clipT; y < clipB; ++y) {
				int ty = (int) (((long long) (y - dstT) * 2 + 1) * stepV / 2 + ((long long) srcT << 16)) >> 16;
				if (ty < 0) {
					ty = 0;
				}
				else if (ty >= p_tex->m_height) {
					ty = p_tex->m_height - 1;
				}
				const unsigned short* src =
					(const unsigned short*) ((const unsigned char*) p_tex->m_data + (size_t) ty * p_tex->m_pitch);
				unsigned int* out = (unsigned int*) graph->m_color + (size_t) y * graph->m_pitch + clipL;
				Argb4444AlphaBlendRow(src, out, count, u0, stepU, dr, dg, db, da, modulateShift);
			}
			return;
		}
	}

	for (int y = clipT; y < clipB; ++y) {
		int v = (int) (((long long) (y - dstT) * 2 + 1) * stepV / 2) + (srcT << 16) - (bilinear ? 0x8000 : 0);
		int u0 = (int) (((long long) (clipL - dstL) * 2 + 1) * stepU / 2) + (srcL << 16) - (bilinear ? 0x8000 : 0);

		unsigned int* out = (unsigned int*) graph->m_color + (size_t) y * graph->m_pitch;
		int u = u0;

		for (int x = clipL; x < clipR; ++x, u += stepU) {
			unsigned int texel;
			if (bilinear) {
				texel = SampleBilinear(p_tex, u, v);
			}
			else {
				int tx = u >> 16;
				int ty = v >> 16;
				if (tx < 0) {
					tx = 0;
				}
				else if (tx >= p_tex->m_width) {
					tx = p_tex->m_width - 1;
				}
				if (ty < 0) {
					ty = 0;
				}
				else if (ty >= p_tex->m_height) {
					ty = p_tex->m_height - 1;
				}
				texel = SampleTexel(p_tex, tx, ty);
			}

			unsigned int ta = ((texel >> 24) * (da + 1)) >> 8;
			if (ta == 0 && blend && srcBlend == D3DBLEND_SRCALPHA && dstBlend == D3DBLEND_INVSRCALPHA) {
				continue;
			}

			unsigned int r = ((((texel >> 16) & 0xff) * (dr + 1)) >> modulateShift);
			unsigned int g = ((((texel >> 8) & 0xff) * (dg + 1)) >> modulateShift);
			unsigned int b = (((texel & 0xff) * (db + 1)) >> modulateShift);
			if (useSpecular) {
				r += sr;
				g += sg;
				b += sb;
			}
			if (r > 255) {
				r = 255;
			}
			if (g > 255) {
				g = 255;
			}
			if (b > 255) {
				b = 255;
			}

			if (!blend) {
				out[x] = 0xff000000u | (r << 16) | (g << 8) | b;
				continue;
			}

			unsigned int d = out[x];
			unsigned int dRed = (d >> 16) & 0xff;
			unsigned int dGreen = (d >> 8) & 0xff;
			unsigned int dBlue = d & 0xff;
			unsigned int dAlpha = d >> 24;

			unsigned int outR = (r * GraphBlendFactor(srcBlend, r, dRed, ta, dAlpha) +
								 dRed * GraphBlendFactor(dstBlend, r, dRed, ta, dAlpha)) /
								255;
			unsigned int outG = (g * GraphBlendFactor(srcBlend, g, dGreen, ta, dAlpha) +
								 dGreen * GraphBlendFactor(dstBlend, g, dGreen, ta, dAlpha)) /
								255;
			unsigned int outB = (b * GraphBlendFactor(srcBlend, b, dBlue, ta, dAlpha) +
								 dBlue * GraphBlendFactor(dstBlend, b, dBlue, ta, dAlpha)) /
								255;
			if (outR > 255) {
				outR = 255;
			}
			if (outG > 255) {
				outG = 255;
			}
			if (outB > 255) {
				outB = 255;
			}
			out[x] = 0xff000000u | (outR << 16) | (outG << 8) | outB;
		}
	}
}

// FUNCTION: ALIEN 0x403ae0
int TEXTURE::Draw(const RECT* p_dst, const RECT* p_src, const GAMMA* p_gamma)
{
	((GRAPH_CORE*) Graph)->SetRenderState(D3DRS_SPECULARENABLE, p_gamma->m_b != 0);

	int dst[4] = {(int) p_dst->left, (int) p_dst->top, (int) p_dst->right, (int) p_dst->bottom};
	int src[4] = {(int) p_src->left, (int) p_src->top, (int) p_src->right, (int) p_src->bottom};
	BlitQuad(this, dst, src, p_gamma);
	return 0;
}

// FUNCTION: ALIEN 0x403dd0
char* TEXTURE::Draw_z(float p_z1, int p_z2, const int* p_dst, const int* p_src, const GAMMA* p_gamma)
{
	(void) p_z1;
	(void) p_z2;
	((GRAPH_CORE*) Graph)->SetRenderState(D3DRS_SPECULARENABLE, p_gamma->m_b != 0);
	BlitQuad(this, p_dst, p_src, p_gamma);
	return 0;
}

// FUNCTION: ALIEN 0x404180
int TEXTURE::SetPalette(const void* p_palette)
{
	if (!p_palette) {
		return 0;
	}
	if (m_format != D3DFMT_P8) {
		if (::Error) {
			return MYERROR::Error(
				::Error,
				"TEXTURE",
				8,
				// STRING: ALIEN 0x47f45c
				"palette for non palette texture",
				0
			);
		}
		return 0;
	}
	if (!m_palette) {
		m_palette = (unsigned int*) malloc(256 * sizeof(unsigned int));
		if (!m_palette) {
			return 0;
		}
	}
	memcpy(m_palette, p_palette, 256 * sizeof(unsigned int));
	return 0;
}
