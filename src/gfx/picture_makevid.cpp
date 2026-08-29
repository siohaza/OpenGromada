#define DECOMP_STRING_COPY_CTOR_CALL_COPY_NONNULL
#define DECOMP_UNINITIALIZED_STRING_DEFAULT_CTOR
#define DECOMP_INLINE_STRING_DTOR
#define DECOMP_COLOR_COPY_OUT_OF_LINE
#include "gfx/picture_makevid.h"

#include <string.h>
#include <stdlib.h>

#include <dxsdk/ddraw.h>

#include "compress/qs1_coder.h"
#include "misc.h"
#include "video/vid_hardware.h"

#include "util/crc32.h"
#include "util/myerror.h"
#include "util/resource.h"

inline int VID_TEXCOOR::Intersection(int p_x, int p_y, int p_w, int p_h) const
{
	return abs((m_w + 2 * m_x) / 2 - (p_w + 2 * p_x) / 2) < (m_w + p_w) / 2
		&& abs((m_h + 2 * m_y) / 2 - (p_h + 2 * p_y) / 2) < (m_h + p_h) / 2;
}

static inline int OutOfBounds(PICTURE_BASE* p_impl, int p_x, int p_y)
{
	return p_x < 0 || p_y < 0 || p_x >= p_impl->m_width || p_y >= p_impl->m_height;
}

static inline PICTURE_BASE* ImplOf(const PICTURE& p_picture)
{
	return p_picture.m_impl;
}

static inline void SetColorFromPixel(COLOR* p_color, const int* p_pixel)
{
	p_color->m_value = *p_pixel;
}

static inline int ColorDiff(const COLOR* p_color, const COLOR& p_other)
{
	int dr = (int) ((p_color->m_value >> 19) & 0x1f) - (int) ((p_other.m_value >> 19) & 0x1f);
	int dg = (int) ((p_color->m_value >> 10) & 0x3f) - (int) ((p_other.m_value >> 10) & 0x3f);
	int db = (int) ((p_color->m_value >> 3) & 0x1f) - (int) ((p_other.m_value >> 3) & 0x1f);
	int da = (int) ((p_color->m_value >> 24) / 15) - (int) ((p_other.m_value >> 24) / 15);
	return 1936 * db * db + 6750 * da * da + 14400 * dg * dg + 222784 * dr * dr;
}

static inline COLOR SetColorAlpha(COLOR* p_color, int p_alpha)
{
	if (p_alpha < 0)
		p_alpha = 0;
	else if (p_alpha > 255)
		p_alpha = 255;
	p_color->m_value = (p_color->m_value & 0xffffff) | (p_alpha << 24);
	return COLOR(p_color->m_value);
}

// FUNCTION: ALIEN 0x4125b0
PICTURE_MAKEVID::PICTURE_MAKEVID()
{
	m_unk0x42c = 0;
	m_paletteDecode = 0;
}

// FUNCTION: ALIEN 0x428ce0
int PICTURE_MAKEVID::Load(STRING p_name, STRING p_alpha, STRING p_z)
{
	if (m_paletteDecode)
		::operator delete(m_paletteDecode);
	m_paletteDecode = 0;
	if (strcmp(p_alpha.m_str, empty_str))
		m_unk0x42c |= 2;
	if (strcmp(p_z.m_str, empty_str))
		m_unk0x42c |= 4;
	if (strcmp(p_name.m_str, empty_str)) {
		m_unk0x42c |= 1;
	}
	else {
		p_name = p_alpha;
		p_alpha = empty_str;
	}
	int anyOk = m_color.Load((const char**) &p_name);
	anyOk = anyOk == 0;
	anyOk |= m_alpha.Load((const char**) &p_alpha) == 0;
	m_z.Load((const char**) &p_z);
	if (m_color.m_impl->m_width == 2 && m_color.m_impl->m_height == 2)
		m_unk0x42c = 128;
	return anyOk == 0;
}

// FUNCTION: ALIEN 0x428e90
int PICTURE_MAKEVID::Close()
{
	m_color.m_impl->Close();
	m_alpha.m_impl->Close();
	return m_z.m_impl->Close();
}

// FUNCTION: ALIEN 0x428eb0
int PICTURE_MAKEVID::CalcCRC32()
{
	PICTURE_BASE* impl = m_color.m_impl;
	int result = 0;
	int crc = 0;
	int i = 0;
	for (; i < impl->m_width; ++i) {
		int j = 0;
		if (impl->m_height > 0) {
			do {
				unsigned int pixel;
				if (m_unk0x42c & 4) {
					pixel = GetPixelZ(i, j);
					((CRC32*) &crc)->Add((unsigned char*) &pixel, 2);
				}
				int flags = m_unk0x42c;
#pragma inline_depth(0)
				if (flags & 8) {
					m_unk0x42c = flags & 0xfffffff7;
					pixel = GetPixelT(i, j);
					m_unk0x42c |= 8;
				}
				else
					pixel = GetPixelT(i, j);
#pragma inline_depth(8)
				((CRC32*) &crc)->Add((unsigned char*) &pixel, 2);
				++j;
			} while (j < m_color.m_impl->m_height);
		}
		impl = m_color.m_impl;
	}
	return crc;
}

// FUNCTION: ALIEN 0x428f80
int PICTURE_MAKEVID::Rewind()
{
	((PICTURE_BASE*) &m_color)->Rewind();
	((PICTURE_BASE*) &m_alpha)->Rewind();
	return ((PICTURE_BASE*) &m_z)->Rewind();
}

// FUNCTION: ALIEN 0x428fa0
int PICTURE_MAKEVID::NextFrame()
{
	((PICTURE_BASE*) &m_color)->NextFrame();
	((PICTURE_BASE*) &m_alpha)->NextFrame();
	return ((PICTURE_BASE*) &m_z)->NextFrame();
}

// FUNCTION: ALIEN 0x428fc0
int* PICTURE_MAKEVID::GetPixel(int* p_out, int p_x, int p_y) const
{
	int x;
	PICTURE_BASE* impl = m_color.m_impl;
	int y;
	if (OutOfBounds(impl, p_x, p_y)) {
		*p_out = 0xff000000;
		return p_out;
	}
	if (m_alpha.m_impl->m_pixels) {
		int b;
		int a;
		impl->GetPixel(&x, p_x, p_y);
		m_color.m_impl->GetPixel(&y, p_x, p_y);
		m_color.m_impl->GetPixel(&b, p_x, p_y);
		m_alpha.m_impl->GetPixel(&a, p_x, p_y);
		x &= 0xff;
		y = (y >> 8) & 0xff;
		b = (b >> 16) & 0xff;
		a &= 0xff;
		if (a < 0)
			a = 0;
		else if (a > 255)
			a = 255;
		if (b < 0)
			b = 0;
		else if (b > 255)
			b = 255;
		if (y < 0)
			y = 0;
		else if (y > 255)
			y = 255;
		if (x < 0)
			x = 0;
		else if (x > 255)
			x = 255;
		*p_out = (((a << 8 | b) << 8 | y) << 8) | x;
		return p_out;
	}
	impl->GetPixel(p_out, p_x, p_y);
	return p_out;
}

