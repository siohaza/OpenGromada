#ifndef NET_PROTOCOL_H
#define NET_PROTOCOL_H

#include <bit>
#include <limits>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static_assert(std::endian::native == std::endian::little, "multiplayer protocol requires a little-endian target");
static_assert(
	std::numeric_limits<float>::is_iec559 && sizeof(float) == 4,
	"multiplayer protocol requires IEEE-754 single floats"
);

enum {
	NET_PROTOCOL_VERSION = 2,
	NET_CONNECT_MAGIC = 0x4f470000 | NET_PROTOCOL_VERSION,
	NET_DEFAULT_PORT = 33100,
	NET_MAX_PLAYERS = 16,
	NET_DEFAULT_MAX_PLAYERS = 8,
	NET_DEFAULT_SNAPSHOT_MS = 30,
	NET_NAME_LEN = 16,
	NET_MAP_LEN = 48,
	NET_ADDRESS_LEN = 256,
	NET_ANI_DEATH = 15,
	NET_ANI_MAX = 16,
	NET_WEAPON_SLOTS = 10,
	NET_MAX_PACKET_BYTES = 1024,
	NET_MAX_WAITING_BYTES = 256 * 1024,
	NET_TIMEOUT_LIMIT = 32,
	NET_TIMEOUT_MIN_MS = 10000,
	NET_TIMEOUT_MAX_MS = 30000,
	NET_DENY_LINGER_LIMIT = 8,
	NET_DENY_LINGER_MIN_MS = 1000,
	NET_DENY_LINGER_MAX_MS = 3000
};

constexpr float NET_COORD_LIMIT = 65536.0f;
constexpr float NET_HEIGHT_LIMIT = 8192.0f;
constexpr float NET_SPEED_LIMIT = 4096.0f;

enum {
	NET_CHANNEL_EVENTS = 0,
	NET_CHANNEL_SNAPSHOTS = 1,
	NET_CHANNEL_COUNT = 2
};

enum {
	NET_MSG_HELLO = 1,
	NET_MSG_WELCOME = 2,
	NET_MSG_JOINED = 3,
	NET_MSG_LEFT = 4,
	NET_MSG_SHOOT = 5,
	NET_MSG_SNAPSHOT = 6,
	NET_MSG_SNAPSHOTS = 7,
	NET_MSG_MAP = 8,
	NET_MSG_PRESENCE = 9
};

enum {
	NET_DENY_NONE = 0,
	NET_DENY_VERSION = 1,
	NET_DENY_FULL = 2,
	NET_DENY_HELLO_TIMEOUT = 3,
	NET_DENY_PROTOCOL = 4,
	NET_DENY_SHUTDOWN = 5
};

enum {
	NET_MAPF_PLAYABLE = 1
};

enum {
	NET_WELCOMEF_PLAYER_HOSTED = 1
};

#pragma pack(push, 1)

struct NetActor {
	float m_x;
	float m_y;
	float m_z;
	float m_speed;
	uint16_t m_hp;
	uint8_t m_dirLegs;
	uint8_t m_dirTorso;
	uint8_t m_weapon;
	uint8_t m_ani;
	uint8_t m_pad[2];
};
static_assert(sizeof(NetActor) == 24);

struct NetMsgHello {
	uint8_t m_type;
	uint8_t m_version;
	char m_name[NET_NAME_LEN];
};
static_assert(sizeof(NetMsgHello) == 18);
static_assert(offsetof(NetMsgHello, m_version) == 1);

struct NetMsgWelcome {
	uint8_t m_type;
	uint8_t m_playerId;
	uint8_t m_maxPlayers;
	uint8_t m_flags;
	uint16_t m_snapshotMs;
	uint8_t m_pad[2];
	char m_map[NET_MAP_LEN];
};
static_assert(sizeof(NetMsgWelcome) == 56);

struct NetMsgJoined {
	uint8_t m_type;
	uint8_t m_playerId;
	uint8_t m_mapSeq;
	uint8_t m_mapFlags;
	char m_name[NET_NAME_LEN];
	char m_map[NET_MAP_LEN];
};
static_assert(sizeof(NetMsgJoined) == 68);

struct NetMsgLeft {
	uint8_t m_type;
	uint8_t m_playerId;
};
static_assert(sizeof(NetMsgLeft) == 2);

struct NetMsgMap {
	uint8_t m_type;
	uint8_t m_mapSeq;
	uint8_t m_mapFlags;
	uint8_t m_pad;
	char m_map[NET_MAP_LEN];
};
static_assert(sizeof(NetMsgMap) == 52);

