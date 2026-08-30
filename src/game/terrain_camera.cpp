#include "game/terrain_camera.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <new>
#include <utility>
#include <vector>

namespace
{

struct RUN {
	int m_begin;
	int m_end;
};

static TERRAIN_COVERAGE* s_activeCoverage;
static int s_activeOffsetX;
static int s_activeOffsetY;

static int GreatestCommonDivisor(int p_a, int p_b)
{
	p_a = std::abs(p_a);
	p_b = std::abs(p_b);
	while (p_b) {
		int next = p_a % p_b;
		p_a = p_b;
		p_b = next;
	}
	return p_a > 0 ? p_a : 1;
}

static void FocusRange(float p_focus, int p_view, int p_focusWindow, int p_limit, int* p_min, int* p_max)
{
	if (p_focusWindow <= 0 || p_focusWindow > p_view) {
		p_focusWindow = p_view;
	}
	const int nearMargin = (p_view - p_focusWindow) / 2;
	const int farMargin = nearMargin + p_focusWindow;
	// Camera and raster coordinates are integral. The focus must satisfy the
	// same half-open rule as the renderer: near <= focus-camera < far.
	int minimum = (int) std::floor((double) p_focus - farMargin) + 1;
	int maximum = (int) std::floor((double) p_focus - nearMargin);
	const int mapMaximum = p_limit - p_view;
	if (minimum < 0) {
		minimum = 0;
	}
	if (maximum > mapMaximum) {
		maximum = mapMaximum;
	}
	*p_min = minimum;
	*p_max = maximum;
}

} // namespace

struct TERRAIN_COVERAGE::IMPL {
	int m_width;
	int m_height;
	std::vector<uint64_t> m_bits;

	IMPL(int p_width, int p_height) : m_width(p_width), m_height(p_height)
	{
		if (p_width <= 0 || p_height <= 0 ||
			(size_t) p_width > std::numeric_limits<size_t>::max() / (size_t) p_height) {
			m_width = 0;
			m_height = 0;
			return;
		}
		const size_t pixels = (size_t) p_width * (size_t) p_height;
		try {
			m_bits.assign((pixels + 63) / 64, 0);
		}
		catch (...) {
			m_width = 0;
			m_height = 0;
			m_bits.clear();
		}
	}

	bool Marked(int p_x, int p_y) const
	{
		if (p_x < 0 || p_y < 0 || p_x >= m_width || p_y >= m_height) {
			return true;
		}
		const size_t index = (size_t) p_y * (size_t) m_width + (size_t) p_x;
		return (m_bits[index >> 6] & (uint64_t(1) << (index & 63))) != 0;
	}

	void Mark(int p_x, int p_y)
	{
		if (p_x < 0 || p_y < 0 || p_x >= m_width || p_y >= m_height) {
			return;
		}
		const size_t index = (size_t) p_y * (size_t) m_width + (size_t) p_x;
		m_bits[index >> 6] |= uint64_t(1) << (index & 63);
	}
};

TERRAIN_COVERAGE::TERRAIN_COVERAGE(int p_width, int p_height) : m_impl(new (std::nothrow) IMPL(p_width, p_height))
{
}

TERRAIN_COVERAGE::~TERRAIN_COVERAGE()
{
	delete m_impl;
}

bool TERRAIN_COVERAGE::Valid() const
{
	return m_impl && m_impl->m_width > 0 && m_impl->m_height > 0 && !m_impl->m_bits.empty();
}

void TERRAIN_COVERAGE::Mark(int p_x, int p_y)
{
	if (m_impl) {
		m_impl->Mark(p_x, p_y);
	}
}

bool TERRAIN_COVERAGE::Covered(int p_x, int p_y) const
{
	return m_impl && p_x >= 0 && p_y >= 0 && p_x < m_impl->m_width && p_y < m_impl->m_height &&
		   m_impl->Marked(p_x, p_y);
}

void TerrainCoverageBegin(TERRAIN_COVERAGE* p_coverage, int p_offsetX, int p_offsetY)
{
	s_activeCoverage = p_coverage && p_coverage->Valid() ? p_coverage : nullptr;
	s_activeOffsetX = p_offsetX;
	s_activeOffsetY = p_offsetY;
}

