uint GraphFactor(uint factor, uint source, uint destination, uint sa, uint da)
{
	switch (factor) {
	case 1: return 0;
	case 3: return source;
	case 4: return 255 - source;
	case 5: return sa;
	case 6: return 255 - sa;
	case 7: return da;
	case 8: return 255 - da;
	case 9: return destination;
	case 10: return 255 - destination;
	default: return 255;
	}
}

uint3 GraphRGB(uint color)
{
	return uint3((color >> 16) & 255, (color >> 8) & 255, color & 255);
}

uint GraphPack(uint3 color)
{
	color = min(color, 255u);
	return 0xff000000u | (color.r << 16) | (color.g << 8) | color.b;
}

uint GraphBlend(Command c, uint source, uint destination)
{
	uint3 src = GraphRGB(source), dst = GraphRGB(destination);
	if (c.p[40] == 0) return GraphPack(src);
	uint3 result;
	for (uint channel = 0; channel < 3; ++channel) {
		result[channel] = (src[channel] * GraphFactor(c.p[41], src[channel], dst[channel], source >> 24, destination >> 24)
			+ dst[channel] * GraphFactor(c.p[42], src[channel], dst[channel], source >> 24, destination >> 24)) / 255;
	}
	return GraphPack(result);
}

bool GraphCompare(uint function, uint source, uint destination)
{
	switch (function) {
	case 1: return false;
	case 2: return source < destination;
	case 3: return source == destination;
	case 4: return source <= destination;
	case 5: return source > destination;
	case 6: return source != destination;
	case 7: return source >= destination;
	default: return true;
	}
}

bool GraphFragment(Command c, uint index, float z, uint source)
{
	if (c.p[45] != 0 && !GraphCompare(c.p[46], source >> 24, c.p[47])) return false;
	uint fixedZ = (uint) floor(clamp(z * 65536.0, 0.0, 65535.0) + 0.5);
	if (!GraphCompare(c.p[44], fixedZ, heap[depthOffset + index] & 65535u)) return false;
	if (c.p[43] != 0) heap[depthOffset + index] = fixedZ;
	heap[colorOffset + index] = GraphBlend(c, source, heap[colorOffset + index]);
	return true;
}

float2 GraphAdd(float2 a, float2 b)
{
	precise float sum = a.x + b.x;
	precise float bridge = sum - a.x;
	precise float remainder = ((a.x - (sum - bridge)) + (b.x - bridge)) + a.y + b.y;
	precise float high = sum + remainder;
	precise float low = remainder - (high - sum);
	return float2(high, low);
}

float2 GraphMultiply(float2 a, float2 b)
{
	precise float product = a.x * b.x;
	precise float asplit = a.x * 4097.0, bsplit = b.x * 4097.0;
	precise float ahi = asplit - (asplit - a.x), bhi = bsplit - (bsplit - b.x);
	precise float alo = a.x - ahi, blo = b.x - bhi;
	precise float error = ((ahi * bhi - product) + ahi * blo + alo * bhi) + alo * blo;
	precise float remainder = error + a.x * b.y + a.y * b.x;
	precise float high = product + remainder;
	precise float low = remainder - (high - product);
	return float2(high, low);
}

float2 GraphDivide(float2 numerator, float2 denominator)
{
	precise float high = numerator.x / denominator.x;
	float2 remainder = GraphAdd(numerator, -GraphMultiply(denominator, float2(high, 0)));
	precise float low = (remainder.x + remainder.y) / denominator.x;
	return GraphAdd(float2(high, 0), float2(low, 0));
}

float2 GraphPair(Command c, uint offset)
{
	return asfloat(uint2(c.p[offset], c.p[offset + 1]));
}

float2 GraphPreciseEdge(Command c, uint edge, int2 pixel)
{
	uint offset = 13 + edge * 6;
	float2 value = GraphAdd(GraphPair(c, offset), GraphMultiply(GraphPair(c, offset + 4), float2(pixel.y - c.top, 0)));
	return GraphAdd(value, GraphMultiply(GraphPair(c, offset + 2), float2(pixel.x - c.left, 0)));
}

uint GraphFixedDepth(float2 value)
{
	if (value.x < 0) return 0;
	if (value.x >= 65535) return 65535;
	precise float whole = floor(value.x);
	precise float remainder = (value.x - whole) + value.y;
	if (remainder < 0) { whole -= 1; remainder += 1; }
	return (uint) clamp(whole + (remainder >= 0.5 ? 1 : 0), 0.0, 65535.0);
}

float GraphEdge(float2 a, float2 b, float2 p)
{
	precise float dx = b.x - a.x;
	precise float dy = b.y - a.y;
	precise float first = dx * (p.y - a.y);
	precise float second = dy * (p.x - a.x);
	return first - second;
}

