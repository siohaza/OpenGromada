#include "net/net_relay.h"

#include "3rdparty/enet.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__GNUC__)
#define NET_RELAY_PRINTF(p_format, p_args) __attribute__((format(printf, p_format, p_args)))
#else
#define NET_RELAY_PRINTF(p_format, p_args)
#endif

enum {
	NET_RELAY_REJECT_MARGIN = 4,
	NET_RELAY_MAX_EVENTS_PER_SERVICE = 512,
	NET_RELAY_STRIKE_LIMIT = 200,
	NET_RELAY_SHOOT_BURST = 8,
	NET_RELAY_SHOOT_REFILL_MS = 10,
	NET_RELAY_STALL_LOG_MS = 250,
	NET_RELAY_LOG_LINE_BYTES = 256,
	NET_RELAY_LOG_BURST = 64,
	NET_RELAY_LOG_REFILL_MS = 50,
	NET_RELAY_PORT_MIN = 1,
	NET_RELAY_PORT_MAX = 65535,
	NET_RELAY_PLAYERS_MIN = 2,
	NET_RELAY_RATE_MIN_MS = 10,
	NET_RELAY_RATE_MAX_MS = 1000,
	NET_RELAY_HELLO_TIMEOUT_MIN_MS = 100,
	NET_RELAY_HELLO_TIMEOUT_MAX_MS = 60000,
	NET_RELAY_HELLO_TIMEOUT_DEFAULT_MS = 5000
};

struct NET_RELAY_BUCKET {
	enet_uint32 m_tokens;
	enet_uint32 m_refillMs;
};

struct NET_RELAY_SLOT {
	ENetPeer* m_peer;
	int m_ready;
	int m_hasActor;
	int m_fresh;
	int m_presenceDirty;
	int m_strikes;
	enet_uint32 m_connectMs;
	NET_RELAY_BUCKET m_shootBudget;
	uint8_t m_mapSeq;
	uint8_t m_mapFlags;
	char m_name[NET_NAME_LEN];
	char m_map[NET_MAP_LEN];
	NetActor m_actor;
};

struct NET_RELAY {
	ENetHost* m_host;
	NET_RELAY_CONFIG m_config;
	NET_RELAY_SLOT m_slots[NET_MAX_PLAYERS];
	int m_readyCount;
	char m_startMap[NET_MAP_LEN];
	enet_uint32 m_lastTickMs;
	enet_uint32 m_lastServiceMs;
	NET_RELAY_BUCKET m_logBudget;
	unsigned int m_suppressedLines;
	unsigned int m_dropped;
	unsigned int m_worstStallMs;
};

static enet_uint32 RelayNow(const NET_RELAY* p_relay)
{
	return enet_time_get() + (enet_uint32) p_relay->m_config.m_tickBias;
}

static void BucketFill(NET_RELAY_BUCKET* p_bucket, enet_uint32 p_capacity, enet_uint32 p_now)
{
	p_bucket->m_tokens = p_capacity;
	p_bucket->m_refillMs = p_now;
}

static int BucketTake(NET_RELAY_BUCKET* p_bucket, enet_uint32 p_capacity, enet_uint32 p_periodMs, enet_uint32 p_now)
{
	enet_uint32 gained = NetElapsedMs(p_now, p_bucket->m_refillMs) / p_periodMs;
	if (gained >= p_capacity - p_bucket->m_tokens) {
		p_bucket->m_tokens = p_capacity;
		p_bucket->m_refillMs = p_now;
	}
	else {
		p_bucket->m_tokens += gained;
		p_bucket->m_refillMs += gained * p_periodMs;
	}
	if (!p_bucket->m_tokens) {
		return 0;
	}
	--p_bucket->m_tokens;
	return 1;
}

static void LogLine(NET_RELAY* p_relay, const char* p_format, va_list p_args) NET_RELAY_PRINTF(2, 0);
static void Log(NET_RELAY* p_relay, const char* p_format, ...) NET_RELAY_PRINTF(2, 3);
static void LogPeerEvent(NET_RELAY* p_relay, const char* p_format, ...) NET_RELAY_PRINTF(2, 3);
static void SetError(char* p_error, size_t p_errorSize, const char* p_format, ...) NET_RELAY_PRINTF(3, 4);