void TerrainCoverageMarkPixel(int p_x, int p_y)
{
	if (s_activeCoverage) {
		s_activeCoverage->Mark(p_x + s_activeOffsetX, p_y + s_activeOffsetY);
	}
}

void TerrainCoverageEnd()
{
	s_activeCoverage = nullptr;
	s_activeOffsetX = 0;
	s_activeOffsetY = 0;
}

struct TERRAIN_CAMERA::IMPL {
	static constexpr int BLOCK = 4;

	int m_width = 0;
	int m_height = 0;
	int m_viewWidth = 0;
	int m_viewHeight = 0;
	int m_blockWidth = 0;
	int m_blockHeight = 0;
	std::vector<std::vector<RUN>> m_exteriorRows;
	std::vector<unsigned int> m_blockIntegral;
	std::vector<size_t> m_safeOffsets;
	std::vector<RUN> m_safeRuns;

	explicit IMPL(TERRAIN_COVERAGE* p_coverage)
	{
		if (!p_coverage || !p_coverage->Valid()) {
			return;
		}
		TERRAIN_COVERAGE::IMPL* coverage = p_coverage->m_impl;
		m_width = coverage->m_width;
		m_height = coverage->m_height;
		m_exteriorRows.resize((size_t) m_height);

		std::vector<uint32_t> stack;
		try {
			stack.reserve((size_t) 2 * (m_width + m_height));
		}
		catch (...) {
			m_width = 0;
			m_height = 0;
			m_exteriorRows.clear();
			return;
		}

		auto flood = [&](int p_seedX, int p_seedY) {
			if (coverage->Marked(p_seedX, p_seedY)) {
				return;
			}
			stack.push_back((uint32_t) ((size_t) p_seedY * m_width + p_seedX));
			while (!stack.empty()) {
				uint32_t packed = stack.back();
				stack.pop_back();
				int y = (int) (packed / (uint32_t) m_width);
				int x = (int) (packed % (uint32_t) m_width);
				if (coverage->Marked(x, y)) {
					continue;
				}

				int left = x;
				int right = x + 1;
				while (left > 0 && !coverage->Marked(left - 1, y)) {
					--left;
				}
				while (right < m_width && !coverage->Marked(right, y)) {
					++right;
				}
				for (int fill = left; fill < right; ++fill) {
					coverage->Mark(fill, y);
				}
				m_exteriorRows[(size_t) y].push_back({left, right});

				for (int direction = -1; direction <= 1; direction += 2) {
					int nextY = y + direction;
					if (nextY < 0 || nextY >= m_height) {
						continue;
					}
					int scan = left;
					while (scan < right) {
						while (scan < right && coverage->Marked(scan, nextY)) {
							++scan;
						}
						if (scan >= right) {
							break;
						}
						stack.push_back((uint32_t) ((size_t) nextY * m_width + scan));
						while (scan < right && !coverage->Marked(scan, nextY)) {
							++scan;
						}
					}
				}
			}
		};

		for (int x = 0; x < m_width; ++x) {
			flood(x, 0);
			if (m_height > 1) {
				flood(x, m_height - 1);
			}
		}
		for (int y = 1; y + 1 < m_height; ++y) {
			flood(0, y);
			if (m_width > 1) {
				flood(m_width - 1, y);
			}
		}

		for (std::vector<RUN>& row : m_exteriorRows) {
			if (row.size() < 2) {
				continue;
			}
			std::sort(row.begin(), row.end(), [](const RUN& p_a, const RUN& p_b) { return p_a.m_begin < p_b.m_begin; });
			size_t write = 0;
			for (const RUN& run : row) {
				if (write && run.m_begin <= row[write - 1].m_end) {
					if (run.m_end > row[write - 1].m_end) {
						row[write - 1].m_end = run.m_end;
					}
				}
				else {
					row[write++] = run;
				}
			}
			row.resize(write);
		}

		m_blockWidth = (m_width + BLOCK - 1) / BLOCK;
		m_blockHeight = (m_height + BLOCK - 1) / BLOCK;
		std::vector<unsigned char> blocked((size_t) m_blockWidth * (size_t) m_blockHeight, 0);
		for (int y = 0; y < m_height; ++y) {
			int by = y / BLOCK;
			for (const RUN& run : m_exteriorRows[(size_t) y]) {
				int first = run.m_begin / BLOCK;
				int last = (run.m_end - 1) / BLOCK;
				for (int bx = first; bx <= last; ++bx) {
					blocked[(size_t) by * m_blockWidth + bx] = 1;
				}
			}
		}

		const int stride = m_blockWidth + 1;
		m_blockIntegral.assign((size_t) stride * (m_blockHeight + 1), 0);
		for (int by = 0; by < m_blockHeight; ++by) {
			unsigned int rowSum = 0;
			for (int bx = 0; bx < m_blockWidth; ++bx) {
				rowSum += blocked[(size_t) by * m_blockWidth + bx];
				m_blockIntegral[(size_t) (by + 1) * stride + bx + 1] =
					m_blockIntegral[(size_t) by * stride + bx + 1] + rowSum;
			}
		}
	}

