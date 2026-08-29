#include "gfx/movie.h"

#include <dxsdk/dshow.h>

#include "game/map.h"
#include "gfx/graph.h"
#include "gfx/graph_core.h"
#include "util/myerror.h"
#include "util/string.h"

extern const GUID DSHOW_CLSID_FilterGraph;
extern const GUID DSHOW_IID_IGraphBuilder;
extern const GUID DSHOW_IID_IMediaControl;
// GLOBAL: ALIEN 0x47cee0
static const GUID MOVIE_IID_IMediaEvent =
	{ 0x56a868b6, 0x0ad4, 0x11ce, { 0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 } };
// GLOBAL: ALIEN 0x47cec0
static const GUID MOVIE_IID_IVideoWindow =
	{ 0x56a868b4, 0x0ad4, 0x11ce, { 0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 } };

// FUNCTION: ALIEN 0x434480
int MOVIE::Play(const char* p_filename, int p_x, int p_y)
{
	Stop();
	((GRAPH_CORE*) Graph)->FlipToGDI();
	int hr = CoCreateInstance(DSHOW_CLSID_FilterGraph, 0, CLSCTX_INPROC_SERVER,
		DSHOW_IID_IGraphBuilder, (void**) &m_graph);
	if (hr < 0) {
		if (::Error)
			MYERROR::Error(::Error,
				// STRING: ALIEN 0x483fa0
				"MOVIE", 3,
				"GraphBuilder", hr);
	}
	else {
		hr = m_graph->QueryInterface(DSHOW_IID_IMediaControl, (void**) &m_mediaControl);
		if (hr < 0) {
			if (::Error)
				MYERROR::Error(::Error, "MOVIE", 3,
					"MediaControl", hr);
		}
		else {
			m_graph->QueryInterface(MOVIE_IID_IMediaEvent, (void**) &m_mediaEvent);

			unsigned short wide[1024];
			((STRING*) p_filename)->ToUnicode(wide, 1024);
			hr = m_graph->RenderFile((LPCWSTR) wide, 0);
			if (hr < 0) {
				if (::Error)
					MYERROR::Error(::Error, "MOVIE", 4,
						"RenderFile", hr);
			}
			else {
				m_graph->QueryInterface(MOVIE_IID_IVideoWindow, (void**) &m_videoWindow);
				m_videoWindow->put_Owner((OAHWND) Map->m_hWnd);
				m_videoWindow->put_WindowStyle(0x44000000); // WS_CHILD | WS_CLIPSIBLINGS
				GRAPH_CORE* core = (GRAPH_CORE*) Graph;
				int x = (int) core->m_viewYMin;
				int y = (int) core->m_viewXMin;
				m_videoWindow->SetWindowPosition(y, x, (int) core->m_viewXMax - y + 1,
					(int) core->m_viewYMax - x + 1);
				m_mediaControl->Run();
				SetCapture((HWND) Map->m_hWnd);
				return SetCursorPos((int) ((GRAPH_CORE*) Graph)->m_width,
					(int) ((GRAPH_CORE*) Graph)->m_height);
			}
		}
		Stop();
	}
}

// FUNCTION: ALIEN 0x434650
int MOVIE::IsComplete()
{
	int code;
	if (m_mediaEvent) {
		m_mediaEvent->WaitForCompletion(0, (long*) &code);
		if (code != 1)
			return 0;
	}
	return 1;
}

// FUNCTION: ALIEN 0x434680
void MOVIE::Stop()
{
	ReleaseCapture();
	if (m_videoWindow)
		m_videoWindow->Release();
	m_videoWindow = 0;
	if (m_mediaControl)
		m_mediaControl->Release();
	m_mediaControl = 0;
	if (m_mediaEvent)
		m_mediaEvent->Release();
	m_mediaEvent = 0;
	if (m_graph)
		m_graph->Release();
	m_graph = 0;
}

// FUNCTION: ALIEN 0x4346e0
int MOVIE::Pause()
{
	int result = (int) m_mediaControl;
	if (result)
		result = ((IMediaControl*) result)->Pause();
	return result;
}

// FUNCTION: ALIEN 0x4346f0
int MOVIE::Resume()
{
	int result = (int) m_mediaControl;
	if (result)
		result = ((IMediaControl*) result)->Run();
	return result;
}