// FUNCTION: ALIEN 0x4290f0
unsigned int PICTURE_MAKEVID::GetAlpha(int p_x, int p_y) const
{
	PICTURE_BASE* v3 = m_color.m_impl;
	if (OutOfBounds(v3, p_x, p_y)) {
		return 0;
	}
	else {
		PICTURE_BASE* v5 = m_alpha.m_impl;
		if (v5->m_pixels) {
			v5->GetPixel(&p_x, p_x, p_y);
			return p_x & 0xff;
		}
		else if (v3->m_bpp == 4)
			return (unsigned int) v3->GetData(p_x, p_y) >> 24;
		else
			return 255;
	}
}

// FUNCTION: ALIEN 0x429160
short PICTURE_MAKEVID::GetPixelZ(int p_x, int p_y) const
{
	PICTURE_BASE* v3 = m_color.m_impl;
	short data = 0;
	if (OutOfBounds(v3, p_x, p_y))
		return 1024;
	PICTURE_BASE* v6 = m_z.m_impl;
	if (v6->m_pixels && m_z.m_format == 5 && (data = v6->GetData(p_x, p_y)) == (short) 0x8000)
		return 0;
	return data + 1024;
}

// FUNCTION: ALIEN 0x4291d0
short PICTURE_MAKEVID::GetBoxZ(int p_x, int p_y, int p_w, int p_h)
{
	int ey = p_y + p_h;
	int ex = p_x + p_w;
	if (IsPixel(ex - 1, ey - 1))
		return GetPixelZ(ex - 1, ey - 1);
	int x = p_x;
	for (; p_y < ey; ++p_y)
		for (p_x = x; p_x < ex; ++p_x)
			if (IsPixel(p_x, p_y))
				return GetPixelZ(p_x, p_y);
	return 0;
}

// FUNCTION: ALIEN 0x429270
int PICTURE_MAKEVID::GetPaletteNumber(COLOR p_color) const
{
	unsigned int c = p_color.m_value;
	if (m_paletteDecode)
		return m_paletteDecode[16 * (((c & 0xf8) | (((c & 0xfc00) | ((c >> 3) & 0x1f0000)) >> 2)) >> 3) + (c >> 28)];
	int bestIdx = 0;
	int i = 0;
	int best = 9999999;
	int r = (c >> 19) & 0x1f;
	int g = (c >> 10) & 0x3f;
	int b = (c >> 3) & 0x1f;
	unsigned int a = (c >> 24) / 15;
	const unsigned int* entry = m_palette;
	for (; i < 256; ++i, ++entry) {
		unsigned int e = *entry;
		int dr = r - (int) ((e >> 19) & 0x1f);
		int dg = g - (int) ((e >> 10) & 0x3f);
		int db = b - (int) ((e >> 3) & 0x1f);
		int da = a - (e >> 24) / 15;
		int diff = 14400 * dg * dg + 222784 * dr * dr + 6750 * da * da + 1936 * db * db;
		if (diff < best) {
			best = diff;
			bestIdx = i;
		}
	}
	return bestIdx;
}

// FUNCTION: ALIEN 0x4293f0
void PICTURE_MAKEVID::SetPaletteDecodeNumber(COLOR p_color, unsigned char p_number)
{
	unsigned char* table = m_paletteDecode;
	if (table) {
		unsigned int c = p_color.m_value;
		unsigned char* slot = table
			+ 16 * (((c & 0xf8) | (((c & 0xfc00) | ((c >> 3) & 0x1f0000)) >> 2)) >> 3)
			+ (c >> 28);
		int old = *slot;
		if (!old || old == p_number) {
			*slot = p_number;
		}
		else {
			for (int i = 0; i < 0x100000; ++i) {
				if (m_paletteDecode[i] == old)
					m_paletteDecode[i] = p_number;
			}
		}
	}
}

inline unsigned int PICTURE_MAKEVID::GetPixelT(int p_x, int p_y)
{
	PICTURE_BASE* color = m_color.m_impl;
	if (OutOfBounds(color, p_x, p_y))
		return 0;

	if (m_unk0x42c & 0x1000) {
		if (m_unk0x42c & 8)
			return color->GetData(p_x, p_y);
		unsigned int pixel;
		GetPixel((int*) &pixel, p_x, p_y);
		int a = pixel >> 24;
		int r = (pixel >> 16) & 0xff;
		int g = (pixel >> 8) & 0xff;
		int b = pixel & 0xff;
		if (!IsPixel(p_x, p_y))
			a = 0;
		if ((m_unk0x42c & 1) && (m_unk0x42c & 2)) {
			r = 255 * r / a;
			g = 255 * g / a;
			b = 255 * b / a;
			if (r > 255)
				r = 255;
			if (g > 255)
				g = 255;
			if (b > 255)
				b = 255;
			return (a << 24) | (r << 16) | (g << 8) | b;
		}
		return ((a >> 7) << 15) | ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
	}
	else if (!(m_unk0x42c & 8)) {
		unsigned int pixel;
		GetPixel((int*) &pixel, p_x, p_y);
		unsigned char b = pixel;
		unsigned char a = pixel >> 24;
		unsigned char r = pixel >> 16;
		unsigned char g = pixel >> 8;
		if ((m_unk0x42c & 1) && (m_unk0x42c & 2)) {
			if (a >> 4) {
				r = (255 * r / a) >> 4;
				g = (255 * g / a) >> 4;
				b = (255 * b / a) >> 4;
				if (r > 15)
					r = 15;
				if (g > 15)
					g = 15;
				if (b > 15)
					b = 15;
				return ((a >> 4) << 12) | (r << 8) | (g << 4) | b;
			}
			return 0;
		}
		if ((m_unk0x42c & 4) && (m_unk0x42c & 2))
			return ((pixel & 0xff) >> 4) | ((r >> 4) << 8) | ((g >> 4) << 4);
		return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
	}
	else if (m_unk0x42c & 2) {
		unsigned int pixel;
		return GetPaletteNumber(COLOR(*GetPixel((int*) &pixel, p_x, p_y)));
	}
	else
		return color->GetData(p_x, p_y);
}