struct NetMsgPresence {
	uint8_t m_type;
	uint8_t m_playerId;
	uint8_t m_mapSeq;
	uint8_t m_mapFlags;
	char m_map[NET_MAP_LEN];
};
static_assert(sizeof(NetMsgPresence) == 52);

struct NetMsgShoot {
	uint8_t m_type;
	uint8_t m_playerId;
	uint8_t m_mapSeq;
	uint8_t m_pad;
	float m_x;
	float m_y;
};
static_assert(sizeof(NetMsgShoot) == 12);

struct NetMsgSnapshot {
	uint8_t m_type;
	uint8_t m_mapSeq;
	uint8_t m_pad[2];
	NetActor m_actor;
};
static_assert(sizeof(NetMsgSnapshot) == 28);
static_assert(offsetof(NetMsgSnapshot, m_actor) == 4);

struct NetSnapshotEntry {
	uint8_t m_playerId;
	uint8_t m_mapSeq;
	uint8_t m_pad[2];
	NetActor m_actor;
};
static_assert(sizeof(NetSnapshotEntry) == 28);
static_assert(offsetof(NetSnapshotEntry, m_actor) == 4);

struct NetMsgSnapshots {
	uint8_t m_type;
	uint8_t m_count;
	uint8_t m_pad[2];
	NetSnapshotEntry m_entries[NET_MAX_PLAYERS];
};
static_assert(sizeof(NetMsgSnapshots) == 4 + NET_MAX_PLAYERS * 28);
static_assert(offsetof(NetMsgSnapshots, m_entries) == 4);
static_assert(sizeof(NetMsgSnapshots) <= NET_MAX_PACKET_BYTES);

#pragma pack(pop)

inline size_t NetMsgSnapshotsSize(int p_count)
{
	return offsetof(NetMsgSnapshots, m_entries) + (size_t) p_count * sizeof(NetSnapshotEntry);
}

inline uint32_t NetElapsedMs(uint32_t p_now, uint32_t p_then)
{
	return p_now - p_then;
}

inline void NetSanitizeName(char* p_name)
{
	p_name[NET_NAME_LEN - 1] = 0;
	int length = 0;
	while (p_name[length]) {
		unsigned char c = (unsigned char) p_name[length];
		if (c < 0x20 || c > 0x7e) {
			p_name[length] = '_';
		}
		++length;
	}
	memset(p_name + length, 0, (size_t) (NET_NAME_LEN - length));
	if (!p_name[0]) {
		memcpy(p_name, "Player", 7);
	}
}

inline bool NetFloatFinite(float p_value)
{
	uint32_t bits;
	memcpy(&bits, &p_value, sizeof(bits));
	return (bits & 0x7f800000u) != 0x7f800000u;
}

inline bool NetFloatWithin(float p_value, float p_limit)
{
	return NetFloatFinite(p_value) && p_value >= -p_limit && p_value <= p_limit;
}

inline bool NetActorValid(const NetActor& p_actor)
{
	return NetFloatWithin(p_actor.m_x, NET_COORD_LIMIT) && NetFloatWithin(p_actor.m_y, NET_COORD_LIMIT) &&
		   NetFloatWithin(p_actor.m_z, NET_HEIGHT_LIMIT) && NetFloatWithin(p_actor.m_speed, NET_SPEED_LIMIT) &&
		   p_actor.m_ani <= NET_ANI_MAX && p_actor.m_weapon < NET_WEAPON_SLOTS;
}

inline bool NetShootValid(const NetMsgShoot& p_shoot)
{
	return NetFloatWithin(p_shoot.m_x, NET_COORD_LIMIT) && NetFloatWithin(p_shoot.m_y, NET_COORD_LIMIT);
}

inline void NetActorCanonical(NetActor* p_actor)
{
	p_actor->m_pad[0] = 0;
	p_actor->m_pad[1] = 0;
}

inline bool NetMapComponentValid(const char* p_component, size_t p_length)
{
	if (!p_length) {
		return false;
	}
	if (p_component[p_length - 1] == '.' || p_component[p_length - 1] == ' ') {
		return false;
	}
	return true;
}