	unsigned int BlockExterior(int p_x, int p_y, int p_w, int p_h) const
	{
		if (p_x < 0 || p_y < 0 || p_w <= 0 || p_h <= 0 || p_x + p_w > m_width || p_y + p_h > m_height) {
			return 1;
		}
		int x0 = p_x / BLOCK;
		int y0 = p_y / BLOCK;
		int x1 = (p_x + p_w - 1) / BLOCK + 1;
		int y1 = (p_y + p_h - 1) / BLOCK + 1;
		const int stride = m_blockWidth + 1;
		return m_blockIntegral[(size_t) y1 * stride + x1] - m_blockIntegral[(size_t) y0 * stride + x1] -
			   m_blockIntegral[(size_t) y1 * stride + x0] + m_blockIntegral[(size_t) y0 * stride + x0];
	}

	bool HasBlockPlacement(int p_viewWidth, int p_viewHeight, float p_focusX, float p_focusY) const
	{
		if (p_viewWidth <= 0 || p_viewHeight <= 0 || p_viewWidth > m_width || p_viewHeight > m_height) {
			return false;
		}
		int minX;
		int maxX;
		int minY;
		int maxY;
		FocusRange(p_focusX, p_viewWidth, std::min(640, p_viewWidth), m_width, &minX, &maxX);
		FocusRange(p_focusY, p_viewHeight, std::min(480, p_viewHeight), m_height, &minY, &maxY);
		if (minX > maxX || minY > maxY) {
			return false;
		}

		for (int y = minY; y <= maxY;) {
			for (int x = minX; x <= maxX;) {
				if (BlockExterior(x, y, p_viewWidth, p_viewHeight) == 0) {
					return true;
				}
				int nextX = ((x / BLOCK) + 1) * BLOCK;
				if (nextX <= x) {
					++nextX;
				}
				x = nextX < maxX ? nextX : (x < maxX ? maxX : maxX + 1);
			}
			int nextY = ((y / BLOCK) + 1) * BLOCK;
			if (nextY <= y) {
				++nextY;
			}
			y = nextY < maxY ? nextY : (y < maxY ? maxY : maxY + 1);
		}
		return false;
	}

	void AddExteriorRow(std::vector<int>& p_columns, int p_y, int p_delta) const
	{
		if (p_y < 0 || p_y >= m_height) {
			return;
		}
		for (const RUN& run : m_exteriorRows[(size_t) p_y]) {
			for (int x = run.m_begin; x < run.m_end; ++x) {
				p_columns[(size_t) x] += p_delta;
			}
		}
	}

