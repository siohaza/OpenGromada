#include "platform/cursor.h"

#include "platform/paths.h"

#include <SDL3/SDL.h>
#include <stdlib.h>
#include <string.h>

static unsigned int ReadU16(const unsigned char* p_at)
{
	return (unsigned int) p_at[0] | ((unsigned int) p_at[1] << 8);
}

static unsigned int ReadU32(const unsigned char* p_at)
{
	return (unsigned int) p_at[0] | ((unsigned int) p_at[1] << 8) | ((unsigned int) p_at[2] << 16) |
		   ((unsigned int) p_at[3] << 24);
}

static const unsigned char* FindAconFrame(const unsigned char* p_data, size_t p_size, size_t* p_frameSize)
{
	if (p_size < 12 || memcmp(p_data, "RIFF", 4) != 0 || memcmp(p_data + 8, "ACON", 4) != 0) {
		return 0;
	}

	size_t at = 12;
	while (at + 8 <= p_size) {
		size_t size = ReadU32(p_data + at + 4);
		if (size > p_size - at - 8) {
			return 0;
		}
		if (memcmp(p_data + at, "LIST", 4) == 0 && size >= 4) {
			size_t inner = at + 12;
			size_t end = at + 8 + size;
			while (inner + 8 <= end) {
				size_t innerSize = ReadU32(p_data + inner + 4);
				if (innerSize > end - inner - 8) {
					return 0;
				}
				if (memcmp(p_data + inner, "icon", 4) == 0) {
					*p_frameSize = innerSize;
					return p_data + inner + 8;
				}
				inner += 8 + innerSize + (innerSize & 1);
			}
		}
		at += 8 + size + (size & 1);
	}
	return 0;
}

static SDL_Surface* DecodeIcon(const unsigned char* p_data, size_t p_size, int* p_hotX, int* p_hotY)
{
	if (p_size < 22) {
		return 0;
	}
	unsigned int type = ReadU16(p_data + 2);
	unsigned int count = ReadU16(p_data + 4);
	if (count == 0) {
		return 0;
	}

	const unsigned char* entry = p_data + 6;
	*p_hotX = type == 2 ? (int) ReadU16(entry + 4) : 0;
	*p_hotY = type == 2 ? (int) ReadU16(entry + 6) : 0;
	size_t offset = ReadU32(entry + 12);
	if (offset + 40 > p_size) {
		return 0;
	}

	const unsigned char* bmp = p_data + offset;
	unsigned int headerSize = ReadU32(bmp);
	int width = (int) ReadU32(bmp + 4);
	int doubleHeight = (int) ReadU32(bmp + 8);
	unsigned int bpp = ReadU16(bmp + 14);
	unsigned int compression = ReadU32(bmp + 16);
	if (headerSize < 40 || compression != 0 || width <= 0 || doubleHeight <= 0) {
		return 0;
	}

	int height = doubleHeight / 2;
	if (height <= 0 || width > 256 || height > 256) {
		return 0;
	}

	unsigned int paletteEntries = 0;
	if (bpp <= 8) {
		paletteEntries = ReadU32(bmp + 32);
		if (paletteEntries == 0) {
			paletteEntries = 1u << bpp;
		}
	}
	const unsigned char* palette = bmp + headerSize;
	const unsigned char* xorBits = palette + 4 * paletteEntries;

	size_t xorPitch = ((size_t) width * bpp + 31) / 32 * 4;
	size_t maskPitch = ((size_t) width + 31) / 32 * 4;
	const unsigned char* andBits = xorBits + xorPitch * height;
	if ((size_t) (andBits + maskPitch * height - p_data) > p_size) {
		return 0;
	}

	SDL_Surface* surface = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_ARGB8888);
	if (!surface) {
		return 0;
	}

	for (int y = 0; y < height; ++y) {
		const unsigned char* xorRow = xorBits + xorPitch * (height - 1 - y);
		const unsigned char* andRow = andBits + maskPitch * (height - 1 - y);
		unsigned int* out = (unsigned int*) ((unsigned char*) surface->pixels + surface->pitch * y);

		for (int x = 0; x < width; ++x) {
			unsigned int masked = (andRow[x / 8] >> (7 - (x & 7))) & 1;
			unsigned int color;
			unsigned int alpha = 0xff;

			switch (bpp) {
			case 1: {
				unsigned int bit = (xorRow[x / 8] >> (7 - (x & 7))) & 1;
				const unsigned char* pe = palette + 4 * bit;
				color = ((unsigned int) pe[2] << 16) | ((unsigned int) pe[1] << 8) | pe[0];
				break;
			}
			case 4: {
				unsigned int idx = (x & 1) ? (xorRow[x / 2] & 0xf) : (unsigned int) (xorRow[x / 2] >> 4);
				const unsigned char* pe = palette + 4 * idx;
				color = ((unsigned int) pe[2] << 16) | ((unsigned int) pe[1] << 8) | pe[0];
				break;
			}
			case 8: {
				const unsigned char* pe = palette + 4 * xorRow[x];
				color = ((unsigned int) pe[2] << 16) | ((unsigned int) pe[1] << 8) | pe[0];
				break;
			}
			case 24: {
				const unsigned char* px = xorRow + 3 * x;
				color = ((unsigned int) px[2] << 16) | ((unsigned int) px[1] << 8) | px[0];
				break;
			}
			case 32: {
				const unsigned char* px = xorRow + 4 * x;
				color = ((unsigned int) px[2] << 16) | ((unsigned int) px[1] << 8) | px[0];
				alpha = px[3];
				masked = 0;
				break;
			}
			default:
				SDL_DestroySurface(surface);
				return 0;
			}

			if (masked) {
				alpha = color ? 0xff : 0x00;
			}

			out[x] = (alpha << 24) | color;
		}
	}

	return surface;
}

SDL_Cursor* Platform_LoadCursor(const char* p_path)
{
	if (!p_path) {
		return 0;
	}
	FILE* file = Platform_FOpen(p_path, "rb");
	if (!file) {
		return 0;
	}

	fseek(file, 0, SEEK_END);
	long length = ftell(file);
	fseek(file, 0, SEEK_SET);
	if (length <= 0 || length > (1 << 20)) {
		fclose(file);
		return 0;
	}
	unsigned char* data = (unsigned char*) malloc((size_t) length);
	if (!data) {
		fclose(file);
		return 0;
	}
	size_t got = fread(data, 1, (size_t) length, file);
	fclose(file);

	size_t frameSize = got;
	const unsigned char* image = FindAconFrame(data, got, &frameSize);
	if (!image) {
		image = data;
		frameSize = got;
	}

	int hotX = 0;
	int hotY = 0;
	SDL_Surface* surface = DecodeIcon(image, frameSize, &hotX, &hotY);
	free(data);
	if (!surface) {
		return 0;
	}

	SDL_Cursor* cursor = SDL_CreateColorCursor(surface, hotX, hotY);
	SDL_DestroySurface(surface);
	return cursor;
}

SDL_Cursor* Platform_DefaultCursor()
{
	return SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
}

void Platform_SetCursor(SDL_Cursor* p_cursor)
{
	if (p_cursor) {
		SDL_SetCursor(p_cursor);
		SDL_ShowCursor();
	}
	else {
		SDL_HideCursor();
	}
}

void Platform_FreeCursor(SDL_Cursor* p_cursor)
{
	if (p_cursor) {
		SDL_DestroyCursor(p_cursor);
	}
}
