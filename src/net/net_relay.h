#ifndef NET_RELAY_H
#define NET_RELAY_H

#include "net/net_protocol.h"

#include <stddef.h>

struct NET_RELAY;

typedef void (*NET_RELAY_LOG_FN)(void* p_user, const char* p_line);

struct NET_RELAY_CONFIG {
	int m_port;
	int m_maxPlayers;
	int m_rateMs;
	int m_helloTimeoutMs;
	int m_followPlayers;
	int m_playerHosted;
	unsigned int m_tickBias;
	char m_map[NET_MAP_LEN];
	NET_RELAY_LOG_FN m_log;
	void* m_logUser;
};

struct NET_RELAY_STATS {
	int m_port;
	int m_connected;
	int m_ready;
	unsigned int m_dropped;
	unsigned int m_worstStallMs;
};

void NetRelay_DefaultConfig(NET_RELAY_CONFIG* p_config);
const char* NetRelay_CheckConfig(const NET_RELAY_CONFIG* p_config);
NET_RELAY* NetRelay_Create(const NET_RELAY_CONFIG* p_config, char* p_error, size_t p_errorSize);
int NetRelay_Service(NET_RELAY* p_relay, unsigned int p_maxWaitMs);
void NetRelay_SetStartMap(NET_RELAY* p_relay, const char* p_map);
void NetRelay_SetLog(NET_RELAY* p_relay, NET_RELAY_LOG_FN p_log, void* p_logUser);
void NetRelay_GetStats(const NET_RELAY* p_relay, NET_RELAY_STATS* p_out);
void NetRelay_Destroy(NET_RELAY* p_relay);

#endif