// FUNCTION: ALIEN 0x429760
void PICTURE_MAKEVID::GetRectangle(int* p_left, int* p_top, int* p_right, int* p_bottom)
{
	if (p_top && p_bottom) {
		*p_top = -1;
		for (int y = 0; y < m_color.m_impl->m_height; ++y) {
			if (*p_top != -1)
				break;
			for (int x = 0; x < m_color.m_impl->m_width; ++x) {
				if (IsPixel(x, y))
					*p_top = y;
			}
		}
		int y2 = m_color.m_impl->m_height - 1;
		for (*p_bottom = -1; y2 >= 0; --y2) {
			if (*p_bottom != -1)
				break;
			for (int x = 0; x < m_color.m_impl->m_width; ++x) {
				if (IsPixel(x, y2))
					*p_bottom = y2;
			}
		}
		if (*p_bottom >= 0)
			++*p_bottom;
		if (*p_bottom <= *p_top || *p_top < 0 || *p_bottom < 0) {
			*p_top = 0;
			*p_bottom = 0;
		}
	}
	if (p_left && p_right) {
		*p_left = -1;
		for (int x = 0; x < m_color.m_impl->m_width; ++x) {
			if (*p_left != -1)
				break;
			for (int y = 0; y < m_color.m_impl->m_height; ++y) {
				if (IsPixel(x, y))
					*p_left = x;
			}
		}
		int x2 = m_color.m_impl->m_width - 1;
		for (*p_right = -1; x2 >= 0; --x2) {
			if (*p_right != -1)
				break;
			for (int y = 0; y < m_color.m_impl->m_height; ++y) {
				if (IsPixel(x2, y))
					*p_right = x2;
			}
		}
		if (*p_right >= 0)
			++*p_right;
		if (*p_right <= *p_left || *p_left < 0 || *p_right < 0) {
			*p_top = 0;
			*p_bottom = 0;
		}
	}
}

// GLOBAL: ALIEN 0x492764
IDirectDraw* PictureDirectDraw = 0;

// GLOBAL: ALIEN 0x47a6d8
static const DDPIXELFORMAT PixelFormatDXT1 = { 32, DDPF_FOURCC, 0x31545844 /* 'DXT1' */  };
// GLOBAL: ALIEN 0x47a6f8
static const DDPIXELFORMAT PixelFormatDXT3 = { 32, DDPF_FOURCC, 0x33545844 /* 'DXT3' */  };

// FUNCTION: ALIEN 0x429920
int PICTURE_MAKEVID::WriteSurfaces(unsigned char* p_out, char* p_src, int p_width,
	int p_height)
{
	int pos = 0;
	if ((m_unk0x42c & 8) || !(m_unk0x42c & 0x800)) {
		for (int row = 0; row < p_height; ++row) {
			char* cell = p_src + 512 * row;
			if (m_unk0x42c & 8) {
				for (int x = 0; x < p_width; ++x)
					p_out[pos++] = cell[2 * x];
			}
			else {
				memcpy(p_out + pos, cell, 2 * p_width);
				pos += 2 * p_width;
			}
		}
		return pos;
	}

	DDSURFACEDESC desc;
	memset(&desc, 0, sizeof(desc));
	desc.dwSize = 108;
	desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
	if (!(m_unk0x42c & 6))
		desc.dwFlags = 69639;
	desc.dwWidth = p_width;
	desc.dwHeight = p_height;
	desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
	desc.ddpfPixelFormat.dwSize = 32;
	desc.ddpfPixelFormat.dwFlags = DDPF_RGB;
	desc.ddpfPixelFormat.dwRGBBitCount = 16;
	if ((m_unk0x42c & 2) && (m_unk0x42c & 1)) {
		desc.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
		desc.ddpfPixelFormat.dwRBitMask = 0xf00;
		desc.ddpfPixelFormat.dwGBitMask = 0xf0;
		desc.ddpfPixelFormat.dwBBitMask = 0xf;
		desc.ddpfPixelFormat.dwRGBAlphaBitMask = 0xf000;
	}
	else {
		desc.ddpfPixelFormat.dwRBitMask = 0xf800;
		desc.ddpfPixelFormat.dwGBitMask = 0x7e0;
		desc.ddpfPixelFormat.dwBBitMask = 0x1f;
	}
	IDirectDrawSurface* source;
	if (PictureDirectDraw->CreateSurface(&desc, &source, 0)) {
		STRING name(m_color.m_impl->m_name);
		MYERROR::Error(::Error, "PICTURE '%s'", 3,
			// STRING: ALIEN 0x483990
			"DD surface", 0, name.m_str);
		return 0;
	}
	desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
	desc.ddsCaps.dwCaps = 6144;
	if ((m_unk0x42c & 2) && (m_unk0x42c & 1))
		desc.ddpfPixelFormat = PixelFormatDXT3;
	else
		desc.ddpfPixelFormat = PixelFormatDXT1;
	IDirectDrawSurface* compressed;
	if (PictureDirectDraw->CreateSurface(&desc, &compressed, 0)) {
		STRING name(m_color.m_impl->m_name);
		MYERROR::Error(::Error, "PICTURE '%s'", 3,
			// STRING: ALIEN 0x483984
			"DXT surface", 0, name.m_str);
		return 0;
	}
	if (source->Lock(0, &desc, DDLOCK_WAIT, 0)) {
		STRING name(m_color.m_impl->m_name);
		MYERROR::Error(::Error, "PICTURE '%s'", 0,
			// STRING: ALIEN 0x48397c
			"surface", 0, name.m_str);
		return 0;
	}
	char* dst = (char*) desc.lpSurface;
	for (int row = 0; row < p_height; ++row)
		memcpy(dst + 2 * p_width * row, p_src + 512 * row, 2 * p_width);
	source->Unlock(0);
	if (compressed->BltFast(0, 0, source, 0, DDBLTFAST_WAIT)) {
		STRING name(m_color.m_impl->m_name);
		MYERROR::Error(::Error, "PICTURE '%s'", 1, "surface", 0, name.m_str);
		return 0;
	}
	if (compressed->Lock(0, &desc, DDLOCK_WAIT, 0)) {
		STRING name(m_color.m_impl->m_name);
		MYERROR::Error(::Error, "PICTURE '%s'", 0, "DXT surface", 0, name.m_str);
		return 0;
	}
	if (desc.dwFlags & DDSD_LINEARSIZE) {
		memcpy(p_out, desc.lpSurface, desc.dwLinearSize);
		pos = desc.dwLinearSize;
	}
	else {
		MYERROR::Error(::Error, "PICTURE '%s'", 10,
			// STRING: ALIEN 0x483968
			"not DDSD_LINEARSIZE", 0,
			STRING(m_color.m_impl->m_name, STRING::CALL_COPY_NONNULL).m_str);
	}
	compressed->Unlock(0);
	compressed->Release();
	source->Release();
	return pos;
}

// FUNCTION: ALIEN 0x429e40
int SortCallBack(const void* p_a, const void* p_b)
{
	const int* b = *(const int**) p_b;
	const int* a = *(const int**) p_a;
	int bw = b[4];
	int aw = a[4];
	int maxw = aw > bw ? aw : bw;
	int ah = a[5];
	int bh = b[5];
	int maxh = ah > bh ? ah : bh;
	if (maxw > maxh)
		return bw - aw;
	return bh - ah;
}

