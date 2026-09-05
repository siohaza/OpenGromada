#pragma once

#include "util/resource.h"

#include <bit>
#include <cmath>
#include <cstdint>




struct LEGACY_MAP_HEADER {
	float width = 0, height = 0, shiftX = 0, shiftY = 0;
	uint32_t time = 0;
	int version = 0;

	bool Read(RESOURCE& resource, bool old)
	{
		LEGACY_MAP_HEADER value;
		if (resource.Remaining() < (old ? 20 : 24)) return resource.Fail("truncated MAP HEAD prefix");
		uint32_t x = 0, y = 0;
		resource.ReadWords(&x, 4);
		resource.ReadWords(&y, 4);
		if (old) {
			int16_t sx = 0, sy = 0;
			resource.ReadWords(&sx, 2, 2);
			resource.ReadWords(&sy, 2, 2);
			value.shiftX = sx;
			value.shiftY = sy;
		}
		else {
			resource.ReadWords(&value.shiftX, 4);
			resource.ReadWords(&value.shiftY, 4);
		}
		resource.ReadWords(&value.time, 4);
		resource.ReadWords(&value.version, 4);
		if (!resource.Good()) return false;
		if (value.version < 0 || value.version > 13) return resource.Fail("unsupported legacy MAP version");
		if (old || value.version <= 9) {
			value.width = (float) std::bit_cast<int32_t>(x);
			value.height = (float) std::bit_cast<int32_t>(y);
			if (!old) {
				value.shiftX = (float) std::bit_cast<int32_t>(value.shiftX);
				value.shiftY = (float) std::bit_cast<int32_t>(value.shiftY);
			}
		}
		else {
			value.width = std::bit_cast<float>(x);
			value.height = std::bit_cast<float>(y);
		}


		if (!std::isfinite(value.width) || !std::isfinite(value.height) ||
			!std::isfinite(value.shiftX) || !std::isfinite(value.shiftY) ||
			value.width < 1 || value.height < 1 || value.width > 65536 || value.height > 65536 ||
			std::fabs(value.shiftX) > 16777216 || std::fabs(value.shiftY) > 16777216) {
			return resource.Fail("MAP dimensions or camera exceed supported storage bounds");
		}
		const uint64_t columns = (int) (value.width + 7.0f) / 8;
		const uint64_t rows = (int) (value.height + 7.0f) / 8;
		if (!columns || !rows || columns * rows > 16777216) return resource.Fail("MAP ground grid is too large");
		*this = value;
		return true;
	}
};