	bool Configure(int p_viewWidth, int p_viewHeight)
	{
		m_viewWidth = 0;
		m_viewHeight = 0;
		m_safeOffsets.clear();
		m_safeRuns.clear();
		if (p_viewWidth <= 0 || p_viewHeight <= 0 || p_viewWidth > m_width || p_viewHeight > m_height) {
			return false;
		}

		const int maxX = m_width - p_viewWidth;
		const int maxY = m_height - p_viewHeight;
		std::vector<int> columns((size_t) m_width, 0);
		for (int y = 0; y < p_viewHeight; ++y) {
			AddExteriorRow(columns, y, 1);
		}

		m_safeOffsets.resize((size_t) maxY + 2);
		for (int y = 0; y <= maxY; ++y) {
			m_safeOffsets[(size_t) y] = m_safeRuns.size();
			int exterior = 0;
			for (int x = 0; x < p_viewWidth; ++x) {
				exterior += columns[(size_t) x];
			}
			int runBegin = -1;
			for (int x = 0; x <= maxX; ++x) {
				if (exterior == 0) {
					if (runBegin < 0) {
						runBegin = x;
					}
				}
				else if (runBegin >= 0) {
					m_safeRuns.push_back({runBegin, x});
					runBegin = -1;
				}
				if (x < maxX) {
					exterior += columns[(size_t) (x + p_viewWidth)] - columns[(size_t) x];
				}
			}
			if (runBegin >= 0) {
				m_safeRuns.push_back({runBegin, maxX + 1});
			}
			m_safeOffsets[(size_t) y + 1] = m_safeRuns.size();
			if (y < maxY) {
				AddExteriorRow(columns, y, -1);
				AddExteriorRow(columns, y + p_viewHeight, 1);
			}
		}
		m_viewWidth = p_viewWidth;
		m_viewHeight = p_viewHeight;
		return !m_safeRuns.empty();
	}

	bool ProjectRange(
		int p_desiredX,
		int p_desiredY,
		int p_minX,
		int p_maxX,
		int p_minY,
		int p_maxY,
		float* p_x,
		float* p_y
	) const
	{
		if (!p_x || !p_y || m_viewWidth <= 0 || m_viewHeight <= 0 || m_safeOffsets.empty()) {
			return false;
		}
		const int domainMaxX = m_width - m_viewWidth;
		const int domainMaxY = m_height - m_viewHeight;
		p_minX = std::max(0, p_minX);
		p_maxX = std::min(domainMaxX, p_maxX);
		p_minY = std::max(0, p_minY);
		p_maxY = std::min(domainMaxY, p_maxY);
		if (p_minX > p_maxX || p_minY > p_maxY) {
			return false;
		}

		uint64_t bestDistance = std::numeric_limits<uint64_t>::max();
		int bestX = 0;
		int bestY = 0;
		bool found = false;
		for (int y = p_minY; y <= p_maxY; ++y) {
			const size_t begin = m_safeOffsets[(size_t) y];
			const size_t end = m_safeOffsets[(size_t) y + 1];
			for (size_t i = begin; i < end; ++i) {
				int left = std::max(p_minX, m_safeRuns[i].m_begin);
				int right = std::min(p_maxX + 1, m_safeRuns[i].m_end);
				if (left >= right) {
					continue;
				}
				int x = p_desiredX < left ? left : p_desiredX >= right ? right - 1 : p_desiredX;
				int64_t dx = (int64_t) x - p_desiredX;
				int64_t dy = (int64_t) y - p_desiredY;
				uint64_t distance = (uint64_t) (dx * dx + dy * dy);
				if (!found || distance < bestDistance) {
					found = true;
					bestDistance = distance;
					bestX = x;
					bestY = y;
				}
			}
		}
		if (!found) {
			return false;
		}
		*p_x = (float) bestX;
		*p_y = (float) bestY;
		return true;
	}
};

TERRAIN_CAMERA::TERRAIN_CAMERA(TERRAIN_COVERAGE* p_coverage) : m_impl(new (std::nothrow) IMPL(p_coverage))
{
	delete p_coverage;
}

TERRAIN_CAMERA::~TERRAIN_CAMERA()
{
	delete m_impl;
}

bool TERRAIN_CAMERA::Valid() const
{
	return m_impl && m_impl->m_width > 0 && m_impl->m_height > 0 && !m_impl->m_blockIntegral.empty();
}

