#include "util/decomp.h"

#if defined(__x86_64__) || defined(_M_X64)
#include <emmintrin.h>
#define ALIEN_ASMDRAW_SSE2 1
#elif defined(__aarch64__) || defined(__arm64__)
#include <arm_neon.h>
#define ALIEN_ASMDRAW_NEON 1
#endif

#include "gfx/color.h"
#include "util/packed.h"

// FUNCTION: ALIEN 0x4165c0
void AsmDraw32(unsigned char* p_src, short* p_zbuf, int* p_dest, int p_count, short p_z, const int* p_palette)
{
	for (int i = 0; i < p_count; ++i) {
		if (p_z > p_zbuf[i]) {
			p_zbuf[i] = p_z;
			p_dest[i] = p_palette[p_src[i]];
		}
	}
}

// FUNCTION: ALIEN 0x416600
void AsmDrawWithAlpha32(
	unsigned char* p_src,
	unsigned short* p_zbuf,
	COLOR* p_dest,
	int p_count,
	unsigned short p_z,
	const int* p_palette
)
{
	int i = 0;
#if defined(ALIEN_ASMDRAW_SSE2)
	const __m128i zero = _mm_setzero_si128();
	const __m128i alphaBase = _mm_set1_epi16(256);
	const __m128i alphaOne = _mm_set1_epi16(1);
	const __m128i depthBias = _mm_set1_epi16((short) 0x8000);
	const __m128i z = _mm_xor_si128(_mm_set1_epi16((short) p_z), depthBias);
	for (; i + 4 <= p_count; i += 4) {
		alignas(16) unsigned int gathered[4] = {
			(unsigned int) p_palette[p_src[i]],
			(unsigned int) p_palette[p_src[i + 1]],
			(unsigned int) p_palette[p_src[i + 2]],
			(unsigned int) p_palette[p_src[i + 3]],
		};
		__m128i src = _mm_load_si128((const __m128i*) gathered);
		__m128i dst = _mm_loadu_si128((const __m128i*) (p_dest + i));
		__m128i srcLo = _mm_unpacklo_epi8(src, zero);
		__m128i srcHi = _mm_unpackhi_epi8(src, zero);
		__m128i dstLo = _mm_unpacklo_epi8(dst, zero);
		__m128i dstHi = _mm_unpackhi_epi8(dst, zero);

		__m128i alphaLo = _mm_shufflelo_epi16(srcLo, _MM_SHUFFLE(3, 3, 3, 3));
		alphaLo = _mm_shufflehi_epi16(alphaLo, _MM_SHUFFLE(3, 3, 3, 3));
		alphaLo = _mm_add_epi16(alphaLo, alphaOne);
		__m128i alphaHi = _mm_shufflelo_epi16(srcHi, _MM_SHUFFLE(3, 3, 3, 3));
		alphaHi = _mm_shufflehi_epi16(alphaHi, _MM_SHUFFLE(3, 3, 3, 3));
		alphaHi = _mm_add_epi16(alphaHi, alphaOne);
		__m128i outLo = _mm_srli_epi16(
			_mm_add_epi16(_mm_mullo_epi16(srcLo, alphaLo), _mm_mullo_epi16(dstLo, _mm_sub_epi16(alphaBase, alphaLo))),
			8
		);
		__m128i outHi = _mm_srli_epi16(
			_mm_add_epi16(_mm_mullo_epi16(srcHi, alphaHi), _mm_mullo_epi16(dstHi, _mm_sub_epi16(alphaBase, alphaHi))),
			8
		);
		__m128i blended = _mm_or_si128(_mm_packus_epi16(outLo, outHi), _mm_set1_epi32((int) 0xff000000u));

		__m128i depths = _mm_xor_si128(_mm_loadl_epi64((const __m128i*) (p_zbuf + i)), depthBias);
		__m128i rejected = _mm_cmpgt_epi16(depths, z);
		__m128i pass16 = _mm_cmpeq_epi16(rejected, zero);
		__m128i pass32 = _mm_unpacklo_epi16(pass16, pass16);
		__m128i result = _mm_or_si128(_mm_and_si128(pass32, blended), _mm_andnot_si128(pass32, dst));
		_mm_storeu_si128((__m128i*) (p_dest + i), result);
	}
#elif defined(ALIEN_ASMDRAW_NEON)
	const uint32x4_t channelMask = vdupq_n_u32(0xff);
	for (; i + 4 <= p_count; i += 4) {
		alignas(16) unsigned int gathered[4] = {
			(unsigned int) p_palette[p_src[i]],
			(unsigned int) p_palette[p_src[i + 1]],
			(unsigned int) p_palette[p_src[i + 2]],
			(unsigned int) p_palette[p_src[i + 3]],
		};
		uint32x4_t src = vld1q_u32(gathered);
		uint32x4_t dst = vld1q_u32((const unsigned int*) (p_dest + i));
		uint32x4_t alpha = vaddq_u32(vshrq_n_u32(src, 24), vdupq_n_u32(1));
		uint32x4_t inverse = vsubq_u32(vdupq_n_u32(256), alpha);
		uint32x4_t srcR = vandq_u32(vshrq_n_u32(src, 16), channelMask);
		uint32x4_t srcG = vandq_u32(vshrq_n_u32(src, 8), channelMask);
		uint32x4_t srcB = vandq_u32(src, channelMask);
		uint32x4_t dstR = vandq_u32(vshrq_n_u32(dst, 16), channelMask);
		uint32x4_t dstG = vandq_u32(vshrq_n_u32(dst, 8), channelMask);
		uint32x4_t dstB = vandq_u32(dst, channelMask);
		uint32x4_t outR = vshrq_n_u32(vaddq_u32(vmulq_u32(srcR, alpha), vmulq_u32(dstR, inverse)), 8);
		uint32x4_t outG = vshrq_n_u32(vaddq_u32(vmulq_u32(srcG, alpha), vmulq_u32(dstG, inverse)), 8);
		uint32x4_t outB = vshrq_n_u32(vaddq_u32(vmulq_u32(srcB, alpha), vmulq_u32(dstB, inverse)), 8);
		uint32x4_t blended = vorrq_u32(
			vdupq_n_u32(0xff000000u),
			vorrq_u32(vshlq_n_u32(outR, 16), vorrq_u32(vshlq_n_u32(outG, 8), outB))
		);

		uint16x4_t pass16 = vcge_u16(vdup_n_u16(p_z), vld1_u16(p_zbuf + i));
		uint32x4_t pass32 = vceqq_u32(vmovl_u16(pass16), vdupq_n_u32(0xffff));
		vst1q_u32((unsigned int*) (p_dest + i), vbslq_u32(pass32, blended, dst));
	}
#endif
	for (; i < p_count; ++i) {
		if (p_z >= p_zbuf[i]) {
			const COLOR* color = (const COLOR*) &p_palette[p_src[i]];
			p_dest[i].AlphaAdd(*color, (unsigned int) color->m_value >> 24);
		}
	}
}