inline int NetNormalizeMapName(const char* p_in, size_t p_inMax, char* p_out)
{
	memset(p_out, 0, NET_MAP_LEN);
	size_t length = 0;
	while (length < p_inMax && p_in[length]) {
		++length;
	}
	size_t i = 0;
	while (i + 1 < length && p_in[i] == '.' && (p_in[i + 1] == '\\' || p_in[i + 1] == '/')) {
		i += 2;
	}
	size_t out = 0;
	size_t component = 0;
	bool ok = true;
	for (; ok && i < length; ++i) {
		unsigned char c = (unsigned char) p_in[i];
		if (c == '/') {
			c = '\\';
		}
		if (c >= 'A' && c <= 'Z') {
			c = (unsigned char) (c - 'A' + 'a');
		}
		if (c < 0x20 || c > 0x7e || strchr(":*?\"<>|", c)) {
			ok = false;
		}
		else if (c == '\\') {
			if (!out) {
				ok = false;
			}
			else if (p_out[out - 1] != '\\') {
				ok = NetMapComponentValid(p_out + component, out - component);
				if (ok && out + 1 < NET_MAP_LEN) {
					p_out[out++] = '\\';
					component = out;
				}
				else {
					ok = false;
				}
			}
		}
		else if (out + 1 < NET_MAP_LEN) {
			p_out[out++] = (char) c;
		}
		else {
			ok = false;
		}
	}
	if (ok) {
		ok = out && NetMapComponentValid(p_out + component, out - component);
	}
	if (!ok) {
		memset(p_out, 0, NET_MAP_LEN);
		return 0;
	}
	return 1;
}

inline bool NetMapNameEqual(const char* p_a, const char* p_b)
{
	return memcmp(p_a, p_b, NET_MAP_LEN) == 0;
}

inline bool NetMapNameIsMapFile(const char* p_normalized)
{
	size_t length = strnlen(p_normalized, NET_MAP_LEN);
	return length > 9 && length < NET_MAP_LEN && !memcmp(p_normalized, "maps\\", 5) &&
		   !memcmp(p_normalized + length - 4, ".map", 4);
}

inline const char* NetMapLeaf(const char* p_normalized)
{
	const char* slash = strrchr(p_normalized, '\\');
	return slash ? slash + 1 : p_normalized;
}

inline bool NetMapNameIsGameplay(const char* p_normalized)
{
	const char* leaf = NetMapLeaf(p_normalized);
	return NetMapNameIsMapFile(p_normalized) && (!strncmp(leaf, "level_", 6) || !strncmp(leaf, "survive_", 8));
}

inline bool NetMapNameIsShop(const char* p_normalized)
{
	return NetMapNameIsMapFile(p_normalized) && !strcmp(NetMapLeaf(p_normalized), "shop.map");
}

inline bool NetMapNameIsParty(const char* p_normalized)
{
	return NetMapNameIsGameplay(p_normalized) || NetMapNameIsShop(p_normalized);
}

inline int NetParsePort(const char* p_text, int* p_port)
{
	int value = 0;
	int digits = 0;
	for (const char* c = p_text; *c; ++c) {
		if (*c < '0' || *c > '9' || ++digits > 5) {
			return 0;
		}
		value = value * 10 + (*c - '0');
	}
	if (!digits || value < 1 || value > 65535) {
		return 0;
	}
	*p_port = value;
	return 1;
}

inline bool NetHostCharValid(unsigned char p_c)
{
	return (p_c >= 'a' && p_c <= 'z') || (p_c >= 'A' && p_c <= 'Z') || (p_c >= '0' && p_c <= '9') || p_c == '.' ||
		   p_c == '-' || p_c == '_' || p_c == ':' || p_c == '%';
}

inline int NetParseAddress(const char* p_in, char* p_host, size_t p_hostSize, int* p_port)
{
	*p_port = NET_DEFAULT_PORT;
	if (!p_hostSize) {
		return 0;
	}
	p_host[0] = 0;
	size_t length = strlen(p_in);
	if (!length || length >= p_hostSize) {
		return 0;
	}
	const char* hostBegin = p_in;
	size_t hostLength = length;
	const char* portText = 0;
	if (p_in[0] == '[') {
		const char* close = strchr(p_in, ']');
		if (!close) {
			return 0;
		}
		hostBegin = p_in + 1;
		hostLength = (size_t) (close - hostBegin);
		if (close[1] == ':') {
			portText = close + 2;
		}
		else if (close[1]) {
			return 0;
		}
	}
	else {
		const char* first = strchr(p_in, ':');
		const char* last = strrchr(p_in, ':');
		if (first && first == last) {
			hostLength = (size_t) (first - p_in);
			portText = first + 1;
		}
	}
	if (!hostLength) {
		return 0;
	}
	for (size_t i = 0; i < hostLength; ++i) {
		if (!NetHostCharValid((unsigned char) hostBegin[i])) {
			return 0;
		}
	}
	if (portText && !NetParsePort(portText, p_port)) {
		return 0;
	}
	memcpy(p_host, hostBegin, hostLength);
	p_host[hostLength] = 0;
	return 1;
}

#endif
