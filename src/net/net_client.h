#ifndef NET_CLIENT_H
#define NET_CLIENT_H

class SPRITE;
class STRING;

struct NET_REMOTE_INFO {
	const char* m_name;
	float m_x;
	float m_y;
	float m_z;
	int m_hp;
	int m_maxHp;
};

struct NET_ROSTER_INFO {
	const char* m_name;
	const char* m_map;
	int m_local;
	int m_sameMap;
	int m_away;
};

enum NET_UI_STATE {
	NET_UI_IDLE,
	NET_UI_BUSY,
	NET_UI_ONLINE
};

#if defined(OPENGROMADA_HAVE_MULTIPLAYER)

inline int Net_Compiled()
{
	return 1;
}

void Net_Init();
void Net_Shutdown();
void Net_Pump();

int Net_Available();
int Net_Connect(const char* p_address, const char* p_name);
int Net_Host(int p_port, const char* p_name);
void Net_Disconnect();

int Net_UiState();
int Net_Hosting();
int Net_Playing();
const char* Net_Status();
const char* Net_ActiveAddress();
const char* Net_ActiveName();

void Net_SetAutoConnect(const char* p_address, const char* p_name);
void Net_SetAutoHost(int p_port, const char* p_name);

int Net_RemoteInfo(int p_index, NET_REMOTE_INFO* p_out);
int Net_RosterInfo(int p_index, NET_ROSTER_INFO* p_out);

void Net_OnLocalAttack(SPRITE* p_sprite, float p_x, float p_y);
void Net_OnAmmoSpent(SPRITE* p_sprite, int p_amount);
void Net_OnSpriteDeleted(SPRITE* p_sprite);
void Net_OnMapRelease();
void Net_OnMapLoaded();
void Net_OnMapChangeRequest(STRING& p_target);
void Net_OnBeforeSave();

extern int g_netProxyCount;
int Net_IsProxySlow(const SPRITE* p_sprite);

inline int Net_IsProxy(const SPRITE* p_sprite)
{
	return g_netProxyCount && Net_IsProxySlow(p_sprite);
}

#else

inline int Net_Compiled()
{
	return 0;
}

inline void Net_Init()
{
}

inline void Net_Shutdown()
{
}

inline void Net_Pump()
{
}

inline void Net_Disconnect()
{
}

inline void Net_SetAutoConnect(const char*, const char*)
{
}

inline void Net_SetAutoHost(int, const char*)
{
}

inline void Net_OnLocalAttack(SPRITE*, float, float)
{
}

inline void Net_OnAmmoSpent(SPRITE*, int)
{
}

inline void Net_OnSpriteDeleted(SPRITE*)
{
}

inline void Net_OnMapRelease()
{
}

inline void Net_OnMapLoaded()
{
}

inline void Net_OnMapChangeRequest(STRING&)
{
}

inline void Net_OnBeforeSave()
{
}

inline int Net_IsProxy(const SPRITE*)
{
	return 0;
}

#endif

#endif