static void LogLine(NET_RELAY* p_relay, const char* p_format, va_list p_args)
{
	char line[NET_RELAY_LOG_LINE_BYTES];
	vsnprintf(line, sizeof(line), p_format, p_args);
	p_relay->m_config.m_log(p_relay->m_config.m_logUser, line);
}

static void Log(NET_RELAY* p_relay, const char* p_format, ...)
{
	if (!p_relay->m_config.m_log) {
		return;
	}
	va_list args;
	va_start(args, p_format);
	LogLine(p_relay, p_format, args);
	va_end(args);
}

static void LogPeerEvent(NET_RELAY* p_relay, const char* p_format, ...)
{
	if (!p_relay->m_config.m_log) {
		return;
	}
	if (!BucketTake(&p_relay->m_logBudget, NET_RELAY_LOG_BURST, NET_RELAY_LOG_REFILL_MS, RelayNow(p_relay))) {
		++p_relay->m_suppressedLines;
		return;
	}
	if (p_relay->m_suppressedLines) {
		Log(p_relay, "suppressed %u log lines", p_relay->m_suppressedLines);
		p_relay->m_suppressedLines = 0;
	}
	va_list args;
	va_start(args, p_format);
	LogLine(p_relay, p_format, args);
	va_end(args);
}

static void SetError(char* p_error, size_t p_errorSize, const char* p_format, ...)
{
	if (!p_error || !p_errorSize) {
		return;
	}
	va_list args;
	va_start(args, p_format);
	vsnprintf(p_error, p_errorSize, p_format, args);
	va_end(args);
}

static const char* DenyReason(int p_code)
{
	switch (p_code) {
	case NET_DENY_VERSION:
		return "protocol version mismatch";
	case NET_DENY_FULL:
		return "server is full";
	case NET_DENY_HELLO_TIMEOUT:
		return "no hello received";
	case NET_DENY_PROTOCOL:
		return "too many invalid packets";
	case NET_DENY_SHUTDOWN:
		return "server shutting down";
	default:
		return "unknown reason";
	}
}