// FUNCTION: ALIEN 0x416740
void AsmDrawWithAlpha16(
	unsigned char* p_src,
	unsigned short* p_zbuf,
	unsigned short* p_dest,
	int p_count,
	unsigned short p_z,
	const int* p_palette
)
{
	for (int i = 0; i < p_count; ++i) {
		if (p_z >= p_zbuf[i]) {
			const COLOR* color = (const COLOR*) &p_palette[p_src[i]];
			unsigned int c =
				(unsigned int) COLOR(&p_dest[i]).AlphaAdd(*color, (unsigned int) color->m_value >> 24).m_value;
			p_dest[i] = (unsigned short) (((c >> 3) & 0x1f) | (RGB16_rMask & (c >> (16 - RGB16_rShift))) |
										  (RGB16_gMask & (c >> (8 - RGB16_gShift))));
		}
	}
}

// FUNCTION: ALIEN 0x41af80
void AsmDrawAlphaWithZ(const void* p_zdelta, const void* p_src, short* p_zdst, short* p_dst, int p_count, int p_baseZ)
{
	const unsigned char* zdelta = (const unsigned char*) p_zdelta;
	const unsigned char* source = (const unsigned char*) p_src;
	for (int i = 0; i < p_count; ++i) {
		int z = p_baseZ + PackedRead<short>(zdelta + 2 * i);
		int zdst = p_zdst[i];

		if (z < zdst) {
			p_dst[i] = 0;
		}
		else if (z > zdst + 0x7f) {
			p_dst[i] = PackedRead<short>(source + 2 * i);
		}
		else {
			unsigned int src = PackedRead<unsigned short>(source + 2 * i);
			int alpha = ((z - zdst) * (int) src) >> 7;
			alpha = alpha > 0xffff ? 0xf000 : (alpha & 0xf000);
			p_dst[i] = (short) ((src & 0xfff) + alpha);
		}
	}
}

// FUNCTION: ALIEN 0x41b040
void AsmDrawLightWithZ(const void* p_zdelta, const void* p_src, short* p_zdst, short* p_dst, int p_count, int p_baseZ)
{
	const unsigned char* zdelta = (const unsigned char*) p_zdelta;
	const unsigned char* source = (const unsigned char*) p_src;
	for (int i = 0; i < p_count; ++i) {
		int z = p_baseZ + PackedRead<short>(zdelta + 2 * i);
		int zdst = p_zdst[i];

		if (z < zdst) {
			p_dst[i] = 0;
		}
		else if (z > zdst + 0x7f) {
			p_dst[i] = (short) (PackedRead<unsigned short>(source + 2 * i) | 0xf000);
		}
		else {
			int alpha = (z - zdst) / 8;
			p_dst[i] = (short) ((alpha << 12) | PackedRead<unsigned short>(source + 2 * i));
		}
	}
}
