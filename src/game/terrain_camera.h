#ifndef TERRAIN_CAMERA_H
#define TERRAIN_CAMERA_H

// Tracks authored terrain coverage independently of pixel color.
class TERRAIN_COVERAGE {
public:
	TERRAIN_COVERAGE(int p_width, int p_height);
	~TERRAIN_COVERAGE();

	TERRAIN_COVERAGE(const TERRAIN_COVERAGE&) = delete;
	TERRAIN_COVERAGE& operator=(const TERRAIN_COVERAGE&) = delete;

	bool Valid() const;
	void Mark(int p_x, int p_y);
	bool Covered(int p_x, int p_y) const;

private:
	struct IMPL;
	IMPL* m_impl;

	friend class TERRAIN_CAMERA;
};

// Carries coverage across the fixed DrawToVid seam in single-threaded composition.
void TerrainCoverageBegin(TERRAIN_COVERAGE* p_coverage, int p_offsetX, int p_offsetY);
void TerrainCoverageMarkPixel(int p_x, int p_y);
void TerrainCoverageEnd();

class TERRAIN_CAMERA {
public:
	// Owns coverage; only perimeter-connected uncovered pixels are exterior.
	explicit TERRAIN_CAMERA(TERRAIN_COVERAGE* p_coverage);
	~TERRAIN_CAMERA();

	TERRAIN_CAMERA(const TERRAIN_CAMERA&) = delete;
	TERRAIN_CAMERA& operator=(const TERRAIN_CAMERA&) = delete;

	bool Valid() const;

	// Selects the largest void-free exact-aspect frame, never below p_min.
	bool SelectFrame(
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
	) const;

	// Builds safe integer camera origins for the selected frame.
	bool Configure(int p_viewWidth, int p_viewHeight);

	// Projects to the nearest void-free origin satisfying the focus window.
	bool Project(
		float p_desiredX,
		float p_desiredY,
		float p_focusX,
		float p_focusY,
		int p_focusWindowWidth,
		int p_focusWindowHeight,
		float* p_x,
		float* p_y
	) const;

	// Final fallback preserves authored terrain without a focus constraint.
	bool ProjectSafe(float p_desiredX, float p_desiredY, float* p_x, float* p_y) const;

	bool ViewSafe(int p_x, int p_y) const;
	int ViewWidth() const;
	int ViewHeight() const;

private:
	struct IMPL;
	IMPL* m_impl;
};

#endif