bool TERRAIN_CAMERA::SelectFrame(
	int p_maxWidth,
	int p_maxHeight,
	int p_minWidth,
	int p_minHeight,
	int p_aspectWidth,
	int p_aspectHeight,
	float p_focusX,
	float p_focusY,
	int* p_width,
	int* p_height
) const
{
	if (!Valid() || !p_width || !p_height || p_aspectWidth <= 0 || p_aspectHeight <= 0) {
		return false;
	}
	const int divisor = GreatestCommonDivisor(p_aspectWidth, p_aspectHeight);
	p_aspectWidth /= divisor;
	p_aspectHeight /= divisor;
	int high = std::min(
		{p_maxWidth / p_aspectWidth,
		 p_maxHeight / p_aspectHeight,
		 m_impl->m_width / p_aspectWidth,
		 m_impl->m_height / p_aspectHeight}
	);
	int low =
		std::max((p_minWidth + p_aspectWidth - 1) / p_aspectWidth, (p_minHeight + p_aspectHeight - 1) / p_aspectHeight);
	if (high < low) {
		return false;
	}

	int best = -1;
	int left = low;
	int right = high;
	while (left <= right) {
		int scale = left + (right - left) / 2;
		int width = p_aspectWidth * scale;
		int height = p_aspectHeight * scale;
		if (m_impl->HasBlockPlacement(width, height, p_focusX, p_focusY)) {
			best = scale;
			left = scale + 1;
		}
		else {
			right = scale - 1;
		}
	}
	if (best < 0) {
		return false;
	}
	*p_width = p_aspectWidth * best;
	*p_height = p_aspectHeight * best;
	return true;
}

bool TERRAIN_CAMERA::Configure(int p_viewWidth, int p_viewHeight)
{
	return Valid() && m_impl->Configure(p_viewWidth, p_viewHeight);
}

bool TERRAIN_CAMERA::Project(
	float p_desiredX,
	float p_desiredY,
	float p_focusX,
	float p_focusY,
	int p_focusWindowWidth,
	int p_focusWindowHeight,
	float* p_x,
	float* p_y
) const
{
	if (!m_impl || !p_x || !p_y || m_impl->m_viewWidth <= 0 || m_impl->m_viewHeight <= 0 ||
		m_impl->m_safeOffsets.empty()) {
		return false;
	}
	int minX;
	int maxX;
	int minY;
	int maxY;
	FocusRange(p_focusX, m_impl->m_viewWidth, p_focusWindowWidth, m_impl->m_width, &minX, &maxX);
	FocusRange(p_focusY, m_impl->m_viewHeight, p_focusWindowHeight, m_impl->m_height, &minY, &maxY);
	if (minX > maxX || minY > maxY) {
		return false;
	}

	const int desiredX = (int) std::floor((double) p_desiredX + 0.5);
	const int desiredY = (int) std::floor((double) p_desiredY + 0.5);
	return m_impl->ProjectRange(desiredX, desiredY, minX, maxX, minY, maxY, p_x, p_y);
}

bool TERRAIN_CAMERA::ProjectSafe(float p_desiredX, float p_desiredY, float* p_x, float* p_y) const
{
	if (!m_impl) {
		return false;
	}
	const int desiredX = (int) std::floor((double) p_desiredX + 0.5);
	const int desiredY = (int) std::floor((double) p_desiredY + 0.5);
	return m_impl->ProjectRange(
		desiredX,
		desiredY,
		0,
		m_impl->m_width - m_impl->m_viewWidth,
		0,
		m_impl->m_height - m_impl->m_viewHeight,
		p_x,
		p_y
	);
}

bool TERRAIN_CAMERA::ViewSafe(int p_x, int p_y) const
{
	if (!m_impl || p_y < 0 || m_impl->m_safeOffsets.empty() || p_y + 1 >= (int) m_impl->m_safeOffsets.size()) {
		return false;
	}
	const size_t begin = m_impl->m_safeOffsets[(size_t) p_y];
	const size_t end = m_impl->m_safeOffsets[(size_t) p_y + 1];
	for (size_t i = begin; i < end; ++i) {
		if (p_x >= m_impl->m_safeRuns[i].m_begin && p_x < m_impl->m_safeRuns[i].m_end) {
			return true;
		}
	}
	return false;
}

int TERRAIN_CAMERA::ViewWidth() const
{
	return m_impl ? m_impl->m_viewWidth : 0;
}

int TERRAIN_CAMERA::ViewHeight() const
{
	return m_impl ? m_impl->m_viewHeight : 0;
}