// STUB: ALIEN 0x429e80
void PICTURE_MAKEVID::WriteHardware(RESOURCE* p_res)
{
	QS1_CODER* colorCoder = 0;
	QS1_CODER* zCoder = 0;
	int pages = 0;
	if (m_unk0x42c & 0x100) {
		if (m_unk0x42c & 0x800)
			colorCoder = new QS1_CODER(1);
		else
			colorCoder = new QS1_CODER(2);
		zCoder = new QS1_CODER(2);
	}

	int cells = m_color.m_impl->m_noFrames * (m_color.m_impl->m_width / 256 + 1)
		* (m_color.m_impl->m_height / 256 + 1) + 1;
	int* crcs = (int*) operator new(4 * m_color.m_impl->m_noFrames);
	if (!crcs) {
		{
			STRING name(m_color.m_impl->m_name);
			MYERROR::Error(::Error, "PICTURE '%s'", 2,
				// STRING: ALIEN 0x4839fc
				"cntrl", 0, name.m_str);
		}
		exit(1);
	}
	char* buf = (char*) operator new(0x200000);
	if (!buf) {
		{
			STRING name(m_color.m_impl->m_name);
			MYERROR::Error(::Error, "PICTURE '%s'", 2,
				// STRING: ALIEN 0x4839f4
				"shadow", 0, name.m_str);
		}
		exit(1);
	}
	VID_CHILD* coords = (VID_CHILD*) operator new(sizeof(VID_CHILD) * (cells + 900));
	if (!coords) {
		{
			STRING name(m_color.m_impl->m_name);
			MYERROR::Error(::Error, "PICTURE '%s'", 2,
				// STRING: ALIEN 0x482c34
				"texcoor", cells + 900, name.m_str);
		}
		exit(1);
	}

	int count = m_color.m_impl->m_noFrames;
	int frame;
	int i;
	for (frame = 0; frame < m_color.m_impl->m_noFrames; ++frame) {
		VID_CHILD* head = &coords[frame];
		crcs[frame] = CalcCRC32();
		for (i = 0; i < frame; ++i) {
			if (crcs[i] == crcs[frame]) {
				head->m_texture = -1;
				head->m_w = i;
				break;
			}
		}
		if (i >= frame) {
			head->m_texture = 0;
			int left;
			int top;
			int right;
			int bottom;
			GetRectangle(&left, &top, &right, &bottom);
			if (right - left <= 256 && bottom - top <= 256) {
				head->m_next = 0;
				head->m_offsetX = left;
				head->m_w = right - left;
				head->m_offsetY = top;
				head->m_h = bottom - top;
			}
			else {
				int first = 1;
				for (int cx = left; cx < right; cx += 256) {
					for (int cy = top; cy < bottom; cy += 256) {
						int w = right - cx;
						if (w > 256)
							w = 256;
						int h = bottom - cy;
						if (h > 256)
							h = 256;
						VID_CHILD* cell;
						if (first) {
							cell = head;
							head->m_next = count;
							first = 0;
						}
						else {
							cell = &coords[count];
							cell->m_texture = 0;
							cell->m_next = count + 1;
							++count;
						}
						cell->m_offsetX = cx;
						cell->m_w = w;
						cell->m_offsetY = cy;
						cell->m_h = h;
					}
				}
				coords[count - 1].m_next = 0;
			}
		}
		NextFrame();
	}
	operator delete(crcs);

	VID_CHILD** sorted = (VID_CHILD**) operator new(4 * count);
	if (!sorted) {
		{
			STRING name(m_color.m_impl->m_name, STRING::CALL_COPY_NONNULL);
			MYERROR::Error(::Error, "PICTURE '%s'", 2,
				// STRING: ALIEN 0x4839e0
				"indices for sort", count, name.m_str);
		}
		exit(1);
	}
	for (i = 0; i < count; ++i)
		sorted[i] = &coords[i];
	qsort(sorted, count, 4, SortCallBack);

	int pos = 0;
	for (frame = 0; frame < count; ++frame) {
		VID_CHILD* cell = sorted[frame];
		if (cell->m_texture < 0 || !cell->m_h)
			continue;
		if (frame == 0 || cell->m_w < sorted[frame - 1]->m_w || cell->m_h < sorted[frame - 1]->m_h)
			pos = 0;
		int x = 0;
		int placed;
		while (pos <= (cells << 8) - cell->m_h) {
			if (pos > (pos & ~255) + 256 - cell->m_h)
				pos = (pos & ~255) + 256;
			x = 0;
			while (x <= 256 - cell->m_w) {
				placed = 1;
				for (i = frame - 1; i >= 0; --i) {
					VID_CHILD* other = sorted[i];
					if (other->m_texture < 0)
						continue;
					if (other->Intersection(x, pos, cell->m_w, cell->m_h)) {
						placed = 0;
						x = other->m_x + other->m_w;
						break;
					}
				}
				if (placed)
					break;
			}
			if (placed)
				break;
			++pos;
		}
		if (!placed) {
			{
				STRING name(m_color.m_impl->m_name, STRING::CALL_COPY_NONNULL);
				MYERROR::Error(::Error, "PICTURE '%s'", 10,
					// STRING: ALIEN 0x4839c8
					"Can't replace rectangle", 0, name.m_str);
			}
			exit(1);
		}
		cell->m_x = x;
		cell->m_y = pos;
		int page = pos / 256;
		cell->m_texture = (m_unk0x42c & 4) ? 2 * page : page;
		if (page >= pages)
			pages = page + 1;
	}
	operator delete(sorted);
	Rewind();

	char* stream = (char*) operator new(0x200000);
	if (!stream) {
		{
			STRING name(m_color.m_impl->m_name, STRING::CALL_COPY_NONNULL);
			MYERROR::Error(::Error, "PICTURE '%s'", 2,
				// STRING: ALIEN 0x4839c0
				"2097152", 0, name.m_str);
		}
		exit(1);
	}
	unsigned short* colorPages = (unsigned short*) operator new(pages << 17);
	if (!colorPages) {
		{
			STRING name(m_color.m_impl->m_name, STRING::CALL_COPY_NONNULL);
			MYERROR::Error(::Error, "PICTURE '%s'", 2,
				// STRING: ALIEN 0x4839bc
				"Buf", pages, name.m_str);
		}
		exit(1);
	}
	unsigned short* zPages = (unsigned short*) operator new(pages << 17);
	if (!zPages) {
		{
			STRING name(m_color.m_impl->m_name.m_str, STRING::INLINE_CHARP);
			MYERROR::Error(::Error, "PICTURE '%s'", 2,
				// STRING: ALIEN 0x4839b4
				"ZBuf", pages, name.m_str);
		}
		exit(1);
	}
	memset(colorPages, 0, pages << 17);
	memset(zPages, 0, pages << 17);

	for (frame = 0; frame < m_color.m_impl->m_noFrames; ++frame) {
		if (coords[frame].m_texture >= 0) {
			VID_CHILD* cell = &coords[frame];
			while (cell) {
				for (int x = cell->m_offsetX; x < cell->m_offsetX + cell->m_w; ++x) {
					for (int y = cell->m_offsetY; y < cell->m_offsetY + cell->m_h;
						 ++y) {
						int at = cell->m_x + x - cell->m_offsetX
							+ ((y + cell->m_y - cell->m_offsetY) << 8);
						if (m_unk0x42c & 4)
							zPages[at] = GetPixelZ(x, y);
#pragma inline_depth(0)
						colorPages[at] = (unsigned short) GetPixelT(x, y);
#pragma inline_depth(8)
					}
				}
				if (!cell->m_next)
					break;
				cell = &coords[cell->m_next];
			}
		}
		NextFrame();
	}

	for (i = 0; i < count; ++i)
		coords[i].m_y %= 256;
	for (i = 0; i < count; ++i) {
		if (coords[i].m_texture < 0)
			memcpy(&coords[i], &coords[coords[i].m_w], sizeof(VID_CHILD));
	}

	p_res->PreAppend(0x46525553 /* 'SURF' */ , 0);
	short pageWord =
		(short) ((m_unk0x42c & 4) ? 2 * pages : pages);
	p_res->Write(&pageWord, 2);
	for (int page = 0; page < pages; ++page) {
		int maxX = 0;
		int maxY = 0;
		int match = (m_unk0x42c & 4) ? 2 * page : page;
		for (i = 0; i < count; ++i) {
			VID_CHILD* cell = &coords[i];
			if (cell->m_texture != match)
				continue;
			if (maxX < cell->m_x + cell->m_w)
				maxX = cell->m_x + cell->m_w;
			if (maxY < cell->m_y + cell->m_h)
				maxY = cell->m_y + cell->m_h;
		}
		int surfW = 32;
		while (surfW < maxX)
			surfW *= 2;
		int surfH = 32;
		while (surfH < maxY)
			surfH *= 2;
		if (surfW > 256) {
			STRING name(m_color.m_impl->m_name.m_str, STRING::INLINE_CHARP);
			MYERROR::Error(::Error, "PICTURE '%s'", 4,
				// STRING: ALIEN 0x4839a8
				"SurfSizeX", surfW, name.m_str);
		}
		if (surfH > 256) {
			STRING name(m_color.m_impl->m_name.m_str, STRING::INLINE_CHARP);
			MYERROR::Error(::Error, "PICTURE '%s'", 4,
				// STRING: ALIEN 0x48399c
				"SurfSizeY", surfH, name.m_str);
		}
		short word = (short) surfW;
		p_res->Write(&word, 2);
		word = (short) surfH;
		p_res->Write(&word, 2);
		int size = WriteSurfaces((unsigned char*) stream,
			(char*) colorPages + 0x20000 * page, surfW, surfH);
		p_res->Write(&size, 4);
		p_res->WritePacked(stream, size, colorCoder);
		if (m_unk0x42c & 4) {
			size = 0;
			for (int row = 0; row < surfH; ++row) {
				memcpy(stream + size, (char*) zPages + 512 * (row + (page << 8)),
					2 * surfW);
				size += 2 * surfW;
			}
			p_res->Write(&size, 4);
			p_res->WritePacked(stream, size, zCoder);
		}
	}
	p_res->PostAppend();

	p_res->PreAppend(0x41544144 /* 'DATA' */ , 0);
	p_res->Write(coords, sizeof(VID_CHILD) * count);
	p_res->PostAppend();

	if (colorCoder)
		delete colorCoder;
	if (zCoder)
		delete zCoder;
	operator delete(colorPages);
	operator delete(zPages);
	operator delete(coords);
	operator delete(stream);
	operator delete(buf);
}

