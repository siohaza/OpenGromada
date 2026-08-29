#define DECOMP_INLINE_STRING_COPY_LIFETIME
#include "audio/music_dshow.h"

#include <dxsdk/dshow.h>
#include <stdio.h>

#include "util/myerror.h"

// GLOBAL: ALIEN 0x47cea0
extern const GUID DSHOW_IID_IGraphBuilder =
	{ 0x56a868a9, 0x0ad4, 0x11ce, { 0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 } };
// GLOBAL: ALIEN 0x47ceb0
static const GUID DSHOW_IID_IMediaSeeking =
	{ 0x36b73880, 0xc2c8, 0x11cf, { 0x8b, 0x46, 0x00, 0x80, 0x5f, 0x6c, 0xef, 0x60 } };
// GLOBAL: ALIEN 0x47cef0
extern const GUID DSHOW_IID_IMediaControl =
	{ 0x56a868b1, 0x0ad4, 0x11ce, { 0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 } };
// GLOBAL: ALIEN 0x47cf00
extern const GUID DSHOW_CLSID_FilterGraph =
	{ 0xe436ebb3, 0x524f, 0x11ce, { 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 } };

// FUNCTION: ALIEN 0x41c7c0
MUSIC_DSHOW::MUSIC_DSHOW(const STRING& p_name)
	: MUSIC(p_name, STRING::CALL_COPY_NONNULL)
{
	m_graphBuilder = 0;
	m_mediaControl = 0;
	m_mediaSeeking = 0;
	int result = CoCreateInstance(DSHOW_CLSID_FilterGraph, 0, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
		DSHOW_IID_IGraphBuilder, (void**) &m_graphBuilder);
	if (result < 0) {
		MYERROR::Error(::Error,
			// STRING: ALIEN 0x482d30
			"MUSIC '%s'", 3,
			// STRING: ALIEN 0x482d94
			"GraphBuilder", result, m_name.m_str);
	}
	else if (!m_mediaControl
		&& m_graphBuilder->QueryInterface(DSHOW_IID_IMediaControl, (void**) &m_mediaControl) < 0) {
		MYERROR::Error(::Error, "MUSIC '%s'", 3,
			// STRING: ALIEN 0x482d84
			"MediaControl", result, m_name.m_str);
	}
	else if (!m_mediaSeeking
		&& m_graphBuilder->QueryInterface(DSHOW_IID_IMediaSeeking, (void**) &m_mediaSeeking) < 0) {
		MYERROR::Error(::Error, "MUSIC '%s'", 3,
			// STRING: ALIEN 0x482d74
			"MediaSeeking", 0, m_name.m_str);
	}
	else {
		int exists;
		if (!*p_name.m_str) {
			exists = 0;
		}
		else {
			FILE* file = fopen(p_name.m_str,
				// STRING: ALIEN 0x481810
				"rb");
			FILE* opened = file;
			if (file)
				fclose(file);
			exists = opened != 0;
		}
		if (!exists) {
			MYERROR::Error(::Error, "MUSIC '%s'", 7, p_name.m_str, 0, m_name.m_str);
		}
		else {
			unsigned short wide[1024];
			((STRING&) p_name).ToUnicode(wide, 1024);
			int render = m_graphBuilder->RenderFile((unsigned short*) wide, 0);
			if (render < 0)
				MYERROR::Error(::Error, "MUSIC '%s'", 4,
					// STRING: ALIEN 0x482d68
					"RenderFile", render, m_name.m_str);
		}
	}
}

// FUNCTION: ALIEN 0x41c980
MUSIC_DSHOW::~MUSIC_DSHOW()
{
	Stop();
	if (m_mediaSeeking) {
		m_mediaSeeking->Release();
		m_mediaSeeking = 0;
	}
	if (m_mediaControl) {
		m_mediaControl->Release();
		m_mediaControl = 0;
	}
	if (m_graphBuilder) {
		m_graphBuilder->Release();
		m_graphBuilder = 0;
	}
}

// FUNCTION: ALIEN 0x41c9f0
void MUSIC_DSHOW::Play()
{
	if (m_mediaSeeking) {
		__int64 pos = 0;
		if (m_mediaSeeking->SetPositions(&pos, 1, 0, 0) < 0) {
			if (m_mediaControl)
				m_mediaControl->Stop();
		}
	}
	if (m_mediaControl)
		m_mediaControl->Run();
}

// FUNCTION: ALIEN 0x41ca50
void MUSIC_DSHOW::Stop()
{
	int result = (int) m_mediaControl;
	if (result)
		result = ((IMediaControl*) result)->Stop();
}

// FUNCTION: ALIEN 0x41ca60
void MUSIC_DSHOW::Pause()
{
	int result = (int) m_mediaControl;
	if (result)
		result = ((IMediaControl*) result)->Pause();
}

// FUNCTION: ALIEN 0x41ca70
void MUSIC_DSHOW::Resume()
{
	int result = (int) m_mediaControl;
	if (result)
		result = ((IMediaControl*) result)->Run();
}

// FUNCTION: ALIEN 0x41ca80
int MUSIC_DSHOW::IsPlaying()
{
	if (m_mediaControl) {
		int state;
		m_mediaControl->GetState(3000, (OAFilterState*) &state);
		if (state == 2) {
			__int64 cur;
			__int64 stop;
			m_mediaSeeking->GetPositions(&cur, &stop);
			if (stop - cur)
				return 1;
		}
	}
	return 0;
}

// GLOBAL: ALIEN 0x47ced0
static const GUID DSHOW_IID_IBasicAudio =
	{ 0x56a868b3, 0x0ad4, 0x11ce, { 0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 } };

// FUNCTION: ALIEN 0x41cae0
void MUSIC_DSHOW::SetVolume(int p_volume)
{
	IBasicAudio* audio = 0;
	if (!m_graphBuilder)
		return;
	int hr = m_graphBuilder->QueryInterface(DSHOW_IID_IBasicAudio, (void**) &audio);
	if (hr < 0) {
		MYERROR::Error(::Error,
			"MUSIC '%s'", 9,
			// STRING: ALIEN 0x482dac
			"BasicAudio", hr, m_name.m_str);
		return;
	}
	long current;
	int gv = audio->get_Volume(&current);
	if (gv != (int) 0x80004001) {
		if (gv >= 0) {
			int pv = audio->put_Volume(p_volume);
			if (pv < 0)
				MYERROR::Error(::Error, "MUSIC '%s'", 8, "Volume", pv, m_name.m_str);
		}
		else
			MYERROR::Error(::Error, "MUSIC '%s'", 9,
				// STRING: ALIEN 0x482da4
				"Volume", gv, m_name.m_str);
	}
	audio->Release();
}
