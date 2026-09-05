uint TextureExpand5(uint value) { return (value << 3) | (value >> 2); }
uint TextureExpand6(uint value) { return (value << 2) | (value >> 4); }
uint TextureExpand565(uint value) {
	return 0xff000000u | (TextureExpand5((value >> 11) & 31) << 16) |
		(TextureExpand6((value >> 5) & 63) << 8) | TextureExpand5(value & 31);
}
uint TextureDecode(uint value, uint format, uint palette) {
	if (format == 21) return value;
	if (format == 20 || format == 22) return value | 0xff000000u;
	if (format == 23) return TextureExpand565(value);
	if (format == 24 || format == 25) {
		uint alpha = format == 25 && (value & 32768) == 0 ? 0 : 255;
		return (alpha << 24) | (TextureExpand5((value >> 10) & 31) << 16) |
			(TextureExpand5((value >> 5) & 31) << 8) | TextureExpand5(value & 31);
	}
	if (format == 26) return (((value >> 12) & 15) * 17u << 24) | ((value & 0x0f00) << 8) * 17u |
		(value & 0xf0) * 0x110u | (value & 15) * 17u;
	if (format == 41) return palette != 0 ? heap[palette + (value & 255)] : 0;
	return 0;
}
uint TextureSample(Command c, int2 coordinate) {
	coordinate = clamp(coordinate, int2(0, 0), int2(c.p[4], c.p[5]) - 1);
	uint raw = heap[c.p[0] + coordinate.y * c.p[4] + coordinate.x];
	return TextureDecode(raw, c.p[6], c.p[7]);
}
uint TextureLerp(uint first, uint second, uint fraction) {
	uint result = 0;
	for (uint shift = 0; shift < 32; shift += 8) {
		int a = (first >> shift) & 255;
		int b = (second >> shift) & 255;
		result |= uint(a + (((b - a) * int(fraction)) >> 8)) << shift;
	}
	return result;
}
uint TextureBlendFactor(uint factor, uint source, uint destination, uint sourceAlpha, uint destinationAlpha) {
	if (factor == 1) return 0;
	if (factor == 3) return source;
	if (factor == 4) return 255 - source;
	if (factor == 5) return sourceAlpha;
	if (factor == 6) return 255 - sourceAlpha;
	if (factor == 7) return destinationAlpha;
	if (factor == 8) return 255 - destinationAlpha;
	if (factor == 9) return destination;
	if (factor == 10) return 255 - destination;
	return 255;
}
void DrawTexture(Command c, int2 pixel) {
	if (c.op == 100) {
		uint byteOffset = pixel.y * c.p[8];
		uint value = 0;
		if (c.p[6] == 20) {
			byteOffset += pixel.x * 3;
			value = LoadByte(c.p[0], byteOffset) | (LoadByte(c.p[0], byteOffset + 1) << 8) |
				(LoadByte(c.p[0], byteOffset + 2) << 16);
		} else if (c.p[6] == 21 || c.p[6] == 22) {
			value = heap[c.p[0] + byteOffset / 4 + pixel.x];
		} else if (c.p[6] == 41 || c.p[6] == 28) {
			value = LoadByte(c.p[0], byteOffset + pixel.x);
		} else value = LoadWord16(c.p[0], byteOffset + pixel.x * 2);
		heap[c.p[1] + pixel.y * c.p[3] + pixel.x] = value;
		return;
	}
	if (c.op == 104) {
		bool visible = c.p[30] != 0 && heap[c.p[30]] != 0;
		int left = int(c.p[9]), top = int(c.p[10]), right = int(c.p[4]), bottom = int(c.p[5]);
		if (!visible && left < right && top < bottom) {
			if (c.p[27] != 0) {
				int xs[3] = {left, left + (right - left) / 2, right - 1};
				int ys[3] = {top, top + (bottom - top) / 2, bottom - 1};
				for (int yy = 0; yy < 3; ++yy)
					for (int xx = 0; xx < 3; ++xx)
						visible = visible || int(heap[c.p[0] + ys[yy] * c.p[3] + xs[xx]] & 65535) <= int(c.p[18]);
			} else {
				int x = int(c.p[13]), y = int(c.p[14]);
				if (x >= left && x < right && y >= top && y < bottom)
					visible = int(heap[c.p[0] + y * c.p[3] + x] & 65535) <= int(c.p[18]);
			}
		}
		heap[c.p[1]] = visible ? 1 : 0;
		return;
	}
	if (c.p[30] != 0 && heap[c.p[30]] == 0) return;
	uint destinationIndex = c.p[1] + pixel.y * c.p[3] + pixel.x;
	if (c.op == 103) {
		uint source = heap[c.p[0] + (pixel.y - c.top + c.p[14]) * c.p[4] + pixel.x - c.left + c.p[13]];
		if (c.p[6] == 23) source = ((source >> 8) & 0xf800) | ((source >> 5) & 0x7e0) | ((source >> 3) & 31);
		heap[destinationIndex] = source;
		return;
	}
	if (c.op == 102) {
		int2 delta = pixel - int2(c.p[9], c.p[10]);
		int2 coordinate = int2(c.p[13], c.p[14]) + int2(
			((2 * delta.x + 1) * c.p[31]) / (2 * c.p[11]),
			((2 * delta.y + 1) * c.p[32]) / (2 * c.p[12]));
		coordinate = clamp(coordinate, int2(0, 0), int2(c.p[4], c.p[5]) - 1);
		heap[destinationIndex] = heap[c.p[0] + coordinate.y * c.p[4] + coordinate.x] & 65535;
		return;
	}
	uint depthValue = 0;
	uint depthIndex = c.p[2] + pixel.y * c.p[3] + pixel.x;
	if (c.p[27] != 0) {
		depthValue = heap[c.p[29] + pixel.y - c.top];
		if (depthValue < (heap[depthIndex] & 65535)) return;
	}
	int2 delta = pixel - int2(c.p[9], c.p[10]);
	int2 uv = delta * int2(c.p[15], c.p[16]) + int2(c.p[15], c.p[16]) / 2 + (int2(c.p[13], c.p[14]) << 16);
	uint texel;
	if (c.p[17] != 0) {
		uv -= 32768;
		int2 xy = uv >> 16;
		uint2 fraction = uint2((uv >> 8) & 255);
		texel = TextureLerp(TextureLerp(TextureSample(c, xy), TextureSample(c, xy + int2(1, 0)), fraction.x),
			TextureLerp(TextureSample(c, xy + int2(0, 1)), TextureSample(c, xy + int2(1, 1)), fraction.x), fraction.y);
	} else texel = TextureSample(c, uv >> 16);
	uint alpha = ((texel >> 24) * ((c.p[18] >> 24) + 1)) >> 8;
	if (c.p[25] != 0 && alpha <= c.p[26]) return;
	if (alpha == 0 && c.p[22] != 0 && c.p[23] == 5 && c.p[24] == 6) return;
	if (c.p[27] != 0 && c.p[28] != 0) heap[depthIndex] = depthValue;
	uint previous = heap[destinationIndex];
	uint result = 0xff000000u;
	for (uint shift = 0; shift < 24; shift += 8) {
		uint source = (((texel >> shift) & 255) * (((c.p[18] >> shift) & 255) + 1)) >> c.p[20];
		if (c.p[21] != 0) source += (c.p[19] >> shift) & 255;
		source = min(source, 255u);
		uint destination = (previous >> shift) & 255;
		if (c.p[22] != 0) source = min(255u, (source * TextureBlendFactor(c.p[23], source, destination, alpha, previous >> 24) +
			destination * TextureBlendFactor(c.p[24], source, destination, alpha, previous >> 24)) / 255);
		result |= source << shift;
	}
	heap[destinationIndex] = result;
}

