#ifndef MOVIE_H
#define MOVIE_H

struct IGraphBuilder;
struct IMediaControl;
struct IMediaEvent;
struct IVideoWindow;

class MOVIE {
public:
	IGraphBuilder* m_graph; // 0x00
	IMediaControl* m_mediaControl; // 0x04
	IMediaEvent* m_mediaEvent; // 0x08
	IVideoWindow* m_videoWindow; // 0x0c

	MOVIE()
	{
		m_graph = 0;
		m_mediaControl = 0;
		m_mediaEvent = 0;
		m_videoWindow = 0;
	}

	int Play(const char* p_filename, int p_x, int p_y);
	int IsComplete();
	void Stop();
	int Pause();
	int Resume();
};

#endif