static inline void SetShadowWords(int* p_words, int p_value)
{
	*p_words = p_value;
}

static inline int ShadowDimension(int p_value)
{
	return p_value;
}

static inline int ShadowLengthSquared(int p_x, int p_y)
{
	return p_x * p_x + p_y * p_y;
}

// STUB: ALIEN 0x42ad80
int PICTURE_MAKEVID::GetShadow(char* p_out)
{
	short dx[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
	short dy[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };

	*(short*) p_out = 0;
	if (!(m_unk0x42c & 0x20000))
		return 2;

	short* points = (short*) operator new(4 * ShadowDimension(m_color.m_impl->m_height)
		* ShadowDimension(m_color.m_impl->m_width));
	if (!points) {
		STRING name(m_color.m_impl->m_name);
		MYERROR::Error(::Error, "PICTURE '%s'", 2,
			// STRING: ALIEN 0x483a18
			"shadow dot", 0, name.m_str);
		return 2;
	}
	unsigned char* visited = (unsigned char*) operator new(ShadowDimension(m_color.m_impl->m_height)
		* ShadowDimension(m_color.m_impl->m_width));
	if (!visited) {
		STRING name(m_color.m_impl->m_name);
		MYERROR::Error(::Error, "PICTURE '%s'", 2,
			// STRING: ALIEN 0x483a04
			"dot without shadow", 0, name.m_str);
		return 2;
	}
	memset(visited, 0, m_color.m_impl->m_height * m_color.m_impl->m_width);

	int words;
	for (;;) {
		SetShadowWords(&words, visited != 0 && points != 0);

		int x;
		int y;
		for (x = 0; x < m_color.m_impl->m_width; ++x) {
			for (y = 0; y <= x; ++y) {
				if (IsPixel(x, y) && !visited[x + y * m_color.m_impl->m_width])
					break;
			}
			if (y <= x)
				break;
			int k;
			for (k = 0; k < x; ++k) {
				if (IsPixel(k, x) && !visited[k + x * m_color.m_impl->m_width])
					break;
			}
			if (k < x) {
				y = x;
				x = k;
				break;
			}
		}
		if (x >= m_color.m_impl->m_width)
			return 2;

		points[0] = (short) x;
		int sx = (short) x;
		int sy = (short) y;
		points[1] = (short) y;
		int count = 1;
		short* p = points + 2;
		int backDir = points == 0;
		for (;;) {
			int dir = (backDir + 1) & 7;
			while (dir != backDir) {
				if (IsPixel(x + dx[dir], y + dy[dir])
					&& !visited[x + dx[dir] + (y + dy[dir]) * m_color.m_impl->m_width]) {
					x += dx[dir];
					y += dy[dir];
					p[0] = (short) x;
					p[1] = (short) y;
					++count;
					p += 2;
					backDir = dir ^ 4;
					break;
				}
				dir = (dir + 1) & 7;
			}
			if (dir == backDir) {
				--count;
				p -= 2;
				int d;
				for (d = 0; d < 8; ++d) {
					if (p[0] == dx[d] + p[-2] && p[1] == dy[d] + p[-1])
						break;
				}
				backDir = d ^ 4;
			}
			if (points[0] == x && points[1] == y)
				break;
		}

		int keptCount = p != 0;
		short* dst = points + 2;
		int base = 0;
		while (base < count) {
			int probe;
			for (probe = base + 3; probe < count; ++probe) {
				if (points[2 * probe + 1] - points[2 * base + 1] == 0
					&& points[2 * probe] - points[2 * base] == 0)
					continue;
				if (abs(points[2 * base] - points[2 * probe - 1] - points[2 * probe - 2] + points[2 * base + 1]) < 1)
					continue;
				int mid;
				for (mid = base + 1; mid < probe; ++mid) {
					int len = Sqrt(ShadowLengthSquared(points[2 * base] - points[2 * probe],
						points[2 * base + 1] - points[2 * probe + 1]));
					if (abs((points[2 * probe + 1] - points[2 * base + 1]) * points[2 * mid]
							+ (points[2 * base] - points[2 * probe]) * points[2 * mid + 1]
							- points[2 * base] * points[2 * probe + 1] + points[2 * probe] * points[2 * base + 1]) / len
						> ((m_unk0x42c & 0x80000) ? 3 : 0))
						break;
				}
				if (mid < probe)
					break;
			}
			base = probe - 1;
			if (probe < count) {
				*(int*) dst = *(int*) &points[2 * probe - 2];
				++keptCount;
				dst += 2;
			}
		}

		if (keptCount < 3) {
			visited[sx + sy * m_color.m_impl->m_width] = 1;
			continue;
		}

		while (keptCount > 255) {
			keptCount /= 2;
			for (int k = 0; k < keptCount; ++k)
				((int*) points)[k] = ((int*) points)[2 * k];
		}
		*(short*) p_out = (short) keptCount;
		for (int k = 0; k < keptCount; ++k) {
			int z = GetPixelZ(points[2 * k], points[2 * k + 1]);
			int lift = z / 8 - 128;
			((short*) p_out)[words] = points[2 * k];
			++words;
			((short*) p_out)[words] = (short) (points[2 * k + 1] + lift);
			++words;
			((short*) p_out)[words] = (short) lift;
			++words;
		}
		operator delete(points);
		operator delete(visited);
		return 2 * words;
	}
}

// FUNCTION: ALIEN 0x42b3a0
void PICTURE_MAKEVID::CreateOnePalette()
{
	int distRow[256];
	int matrix[256][256];

	if (m_paletteDecode)
		::operator delete(m_paletteDecode);
	m_paletteDecode = (unsigned char*) operator new(0x100000);
	memset(m_paletteDecode, 0, 0x100000);
	unsigned int* pal = m_palette;
	pal[0] = 0;
	int count = 1;
	matrix[0][0] = 0;

	for (int frame = 0; frame < m_color.m_impl->m_noFrames; ++frame) {
		for (int y = 0; y < m_color.m_impl->m_height; ++y) {
			for (int x = 0; x < m_color.m_impl->m_width; ++x) {
				if (!IsPixel(x, y))
					continue;
				int pixel;
				COLOR c;
				int bestIdx;
				SetColorFromPixel(&c, GetPixel(&pixel, x, y));
				unsigned int hash =
					((c.m_value & 0xf8) | (((c.m_value & 0xfc00) | ((c.m_value >> 3) & 0x1f0000)) >> 2)) >> 3;
				if (m_paletteDecode[16 * hash + (c.m_value >> 28)])
					continue;
				bestIdx = 0;

				int k;
				for (k = 0; k < count; ++k) {
					int dr;
					int dg;
					int db;
					int da;
					distRow[k] = ColorDiff((const COLOR*) &m_palette[k], c);
					if (!distRow[k])
						break;
					if (distRow[k] < distRow[bestIdx])
						bestIdx = k;
				}
				if (k < count) {
					SetPaletteDecodeNumber(c, (unsigned char) k);
					continue;
				}
				if (count < 256) {
					for (k = 0; k < count; ++k) {
						matrix[count][k] = distRow[k];
						matrix[k][count] = distRow[k];
					}
					m_palette[count] = c.m_value;
					SetPaletteDecodeNumber(c, (unsigned char) count++);
					continue;
				}
				int pairJ = 1;
				int pairI = 0;
				for (int j = 1; j < 256; ++j) {
					for (int i = 0; i < j; ++i) {
						if (matrix[j][i] < matrix[pairJ][pairI]) {
							pairJ = j;
							pairI = i;
						}
					}
				}
				if (matrix[pairJ][pairI] < distRow[bestIdx]) {
					SetPaletteDecodeNumber(*(COLOR*) &m_palette[pairI], (unsigned char) bestIdx);
					m_palette[pairI] = c.m_value;
					SetPaletteDecodeNumber(c, (unsigned char) pairI);
					for (k = 0; k < 256; ++k) {
						matrix[pairI][k] = distRow[k];
						matrix[k][pairI] = distRow[k];
					}
				}
				else {
					SetPaletteDecodeNumber(c, (unsigned char) bestIdx);
				}
			}
		}
		NextFrame();
	}
	Rewind();

	int cnt[256];
	int sumR[256];
	int sumB[256];
	int sumG[256];
	int sumA[256];
	memset(cnt, 0, sizeof(cnt));
	memset(sumR, 0, sizeof(sumR));
	memset(sumG, 0, sizeof(sumG));
	memset(sumB, 0, sizeof(sumB));
	memset(sumA, 0, sizeof(sumA));
	int i;
	for (i = 0; i < 0x100000; ++i) {
		if (!m_paletteDecode[i])
			continue;
		++cnt[m_paletteDecode[i]];
		sumR[m_paletteDecode[i]] += ((i / 16) >> 8) & 0xf8;
		sumG[m_paletteDecode[i]] += ((i / 16) >> 3) & 0xfc;
		sumB[m_paletteDecode[i]] += 8 * ((i / 16) & 0x1f);
		sumA[m_paletteDecode[i]] += 16 * (i & 0xf) + 15;
	}
	for (i = 0; i < 256; ++i) {
		if (!cnt[i])
			continue;
		int b = sumB[i] / cnt[i];
		int g = sumG[i] / cnt[i];
		int r = sumR[i] / cnt[i];
		int a = sumA[i] / cnt[i];
		if (a < 0)
			a = 0;
		else if (a > 255)
			a = 255;
		if (r < 0)
			r = 0;
		else if (r > 255)
			r = 255;
		if (g < 0)
			g = 0;
		else if (g > 255)
			g = 255;
		if (b < 0)
			b = 0;
		else if (b > 255)
			b = 255;
		m_palette[i] = b | (g << 8) | (r << 16) | (a << 24);
	}

	if ((m_unk0x42c & 2) && count > 0) {
		for (i = 0; i < count; ++i) {
			if ((pal[i] & 0xff000000) > 0xef000000)
				SetColorAlpha((COLOR*) &pal[i], 255);
			else
				SetColorAlpha((COLOR*) &pal[i], 16 * (pal[i] >> 28));
			unsigned int c = pal[i];
			if (c & 0xff000000) {
				int a = c >> 24;
				int b = 255 * (c & 0xff) / a;
				int g = 255 * ((c >> 8) & 0xff) / a;
				int r = 255 * ((c >> 16) & 0xff) / a;
				if (a < 0)
					a = 0;
				else if (a > 255)
					a = 255;
				if (r < 0)
					r = 0;
				else if (r > 255)
					r = 255;
				if (g < 0)
					g = 0;
				else if (g > 255)
					g = 255;
				if (b < 0)
					b = 0;
				else if (b > 255)
					b = 255;
				pal[i] = b | (g << 8) | (r << 16) | (a << 24);
			}
		}
	}
}

// FUNCTION: ALIEN 0x42bae0
void PICTURE_MAKEVID::WriteSoftware(RESOURCE* p_res)
{
	QS1_CODER* coder = 0;
	if (m_unk0x42c & 0x100)
		coder = new QS1_CODER(1);

	char* buf = (char*) operator new(0x200000);
	if (!buf) {
		{
			STRING name;
			m_color.m_impl->GetName_impl((char**) &name);
			MYERROR::Error(::Error, "PICTURE '%s'", 2,
				// STRING: ALIEN 0x483a2c
				"cdr_s", 0, name.m_str);
		}
		exit(1);
	}
	int* crcs = (int*) operator new(4 * GetNoFrames());
	if (!crcs) {
		{
			STRING name;
			m_color.m_impl->GetName_impl((char**) &name);
			MYERROR::Error(::Error, "PICTURE '%s'", 2,
				// STRING: ALIEN 0x483a24
				"cntrl_s", 0, name.m_str);
		}
		exit(1);
	}

	if (m_unk0x42c & 8) {
		p_res->PreAppend(0x204c4150 /* 'PAL ' */ , 0);
		if (m_unk0x42c & 0x10) {
			if (m_unk0x42c & 2) {
				CreateOnePalette();
				p_res->Write(m_palette, 1024);
			}
			else {
				p_res->Write(m_color.m_impl->m_palette, 1024);
			}
			p_res->PostAppend();
		}
		else {
			unsigned char triplets[768];
			for (int e = 0; e < 256; ++e) {
				triplets[3 * e] = (unsigned char) (m_color.m_impl->m_palette[e] >> 16);
				triplets[3 * e + 1] = (unsigned char) (m_color.m_impl->m_palette[e] >> 8);
				triplets[3 * e + 2] = (unsigned char) m_color.m_impl->m_palette[e];
			}
			p_res->Write(triplets, 768);
			p_res->PostAppend();
		}
	}

	for (int frame = 0; frame < m_color.m_impl->m_noFrames; ++frame) {
		p_res->PreAppend(0x41544144 /* 'DATA' */ , 0);
		int pos = 0;
		int crc = CalcCRC32();
		crcs[frame] = crc;
		for (int dup = 0; dup < frame; ++dup) {
			if (crc == crcs[dup]) {
				pos = 2;
				*(short*) buf = (short) dup;
				break;
			}
		}
		if (dup >= frame) {
			pos += GetShadow(buf + pos);
			int top;
			int bottom;
			GetRectangle(0, &top, 0, &bottom);
			if (m_unk0x42c & 0x100000) {
				bottom = 0;
				top = 0;
			}
			*(short*) (buf + pos) = (short) top;
			pos += 2;
			*(short*) (buf + pos) = (short) (bottom - top);
			pos += 2;
			for (int y = top; y < bottom; ++y) {
				int x = 0;
				for (;;) {
					int runStart = x;
					if (!IsPixel(x, y)) {
						do {
							++x;
						} while (x < m_color.m_impl->m_width && !IsPixel(x, y));
					}
					if (x >= m_color.m_impl->m_width)
						break;
					while (x - runStart > 255) {
						buf[pos++] = (char) 255;
						buf[pos++] = 0;
						runStart += 255;
					}
					buf[pos++] = (char) (x - runStart);
					runStart = x;
					while (x - runStart < 255) {
						if (m_unk0x42c & 4) {
							if (!IsPixel(x, y) && !IsPixel(x + 1, y)
								&& !IsPixel(x + 2, y))
								break;
						}
						else if (!IsPixel(x, y)) {
							break;
						}
						++x;
					}
					buf[pos++] = (char) (x - runStart);
					if (m_unk0x42c & 4) {
						for (int zx = runStart; zx < x; ++zx) {
							*(short*) (buf + pos) = GetPixelZ(zx, y);
							pos += 2;
						}
					}
#pragma inline_depth(0)
					for (int px = runStart; px < x; ++px) {
						if (m_unk0x42c & 8) {
							buf[pos] = (char) GetPixelT(px, y);
							++pos;
						}
						else {
							*(short*) (buf + pos) = (short) GetPixelT(px, y);
							pos += 2;
						}
					}
#pragma inline_depth(8)
				}
				buf[pos++] = 0;
				buf[pos++] = 0;
			}
		}
		p_res->Write(&pos, 4);
		p_res->WritePacked(buf, pos, coder);
		p_res->PostAppend();
		NextFrame();
	}

	if (coder)
		delete coder;
	operator delete(crcs);
	operator delete(buf);
}

// FUNCTION: ALIEN 0x42c070
void PICTURE_MAKEVID::WritePseudo3d(RESOURCE* p_res)
{
	char* buf = (char*) operator new(0x200000);
	if (!buf) {
		MYERROR::Error(::Error, "PICTURE '%s'", 2, "cdr_s", 0,
			STRING(m_color.m_impl->m_name, STRING::CALL_COPY_NONNULL).m_str);
		exit(1);
	}
	int* crcs = (int*) operator new(4 * GetNoFrames());
	if (!crcs) {
		MYERROR::Error(::Error, "PICTURE '%s'", 2, "cntrl_s", 0,
			STRING(m_color.m_impl->m_name, STRING::CALL_COPY_NONNULL).m_str);
		exit(1);
	}
	QS1_CODER* coder;
	if (m_unk0x42c & 0x100)
		coder = new QS1_CODER(1);
	else
		coder = 0;

	if (m_unk0x42c & 2)
		m_unk0x42c &= ~8;
	if (m_unk0x42c & 8) {
		p_res->PreAppend(0x204c4150 /* 'PAL ' */ , 0);
		p_res->Write(m_color.m_impl->m_palette, 1024);
		p_res->PostAppend();
	}

	for (int frame = 0; frame < m_color.m_impl->m_noFrames; ++frame) {
		p_res->PreAppend(0x41544144 /* 'DATA' */ , 0);
		int pos = 0;
		int crc = CalcCRC32();
		crcs[frame] = crc;
		for (int dup = 0; dup < frame; ++dup) {
			if (crc == crcs[dup]) {
				pos = 4;
				*(int*) buf = dup;
				break;
			}
		}
		if (dup >= frame) {
			int left;
			int top;
			int right;
			int bottom;
			GetRectangle(&left, &top, &right, &bottom);
			if (m_unk0x42c & 8)
				*(int*) (buf + pos) = 41;
			else if (m_unk0x42c & 2)
				*(int*) (buf + pos) = 21;
			else
				*(int*) (buf + pos) = 25;
			pos += 4;
			*(short*) (buf + pos) = (short) (right - left);
			pos += 2;
			*(short*) (buf + pos) = (short) (bottom - top);
			pos += 2;
			int y;
			for (y = top; y < bottom; ++y) {
#pragma inline_depth(0)
				for (int x = left; x < right; ++x) {
					if (m_unk0x42c & 8) {
						buf[pos] = (char) GetPixelT(x, y);
						++pos;
					}
					else if (m_unk0x42c & 2) {
						*(unsigned int*) (buf + pos) = GetPixelT(x, y);
						pos += 4;
					}
					else {
						*(short*) (buf + pos) = (short) GetPixelT(x, y);
						pos += 2;
					}
				}
#pragma inline_depth(8)
			}

			int countPos = pos;
			*(int*) (buf + pos) = 0;
			pos += 4;
			*(int*) (buf + pos) = 0;
			pos += 4;
			int vertices = 0;
			int indices = 0;
			short* grid = (short*) operator new(2 * (right - left + 9) * (bottom - top + 9));
			int gridW = right - left + 9;
			for (y = top; y < bottom + 8; y += 8) {
				for (int x = left; x < right + 8; x += 8) {
					if (IsPixelInBox(x - 8, y - 8, 9, 9)) {
						short z = GetBoxZ(x - 8, y - 8, 9, 9);
						*(short*) (buf + pos) = (short) x;
						pos += 2;
						*(short*) (buf + pos) = (short) y;
						pos += 2;
						*(short*) (buf + pos) = z;
						pos += 2;
						*(short*) (buf + pos) = (short) (x - left);
						pos += 2;
						*(short*) (buf + pos) = (short) (y - top);
						pos += 2;
						grid[x - left + gridW * (y - top)] = (short) vertices;
						++vertices;
					}
				}
			}
			*(int*) (buf + countPos) = vertices;
			for (y = top; y < bottom + 8; y += 8) {
				for (int x = left; x < right + 8; x += 8) {
					if (IsPixelInBox(x, y, 8, 8)) {
						*(short*) (buf + pos) = grid[x - left + gridW * (y - top)];
						pos += 2;
						*(short*) (buf + pos) = grid[x + 8 - left + gridW * (y - top)];
						pos += 2;
						*(short*) (buf + pos) = grid[x - left + gridW * (y - top + 8)];
						pos += 2;
						*(short*) (buf + pos) = grid[x + 8 - left + gridW * (y - top + 8)];
						pos += 2;
						*(short*) (buf + pos) = grid[x + 8 - left + gridW * (y - top)];
						pos += 2;
						*(short*) (buf + pos) = grid[x - left + gridW * (y - top + 8)];
						pos += 2;
						indices += 6;
					}
				}
			}
			*(int*) (buf + countPos + 4) = indices;
			operator delete(grid);
		}
		p_res->Write(&pos, 4);
		p_res->WritePacked(buf, pos, coder);
		p_res->PostAppend();
		NextFrame();
	}
	if (coder)
		delete coder;
	operator delete(crcs);
	operator delete(buf);
}

// FUNCTION: ALIEN 0x42c730
int PICTURE_MAKEVID::WriteLight(RESOURCE* p_res)
{
	p_res->PreAppend(0x41544144, 0);
	for (int i = 0; i < m_color.m_impl->m_noFrames; ++i) {
		int c;
		int* pixel = GetPixel(&c, 0, 0);
		p_res->Write(pixel, 4);
		NextFrame();
	}
	return p_res->PostAppend();
}

// FUNCTION: ALIEN 0x42c860
int PICTURE_MAKEVID::IsPixelInBox(int p_x, int p_y, int p_w, int p_h)
{
	int x = p_x;
	p_w += x;
	p_h += p_y;
	for (; p_y < p_h; ++p_y) {
		for (p_x = x; p_x < p_w; ++p_x) {
			if (IsPixel(p_x, p_y))
				return 1;
		}
	}
	return 0;
}

// FUNCTION: ALIEN 0x42c8c0
int PICTURE_MAKEVID::MakeVid(unsigned int p_flags, STRING p_name)
{
	RESOURCE out;
	int word;
	if (p_flags & 0x10)
		m_unk0x42c |= 0x1000;
	else if (p_flags & 1)
		m_unk0x42c |= 0x20;
	if (p_flags & 2)
		m_unk0x42c |= 0x100;
	if (p_flags & 4)
		m_unk0x42c |= 0x20000;
	if (p_flags & 8)
		m_unk0x42c |= 0x800;
	if (ImplOf(m_color)->m_pixels && ImplOf(m_color)->m_bpp == 1)
		m_unk0x42c |= 8;
	if (m_unk0x42c & 0x20)
		m_unk0x42c &= ~8;

	if (!m_color.m_impl->m_pixels && !m_alpha.m_impl->m_pixels) {
		{
			STRING name;
			m_color.m_impl->GetName_impl((char**) &name);
			MYERROR::Error(::Error,
				"PICTURE '%s'", 4,
				// STRING: ALIEN 0x483a68
				"not picture3", 0, name.m_str);
			#pragma inline_depth(0)
		}
		#pragma inline_depth(8)
		return 1;
	}
	if (!(m_unk0x42c & 4) && (m_unk0x42c & 0x1000)) {
		{
			STRING name;
			m_color.m_impl->GetName_impl((char**) &name);
			MYERROR::Error(::Error, "PICTURE '%s'", 10,
				// STRING: ALIEN 0x483a48
				"Unsupported files combination", 0, name.m_str);
			#pragma inline_depth(0)
		}
		#pragma inline_depth(8)
		return 1;
	}
	if (!strcmp(p_name.m_str, empty_str)) {
		STRING src(m_color.m_impl->m_name);
		{
			STRING base;
			p_name = *(STRING*) src.Before((char**) &base, ".") +
				// STRING: ALIEN 0x483a40
				".vid";
		}
	}

	if (out.OpenForWrite(p_name, 0x20444956 /* 'VID ' */ )) {
		{
			STRING name;
			m_color.m_impl->GetName_impl((char**) &name);
			MYERROR::Error(::Error, "PICTURE '%s'", 3,
				// STRING: ALIEN 0x483a34
				"file (.vid)", 0, name.m_str);
			#pragma inline_depth(0)
		}
		#pragma inline_depth(8)
		return 1;
	}
	out.PreAppend(0x44414548 /* 'HEAD' */ , 0);
	m_unk0x42c |= 0x10;
	word = m_unk0x42c;
	out.Write(&word, 2);
	word = m_color.m_impl->m_unk0x0c;
	out.Write(&word, 2);
	word = m_color.m_impl->m_noFrames;
	out.Write(&word, 2);
	word = m_color.m_impl->m_width;
	out.Write(&word, 2);
	word = m_color.m_impl->m_height;
	out.Write(&word, 2);
	out.PostAppend();

	if (p_flags & 0x20)
		m_unk0x42c |= 0x80000;
	if (p_flags & 0x40)
		m_unk0x42c |= 0x100000;
	if (m_unk0x42c & 0x1000)
		WritePseudo3d(&out);
	else if (m_unk0x42c & 0x80)
		WriteLight(&out);
	else if ((m_unk0x42c & 0x20) && (m_unk0x42c & 2) && (m_unk0x42c & 4))
		WriteSoftware(&out);
	else if (m_unk0x42c & 0x20)
		WriteHardware(&out);
	else
		WriteSoftware(&out);
	out.Close();
	return 0;
}
