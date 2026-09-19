#ifndef NET_UI_H
#define NET_UI_H

union SDL_Event;

#if defined(OPENGROMADA_HAVE_MULTIPLAYER)

int NetUi_ProcessEvent(const SDL_Event& p_event);
void NetUi_Pump();
void NetUi_Draw();
void NetUi_OnMenuLoaded(const char* p_name);

#else

inline int NetUi_ProcessEvent(const SDL_Event&)
{
	return 0;
}

inline void NetUi_Draw()
{
}

inline void NetUi_OnMenuLoaded(const char*)
{
}

#endif

#endif