uint VidExpand16(uint value, uint redShift, uint greenShift) {
	return 0xff000000u | ((value & 31) << 3) | ((value << (8 - greenShift)) & 0xff00) |
		((value << (16 - redShift)) & 0xff0000);
}
void DrawVid(Command c, int2 pixel) {
	if (c.op == 200) {
		uint destination = c.p[2] + (pixel.y * c.p[3] + pixel.x) * 2;
		heap[destination] = 0;
		heap[destination + 1] = 0;
		uint cursor = heap[c.p[1] + pixel.y], x = 0;
		for (;;) {
			uint skip = LoadByte(c.p[0], cursor), count = LoadByte(c.p[0], cursor + 1);
			cursor += 2;
			if ((skip | count) == 0) return;
			x += skip;
			if (uint(pixel.x) >= x && uint(pixel.x) < x + count) {
				uint index = uint(pixel.x) - x;
				uint value, depth = 0;
				if (c.p[4] == 3) {
					depth = LoadWord16(c.p[0], cursor + index * 2);
					value = LoadByte(c.p[0], cursor + count * 2 + index);
				} else if (c.p[4] == 2) value = LoadWord16(c.p[0], cursor + index * 2);
				else value = LoadByte(c.p[0], cursor + index);
				heap[destination] = value;
				heap[destination + 1] = 0x10000u | depth;
				return;
			}
			x += count;
			if (x > uint(pixel.x)) return;
			cursor += count * c.p[4];
		}
	}
	uint coverageIndex = c.p[22] + (pixel.y - c.top) * c.p[23] + pixel.x - c.left;
	if (c.p[22] != 0) heap[coverageIndex] = 0;
	uint sourceX = heap[c.p[10] + pixel.x - int(c.p[13])];
	uint sourceY = heap[c.p[11] + pixel.y - int(c.p[14])];
	if (sourceX == 0xffffffffu || sourceY == 0xffffffffu) return;
	if ((sourceX & 0x80000000u) != 0) {
		if (sourceY >= 8) return;
		uint bridge = c.p[0] + (sourceY * c.p[4] + c.p[20]) * 2;
		if (heap[bridge + 1] == 0 || heap[bridge + 3] == 0) return;
		sourceX &= 0x7fffffffu;
	}
	uint input = c.p[0] + (sourceY * c.p[4] + sourceX) * 2;
	uint presence = heap[input + 1];
	if (presence == 0) return;
	uint raw = heap[input];
	int depth = int(heap[c.p[12] + pixel.y - int(c.p[14])]);
	if (c.p[7] != 0) depth += int(presence << 16) >> 16;
	uint depthWord = uint(depth) & 65535;
	uint index = pixel.y * c.p[3] + pixel.x;
	uint oldDepth = heap[c.p[2] + index] & 65535;
	if (c.p[6] == 1) {
		if ((int(depthWord << 16) >> 16) <= (int(oldDepth << 16) >> 16)) return;
	} else if (c.p[6] == 0 || c.p[21] != 0) {
		if (depthWord < oldDepth) return;
	} else if (depth < int(oldDepth)) return;
	uint color;
	uint packed = raw;
	if (c.p[8] != 0) color = VidExpand16(raw, c.p[16], c.p[17]);
	else if (c.p[9] != 0) {
		packed = LoadWord16(c.p[5], raw * 2);
		color = VidExpand16(packed, c.p[16], c.p[17]);
	} else color = heap[c.p[5] + raw];
	if (c.p[6] == 0) {
		uint previous = heap[c.p[1] + index];
		if (c.p[15] != 0) previous = VidExpand16(previous, c.p[16], c.p[17]);
		uint alpha = (color >> 24) + 1;
		uint result = 0xff000000u;
		for (uint shift = 0; shift < 24; shift += 8)
			result |= ((((color >> shift) & 255) * alpha + ((previous >> shift) & 255) * (256 - alpha)) >> 8) << shift;
		if (c.p[22] != 0 && alpha == 256) heap[coverageIndex] = 1;
		if (c.p[15] != 0) result = ((result >> 3) & 31) | (c.p[18] & (result >> (16 - c.p[16]))) |
			(c.p[19] & (result >> (8 - c.p[17])));
		heap[c.p[1] + index] = result;
	} else {
		heap[c.p[2] + index] = depthWord;
		heap[c.p[1] + index] = c.p[15] != 0 ? packed : color;
		if (c.p[22] != 0) heap[coverageIndex] = 1;
	}
}