bool GraphInside(float value, float2 a, float2 b)
{
	return value > 0 || (value == 0 && (b.y < a.y || (b.y == a.y && b.x > a.x)));
}

uint GraphInterpolate(uint a, uint b, uint c, float3 weights)
{
	uint result = 0;
	for (uint shift = 0; shift < 32; shift += 8) {
		float value = ((a >> shift) & 255) * weights.x + ((b >> shift) & 255) * weights.y + ((c >> shift) & 255) * weights.z;
		result |= ((uint) clamp(value + 0.5, 0.0, 255.0)) << shift;
	}
	return result;
}

int GraphLightDepth(Command c, float x, float y, int ix, int iy)
{
	if (x < asfloat(c.p[11]) || y < asfloat(c.p[12]) || x >= asfloat(c.p[13]) || y >= asfloat(c.p[14]) ||
		ix < 0 || iy < 0 || ix >= (int) frameWidth || iy >= (int) frameHeight) return 32767;
	return (int) ((heap[depthOffset + iy * frameWidth + ix] & 65535u) >> 3) - 128;
}

void DrawGraph(Command c, int2 pixel)
{
	uint index = pixel.y * frameWidth + pixel.x;
	if (c.op == 300) {
		heap[colorOffset + index] = c.p[0];
	}
	else if (c.op == 301) {
		uint3 color = GraphRGB(c.p[0]);
		if (c.p[2] != 0) color = min(color + GraphRGB(c.p[1]), 255u);
		uint source = (c.p[0] & 0xff000000u) | (color.r << 16) | (color.g << 8) | color.b;
		heap[colorOffset + index] = GraphBlend(c, source, heap[colorOffset + index]);
	}
	else if (c.op == 302) {
		float3 a = asfloat(uint3(c.p[0], c.p[1], c.p[2]));
		float3 b = asfloat(uint3(c.p[3], c.p[4], c.p[5]));
		float3 d = asfloat(uint3(c.p[6], c.p[7], c.p[8]));
		float2 e0 = GraphPreciseEdge(c, 0, pixel), e1 = GraphPreciseEdge(c, 1, pixel), e2 = GraphPreciseEdge(c, 2, pixel);
		float3 edges = float3(e0.x + e0.y, e1.x + e1.y, e2.x + e2.y);
		if (!GraphInside(edges.x, a.xy, b.xy) || !GraphInside(edges.y, b.xy, d.xy) || !GraphInside(edges.z, d.xy, a.xy)) return;
		float2 areaPair = GraphPair(c, 31);
		float area = areaPair.x + areaPair.y;
		if (!(area > 0) || !isfinite(area)) return;
		float3 weights = edges.yzx / area;
		uint source = c.p[12] != 0 ? c.p[9] : GraphInterpolate(c.p[9], c.p[10], c.p[11], weights);
		if (c.p[45] != 0 && !GraphCompare(c.p[46], source >> 24, c.p[47])) return;
		float2 numerator = GraphAdd(GraphAdd(GraphMultiply(e1, float2(a.z * 65536.0, 0)),
			GraphMultiply(e2, float2(b.z * 65536.0, 0))), GraphMultiply(e0, float2(d.z * 65536.0, 0)));
		float2 z = a.z == b.z && a.z == d.z ? float2(a.z * 65536.0, 0) : GraphDivide(numerator, areaPair);
		uint fixedZ = GraphFixedDepth(z);
		if (!GraphCompare(c.p[44], fixedZ, heap[depthOffset + index] & 65535u)) return;
		if (c.p[43] != 0) heap[depthOffset + index] = fixedZ;
		heap[colorOffset + index] = GraphBlend(c, source, heap[colorOffset + index]);
	}
	else if (c.op == 303) {
		float3 a = asfloat(uint3(c.p[0], c.p[1], c.p[2]));
		float3 b = asfloat(uint3(c.p[3], c.p[4], c.p[5]));
		float2 delta = b.xy - a.xy;
		bool horizontal = abs(delta.x) >= abs(delta.y);
		float extent = horizontal ? delta.x : delta.y;
		float numerator = horizontal ? pixel.x - a.x : pixel.y - a.y;
		float scale = asfloat(c.p[8]);
		float2 quotient = extent == 0 ? float2(0, 0) : GraphDivide(float2(numerator * scale, 0), float2(extent * scale, 0));
		precise float t = quotient.x + quotient.y;
		if (!isfinite(t)) return;
		if (t < 0 || t > 1) return;
		float2 projected = a.xy + t * delta;
		if (any(abs(projected - pixel) > 0.5)) return;
		uint source = GraphInterpolate(c.p[6], c.p[7], 0, float3(1 - t, t, 0));
		GraphFragment(c, index, a.z + t * (b.z - a.z), source);
	}
	else if (c.op == 304) {
		heap[c.p[0] + index] = heap[colorOffset + index];
	}
	else if (c.op == 305) {
		uint old = heap[c.p[0] + index];
		uint3 low = uint3((old >> 19) & 31, (old >> 10) & 63, (old >> 3) & 31);
		uint3 oldColor = uint3((low.r << 3) | (low.r >> 2), (low.g << 2) | (low.g >> 4), (low.b << 3) | (low.b >> 2));
		uint3 current = GraphRGB(heap[colorOffset + index]);
		heap[colorOffset + index] = GraphPack((oldColor * (255 - c.p[1]) + current * c.p[1]) / 255);
	}
	else if (c.op == 306) {
		uint source = c.p[0] + (pixel.y - c.top + c.p[3]) * c.p[1] + pixel.x - c.left + c.p[2];
		heap[depthOffset + index] = heap[source] & 65535u;
	}
	else if (c.op == 307) {
		int col = pixel.x * 4 - (int) c.p[6], row = pixel.y * 4 - (int) c.p[7];
		float x = asfloat(c.p[4]), y = asfloat(c.p[5]);
		int za = GraphLightDepth(c, x + col, y + row, (int) x + col, (int) y + row);
		int zb = GraphLightDepth(c, x + col + 3, y + row + 3, (int) x + col + 3, (int) y + row + 3);
		za = min(za, zb);
		int intensity = 0;
		if (za != 32767) {
			int d = row + za - (int) c.p[9], zd = za - (int) c.p[8];
			d = 9 * d * d / 4 + zd * zd / 4 + col * col;
			intensity = clamp(256 - (c.p[10] != 0 ? d / (int) c.p[10] : d), 0, 255);
		}
		heap[c.p[0] + pixel.y * c.p[1] + pixel.x] = c.p[2] != 0 ? (uint) intensity : LoadWord16(c.p[3], intensity * 2);
	}
	else if (c.op == 308) {
		int x = (int) c.p[4] + pixel.x * 4, y = (int) c.p[5] + pixel.y * 4;
		int xEnd = min(x + 3, (int) c.p[6] - 1);
		int z = (int) min(heap[depthOffset + y * frameWidth + x] & 65535u, heap[depthOffset + y * frameWidth + xEnd] & 65535u) - 1024;
		int base = (int) c.p[7], farZ = (int) c.p[8];
		uint value = 0xffffffffu;
		if (z <= base) {
			if (c.p[2] != 0) value = z <= farZ ? 255u : LoadByte(c.p[3], 2 * (base - z));
			else value = LoadWord16(c.p[3], 2 * (base - max(z, farZ)));
		}
		else if (z <= base + 10) value = 0;
		heap[c.p[0] + pixel.y * c.p[1] + pixel.x] = value;
	}
	else if (c.op == 309) {
		uint i = pixel.y * c.p[1] + pixel.x;
		uint value = heap[c.p[0] + i];
		if (value == 0xffffffffu && i >= c.p[3]) value = heap[c.p[0] + i - c.p[3]];
		heap[c.p[2] + i] = value;
	}
	else if (c.op == 310) {
		uint value = heap[c.p[0] + pixel.y * c.p[1] + pixel.x];
		heap[c.p[2] + pixel.y * c.p[3] + pixel.x] = value == 0xffffffffu ? 0 : value;
	}
	else if (c.op == 311) {
		int x = min((int) c.p[4] + pixel.x * 4, (int) frameWidth - 1);
		int y = min((int) c.p[5] + pixel.y * 4, (int) frameHeight - 1);
		int current = (int) (heap[depthOffset + y * frameWidth + x] & 65535u);
		int dz = -1;
		if (y >= 4) {
			int difference = abs(current - (int) (heap[depthOffset + (y - 4) * frameWidth + x] & 65535u));
			if (difference <= 6) dz = difference;
		}
		if (dz < 0 && y + 4 < (int) frameHeight) {
			int difference = abs(current - (int) (heap[depthOffset + (y + 4) * frameWidth + x] & 65535u));
			if (difference <= 6) dz = difference;
		}
		uint intensity = dz < 0 ? 0 : min(255u, ((6u - (uint) dz) * c.p[6]) >> 3);
		heap[c.p[0] + pixel.y * c.p[1] + pixel.x] = c.p[2] != 0 ? intensity : LoadWord16(c.p[3], intensity * 2);
	}
}