static void SendTo(ENetPeer* p_peer, const void* p_data, size_t p_size, int p_channel, int p_reliable)
{
	ENetPacket* packet = enet_packet_create(p_data, p_size, p_reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
	if (!packet) {
		return;
	}
	if (enet_peer_send(p_peer, (enet_uint8) p_channel, packet) < 0 && packet->referenceCount == 0) {
		enet_packet_destroy(packet);
	}
}

static int SlotIsReady(const NET_RELAY_SLOT* p_slot)
{
	return p_slot->m_peer && p_slot->m_ready;
}

static int SlotIsOnMap(const NET_RELAY_SLOT* p_slot)
{
	return SlotIsReady(p_slot) && (p_slot->m_mapFlags & NET_MAPF_PLAYABLE) && p_slot->m_map[0];
}

static int SlotsShareMap(const NET_RELAY_SLOT* p_a, const NET_RELAY_SLOT* p_b)
{
	return SlotIsOnMap(p_a) && SlotIsOnMap(p_b) && NetMapNameEqual(p_a->m_map, p_b->m_map);
}

static int SlotOf(const NET_RELAY* p_relay, const ENetPeer* p_peer)
{
	int idx = (int) (intptr_t) p_peer->data - 1;
	if (idx < 0 || idx >= p_relay->m_config.m_maxPlayers) {
		return -1;
	}
	if (p_relay->m_slots[idx].m_peer != p_peer) {
		return -1;
	}
	return idx;
}

static int FindFreeSlot(const NET_RELAY* p_relay)
{
	for (int i = 0; i < p_relay->m_config.m_maxPlayers; ++i) {
		if (!p_relay->m_slots[i].m_peer) {
			return i;
		}
	}
	return -1;
}

static int ConnectedCount(const NET_RELAY* p_relay)
{
	int count = 0;
	for (int i = 0; i < p_relay->m_config.m_maxPlayers; ++i) {
		if (p_relay->m_slots[i].m_peer) {
			++count;
		}
	}
	return count;
}

static void BroadcastReady(NET_RELAY* p_relay, const void* p_data, size_t p_size, int p_skip)
{
	for (int i = 0; i < p_relay->m_config.m_maxPlayers; ++i) {
		if (i != p_skip && SlotIsReady(&p_relay->m_slots[i])) {
			SendTo(p_relay->m_slots[i].m_peer, p_data, p_size, NET_CHANNEL_EVENTS, 1);
		}
	}
}

static void ReleaseSlot(NET_RELAY* p_relay, int p_idx, const char* p_rejectReason)
{
	NET_RELAY_SLOT* slot = &p_relay->m_slots[p_idx];
	ENetPeer* peer = slot->m_peer;
	int wasReady = slot->m_ready;
	char name[NET_NAME_LEN];
	memcpy(name, slot->m_name, NET_NAME_LEN);
	memset(slot, 0, sizeof(*slot));
	if (peer) {
		peer->data = 0;
	}
	if (p_rejectReason) {
		LogPeerEvent(p_relay, "slot %d: rejected: %s", p_idx, p_rejectReason);
	}
	if (!wasReady) {
		return;
	}
	--p_relay->m_readyCount;
	NetMsgLeft left;
	memset(&left, 0, sizeof(left));
	left.m_type = NET_MSG_LEFT;
	left.m_playerId = (uint8_t) p_idx;
	BroadcastReady(p_relay, &left, sizeof(left), p_idx);
	LogPeerEvent(p_relay, "slot %d: %s left", p_idx, name);
}

static void Deny(NET_RELAY* p_relay, ENetPeer* p_peer, int p_idx, int p_code)
{
	if (p_idx >= 0) {
		ReleaseSlot(p_relay, p_idx, DenyReason(p_code));
	}
	else {
		LogPeerEvent(p_relay, "rejected connection: %s", DenyReason(p_code));
	}
	p_peer->data = 0;
	enet_peer_timeout(p_peer, NET_DENY_LINGER_LIMIT, NET_DENY_LINGER_MIN_MS, NET_DENY_LINGER_MAX_MS);
	enet_peer_disconnect_later(p_peer, (enet_uint32) p_code);
}

static void Strike(NET_RELAY* p_relay, int p_idx, const char* p_kind)
{
	NET_RELAY_SLOT* slot = &p_relay->m_slots[p_idx];
	++p_relay->m_dropped;
	if (!slot->m_strikes) {
		LogPeerEvent(p_relay, "slot %d: dropped invalid %s", p_idx, p_kind);
	}
	++slot->m_strikes;
	if (slot->m_strikes >= NET_RELAY_STRIKE_LIMIT) {
		Deny(p_relay, slot->m_peer, p_idx, NET_DENY_PROTOCOL);
	}
}

static void FlushPresence(NET_RELAY* p_relay, int p_idx)
{
	NET_RELAY_SLOT* slot = &p_relay->m_slots[p_idx];
	if (!SlotIsReady(slot) || !slot->m_presenceDirty) {
		return;
	}
	slot->m_presenceDirty = 0;
	NetMsgPresence presence;
	memset(&presence, 0, sizeof(presence));
	presence.m_type = NET_MSG_PRESENCE;
	presence.m_playerId = (uint8_t) p_idx;
	presence.m_mapSeq = slot->m_mapSeq;
	presence.m_mapFlags = slot->m_mapFlags;
	memcpy(presence.m_map, slot->m_map, NET_MAP_LEN);
	BroadcastReady(p_relay, &presence, sizeof(presence), p_idx);
}

static void SendJoined(const NET_RELAY* p_relay, int p_about, ENetPeer* p_to)
{
	const NET_RELAY_SLOT* slot = &p_relay->m_slots[p_about];
	NetMsgJoined joined;
	memset(&joined, 0, sizeof(joined));
	joined.m_type = NET_MSG_JOINED;
	joined.m_playerId = (uint8_t) p_about;
	joined.m_mapSeq = slot->m_mapSeq;
	joined.m_mapFlags = slot->m_mapFlags;
	memcpy(joined.m_name, slot->m_name, NET_NAME_LEN);
	memcpy(joined.m_map, slot->m_map, NET_MAP_LEN);
	SendTo(p_to, &joined, sizeof(joined), NET_CHANNEL_EVENTS, 1);
}

static int SlotMapIsOfferable(const NET_RELAY_SLOT* p_slot)
{
	return SlotIsOnMap(p_slot) && NetMapNameIsGameplay(p_slot->m_map);
}

static const char* StartMapOffer(const NET_RELAY* p_relay)
{
	if (p_relay->m_startMap[0]) {
		return p_relay->m_startMap;
	}
	if (p_relay->m_config.m_followPlayers) {
		for (int i = 0; i < p_relay->m_config.m_maxPlayers; ++i) {
			if (SlotMapIsOfferable(&p_relay->m_slots[i])) {
				return p_relay->m_slots[i].m_map;
			}
		}
	}
	return p_relay->m_config.m_map;
}

static void SendWelcome(const NET_RELAY* p_relay, int p_idx)
{
	NetMsgWelcome welcome;
	memset(&welcome, 0, sizeof(welcome));
	welcome.m_type = NET_MSG_WELCOME;
	welcome.m_playerId = (uint8_t) p_idx;
	welcome.m_maxPlayers = (uint8_t) p_relay->m_config.m_maxPlayers;
	welcome.m_flags = p_relay->m_config.m_playerHosted ? NET_WELCOMEF_PLAYER_HOSTED : 0;
	welcome.m_snapshotMs = (uint16_t) p_relay->m_config.m_rateMs;
	memcpy(welcome.m_map, StartMapOffer(p_relay), NET_MAP_LEN);
	SendTo(p_relay->m_slots[p_idx].m_peer, &welcome, sizeof(welcome), NET_CHANNEL_EVENTS, 1);
}

static void HandleHello(NET_RELAY* p_relay, int p_idx, const uint8_t* p_data, size_t p_length)
{
	NET_RELAY_SLOT* slot = &p_relay->m_slots[p_idx];
	if (slot->m_ready) {
		return;
	}
	if (p_length >= offsetof(NetMsgHello, m_version) + 1 &&
		p_data[offsetof(NetMsgHello, m_version)] != NET_PROTOCOL_VERSION) {
		Deny(p_relay, slot->m_peer, p_idx, NET_DENY_VERSION);
		return;
	}
	if (p_length != sizeof(NetMsgHello)) {
		Strike(p_relay, p_idx, "hello");
		return;
	}
	NetMsgHello hello;
	memcpy(&hello, p_data, sizeof(hello));
	memcpy(slot->m_name, hello.m_name, NET_NAME_LEN);
	NetSanitizeName(slot->m_name);
	BucketFill(&slot->m_shootBudget, NET_RELAY_SHOOT_BURST, RelayNow(p_relay));
	slot->m_ready = 1;
	++p_relay->m_readyCount;

	SendWelcome(p_relay, p_idx);
	for (int i = 0; i < p_relay->m_config.m_maxPlayers; ++i) {
		if (i != p_idx && SlotIsReady(&p_relay->m_slots[i])) {
			SendJoined(p_relay, i, slot->m_peer);
		}
	}
	for (int i = 0; i < p_relay->m_config.m_maxPlayers; ++i) {
		if (i != p_idx && SlotIsReady(&p_relay->m_slots[i])) {
			SendJoined(p_relay, p_idx, p_relay->m_slots[i].m_peer);
		}
	}
	LogPeerEvent(p_relay, "slot %d: %s joined", p_idx, slot->m_name);
}

static void HandleMap(NET_RELAY* p_relay, int p_idx, const uint8_t* p_data, size_t p_length)
{
	NET_RELAY_SLOT* slot = &p_relay->m_slots[p_idx];
	if (p_length != sizeof(NetMsgMap)) {
		Strike(p_relay, p_idx, "map");
		return;
	}
	if (!slot->m_ready) {
		return;
	}
	NetMsgMap msg;
	memcpy(&msg, p_data, sizeof(msg));
	char name[NET_MAP_LEN];
	int nameValid = NetNormalizeMapName(msg.m_map, NET_MAP_LEN, name);
	int claimsPlayable = (msg.m_mapFlags & NET_MAPF_PLAYABLE) != 0;
	int garbage = !nameValid && (claimsPlayable || msg.m_map[0]);

	slot->m_mapSeq = msg.m_mapSeq;
	slot->m_mapFlags = (uint8_t) (claimsPlayable && nameValid ? NET_MAPF_PLAYABLE : 0);
	memcpy(slot->m_map, name, NET_MAP_LEN);
	slot->m_hasActor = 0;
	slot->m_fresh = 0;
	slot->m_presenceDirty = 1;

	if (SlotIsOnMap(slot)) {
		LogPeerEvent(p_relay, "slot %d: %s is on %s", p_idx, slot->m_name, slot->m_map);
	}
	else {
		LogPeerEvent(p_relay, "slot %d: %s is away", p_idx, slot->m_name);
	}
	if (garbage) {
		Strike(p_relay, p_idx, "map");
	}
}

static void HandleSnapshot(NET_RELAY* p_relay, int p_idx, const uint8_t* p_data, size_t p_length)
{
	NET_RELAY_SLOT* slot = &p_relay->m_slots[p_idx];
	if (p_length != sizeof(NetMsgSnapshot)) {
		Strike(p_relay, p_idx, "snapshot");
		return;
	}
	NetMsgSnapshot msg;
	memcpy(&msg, p_data, sizeof(msg));
	if (!SlotIsOnMap(slot) || msg.m_mapSeq != slot->m_mapSeq) {
		return;
	}
	NetActor actor = msg.m_actor;
	if (!NetActorValid(actor)) {
		Strike(p_relay, p_idx, "snapshot");
		return;
	}
	NetActorCanonical(&actor);
	slot->m_actor = actor;
	slot->m_hasActor = 1;
	slot->m_fresh = 1;
}

static void HandleShoot(NET_RELAY* p_relay, int p_idx, const uint8_t* p_data, size_t p_length)
{
	NET_RELAY_SLOT* slot = &p_relay->m_slots[p_idx];
	if (p_length != sizeof(NetMsgShoot)) {
		Strike(p_relay, p_idx, "shoot");
		return;
	}
	NetMsgShoot msg;
	memcpy(&msg, p_data, sizeof(msg));
	if (!SlotIsOnMap(slot) || msg.m_mapSeq != slot->m_mapSeq) {
		return;
	}
	if (!NetShootValid(msg)) {
		Strike(p_relay, p_idx, "shoot");
		return;
	}
	if (!BucketTake(&slot->m_shootBudget, NET_RELAY_SHOOT_BURST, NET_RELAY_SHOOT_REFILL_MS, RelayNow(p_relay))) {
		return;
	}
	FlushPresence(p_relay, p_idx);

	NetMsgShoot shoot;
	memset(&shoot, 0, sizeof(shoot));
	shoot.m_type = NET_MSG_SHOOT;
	shoot.m_playerId = (uint8_t) p_idx;
	shoot.m_mapSeq = slot->m_mapSeq;
	shoot.m_x = msg.m_x;
	shoot.m_y = msg.m_y;
	for (int i = 0; i < p_relay->m_config.m_maxPlayers; ++i) {
		if (i != p_idx && SlotsShareMap(slot, &p_relay->m_slots[i])) {
			SendTo(p_relay->m_slots[i].m_peer, &shoot, sizeof(shoot), NET_CHANNEL_EVENTS, 1);
		}
	}
}

static void HandleReceive(NET_RELAY* p_relay, ENetPeer* p_peer, const uint8_t* p_data, size_t p_length)
{
	int idx = SlotOf(p_relay, p_peer);
	if (idx < 0) {
		return;
	}
	if (!p_length || !p_data) {
		Strike(p_relay, idx, "packet");
		return;
	}
	switch (p_data[0]) {
	case NET_MSG_HELLO:
		HandleHello(p_relay, idx, p_data, p_length);
		break;
	case NET_MSG_MAP:
		HandleMap(p_relay, idx, p_data, p_length);
		break;
	case NET_MSG_SNAPSHOT:
		HandleSnapshot(p_relay, idx, p_data, p_length);
		break;
	case NET_MSG_SHOOT:
		HandleShoot(p_relay, idx, p_data, p_length);
		break;
	default:
		Strike(p_relay, idx, "message");
		break;
	}
}

static void HandleConnect(NET_RELAY* p_relay, ENetPeer* p_peer, enet_uint32 p_connectData)
{
	p_peer->data = 0;
	if (p_connectData != (enet_uint32) NET_CONNECT_MAGIC) {
		Deny(p_relay, p_peer, -1, NET_DENY_VERSION);
		return;
	}
	int idx = FindFreeSlot(p_relay);
	if (idx < 0) {
		Deny(p_relay, p_peer, -1, NET_DENY_FULL);
		return;
	}
	NET_RELAY_SLOT* slot = &p_relay->m_slots[idx];
	memset(slot, 0, sizeof(*slot));
	slot->m_peer = p_peer;
	slot->m_connectMs = RelayNow(p_relay);
	p_peer->data = (void*) (intptr_t) (idx + 1);
	enet_peer_timeout(p_peer, NET_TIMEOUT_LIMIT, NET_TIMEOUT_MIN_MS, NET_TIMEOUT_MAX_MS);
}

static void HandleDisconnect(NET_RELAY* p_relay, ENetPeer* p_peer)
{
	int idx = SlotOf(p_relay, p_peer);
	p_peer->data = 0;
	if (idx >= 0) {
		ReleaseSlot(p_relay, idx, 0);
	}
}

static void HandleEvent(NET_RELAY* p_relay, ENetEvent* p_event)
{
	switch (p_event->type) {
	case ENET_EVENT_TYPE_CONNECT:
		HandleConnect(p_relay, p_event->peer, p_event->data);
		break;
	case ENET_EVENT_TYPE_RECEIVE:
		HandleReceive(p_relay, p_event->peer, p_event->packet->data, p_event->packet->dataLength);
		enet_packet_destroy(p_event->packet);
		break;
	case ENET_EVENT_TYPE_DISCONNECT:
	case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
		HandleDisconnect(p_relay, p_event->peer);
		break;
	default:
		break;
	}
}

static void SweepHelloDeadlines(NET_RELAY* p_relay, enet_uint32 p_now)
{
	enet_uint32 deadline = (enet_uint32) p_relay->m_config.m_helloTimeoutMs;
	for (int i = 0; i < p_relay->m_config.m_maxPlayers; ++i) {
		NET_RELAY_SLOT* slot = &p_relay->m_slots[i];
		if (!slot->m_peer || slot->m_ready || NetElapsedMs(p_now, slot->m_connectMs) < deadline) {
			continue;
		}
		ENetPeer* peer = slot->m_peer;
		ReleaseSlot(p_relay, i, DenyReason(NET_DENY_HELLO_TIMEOUT));
		enet_peer_disconnect_now(peer, NET_DENY_HELLO_TIMEOUT);
	}
}

static void SendBundles(NET_RELAY* p_relay)
{
	for (int to = 0; to < p_relay->m_config.m_maxPlayers; ++to) {
		const NET_RELAY_SLOT* recipient = &p_relay->m_slots[to];
		if (!SlotIsOnMap(recipient)) {
			continue;
		}
		NetMsgSnapshots bundle;
		memset(&bundle, 0, sizeof(bundle));
		bundle.m_type = NET_MSG_SNAPSHOTS;
		int count = 0;
		for (int from = 0; from < p_relay->m_config.m_maxPlayers; ++from) {
			const NET_RELAY_SLOT* sender = &p_relay->m_slots[from];
			if (from == to || !sender->m_hasActor || !sender->m_fresh || !SlotsShareMap(sender, recipient)) {
				continue;
			}
			NetSnapshotEntry* entry = &bundle.m_entries[count++];
			entry->m_playerId = (uint8_t) from;
			entry->m_mapSeq = sender->m_mapSeq;
			entry->m_actor = sender->m_actor;
		}
		if (count) {
			bundle.m_count = (uint8_t) count;
			SendTo(recipient->m_peer, &bundle, NetMsgSnapshotsSize(count), NET_CHANNEL_SNAPSHOTS, 0);
		}
	}
}

static void Tick(NET_RELAY* p_relay, enet_uint32 p_now, int p_drainedAllEvents)
{
	if (p_drainedAllEvents) {
		SweepHelloDeadlines(p_relay, p_now);
	}
	for (int i = 0; i < p_relay->m_config.m_maxPlayers; ++i) {
		FlushPresence(p_relay, i);
	}
	SendBundles(p_relay);
	for (int i = 0; i < p_relay->m_config.m_maxPlayers; ++i) {
		p_relay->m_slots[i].m_fresh = 0;
	}
}

static enet_uint32 ServiceWait(const NET_RELAY* p_relay, enet_uint32 p_now, unsigned int p_maxWaitMs)
{
	if (!p_maxWaitMs) {
		return 0;
	}
	if (!p_relay->m_readyCount) {
		return (enet_uint32) p_maxWaitMs;
	}
	enet_uint32 rate = (enet_uint32) p_relay->m_config.m_rateMs;
	enet_uint32 elapsed = NetElapsedMs(p_now, p_relay->m_lastTickMs);
	enet_uint32 untilTick = elapsed >= rate ? 0 : rate - elapsed;
	return untilTick < p_maxWaitMs ? untilTick : (enet_uint32) p_maxWaitMs;
}

static void NoteStall(NET_RELAY* p_relay, enet_uint32 p_now, enet_uint32 p_intendedWaitMs)
{
	enet_uint32 gap = NetElapsedMs(p_now, p_relay->m_lastServiceMs);
	enet_uint32 stall = gap > p_intendedWaitMs ? gap - p_intendedWaitMs : 0;
	if (stall <= NET_RELAY_STALL_LOG_MS || !ConnectedCount(p_relay)) {
		return;
	}
	if (stall > p_relay->m_worstStallMs) {
		p_relay->m_worstStallMs = stall;
	}
	Log(p_relay, "stalled %u ms", stall);
}

void NetRelay_DefaultConfig(NET_RELAY_CONFIG* p_config)
{
	if (!p_config) {
		return;
	}
	memset(p_config, 0, sizeof(*p_config));
	p_config->m_port = NET_DEFAULT_PORT;
	p_config->m_maxPlayers = NET_DEFAULT_MAX_PLAYERS;
	p_config->m_rateMs = NET_DEFAULT_SNAPSHOT_MS;
	p_config->m_helloTimeoutMs = NET_RELAY_HELLO_TIMEOUT_DEFAULT_MS;
	p_config->m_followPlayers = 1;
	const char defaultStartMap[] = "maps\\level_01.map";
	memcpy(p_config->m_map, defaultStartMap, sizeof(defaultStartMap));
}

const char* NetRelay_CheckConfig(const NET_RELAY_CONFIG* p_config)
{
	if (!p_config) {
		return "missing configuration";
	}
	if (p_config->m_port < NET_RELAY_PORT_MIN || p_config->m_port > NET_RELAY_PORT_MAX) {
		return "port must be 1..65535";
	}
	if (p_config->m_maxPlayers < NET_RELAY_PLAYERS_MIN || p_config->m_maxPlayers > NET_MAX_PLAYERS) {
		return "max players must be 2..16";
	}
	if (p_config->m_rateMs < NET_RELAY_RATE_MIN_MS || p_config->m_rateMs > NET_RELAY_RATE_MAX_MS) {
		return "rate must be 10..1000 ms";
	}
	if (p_config->m_helloTimeoutMs < NET_RELAY_HELLO_TIMEOUT_MIN_MS ||
		p_config->m_helloTimeoutMs > NET_RELAY_HELLO_TIMEOUT_MAX_MS) {
		return "hello timeout must be 100..60000 ms";
	}
	char normalized[NET_MAP_LEN];
	if (p_config->m_map[0] && !NetNormalizeMapName(p_config->m_map, NET_MAP_LEN, normalized)) {
		return "map name is not a valid relative path";
	}
	return 0;
}

NET_RELAY* NetRelay_Create(const NET_RELAY_CONFIG* p_config, char* p_error, size_t p_errorSize)
{
	if (p_error && p_errorSize) {
		p_error[0] = 0;
	}
	const char* problem = NetRelay_CheckConfig(p_config);
	if (problem) {
		SetError(p_error, p_errorSize, "%s", problem);
		return 0;
	}
	NET_RELAY* relay = (NET_RELAY*) calloc(1, sizeof(NET_RELAY));
	if (!relay) {
		SetError(p_error, p_errorSize, "out of memory");
		return 0;
	}
	relay->m_config = *p_config;
	NetNormalizeMapName(p_config->m_map, NET_MAP_LEN, relay->m_config.m_map);

	if (enet_initialize() != 0) {
		SetError(p_error, p_errorSize, "cannot initialise networking");
		free(relay);
		return 0;
	}
	ENetAddress address;
	memset(&address, 0, sizeof(address));
	address.host = ENET_HOST_ANY;
	address.port = (enet_uint16) p_config->m_port;
	size_t peerCount = (size_t) p_config->m_maxPlayers + NET_RELAY_REJECT_MARGIN;
	relay->m_host = enet_host_create(&address, peerCount, NET_CHANNEL_COUNT, 0, 0);
	if (!relay->m_host) {
		SetError(p_error, p_errorSize, "cannot listen on udp port %d", p_config->m_port);
		enet_deinitialize();
		free(relay);
		return 0;
	}
	relay->m_host->maximumPacketSize = NET_MAX_PACKET_BYTES;
	relay->m_host->maximumWaitingData = NET_MAX_WAITING_BYTES;

	enet_uint32 now = RelayNow(relay);
	relay->m_lastTickMs = now;
	relay->m_lastServiceMs = now;
	BucketFill(&relay->m_logBudget, NET_RELAY_LOG_BURST, now);
	Log(relay,
		"listening on udp port %d (max %d players, %d ms)",
		p_config->m_port,
		p_config->m_maxPlayers,
		p_config->m_rateMs);
	return relay;
}

int NetRelay_Service(NET_RELAY* p_relay, unsigned int p_maxWaitMs)
{
	if (!p_relay) {
		return -1;
	}
	enet_uint32 wait = ServiceWait(p_relay, RelayNow(p_relay), p_maxWaitMs);
	ENetEvent event;
	int first = enet_host_service(p_relay->m_host, &event, wait);
	NoteStall(p_relay, RelayNow(p_relay), wait);

	int handled = 0;
	int pending = first;
	while (pending > 0) {
		HandleEvent(p_relay, &event);
		++handled;
		if (handled >= NET_RELAY_MAX_EVENTS_PER_SERVICE) {
			break;
		}
		pending = enet_host_service(p_relay->m_host, &event, 0);
	}

	enet_uint32 now = RelayNow(p_relay);
	if (NetElapsedMs(now, p_relay->m_lastTickMs) >= (enet_uint32) p_relay->m_config.m_rateMs) {
		Tick(p_relay, now, pending <= 0);
		p_relay->m_lastTickMs = now;
	}
	enet_host_flush(p_relay->m_host);
	p_relay->m_lastServiceMs = RelayNow(p_relay);
	return first < 0 ? -1 : handled;
}

void NetRelay_SetStartMap(NET_RELAY* p_relay, const char* p_map)
{
	if (!p_relay) {
		return;
	}
	memset(p_relay->m_startMap, 0, NET_MAP_LEN);
	if (p_map && p_map[0]) {
		NetNormalizeMapName(p_map, NET_MAP_LEN, p_relay->m_startMap);
	}
}

void NetRelay_SetLog(NET_RELAY* p_relay, NET_RELAY_LOG_FN p_log, void* p_logUser)
{
	if (!p_relay) {
		return;
	}
	p_relay->m_config.m_log = p_log;
	p_relay->m_config.m_logUser = p_logUser;
}

void NetRelay_GetStats(const NET_RELAY* p_relay, NET_RELAY_STATS* p_out)
{
	if (!p_out) {
		return;
	}
	memset(p_out, 0, sizeof(*p_out));
	if (!p_relay) {
		return;
	}
	p_out->m_port = p_relay->m_config.m_port;
	p_out->m_connected = ConnectedCount(p_relay);
	p_out->m_ready = p_relay->m_readyCount;
	p_out->m_dropped = p_relay->m_dropped;
	p_out->m_worstStallMs = p_relay->m_worstStallMs;
}

void NetRelay_Destroy(NET_RELAY* p_relay)
{
	if (!p_relay) {
		return;
	}
	Log(p_relay, "shutting down");
	for (int i = 0; i < p_relay->m_config.m_maxPlayers; ++i) {
		ENetPeer* peer = p_relay->m_slots[i].m_peer;
		if (peer) {
			peer->data = 0;
			enet_peer_disconnect_now(peer, NET_DENY_SHUTDOWN);
		}
	}
	enet_host_flush(p_relay->m_host);
	enet_host_destroy(p_relay->m_host);
	enet_deinitialize();
	free(p_relay);
}
