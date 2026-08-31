#include "game/map.h"
#include "game/player_arcade.h"
#include "game/terrain_camera.h"
#include "gfx/display_math.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "platform/render.h"
#include "sprite/sprite.h"
#include "ui/mouse.h"
#include "util/myerror.h"
#include "video/vid_hardware.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <new>

namespace
{

void RefreshPointerForTerrainFrame(MAP* p_map)
{
	float x = 0.0f;
	float y = 0.0f;
	SDL_GetMouseState(&x, &y);
	Platform_RenderWindowToFrame(&x, &y);
	SDL_Event event = {};
	event.type = SDL_EVENT_MOUSE_MOTION;
	event.motion.x = x;
	event.motion.y = y;
	p_map->m_input.ProcessEvent(event);
}

} // namespace

void MAP::ClearTerrainCamera()
{
	delete m_terrainCamera;
	m_terrainCamera = 0;
}

void MAP::FinalizeTerrainCamera(int p_gameplay)
{
	ClearTerrainCamera();
	if (m_noVid <= 1024 || !m_vids[1024]) {
		return;
	}

	VID_HARDWARE* ground = dynamic_cast<VID_HARDWARE*>(m_vids[1024]);
	if (!ground) {
		return;
	}
	TERRAIN_COVERAGE* coverage = ground->TakeTerrainCoverage();
	GRAPH_CORE* graph = (GRAPH_CORE*) Graph;
	if (!coverage || !p_gameplay || !graph || !graph->m_nativeResolution) {
		delete coverage;
		return;
	}

	TERRAIN_CAMERA* camera = new (std::nothrow) TERRAIN_CAMERA(coverage);
	if (!camera || !camera->Valid()) {
		delete camera;
		return;
	}

	float focusX = m_shiftX + graph->m_width * 0.5f;
	float focusY = m_shiftY + graph->m_height * 0.5f;
	MAN* flagman = Flagman((int) m_curArmy);
	if (flagman) {
		focusX = flagman->m_x;
		focusY = flagman->m_y - flagman->m_z;
	}

	int targetWidth = (int) graph->m_width;
	int targetHeight = (int) graph->m_height;
	DISPLAY_MATH::RESOLUTION retail = DISPLAY_MATH::ResolveInternal(targetWidth, targetHeight, 0, false);
	if (!camera->SelectFrame(
			targetWidth,
			targetHeight,
			retail.m_width,
			retail.m_height,
			targetWidth,
			targetHeight,
			focusX,
			focusY,
			&targetWidth,
			&targetHeight
		)) {
		targetWidth = retail.m_width;
		targetHeight = retail.m_height;
	}

	const float oldShiftX = m_shiftX;
	const float oldShiftY = m_shiftY;
	int resized = graph->ConfigureFrameForTerrain(targetWidth, targetHeight);
	if (resized < 0) {
		targetWidth = (int) graph->m_width;
		targetHeight = (int) graph->m_height;
	}
	if (!camera->Configure(targetWidth, targetHeight)) {
		delete camera;
		return;
	}
	m_terrainCamera = camera;

	for (int player = 0; player < 4; ++player) {
		if (m_player[player]) {
			((PLAYER_ARCADE*) m_player[player])->RefreshUILayout();
		}
	}
	if (resized > 0) {
		RefreshPointerForTerrainFrame(this);
	}

	SetShiftCoor(oldShiftX + graph->m_width * 0.5f, oldShiftY + graph->m_height * 0.5f, 0);
	MYERROR::Log(::Error, "Terrain-safe frame %ix%i for map %.0fx%.0f", targetWidth, targetHeight, m_w, m_h);
}

bool MAP::CurrentTerrainViewSafe() const
{
	return !m_terrainCamera || m_terrainCamera->ViewSafe((int) m_shiftX, (int) m_shiftY);
}
