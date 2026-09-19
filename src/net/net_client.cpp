#include "net/net_client.h"

#include "game/data_version.h"
#include "game/filedata.h"
#include "game/game_descriptor.h"
#include "game/gametime.h"
#include "game/map.h"
#include "net/net_enet.h"
#include "net/net_protocol.h"
#include "net/net_relay.h"
#include "net/net_ui.h"
#include "platform/portable_config.h"
#include "platform/timing.h"
#include "sprite/sprite.h"
#include "util/decomp.h"
#include "util/myerror.h"
#include "util/registry.h"
#include "util/resource.h"
#include "util/string.h"
#include "video/vid.h"
#include "video/vid_exdata.h"

#include <SDL3/SDL.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum NET_CLIENT_STATE {
	NET_IDLE,
	NET_RESOLVING,
	NET_CONNECTING,
	NET_HANDSHAKE,
	NET_ONLINE
};

enum NET_CLIENT_AUTO {
	NET_AUTO_NONE,
	NET_AUTO_CONNECT,
	NET_AUTO_HOST
};

enum NET_CLIENT_RESOLVE {
	NET_RESOLVE_RUNNING,
	NET_RESOLVE_DONE,
	NET_RESOLVE_FAILED
};

enum NET_CLIENT_HOST_KIND {
	NET_HOST_NAME,
	NET_HOST_LITERAL,
	NET_HOST_MALFORMED
};

enum NET_CLIENT_INVALID {
	NET_INVALID_PACKET,
	NET_INVALID_PLAYER_ID,
	NET_INVALID_ACTOR,
	NET_INVALID_SHOT,
	NET_INVALID_WELCOME,
	NET_INVALID_COUNT
};

enum {
	NET_AS1_PLAYER_VID = 9,
	NET_AS1_PLAYER_SPRCLASS = 7,
	NET_AS1_WEAPON_VID_BASE = 10,
	NET_AS1_WEAPON_ARC = 8,
	NET_AS1_DEFAULT_WEAPON = 1,
	NET_AS1_FALLBACK_MAX_HP = 110,
	NET_AS1_PLAYER_ARMY = 0,
	NET_ELSEWHERE_LOG_INTERVAL_MS = 1000,
	NET_ENGINE_RUN_WHILE_INACTIVE = 2,
	NET_SCRIPT_ENTRY_IS_FUNCTION = 3,
	NET_FOLLOW_IDLE = 0,
	NET_FOLLOW_REQUESTED = 1,
	NET_FOLLOW_COUNTDOWN = 2,
	NET_FOLLOW_GRACE_MS = 10000,
	NET_AS1_GAME_WIN = 1,
	NET_AS1_GAME_LOOSE = 3,
	NET_AS1_CAMPAIGN_LAST_LEVEL = 10,
	NET_AS1_ADDON_LAST_LEVEL = 5,
	NET_LEVEL_TEXT_LEN = 8,
	NET_AS1_PROXY_BEHAVE = 0,
	NET_AS1_SHOT_AMMO_GRANT = 10,
	NET_AS1_SHOT_AMMO_FLOOR = -2,
	NET_AS1_ACT_ATTACK = 0x25,
	NET_AS1_ACT_CHANGE_VID = 0x3e,
	NET_AS1_ACT_DAMAGE = 0x55,
	NET_AS1_ACT_ADD_AMMO = 93,
	NET_AS1_ACT_SET_BEHAVE = 95,
	NET_AS1_CMD_STOP = 0,
	NET_AS1_CMD_ATTACK = 4
};

enum {
	NET_SAMPLE_COUNT = 8,
	NET_EVENTS_PER_PUMP = 256,
	NET_STATUS_LEN = 160,
	NET_LOOPBACK_TEXT_LEN = 32,
	NET_IPV4_OCTETS = 4,
	NET_IPV4_OCTET_DIGITS = 3,
	NET_IPV4_OCTET_MAX = 255
};

constexpr unsigned int NET_ENGINE_FLAG_PAUSED = 0x10;
constexpr unsigned int NET_ENGINE_FLAG_LOADING = 0x20;
constexpr unsigned int NET_ENGINE_FLAG_LOAD_PENDING = 0x40;
constexpr unsigned int NET_ENGINE_FLAG_DEMO_PLAYBACK = 0x200;
constexpr unsigned int NET_ENGINE_FLAG_SCRIPTS_HELD = 0x80000;
constexpr unsigned int NET_ENGINE_MAP_CONTAINER = 0x2050414d;
constexpr unsigned int NET_SPRITE_COMMAND_MASK = 0x7c;
constexpr unsigned int NET_SPRITE_COMMAND_SHIFT = 2;
constexpr unsigned int NET_SPRITE_ARMY_SHIFT = 11;
constexpr unsigned int NET_SPRITE_ARMY_MASK = 3;

constexpr unsigned int NET_INTERP_DELAY_MS = 100;
constexpr unsigned int NET_FIRE_HOLD_MS = 500;
constexpr unsigned int NET_STALE_MS = 2000;
constexpr unsigned int NET_RESPAWN_BACKOFF_MS = 500;
constexpr unsigned int NET_RESOLVE_TIMEOUT_MS = 10000;
constexpr unsigned int NET_CONNECT_TIMEOUT_MS = 8000;
constexpr unsigned int NET_HANDSHAKE_TIMEOUT_MS = 8000;
constexpr unsigned int NET_SHOT_DEDUP_MS = 15;
constexpr unsigned int NET_SNAPSHOT_MIN_MS = 10;
constexpr unsigned int NET_SNAPSHOT_MAX_MS = 1000;

constexpr float NET_SNAP_DISTANCE = 300.0f;
constexpr float NET_AS1_ARC_TARGET_RISE = 80.0f;
constexpr float NET_AS1_MUZZLE_HEIGHT = 19.0f;

struct NET_CLIENT_SAMPLE {
	NetActor m_actor;
	unsigned int m_time;
};

struct NET_CLIENT_POSE {
	float m_x;
	float m_y;
	float m_z;
	unsigned char m_dirLegs;
	unsigned char m_dirTorso;
};

struct NET_CLIENT_REMOTE {
	int m_used;
	char m_name[NET_NAME_LEN];
	char m_map[NET_MAP_LEN];
	unsigned char m_mapSeq;
	unsigned char m_mapFlags;
	int m_sameMap;
	int m_hereLogged;
	int m_elsewhereLogged;
	unsigned int m_elsewhereLogTime;
	char m_lastNamedMap[NET_MAP_LEN];
	int m_lastNamedPlayable;
	SPRITE* m_sprite;
	NET_CLIENT_SAMPLE m_samples[NET_SAMPLE_COUNT];
	int m_sampleNext;
	int m_sampleCount;
	int m_weapon;
	unsigned int m_lastShotTime;
	unsigned int m_spawnTime;
	int m_spawnTried;
};

struct NET_CLIENT_RESOLVE_JOB {
	SDL_AtomicInt m_refs;
	SDL_AtomicInt m_state;
	char m_host[NET_ADDRESS_LEN];
	ENetAddress m_address;
};

struct NET_CLIENT_SESSION {
	int m_enetReady;
	ENetHost* m_host;
	ENetPeer* m_peer;
	NET_RELAY* m_relay;
	NET_CLIENT_RESOLVE_JOB* m_resolveJob;
	int m_state;
	unsigned int m_stateTime;
	char m_status[NET_STATUS_LEN];
	char m_name[NET_NAME_LEN];
	char m_address[NET_ADDRESS_LEN];
	char m_hostName[NET_ADDRESS_LEN];
	int m_port;
	int m_playerId = -1;
	int m_maxPlayers;
	int m_playerHosted;
	unsigned int m_snapshotMs;
	unsigned int m_lastSendTime;
	int m_snapshotSent;
	char m_mapKey[NET_MAP_LEN];
	unsigned char m_mapSeq;
	int m_mapAnnounced;
	int m_announcedPlayable;
	int m_playingLogged;
	char m_pendingStartMap[NET_MAP_LEN];
	int m_pendingRequeued;
	int m_following;
	int m_followSecondsShown;
	int m_followPauseNoticed;
	char m_followLeader[NET_NAME_LEN];
	float m_attackX;
	float m_attackY;
	unsigned int m_lastShotSent;
	int m_shotSent;
	unsigned int m_invalidLogged;
	int m_autoKind;
	int m_autoPort;
	char m_autoAddress[NET_ADDRESS_LEN];
	char m_autoName[NET_NAME_LEN];
	NET_CLIENT_REMOTE m_remote[NET_MAX_PLAYERS];
};

int g_netProxyCount;

namespace
{
NET_CLIENT_SESSION g_net;
}

#ifdef OPENGROMADA_NET_TESTHOOKS

enum {
	NET_HOOK_MAX = 32,
	NET_HOOK_COMMAND_LEN = 16,
	NET_HOOK_ARGUMENT_LEN = 128,
	NET_HOOK_KILL_DAMAGE = 1000000
};

struct NET_CLIENT_HOOK_ENTRY {
	unsigned int m_when;
	int m_fired;
	char m_command[NET_HOOK_COMMAND_LEN];
	char m_argument[NET_HOOK_ARGUMENT_LEN];
};

struct NET_CLIENT_HOOKS {
	int m_parsed;
	int m_started;
	unsigned int m_startTime;
	unsigned int m_tickBias;
	int m_count;
	NET_CLIENT_HOOK_ENTRY m_entries[NET_HOOK_MAX];
};

namespace
{
NET_CLIENT_HOOKS g_hooks;
}

#endif

static void SetStatus(const char* p_fmt, ...) DECOMP_PRINTF(1, 2);
static void LogLine(const char* p_fmt, ...) DECOMP_PRINTF(1, 2);

static void SetStatus(const char* p_fmt, ...)
{
	va_list args;
	va_start(args, p_fmt);
	vsnprintf(g_net.m_status, sizeof(g_net.m_status), p_fmt, args);
	va_end(args);
	if (!Map) {
		return;
	}
	MYERROR::Log(::Error, "NET: %s", g_net.m_status);
}

static void LogLine(const char* p_fmt, ...)
{
	if (!Map) {
		return;
	}
	char line[NET_STATUS_LEN];
	va_list args;
	va_start(args, p_fmt);
	vsnprintf(line, sizeof(line), p_fmt, args);
	va_end(args);
	MYERROR::Log(::Error, "NET: %s", line);
}

static void CopyText(char* p_out, size_t p_size, const char* p_text)
{
	size_t length = p_text ? strlen(p_text) : 0;
	if (length >= p_size) {
		length = p_size - 1;
	}
	if (length) {
		memcpy(p_out, p_text, length);
	}
	p_out[length] = 0;
}

static unsigned int NetNow()
{
#ifdef OPENGROMADA_NET_TESTHOOKS
	return Platform_Ticks() + g_hooks.m_tickBias;
#else
	return Platform_Ticks();
#endif
}

static bool NetSupported()
{
	return GameDesc && GameDesc->m_id == GAME_AS1 && !GameData_IsSteam();
}

static void RefuseUnsupported()
{
	SetStatus("multiplayer needs Alien Shooter 1 with GOG or retail 1.2 game data");
}

static SPRITE* LocalFlagman()
{
	if (!Map || !Map->m_player[0]) {
		return 0;
	}
	return Map->Flagman(0);
}

struct NET_CLIENT_FLAG_HOLD {
	MAP* m_map;
	unsigned int m_bit;
	bool m_owned;

	NET_CLIENT_FLAG_HOLD(MAP* p_map, unsigned int p_bit) : m_map(p_map), m_bit(p_bit), m_owned(!(p_map->m_flag & p_bit))
	{
		if (m_owned) {
			m_map->m_flag |= m_bit;
		}
	}

	~NET_CLIENT_FLAG_HOLD()
	{
		if (m_owned) {
			m_map->m_flag &= ~m_bit;
		}
	}

	NET_CLIENT_FLAG_HOLD(const NET_CLIENT_FLAG_HOLD&) = delete;
	NET_CLIENT_FLAG_HOLD& operator=(const NET_CLIENT_FLAG_HOLD&) = delete;
};

static float ClampToSpan(float p_value, float p_span)
{
	float last = p_span - 1.0f;
	if (!(last > 0.0f)) {
		last = 0.0f;
	}
	if (!(p_value >= 0.0f)) {
		return 0.0f;
	}
	if (p_value > last) {
		return last;
	}
	return p_value;
}

static void EnterState(int p_state, unsigned int p_now)
{
	g_net.m_state = p_state;
	g_net.m_stateTime = p_now;
}

static void NoteInvalid(int p_kind)
{
	static const char* const kindNames[NET_INVALID_COUNT] = {"packet", "player id", "actor", "shot", "welcome"};
	unsigned int bit = 1u << p_kind;
	if (g_net.m_invalidLogged & bit) {
		return;
	}
	g_net.m_invalidLogged |= bit;
	LogLine("ignored invalid %s", kindNames[p_kind]);
}

static void ClearSamples(NET_CLIENT_REMOTE* p_remote)
{
	p_remote->m_sampleNext = 0;
	p_remote->m_sampleCount = 0;
}

static void PushSample(NET_CLIENT_REMOTE* p_remote, const NetActor& p_actor, unsigned int p_now)
{
	NET_CLIENT_SAMPLE* sample = &p_remote->m_samples[p_remote->m_sampleNext];
	sample->m_actor = p_actor;
	sample->m_time = p_now;
	p_remote->m_sampleNext = (p_remote->m_sampleNext + 1) % NET_SAMPLE_COUNT;
	if (p_remote->m_sampleCount < NET_SAMPLE_COUNT) {
		++p_remote->m_sampleCount;
	}
}

static const NET_CLIENT_SAMPLE* SampleByRank(const NET_CLIENT_REMOTE* p_remote, int p_rank)
{
	int index = (p_remote->m_sampleNext + 2 * NET_SAMPLE_COUNT - 1 - p_rank) % NET_SAMPLE_COUNT;
	return &p_remote->m_samples[index];
}

static void PoseFromActor(const NetActor& p_actor, NET_CLIENT_POSE* p_out)
{
	p_out->m_x = p_actor.m_x;
	p_out->m_y = p_actor.m_y;
	p_out->m_z = p_actor.m_z;
	p_out->m_dirLegs = p_actor.m_dirLegs;
	p_out->m_dirTorso = p_actor.m_dirTorso;
}

static unsigned char LerpDir(unsigned char p_from, unsigned char p_to, float p_alpha)
{
	signed char diff = (signed char) (unsigned char) (p_to - p_from);
	return (unsigned char) (p_from + (int) ((float) diff * p_alpha));
}

static void BlendPose(
	const NET_CLIENT_SAMPLE* p_older,
	const NET_CLIENT_SAMPLE* p_newer,
	unsigned int p_olderAge,
	unsigned int p_newerAge,
	NET_CLIENT_POSE* p_out
)
{
	const NetActor& from = p_older->m_actor;
	const NetActor& to = p_newer->m_actor;
	float dx = to.m_x - from.m_x;
	float dy = to.m_y - from.m_y;
	if (dx * dx + dy * dy > NET_SNAP_DISTANCE * NET_SNAP_DISTANCE) {
		PoseFromActor(from, p_out);
		return;
	}
	float alpha = (float) (p_olderAge - NET_INTERP_DELAY_MS) / (float) (p_olderAge - p_newerAge);
	p_out->m_x = from.m_x + dx * alpha;
	p_out->m_y = from.m_y + dy * alpha;
	p_out->m_z = from.m_z + (to.m_z - from.m_z) * alpha;
	p_out->m_dirLegs = LerpDir(from.m_dirLegs, to.m_dirLegs, alpha);
	p_out->m_dirTorso = LerpDir(from.m_dirTorso, to.m_dirTorso, alpha);
}

static void SamplePose(const NET_CLIENT_REMOTE* p_remote, unsigned int p_now, NET_CLIENT_POSE* p_out)
{
	const NET_CLIENT_SAMPLE* newer = 0;
	for (int rank = 0; rank < p_remote->m_sampleCount; ++rank) {
		const NET_CLIENT_SAMPLE* sample = SampleByRank(p_remote, rank);
		unsigned int age = NetElapsedMs(p_now, sample->m_time);
		if (age >= NET_INTERP_DELAY_MS) {
			if (newer) {
				BlendPose(sample, newer, age, NetElapsedMs(p_now, newer->m_time), p_out);
			}
			else {
				PoseFromActor(sample->m_actor, p_out);
			}
			return;
		}
		newer = sample;
	}
	PoseFromActor(newer->m_actor, p_out);
}

static void DestroyProxy(NET_CLIENT_REMOTE* p_remote)
{
	SPRITE* sprite = p_remote->m_sprite;
	if (!sprite) {
		return;
	}
	p_remote->m_sprite = 0;
	--g_netProxyCount;
	if (!Map) {
		return;
	}
	NET_CLIENT_FLAG_HOLD scriptsHeld(Map, NET_ENGINE_FLAG_SCRIPTS_HELD);
	sprite->ScalarDeletingDestructor(1);
}

static bool ArmyChangeDividesByZero(const SPRITE* p_sprite)
{
	const VID* vid = p_sprite->m_vid;
	if (!vid) {
		return true;
	}
	int army = (int) ((p_sprite->m_flag >> NET_SPRITE_ARMY_SHIFT) & NET_SPRITE_ARMY_MASK);
	return vid->m_maxHp[NET_AS1_PLAYER_ARMY] != vid->m_maxHp[army] && !vid->m_maxHp[army];
}

static bool ProxyArmyChangeSafe(const SPRITE* p_sprite)
{
	const VID* vid = p_sprite->m_vid;
	if (!vid || vid->m_sprClass != (unsigned int) NET_AS1_PLAYER_SPRCLASS) {
		return false;
	}
	for (const SPRITE* link = p_sprite; link; link = link->m_child) {
		if (ArmyChangeDividesByZero(link)) {
			return false;
		}
	}
	return true;
}

static void SpawnProxy(NET_CLIENT_REMOTE* p_remote, const NetActor& p_actor, unsigned int p_now)
{
	if (p_remote->m_spawnTried && NetElapsedMs(p_now, p_remote->m_spawnTime) < NET_RESPAWN_BACKOFF_MS) {
		return;
	}
	if (!Map->m_noTact || !Map->VidExists(NET_AS1_PLAYER_VID)) {
		return;
	}
	VID* vid = Map->Vid(NET_AS1_PLAYER_VID);
	if (vid->m_sprClass != (unsigned int) NET_AS1_PLAYER_SPRCLASS) {
		return;
	}
	p_remote->m_spawnTried = 1;
	p_remote->m_spawnTime = p_now;
	float x = ClampToSpan(p_actor.m_x, Map->m_w);
	float y = ClampToSpan(p_actor.m_y, Map->m_h);
	SPRITE* sprite = 0;
	{
		NET_CLIENT_FLAG_HOLD createScriptsHeld(Map, NET_ENGINE_FLAG_LOADING);
		sprite = Map->CreateSprite(vid, x, y, p_actor.m_z, ANGLE(p_actor.m_dirLegs), 0);
	}
	if (!sprite) {
		return;
	}
	p_remote->m_sprite = sprite;
	++g_netProxyCount;
	if (!ProxyArmyChangeSafe(sprite)) {
		DestroyProxy(p_remote);
		return;
	}
	sprite->ChangeArmy(NET_AS1_PLAYER_ARMY);
	sprite->Action(NET_AS1_ACT_SET_BEHAVE, NET_AS1_PROXY_BEHAVE, 0, 0);
	sprite->m_invulnerable = 1;
	p_remote->m_weapon = -1;
	if (!p_remote->m_hereLogged) {
		p_remote->m_hereLogged = 1;
		SetStatus("%s is here", p_remote->m_name);
	}
}

static void ProxyShoot(NET_CLIENT_REMOTE* p_remote, float p_x, float p_y, unsigned int p_now)
{
	SPRITE* sprite = p_remote->m_sprite;
	if (!sprite || sprite->m_ani >= NET_ANI_DEATH || !Map->m_groundz || !Map->m_tempGroundz) {
		return;
	}
	SPRITE* child = sprite->m_child;
	if (!child) {
		return;
	}
	float x = ClampToSpan(p_x, Map->m_w);
	float y = ClampToSpan(p_y, Map->m_h);
	sprite->Action(NET_AS1_ACT_ADD_AMMO, NET_AS1_SHOT_AMMO_GRANT, 0, 0);
	int weaponType = 0;
	VID* childVid = child->m_vid;
	if (childVid && childVid->m_weaponVid && childVid->m_weapon && childVid->m_exData) {
		weaponType = childVid->m_exData->m_unk0x00;
	}
	else if (sprite->m_vid->m_exData) {
		weaponType = sprite->m_vid->m_exData->m_unk0x00;
	}
	float groundZ;
	float targetY;
	if (weaponType == NET_AS1_WEAPON_ARC) {
		groundZ = Map->GetGroundZ_ff(x, y + NET_AS1_ARC_TARGET_RISE) + NET_AS1_ARC_TARGET_RISE;
		targetY = y + groundZ;
	}
	else {
		groundZ = Map->GetGroundZ_ff(x, y) + NET_AS1_MUZZLE_HEIGHT;
		targetY = y - NET_AS1_MUZZLE_HEIGHT + groundZ;
	}
	child->SetCommand(NET_AS1_CMD_ATTACK, x, targetY, groundZ);
	p_remote->m_lastShotTime = p_now;
}

static bool LocalMapShared()
{
	return g_net.m_state == NET_ONLINE && g_net.m_mapAnnounced && g_net.m_announcedPlayable && g_net.m_mapKey[0];
}

static void ApplyRemote(NET_CLIENT_REMOTE* p_remote, unsigned int p_now)
{
	if (!p_remote->m_sampleCount) {
		DestroyProxy(p_remote);
		return;
	}
	const NetActor newest = SampleByRank(p_remote, 0)->m_actor;
	unsigned int newestAge = NetElapsedMs(p_now, SampleByRank(p_remote, 0)->m_time);
	if (!p_remote->m_sameMap || !LocalMapShared() || newestAge >= NET_STALE_MS) {
		DestroyProxy(p_remote);
		ClearSamples(p_remote);
		return;
	}
	bool dead = newest.m_ani >= NET_ANI_DEATH;
	if (!p_remote->m_sprite) {
		if (dead) {
			return;
		}
		SpawnProxy(p_remote, newest, p_now);
		if (!p_remote->m_sprite) {
			return;
		}
	}
	SPRITE* sprite = p_remote->m_sprite;
	if (dead) {
		if (sprite->m_ani < NET_ANI_DEATH) {
			sprite->m_speed = 0.0f;
			sprite->SetCommand(NET_AS1_CMD_STOP, 0);
			sprite->ChangeAnimation(newest.m_ani);
		}
		return;
	}
	if (sprite->m_ani >= NET_ANI_DEATH) {
		DestroyProxy(p_remote);
		return;
	}

	NET_CLIENT_POSE pose;
	SamplePose(p_remote, p_now, &pose);
	float x = ClampToSpan(pose.m_x, Map->m_w);
	float y = ClampToSpan(pose.m_y, Map->m_h);
	float z = NetFloatWithin(pose.m_z, NET_HEIGHT_LIMIT) ? pose.m_z : 0.0f;
	sprite->ChangeCoor(x, y, z);
	sprite->ChangeDirection(ANGLE(pose.m_dirLegs));
	if (sprite->m_child) {
		sprite->m_child->ChangeDirection(ANGLE(pose.m_dirTorso));
	}
	float speed = newest.m_speed;
	sprite->m_speed = speed >= 0.0f && speed <= NET_SPEED_LIMIT ? speed : 0.0f;
	sprite->m_unk0x54 = newest.m_hp > 0 ? newest.m_hp : 1;
	int weapon = newest.m_weapon;
	if (p_remote->m_weapon != weapon && sprite->m_child && weapon < NET_WEAPON_SLOTS) {
		sprite->m_child->Action(NET_AS1_ACT_CHANGE_VID, NET_AS1_WEAPON_VID_BASE + weapon, 0, 0);
		p_remote->m_weapon = weapon;
	}
	SPRITE* child = sprite->m_child;
	if (child && (child->m_flag & NET_SPRITE_COMMAND_MASK) == (NET_AS1_CMD_ATTACK << NET_SPRITE_COMMAND_SHIFT) &&
		NetElapsedMs(p_now, p_remote->m_lastShotTime) > NET_FIRE_HOLD_MS) {
		child->SetCommand(NET_AS1_CMD_STOP, 0);
	}
}

static void SendPacket(const void* p_data, size_t p_size, int p_channel, bool p_reliable)
{
	if (!g_net.m_peer) {
		return;
	}
	ENetPacket* packet = enet_packet_create(p_data, p_size, p_reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
	if (!packet) {
		return;
	}
	if (enet_peer_send(g_net.m_peer, (enet_uint8) p_channel, packet) < 0) {
		enet_packet_destroy(packet);
	}
}

static void SendHello()
{
	NetMsgHello hello = {};
	hello.m_type = NET_MSG_HELLO;
	hello.m_version = NET_PROTOCOL_VERSION;
	memcpy(hello.m_name, g_net.m_name, NET_NAME_LEN);
	SendPacket(&hello, sizeof(hello), NET_CHANNEL_EVENTS, true);
}

static void SendMap(unsigned char p_flags, const char* p_key)
{
	NetMsgMap msg = {};
	msg.m_type = NET_MSG_MAP;
	msg.m_mapSeq = ++g_net.m_mapSeq;
	msg.m_mapFlags = p_flags;
	memcpy(msg.m_map, p_key, NET_MAP_LEN);
	SendPacket(&msg, sizeof(msg), NET_CHANNEL_EVENTS, true);
}

static void RefreshMapKey()
{
	const char* name = Map->m_mapName.m_str ? Map->m_mapName.m_str : "";
	NetNormalizeMapName(name, strlen(name), g_net.m_mapKey);
}

static void RecomputeSameMap(NET_CLIENT_REMOTE* p_remote)
{
	p_remote->m_sameMap = p_remote->m_map[0] && g_net.m_mapKey[0] && NetMapNameEqual(p_remote->m_map, g_net.m_mapKey);
}

static int CampaignLevelNumber(const char* p_key, size_t* p_directoryLength);
static LOGICSTACK* ScriptGlobal(const char* p_name);

static void AlignCampaignProgressWithMap()
{
	size_t directoryLength;
	int level = CampaignLevelNumber(g_net.m_mapKey, &directoryLength);
	bool baseCampaign = directoryLength == strlen("maps\\");
	int lastLevel = baseCampaign ? NET_AS1_CAMPAIGN_LAST_LEVEL : NET_AS1_ADDON_LAST_LEVEL;
	if (g_net.m_state != NET_ONLINE || !Registry || level < 1 || level > lastLevel) {
		return;
	}
	if (Registry->GetInt(STRING("LevelNumber"), 1) != level) {
		char text[NET_LEVEL_TEXT_LEN];
		snprintf(text, sizeof(text), "%d", level);
		Registry->SetString(STRING("LevelNumber"), STRING(text));
	}
	LOGICSTACK* scriptLevel = Map->m_noTact ? ScriptGlobal("LevelNumber") : 0;
	if (scriptLevel && scriptLevel->Int() >= 1 && scriptLevel->Int() != level) {
		*scriptLevel = LOGICSTACK(level);
	}
}

static void AnnounceMap()
{
	bool demoPlayback = (Map->m_flag & NET_ENGINE_FLAG_DEMO_PLAYBACK) != 0;
	bool playable = Map->m_gameplayMap && g_net.m_mapKey[0] && !demoPlayback;
	g_net.m_mapAnnounced = 1;
	g_net.m_announcedPlayable = playable;
	if (playable) {
		AlignCampaignProgressWithMap();
	}
	SendMap(playable ? NET_MAPF_PLAYABLE : 0, g_net.m_mapKey);
	if (g_net.m_relay) {
		NetRelay_SetStartMap(g_net.m_relay, playable ? g_net.m_mapKey : "");
	}
	SetStatus("map changed to %s", g_net.m_mapKey[0] ? g_net.m_mapKey : "(unshareable map)");
}

static void SendSnapshot(const SPRITE* p_flagman, unsigned int p_now)
{
	if (g_net.m_snapshotSent && NetElapsedMs(p_now, g_net.m_lastSendTime) < g_net.m_snapshotMs) {
		return;
	}
	NetMsgSnapshot msg = {};
	msg.m_type = NET_MSG_SNAPSHOT;
	msg.m_mapSeq = g_net.m_mapSeq;
	NetActor actor = {};
	actor.m_x = p_flagman->m_x;
	actor.m_y = p_flagman->m_y;
	actor.m_z = p_flagman->m_z;
	actor.m_speed = p_flagman->m_speed;
	int hp = p_flagman->m_unk0x54;
	actor.m_hp = (uint16_t) (hp < 0 ? 0 : (hp > 0xffff ? 0xffff : hp));
	actor.m_dirLegs = p_flagman->m_dir;
	actor.m_dirTorso = p_flagman->m_child ? p_flagman->m_child->m_dir : p_flagman->m_dir;
	const VID* link = p_flagman->m_vid ? p_flagman->m_vid->m_linkVid : 0;
	int slot = link ? link->m_idx - NET_AS1_WEAPON_VID_BASE : NET_AS1_DEFAULT_WEAPON;
	actor.m_weapon = (uint8_t) (slot < 0 || slot >= NET_WEAPON_SLOTS ? NET_AS1_DEFAULT_WEAPON : slot);
	int ani = p_flagman->m_ani;
	actor.m_ani = (uint8_t) (ani < 0 ? 0 : (ani > NET_ANI_MAX ? NET_ANI_MAX : ani));
	if (!NetActorValid(actor)) {
		return;
	}
	msg.m_actor = actor;
	g_net.m_snapshotSent = 1;
	g_net.m_lastSendTime = p_now;
	SendPacket(&msg, sizeof(msg), NET_CHANNEL_SNAPSHOTS, false);
	if (!g_net.m_playingLogged) {
		g_net.m_playingLogged = 1;
		SetStatus("playing as %s on %s", g_net.m_name, g_net.m_mapKey);
	}
}

static void ReleaseResolveJob(NET_CLIENT_RESOLVE_JOB* p_job)
{
	if (SDL_AtomicDecRef(&p_job->m_refs)) {
		SDL_free(p_job);
	}
}

static int SDLCALL ResolveThreadMain(void* p_data)
{
	NET_CLIENT_RESOLVE_JOB* job = (NET_CLIENT_RESOLVE_JOB*) p_data;
	ENetAddress address = {};
	int failed = enet_address_set_host_new(&address, job->m_host);
	job->m_address = address;
	SDL_SetAtomicInt(&job->m_state, failed ? NET_RESOLVE_FAILED : NET_RESOLVE_DONE);
	ReleaseResolveJob(job);
	return 0;
}

static int StartResolve(const char* p_host)
{
	NET_CLIENT_RESOLVE_JOB* job = (NET_CLIENT_RESOLVE_JOB*) SDL_calloc(1, sizeof(NET_CLIENT_RESOLVE_JOB));
	if (!job) {
		return 0;
	}
	SDL_SetAtomicInt(&job->m_refs, 2);
	SDL_SetAtomicInt(&job->m_state, NET_RESOLVE_RUNNING);
	CopyText(job->m_host, sizeof(job->m_host), p_host);
	SDL_Thread* thread = SDL_CreateThread(ResolveThreadMain, "NetResolve", job);
	if (!thread) {
		SDL_free(job);
		return 0;
	}
	SDL_DetachThread(thread);
	g_net.m_resolveJob = job;
	return 1;
}

static void CloseSession(const char* p_status)
{
	for (int i = 0; i < NET_MAX_PLAYERS; ++i) {
		DestroyProxy(&g_net.m_remote[i]);
	}
	memset(g_net.m_remote, 0, sizeof(g_net.m_remote));
	if (g_net.m_peer) {
		enet_peer_disconnect_now(g_net.m_peer, 0);
		g_net.m_peer = 0;
	}
	if (g_net.m_host) {
		enet_host_flush(g_net.m_host);
		enet_host_destroy(g_net.m_host);
		g_net.m_host = 0;
	}
	if (g_net.m_resolveJob) {
		ReleaseResolveJob(g_net.m_resolveJob);
		g_net.m_resolveJob = 0;
	}
	if (g_net.m_relay) {
		if (!Map) {
			NetRelay_SetLog(g_net.m_relay, 0, 0);
		}
		NetRelay_Destroy(g_net.m_relay);
		g_net.m_relay = 0;
	}
	g_net.m_state = NET_IDLE;
	g_net.m_playerId = -1;
	g_net.m_maxPlayers = 0;
	g_net.m_playerHosted = 0;
	g_net.m_snapshotSent = 0;
	g_net.m_shotSent = 0;
	g_net.m_mapSeq = 0;
	g_net.m_mapAnnounced = 0;
	g_net.m_announcedPlayable = 0;
	g_net.m_playingLogged = 0;
	g_net.m_pendingRequeued = 0;
	g_net.m_following = NET_FOLLOW_IDLE;
	g_net.m_invalidLogged = 0;
	g_net.m_port = 0;
	memset(g_net.m_mapKey, 0, sizeof(g_net.m_mapKey));
	memset(g_net.m_pendingStartMap, 0, sizeof(g_net.m_pendingStartMap));
	memset(g_net.m_address, 0, sizeof(g_net.m_address));
	memset(g_net.m_hostName, 0, sizeof(g_net.m_hostName));
	memset(g_net.m_name, 0, sizeof(g_net.m_name));
	if (p_status) {
		SetStatus("%s", p_status);
	}
}

static int EnsureEnet()
{
	if (g_net.m_enetReady) {
		return 1;
	}
	if (enet_initialize()) {
		SetStatus("network startup failed");
		return 0;
	}
	g_net.m_enetReady = 1;
	return 1;
}

static void ChooseName(const char* p_name)
{
	const char* name = p_name && p_name[0] ? p_name : PortableConfig_GetString("net", "PlayerName");
	if (!name || !name[0]) {
		name = "Player";
	}
	CopyText(g_net.m_name, sizeof(g_net.m_name), name);
	NetSanitizeName(g_net.m_name);
}

static int BeginConnect(const ENetAddress* p_address, unsigned int p_now)
{
	g_net.m_host = enet_host_create(0, 1, NET_CHANNEL_COUNT, 0, 0);
	if (!g_net.m_host) {
		SetStatus("cannot create network socket");
		return 0;
	}
	g_net.m_host->maximumPacketSize = NET_MAX_PACKET_BYTES;
	g_net.m_host->maximumWaitingData = NET_MAX_WAITING_BYTES;
	g_net.m_peer = enet_host_connect(g_net.m_host, p_address, NET_CHANNEL_COUNT, NET_CONNECT_MAGIC);
	if (!g_net.m_peer) {
		SetStatus("cannot start connection");
		return 0;
	}
	enet_peer_timeout(g_net.m_peer, NET_TIMEOUT_LIMIT, NET_TIMEOUT_MIN_MS, NET_TIMEOUT_MAX_MS);
	EnterState(NET_CONNECTING, p_now);
	return 1;
}

static bool HostIsDottedQuad(const char* p_host)
{
	const char* c = p_host;
	for (int octet = 0; octet < NET_IPV4_OCTETS; ++octet) {
		int digits = 0;
		int value = 0;
		while (*c >= '0' && *c <= '9' && digits < NET_IPV4_OCTET_DIGITS) {
			value = value * 10 + (*c - '0');
			++digits;
			++c;
		}
		if (!digits || value > NET_IPV4_OCTET_MAX) {
			return false;
		}
		bool last = octet == NET_IPV4_OCTETS - 1;
		if (*c != (last ? 0 : '.')) {
			return false;
		}
		if (!last) {
			++c;
		}
	}
	return true;
}

static int ClassifyHost(const char* p_host)
{
	if (strchr(p_host, ':')) {
		return NET_HOST_LITERAL;
	}
	for (const char* c = p_host; *c; ++c) {
		if ((*c < '0' || *c > '9') && *c != '.') {
			return NET_HOST_NAME;
		}
	}
	return HostIsDottedQuad(p_host) ? NET_HOST_LITERAL : NET_HOST_MALFORMED;
}

static void QueueStartMap()
{
	Map->m_scriptName = STRING(g_net.m_pendingStartMap);
	Map->m_flag |= NET_ENGINE_FLAG_LOAD_PENDING;
}

static void ClearPendingStartMap()
{
	memset(g_net.m_pendingStartMap, 0, sizeof(g_net.m_pendingStartMap));
	g_net.m_pendingRequeued = 0;
}

static bool StartMapLoadable(const char* p_name)
{
	if (!FileData_FileExists(p_name)) {
		return false;
	}
	RESOURCE probe;
	return probe.OpenForRead(STRING(p_name), NET_ENGINE_MAP_CONTAINER) == 0;
}

static void ConsiderStartMap(const char* p_raw)
{
	if (!p_raw[0]) {
		return;
	}
	char offer[NET_MAP_LEN];
	if (!NetNormalizeMapName(p_raw, NET_MAP_LEN, offer) || !NetMapNameIsGameplay(offer) || !StartMapLoadable(offer)) {
		LogLine("server start map unavailable, staying here");
		return;
	}
	if (NetMapNameEqual(offer, g_net.m_mapKey) || Map->m_gameplayMap || NetMapNameIsShop(g_net.m_mapKey)) {
		return;
	}
	memcpy(g_net.m_pendingStartMap, offer, NET_MAP_LEN);
	g_net.m_pendingRequeued = 0;
	QueueStartMap();
}

static void ResolvePendingStartMap()
{
	if (!g_net.m_pendingStartMap[0]) {
		return;
	}
	if (NetMapNameEqual(g_net.m_pendingStartMap, g_net.m_mapKey) || Map->m_gameplayMap || g_net.m_pendingRequeued) {
		ClearPendingStartMap();
		return;
	}
	g_net.m_pendingRequeued = 1;
	QueueStartMap();
}

static bool LocalOnGameplayMap()
{
	return g_net.m_state == NET_ONLINE && g_net.m_announcedPlayable && NetMapNameIsGameplay(g_net.m_mapKey) &&
		   !(Map->m_flag & NET_ENGINE_FLAG_DEMO_PLAYBACK);
}

static int CampaignLevelNumber(const char* p_key, size_t* p_directoryLength)
{
	const char* leaf = NetMapLeaf(p_key);
	*p_directoryLength = (size_t) (leaf - p_key);
	bool numbered = !strncmp(leaf, "level_", 6) && leaf[6] >= '0' && leaf[6] <= '9' && leaf[7] >= '0' &&
					leaf[7] <= '9' && !strcmp(leaf + 8, ".map");
	return numbered ? (leaf[6] - '0') * 10 + (leaf[7] - '0') : -1;
}

static bool SameCampaignDirectory(const char* p_a, const char* p_b)
{
	size_t lengthA;
	size_t lengthB;
	CampaignLevelNumber(p_a, &lengthA);
	CampaignLevelNumber(p_b, &lengthB);
	return lengthA == lengthB && !memcmp(p_a, p_b, lengthA);
}

static bool LocalOnCampaignLevel()
{
	size_t directoryLength;
	return LocalOnGameplayMap() && CampaignLevelNumber(g_net.m_mapKey, &directoryLength) >= 1;
}

static bool IsLevelAfterMine(const char* p_key)
{
	size_t directoryLength;
	int mine = CampaignLevelNumber(g_net.m_mapKey, &directoryLength);
	int theirs = CampaignLevelNumber(p_key, &directoryLength);
	return mine >= 1 && theirs == mine + 1 && SameCampaignDirectory(p_key, g_net.m_mapKey);
}

static void FollowTeammateWhoMovedOn(const NET_CLIENT_REMOTE* p_remote)
{
	if (g_net.m_following != NET_FOLLOW_IDLE || !LocalOnCampaignLevel()) {
		return;
	}
	bool leftMyLevel = p_remote->m_lastNamedPlayable && NetMapNameEqual(p_remote->m_lastNamedMap, g_net.m_mapKey);
	bool movedOn = NetMapNameIsShop(p_remote->m_map) || IsLevelAfterMine(p_remote->m_map);
	if (leftMyLevel && movedOn) {
		g_net.m_following = NET_FOLLOW_REQUESTED;
		g_net.m_followSecondsShown = -1;
		g_net.m_followPauseNoticed = 0;
		memcpy(g_net.m_followLeader, p_remote->m_name, NET_NAME_LEN);
	}
}

static void ApplyPresence(
	NET_CLIENT_REMOTE* p_remote,
	unsigned char p_mapSeq,
	unsigned char p_mapFlags,
	const char* p_map
)
{
	DestroyProxy(p_remote);
	ClearSamples(p_remote);
	p_remote->m_mapSeq = p_mapSeq;
	p_remote->m_mapFlags = p_mapFlags;
	p_remote->m_hereLogged = 0;
	NetNormalizeMapName(p_map, NET_MAP_LEN, p_remote->m_map);
	RecomputeSameMap(p_remote);
	if (p_remote->m_map[0]) {
		FollowTeammateWhoMovedOn(p_remote);
		memcpy(p_remote->m_lastNamedMap, p_remote->m_map, NET_MAP_LEN);
		p_remote->m_lastNamedPlayable = (p_remote->m_mapFlags & NET_MAPF_PLAYABLE) != 0;
	}
	if (p_remote->m_map[0] && !p_remote->m_sameMap) {
		unsigned int now = NetNow();
		if (!p_remote->m_elsewhereLogged ||
			NetElapsedMs(now, p_remote->m_elsewhereLogTime) >= NET_ELSEWHERE_LOG_INTERVAL_MS) {
			p_remote->m_elsewhereLogged = 1;
			p_remote->m_elsewhereLogTime = now;
			SetStatus("%s is on another map", p_remote->m_name);
		}
	}
}

static void HandleWelcome(const NetMsgWelcome& p_msg, unsigned int p_now)
{
	if (g_net.m_state != NET_HANDSHAKE) {
		return;
	}
	if (p_msg.m_playerId >= NET_MAX_PLAYERS) {
		NoteInvalid(NET_INVALID_WELCOME);
		return;
	}
	g_net.m_playerId = p_msg.m_playerId;
	int maxPlayers = p_msg.m_maxPlayers;
	if (maxPlayers <= g_net.m_playerId || maxPlayers > NET_MAX_PLAYERS) {
		maxPlayers = NET_MAX_PLAYERS;
	}
	g_net.m_maxPlayers = maxPlayers;
	g_net.m_snapshotMs = p_msg.m_snapshotMs;
	if (g_net.m_snapshotMs < NET_SNAPSHOT_MIN_MS || g_net.m_snapshotMs > NET_SNAPSHOT_MAX_MS) {
		g_net.m_snapshotMs = NET_DEFAULT_SNAPSHOT_MS;
	}
	g_net.m_playerHosted = (p_msg.m_flags & NET_WELCOMEF_PLAYER_HOSTED) != 0;
	g_net.m_mapSeq = 0;
	g_net.m_mapAnnounced = 0;
	g_net.m_announcedPlayable = 0;
	g_net.m_playingLogged = 0;
	g_net.m_snapshotSent = 0;
	EnterState(NET_ONLINE, p_now);
	RefreshMapKey();
	SetStatus("joined as %s (player %d of %d)", g_net.m_name, g_net.m_playerId + 1, g_net.m_maxPlayers);
	ConsiderStartMap(p_msg.m_map);
}

static NET_CLIENT_REMOTE* RemoteForMessage(unsigned int p_playerId)
{
	if (g_net.m_state != NET_ONLINE) {
		return 0;
	}
	if (p_playerId >= NET_MAX_PLAYERS) {
		NoteInvalid(NET_INVALID_PLAYER_ID);
		return 0;
	}
	if ((int) p_playerId == g_net.m_playerId) {
		return 0;
	}
	return &g_net.m_remote[p_playerId];
}

static void HandleJoined(const NetMsgJoined& p_msg)
{
	NET_CLIENT_REMOTE* remote = RemoteForMessage(p_msg.m_playerId);
	if (!remote) {
		return;
	}
	bool known = remote->m_used != 0;
	DestroyProxy(remote);
	memset(remote, 0, sizeof(NET_CLIENT_REMOTE));
	remote->m_used = 1;
	remote->m_weapon = -1;
	memcpy(remote->m_name, p_msg.m_name, NET_NAME_LEN);
	NetSanitizeName(remote->m_name);
	if (!known) {
		SetStatus("%s joined", remote->m_name);
	}
	ApplyPresence(remote, p_msg.m_mapSeq, p_msg.m_mapFlags, p_msg.m_map);
}

static void HandlePresence(const NetMsgPresence& p_msg)
{
	NET_CLIENT_REMOTE* remote = RemoteForMessage(p_msg.m_playerId);
	if (!remote || !remote->m_used) {
		return;
	}
	ApplyPresence(remote, p_msg.m_mapSeq, p_msg.m_mapFlags, p_msg.m_map);
}

static void HandleLeft(const NetMsgLeft& p_msg)
{
	NET_CLIENT_REMOTE* remote = RemoteForMessage(p_msg.m_playerId);
	if (!remote || !remote->m_used) {
		return;
	}
	SetStatus("%s left", remote->m_name);
	DestroyProxy(remote);
	memset(remote, 0, sizeof(NET_CLIENT_REMOTE));
}

static bool RemoteSharesMap(const NET_CLIENT_REMOTE* p_remote, unsigned char p_mapSeq)
{
	return p_remote->m_used && p_remote->m_sameMap && (p_remote->m_mapFlags & NET_MAPF_PLAYABLE) &&
		   p_remote->m_mapSeq == p_mapSeq;
}

static void HandleShoot(const NetMsgShoot& p_msg, unsigned int p_now)
{
	NET_CLIENT_REMOTE* remote = RemoteForMessage(p_msg.m_playerId);
	if (!remote || !LocalMapShared() || !RemoteSharesMap(remote, p_msg.m_mapSeq)) {
		return;
	}
	if (!NetShootValid(p_msg)) {
		NoteInvalid(NET_INVALID_SHOT);
		return;
	}
	ProxyShoot(remote, p_msg.m_x, p_msg.m_y, p_now);
}

static void HandleSnapshots(const ENetPacket* p_packet, unsigned int p_now)
{
	size_t length = p_packet->dataLength;
	if (length < offsetof(NetMsgSnapshots, m_entries)) {
		NoteInvalid(NET_INVALID_PACKET);
		return;
	}
	unsigned int count = p_packet->data[offsetof(NetMsgSnapshots, m_count)];
	if (count > NET_MAX_PLAYERS || length != NetMsgSnapshotsSize((int) count)) {
		NoteInvalid(NET_INVALID_PACKET);
		return;
	}
	if (!LocalMapShared()) {
		return;
	}
	NetMsgSnapshots msg = {};
	memcpy(&msg, p_packet->data, length);
	for (unsigned int i = 0; i < count; ++i) {
		const NetSnapshotEntry& entry = msg.m_entries[i];
		NET_CLIENT_REMOTE* remote = RemoteForMessage(entry.m_playerId);
		if (!remote || !RemoteSharesMap(remote, entry.m_mapSeq)) {
			continue;
		}
		if (!NetActorValid(entry.m_actor)) {
			NoteInvalid(NET_INVALID_ACTOR);
			continue;
		}
		PushSample(remote, entry.m_actor, p_now);
	}
}

template <class T>
static bool ReadMessage(const ENetPacket* p_packet, T* p_out)
{
	if (p_packet->dataLength != sizeof(T)) {
		NoteInvalid(NET_INVALID_PACKET);
		return false;
	}
	memcpy(p_out, p_packet->data, sizeof(T));
	return true;
}

static void HandleMessage(const ENetPacket* p_packet, unsigned int p_now)
{
	if (!p_packet->dataLength) {
		NoteInvalid(NET_INVALID_PACKET);
		return;
	}
	switch (p_packet->data[0]) {
	case NET_MSG_WELCOME: {
		NetMsgWelcome msg;
		if (ReadMessage(p_packet, &msg)) {
			HandleWelcome(msg, p_now);
		}
		break;
	}
	case NET_MSG_JOINED: {
		NetMsgJoined msg;
		if (ReadMessage(p_packet, &msg)) {
			HandleJoined(msg);
		}
		break;
	}
	case NET_MSG_PRESENCE: {
		NetMsgPresence msg;
		if (ReadMessage(p_packet, &msg)) {
			HandlePresence(msg);
		}
		break;
	}
	case NET_MSG_LEFT: {
		NetMsgLeft msg;
		if (ReadMessage(p_packet, &msg)) {
			HandleLeft(msg);
		}
		break;
	}
	case NET_MSG_SHOOT: {
		NetMsgShoot msg;
		if (ReadMessage(p_packet, &msg)) {
			HandleShoot(msg, p_now);
		}
		break;
	}
	case NET_MSG_SNAPSHOTS:
		HandleSnapshots(p_packet, p_now);
		break;
	default:
		NoteInvalid(NET_INVALID_PACKET);
		break;
	}
}

static void HandleDisconnected(unsigned int p_data, bool p_timedOut)
{
	g_net.m_peer = 0;
	switch (p_data) {
	case NET_DENY_VERSION:
		CloseSession("refused: server runs an incompatible version");
		return;
	case NET_DENY_FULL:
		CloseSession("refused: server is full");
		return;
	case NET_DENY_HELLO_TIMEOUT:
		CloseSession("refused: handshake timed out");
		return;
	case NET_DENY_PROTOCOL:
		CloseSession("refused: protocol violation");
		return;
	case NET_DENY_SHUTDOWN:
		CloseSession(g_net.m_playerHosted ? "disconnected: host closed the game" : "disconnected: server shut down");
		return;
	default:
		break;
	}
	if (g_net.m_state == NET_CONNECTING) {
		CloseSession("disconnected: connection failed");
	}
	else if (p_timedOut) {
		CloseSession("disconnected: connection timed out");
	}
	else {
		CloseSession("disconnected: disconnected from server");
	}
}

static void PollResolve(unsigned int p_now)
{
	NET_CLIENT_RESOLVE_JOB* job = g_net.m_resolveJob;
	if (!job) {
		CloseSession("disconnected: connection failed");
		return;
	}
	int state = SDL_GetAtomicInt(&job->m_state);
	if (state == NET_RESOLVE_RUNNING) {
		if (NetElapsedMs(p_now, g_net.m_stateTime) >= NET_RESOLVE_TIMEOUT_MS) {
			SetStatus("name lookup timed out");
			CloseSession(0);
		}
		return;
	}
	ENetAddress address = job->m_address;
	g_net.m_resolveJob = 0;
	ReleaseResolveJob(job);
	if (state == NET_RESOLVE_FAILED) {
		SetStatus("cannot resolve %s", g_net.m_hostName);
		CloseSession(0);
		return;
	}
	address.port = (enet_uint16) g_net.m_port;
	if (!BeginConnect(&address, p_now)) {
		CloseSession(0);
	}
}

static void DrainClientEvents(unsigned int p_now)
{
	for (int handled = 0; handled < NET_EVENTS_PER_PUMP && g_net.m_host; ++handled) {
		ENetEvent event;
		if (enet_host_service(g_net.m_host, &event, 0) <= 0) {
			return;
		}
		switch (event.type) {
		case ENET_EVENT_TYPE_CONNECT:
			if (g_net.m_state == NET_CONNECTING) {
				SendHello();
				EnterState(NET_HANDSHAKE, p_now);
				SetStatus("connected, waiting for server");
			}
			break;
		case ENET_EVENT_TYPE_RECEIVE:
			HandleMessage(event.packet, p_now);
			enet_packet_destroy(event.packet);
			break;
		case ENET_EVENT_TYPE_DISCONNECT:
			HandleDisconnected(event.data, false);
			break;
		case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
			HandleDisconnected(event.data, true);
			break;
		default:
			break;
		}
	}
}

static void CheckStateTimeout(unsigned int p_now)
{
	unsigned int limit = 0;
	if (g_net.m_state == NET_CONNECTING) {
		limit = NET_CONNECT_TIMEOUT_MS;
	}
	else if (g_net.m_state == NET_HANDSHAKE) {
		limit = NET_HANDSHAKE_TIMEOUT_MS;
	}
	if (limit && NetElapsedMs(p_now, g_net.m_stateTime) >= limit) {
		CloseSession("disconnected: connection timed out");
	}
}

static int FindScriptFunction(const char* p_name, int p_parameters)
{
	const LOGIC& logic = Map->m_logic;
	for (int i = 0; i < logic.m_variables.m_n; ++i) {
		const auto& entry = logic.m_variables.m_data[i];
		if (entry.m_var.m_flag == NET_SCRIPT_ENTRY_IS_FUNCTION && entry.m_var.m_extra == p_parameters &&
			entry.m_var.m_a >= 0 && entry.m_name.m_str && !strcmp(entry.m_name.m_str, p_name)) {
			return i;
		}
	}
	return -1;
}

static LOGICSTACK* ScriptGlobal(const char* p_name)
{
	LOGIC& logic = Map->m_logic;
	int index = logic.m_variables.Location(STRING(p_name));
	if (index < 0) {
		return 0;
	}
	const auto& entry = logic.m_variables.m_data[index].m_var;
	if (entry.m_flag == NET_SCRIPT_ENTRY_IS_FUNCTION || entry.m_a < 0 || entry.m_a >= logic.m_stack.m_n) {
		return 0;
	}
	return &((LOGICSTACK*) logic.m_stack.m_data)[entry.m_a];
}

static int ScriptGlobalInt(const char* p_name, int p_missing)
{
	LOGICSTACK* value = ScriptGlobal(p_name);
	return value ? value->Int() : p_missing;
}

static void ShowFollowCountdown()
{
	int leavesAt = ScriptGlobalInt("WinTime", 0);
	int remaining = leavesAt > (int) CurrentTime ? (leavesAt - (int) CurrentTime + 999) / 1000 : 0;
	if (remaining > 0 && remaining != g_net.m_followSecondsShown) {
		g_net.m_followSecondsShown = remaining;
		SetStatus("%s finished the level - leaving in %d", g_net.m_followLeader, remaining);
	}
}

static void BeginFollowCountdown()
{
	const unsigned int busy = NET_ENGINE_FLAG_SCRIPTS_HELD | NET_ENGINE_FLAG_LOADING | NET_ENGINE_FLAG_LOAD_PENDING;
	if (!Map->m_noTact || (Map->m_flag & busy)) {
		return;
	}
	if (Map->m_flag & NET_ENGINE_FLAG_PAUSED) {
		if (!g_net.m_followPauseNoticed) {
			g_net.m_followPauseNoticed = 1;
			SetStatus("%s finished the level - you follow when you resume", g_net.m_followLeader);
		}
		return;
	}
	int gameState = ScriptGlobalInt("GameState", NET_AS1_GAME_LOOSE);
	bool campaignScript = ScriptGlobalInt("LevelNumber", 0) >= 1 && ScriptGlobal("WinTime");
	int levelWon = FindScriptFunction("Win", 0);
	if (!campaignScript || gameState == NET_AS1_GAME_LOOSE || levelWon < 0) {
		g_net.m_following = NET_FOLLOW_IDLE;
		return;
	}
	if (gameState != NET_AS1_GAME_WIN) {
		Map->ScriptRun(levelWon, 0, 0, 0);
	}
	LOGICSTACK* leavesAt = ScriptGlobal("WinTime");
	if (!leavesAt) {
		g_net.m_following = NET_FOLLOW_IDLE;
		return;
	}
	int latest = (int) CurrentTime + NET_FOLLOW_GRACE_MS;
	int armed = leavesAt->Int();
	if (!armed || armed > latest) {
		*leavesAt = LOGICSTACK(latest);
	}
	g_net.m_following = NET_FOLLOW_COUNTDOWN;
}

static void FollowStep()
{
	if (g_net.m_following == NET_FOLLOW_IDLE) {
		return;
	}
	if (!LocalOnCampaignLevel()) {
		g_net.m_following = NET_FOLLOW_IDLE;
		return;
	}
	if (g_net.m_following == NET_FOLLOW_REQUESTED) {
		BeginFollowCountdown();
	}
	if (g_net.m_following == NET_FOLLOW_COUNTDOWN) {
		ShowFollowCountdown();
	}
}

static void PumpSession()
{
	if (g_net.m_state == NET_IDLE) {
		return;
	}
	unsigned int now = NetNow();
	if (g_net.m_state == NET_RESOLVING) {
		PollResolve(now);
	}
	if (g_net.m_state == NET_ONLINE && !g_net.m_mapAnnounced) {
		AnnounceMap();
	}
	DrainClientEvents(now);
	if (g_net.m_state != NET_ONLINE) {
		CheckStateTimeout(now);
		return;
	}
	if (!g_net.m_mapAnnounced) {
		AnnounceMap();
	}
	SPRITE* flagman = LocalFlagman();
	if (flagman && g_net.m_mapKey[0] && g_net.m_announcedPlayable && !(Map->m_flag & NET_ENGINE_FLAG_DEMO_PLAYBACK)) {
		SendSnapshot(flagman, now);
	}
	for (int i = 0; i < NET_MAX_PLAYERS; ++i) {
		if (g_net.m_remote[i].m_used) {
			ApplyRemote(&g_net.m_remote[i], now);
		}
	}
	FollowStep();
	if (g_net.m_host) {
		enet_host_flush(g_net.m_host);
	}
}

static void ConsumeAutoRequest()
{
	int kind = g_net.m_autoKind;
	if (kind == NET_AUTO_NONE) {
		return;
	}
	g_net.m_autoKind = NET_AUTO_NONE;
	if (kind == NET_AUTO_HOST) {
		Net_Host(g_net.m_autoPort, g_net.m_autoName);
		return;
	}
	Net_Connect(g_net.m_autoAddress, g_net.m_autoName);
}

static void RelayLogSink(void*, const char* p_line)
{
	if (!Map) {
		return;
	}
	MYERROR::Log(::Error, "NET: relay: %s", p_line);
}

#ifdef OPENGROMADA_NET_TESTHOOKS

static void ParseHookSchedule(const char* p_schedule)
{
	const char* cursor = p_schedule;
	while (cursor && *cursor && g_hooks.m_count < NET_HOOK_MAX) {
		char* end = 0;
		unsigned long when = strtoul(cursor, &end, 10);
		if (end == cursor || *end != ':') {
			return;
		}
		cursor = end + 1;
		const char* comma = strchr(cursor, ',');
		size_t tokenLength = comma ? (size_t) (comma - cursor) : strlen(cursor);
		const char* equals = (const char*) memchr(cursor, '=', tokenLength);
		size_t commandLength = equals ? (size_t) (equals - cursor) : tokenLength;
		size_t argumentLength = equals ? tokenLength - commandLength - 1 : 0;
		if (commandLength >= NET_HOOK_COMMAND_LEN) {
			commandLength = NET_HOOK_COMMAND_LEN - 1;
		}
		if (argumentLength >= NET_HOOK_ARGUMENT_LEN) {
			argumentLength = NET_HOOK_ARGUMENT_LEN - 1;
		}
		NET_CLIENT_HOOK_ENTRY* entry = &g_hooks.m_entries[g_hooks.m_count++];
		memset(entry, 0, sizeof(NET_CLIENT_HOOK_ENTRY));
		entry->m_when = (unsigned int) when;
		memcpy(entry->m_command, cursor, commandLength);
		if (equals) {
			memcpy(entry->m_argument, equals + 1, argumentLength);
		}
		cursor = comma ? comma + 1 : 0;
	}
}

static void HookSetTickBias(unsigned int p_bias)
{
	unsigned int delta = p_bias - g_hooks.m_tickBias;
	g_hooks.m_tickBias = p_bias;
	g_net.m_stateTime += delta;
	g_net.m_lastSendTime += delta;
	g_net.m_lastShotSent += delta;
	for (int i = 0; i < NET_MAX_PLAYERS; ++i) {
		NET_CLIENT_REMOTE* remote = &g_net.m_remote[i];
		remote->m_lastShotTime += delta;
		remote->m_spawnTime += delta;
		remote->m_elsewhereLogTime += delta;
		for (int k = 0; k < NET_SAMPLE_COUNT; ++k) {
			remote->m_samples[k].m_time += delta;
		}
	}
}

static void HookCensus()
{
	int men = 0;
	for (int layer = 0; layer < MAP::LayerCount(); ++layer) {
		int iterator;
		for (SPRITE* sprite = Map->FirstSprite(layer, &iterator); sprite; sprite = Map->NextSprite(layer, &iterator)) {
			if (sprite->m_vid && sprite->m_vid->m_idx == NET_AS1_PLAYER_VID && sprite->m_ani < NET_ANI_DEATH) {
				++men;
			}
		}
	}
	const SPRITE* flagman = LocalFlagman();
	int x = flagman ? (int) flagman->m_x : -1;
	int y = flagman ? (int) flagman->m_y : -1;
	int hp = flagman ? (int) flagman->m_unk0x54 : -1;
	MYERROR::Log(
		::Error,
		"NET: census men=%d proxies=%d flagman=%d,%d hp=%d tact=%d",
		men,
		g_netProxyCount,
		x,
		y,
		hp,
		(int) Map->m_noTact
	);
	for (int i = 0; i < NET_MAX_PLAYERS; ++i) {
		const SPRITE* proxy = g_net.m_remote[i].m_sprite;
		if (proxy) {
			MYERROR::Log(
				::Error,
				"NET: census proxy %s at=%d,%d army=%d ani=%d",
				g_net.m_remote[i].m_name,
				(int) proxy->m_x,
				(int) proxy->m_y,
				(int) ((proxy->m_flag >> NET_SPRITE_ARMY_SHIFT) & NET_SPRITE_ARMY_MASK),
				(int) proxy->m_ani
			);
		}
	}
}

static int HookParsePoint(const char* p_text, float* p_x, float* p_y)
{
	char* end = 0;
	long x = strtol(p_text, &end, 10);
	if (!end || *end != 'x') {
		return 0;
	}
	long y = strtol(end + 1, &end, 10);
	if (!end || *end) {
		return 0;
	}
	*p_x = (float) x;
	*p_y = (float) y;
	return 1;
}

static void HookPushKey(const char* p_keycode)
{
	SDL_Event event = {};
	event.type = SDL_EVENT_KEY_DOWN;
	event.key.key = (SDL_Keycode) strtoul(p_keycode, 0, 0);
	event.key.scancode = SDL_GetScancodeFromKey(event.key.key, 0);
	event.key.down = true;
	SDL_PushEvent(&event);
	event.type = SDL_EVENT_KEY_UP;
	event.key.down = false;
	SDL_PushEvent(&event);
}

static void HookPushText(const char* p_text)
{
	SDL_Event event = {};
	event.type = SDL_EVENT_TEXT_INPUT;
	event.text.text = p_text;
	SDL_PushEvent(&event);
}

static void RunHookCommand(const NET_CLIENT_HOOK_ENTRY* p_entry)
{
	const char* command = p_entry->m_command;
	const char* argument = p_entry->m_argument;
	if (!strcmp(command, "load")) {
		Map->m_flag |= NET_ENGINE_FLAG_LOAD_PENDING;
		Map->m_scriptName = STRING(argument);
	}
	else if (!strcmp(command, "save")) {
		if (Map->SaveMap(STRING(argument))) {
			MYERROR::Log(::Error, "NETAUTO: save failed");
		}
	}
	else if (!strcmp(command, "census")) {
		HookCensus();
	}
	else if (!strcmp(command, "kill")) {
		SPRITE* flagman = LocalFlagman();
		if (flagman) {
			flagman->Action(NET_AS1_ACT_DAMAGE, NET_HOOK_KILL_DAMAGE, 0, 0);
		}
	}
	else if (!strcmp(command, "goto") || !strcmp(command, "fire")) {
		SPRITE* flagman = LocalFlagman();
		float x;
		float y;
		if (flagman && HookParsePoint(argument, &x, &y)) {
			if (command[0] == 'g') {
				flagman->ChangeCoor(x, y, flagman->m_z);
			}
			else {
				flagman->Action(NET_AS1_ACT_ATTACK, (decomp_intptr) x, (decomp_intptr) y, 0);
			}
		}
	}
	else if (!strcmp(command, "focus")) {
		SDL_Event event = {};
		event.type = atoi(argument) ? SDL_EVENT_WINDOW_FOCUS_GAINED : SDL_EVENT_WINDOW_FOCUS_LOST;
		SDL_PushEvent(&event);
	}
	else if (!strcmp(command, "key")) {
		HookPushKey(argument);
	}
	else if (!strcmp(command, "text")) {
		HookPushText(argument);
	}
	else if (!strcmp(command, "tickbias")) {
		HookSetTickBias((unsigned int) strtoul(argument, 0, 0));
	}
	else if (!strcmp(command, "quit")) {
		Map->m_quit = 1;
	}
}

static void RunTestHooks()
{
	if (!g_hooks.m_parsed) {
		g_hooks.m_parsed = 1;
		ParseHookSchedule(SDL_getenv("ALIEN_NET_AUTO"));
	}
	if (!g_hooks.m_count) {
		return;
	}
	unsigned int ticks = Platform_Ticks();
	if (!g_hooks.m_started) {
		g_hooks.m_started = 1;
		g_hooks.m_startTime = ticks;
	}
	unsigned int elapsed = ticks - g_hooks.m_startTime;
	for (int i = 0; i < g_hooks.m_count; ++i) {
		NET_CLIENT_HOOK_ENTRY* entry = &g_hooks.m_entries[i];
		if (entry->m_fired || elapsed < entry->m_when) {
			continue;
		}
		entry->m_fired = 1;
		MYERROR::Log(::Error, "NETAUTO: %s at %ums", entry->m_command, elapsed);
		RunHookCommand(entry);
	}
}

#endif

void Net_Init()
{
}

void Net_Shutdown()
{
	CloseSession(0);
	if (g_net.m_enetReady) {
		enet_deinitialize();
		g_net.m_enetReady = 0;
	}
}

static void KeepSessionRunningWhileUnfocused()
{
	static int held;
	bool online = false;
	for (int i = 0; g_net.m_state == NET_ONLINE && i < NET_MAX_PLAYERS; ++i) {
		online = online || g_net.m_remote[i].m_used;
	}
	if (online && !held && !(Map->m_unk0x22c0_d & NET_ENGINE_RUN_WHILE_INACTIVE)) {
		Map->m_unk0x22c0_d |= NET_ENGINE_RUN_WHILE_INACTIVE;
		held = 1;
	}
	else if (!online && held) {
		Map->m_unk0x22c0_d &= ~(unsigned int) NET_ENGINE_RUN_WHILE_INACTIVE;
		held = 0;
	}
}

void Net_Pump()
{
	if (!Map) {
		return;
	}
	ConsumeAutoRequest();
	if (!NetSupported()) {
		return;
	}
#ifdef OPENGROMADA_NET_TESTHOOKS
	RunTestHooks();
#endif
	if (Map->m_logic.m_runtimeFault) {
		return;
	}
	PumpSession();
	if (g_net.m_relay) {
		NetRelay_Service(g_net.m_relay, 0);
	}
	KeepSessionRunningWhileUnfocused();
	NetUi_Pump();
}

int Net_Available()
{
	return NetSupported();
}

int Net_Connect(const char* p_address, const char* p_name)
{
	if (!NetSupported()) {
		RefuseUnsupported();
		return 0;
	}
	if (!Map) {
		return 0;
	}
	char address[NET_ADDRESS_LEN];
	char name[NET_NAME_LEN];
	char host[NET_ADDRESS_LEN];
	int port = NET_DEFAULT_PORT;
	bool fits = p_address && strlen(p_address) < sizeof(address);
	CopyText(address, sizeof(address), fits ? p_address : "");
	CopyText(name, sizeof(name), p_name);
	CloseSession(0);
	if (!fits || !NetParseAddress(address, host, sizeof(host), &port)) {
		SetStatus("invalid server address");
		return 0;
	}
	int hostKind = ClassifyHost(host);
	if (hostKind == NET_HOST_MALFORMED) {
		SetStatus("invalid server address");
		return 0;
	}
	if (!EnsureEnet()) {
		return 0;
	}
	ENetAddress literal = {};
	if (hostKind == NET_HOST_LITERAL && enet_address_set_host_ip_new(&literal, host)) {
		SetStatus("invalid server address");
		return 0;
	}
	ChooseName(name);
	CopyText(g_net.m_address, sizeof(g_net.m_address), address);
	CopyText(g_net.m_hostName, sizeof(g_net.m_hostName), host);
	g_net.m_port = port;
	if (strchr(host, ':')) {
		SetStatus("connecting to [%s]:%d", host, port);
	}
	else {
		SetStatus("connecting to %s:%d", host, port);
	}
	if (hostKind == NET_HOST_LITERAL) {
		literal.port = (enet_uint16) port;
		if (!BeginConnect(&literal, NetNow())) {
			CloseSession(0);
			return 0;
		}
		return 1;
	}
	if (!StartResolve(host)) {
		SetStatus("cannot start name lookup");
		CloseSession(0);
		return 0;
	}
	EnterState(NET_RESOLVING, NetNow());
	return 1;
}

int Net_Host(int p_port, const char* p_name)
{
	if (!NetSupported()) {
		RefuseUnsupported();
		return 0;
	}
	if (!Map) {
		return 0;
	}
	char name[NET_NAME_LEN];
	CopyText(name, sizeof(name), p_name);
	CloseSession(0);
	int port = p_port ? p_port : NET_DEFAULT_PORT;
	if (port < 1 || port > 65535 || !EnsureEnet()) {
		SetStatus("cannot host on udp port %d", port);
		return 0;
	}
	NET_RELAY_CONFIG config;
	NetRelay_DefaultConfig(&config);
	config.m_port = port;
	config.m_playerHosted = 1;
	memset(config.m_map, 0, sizeof(config.m_map));
	config.m_log = RelayLogSink;
	config.m_logUser = 0;
	char error[NET_STATUS_LEN];
	error[0] = 0;
	g_net.m_relay = NetRelay_Create(&config, error, sizeof(error));
	if (!g_net.m_relay) {
		SetStatus("cannot host on udp port %d", port);
		return 0;
	}
	SetStatus("hosting on udp port %d", port);
	ChooseName(name);
	char loopback[NET_LOOPBACK_TEXT_LEN];
	snprintf(loopback, sizeof(loopback), "127.0.0.1:%d", port);
	CopyText(g_net.m_address, sizeof(g_net.m_address), loopback);
	CopyText(g_net.m_hostName, sizeof(g_net.m_hostName), "127.0.0.1");
	g_net.m_port = port;
	SetStatus("connecting to %s:%d", g_net.m_hostName, port);
	ENetAddress address = {};
	address.host = enet_v4_localhost;
	address.port = (enet_uint16) port;
	if (!BeginConnect(&address, NetNow())) {
		CloseSession(0);
		return 0;
	}
	return 1;
}

void Net_Disconnect()
{
	if (g_net.m_state != NET_IDLE || g_net.m_relay || g_net.m_resolveJob) {
		CloseSession("disconnected: disconnected");
	}
}

int Net_UiState()
{
	if (g_net.m_state == NET_IDLE) {
		return NET_UI_IDLE;
	}
	return g_net.m_state == NET_ONLINE ? NET_UI_ONLINE : NET_UI_BUSY;
}

int Net_Hosting()
{
	return g_net.m_relay != 0;
}

int Net_Playing()
{
	return g_net.m_state == NET_ONLINE && g_net.m_mapKey[0];
}

const char* Net_Status()
{
	return g_net.m_status;
}

const char* Net_ActiveAddress()
{
	return g_net.m_state == NET_IDLE ? "" : g_net.m_address;
}

const char* Net_ActiveName()
{
	return g_net.m_state == NET_IDLE ? "" : g_net.m_name;
}

void Net_SetAutoConnect(const char* p_address, const char* p_name)
{
	bool fits = p_address && strlen(p_address) < sizeof(g_net.m_autoAddress);
	CopyText(g_net.m_autoAddress, sizeof(g_net.m_autoAddress), fits ? p_address : "");
	CopyText(g_net.m_autoName, sizeof(g_net.m_autoName), p_name);
	g_net.m_autoKind = p_address && p_address[0] ? NET_AUTO_CONNECT : NET_AUTO_NONE;
}

void Net_SetAutoHost(int p_port, const char* p_name)
{
	g_net.m_autoPort = p_port;
	CopyText(g_net.m_autoName, sizeof(g_net.m_autoName), p_name);
	g_net.m_autoKind = NET_AUTO_HOST;
}

int Net_RemoteInfo(int p_index, NET_REMOTE_INFO* p_out)
{
	if (!p_out || g_net.m_state != NET_ONLINE || p_index < 0 || p_index >= NET_MAX_PLAYERS) {
		return 0;
	}
	const NET_CLIENT_REMOTE* remote = &g_net.m_remote[p_index];
	const SPRITE* sprite = remote->m_sprite;
	if (!remote->m_used || !remote->m_sameMap || !remote->m_sampleCount || !sprite || sprite->m_ani >= NET_ANI_DEATH) {
		return 0;
	}
	int hp = SampleByRank(remote, 0)->m_actor.m_hp;
	int army = (int) ((sprite->m_flag >> NET_SPRITE_ARMY_SHIFT) & NET_SPRITE_ARMY_MASK);
	int maxHp = sprite->m_vid ? sprite->m_vid->GetMaxHp(army) : 0;
	if (maxHp <= 0) {
		maxHp = NET_AS1_FALLBACK_MAX_HP;
	}
	if (maxHp < hp) {
		maxHp = hp;
	}
	p_out->m_name = remote->m_name;
	p_out->m_x = sprite->m_x;
	p_out->m_y = sprite->m_y;
	p_out->m_z = sprite->m_z;
	p_out->m_hp = hp;
	p_out->m_maxHp = maxHp;
	return 1;
}

int Net_RosterInfo(int p_index, NET_ROSTER_INFO* p_out)
{
	if (!p_out || g_net.m_state != NET_ONLINE || p_index < 0 || p_index >= NET_MAX_PLAYERS) {
		return 0;
	}
	if (p_index == g_net.m_playerId) {
		bool playable = Map && Map->m_gameplayMap && g_net.m_mapKey[0];
		p_out->m_name = g_net.m_name;
		p_out->m_map = g_net.m_mapKey[0] ? NetMapLeaf(g_net.m_mapKey) : "";
		p_out->m_local = 1;
		p_out->m_sameMap = 1;
		p_out->m_away = !playable;
		return 1;
	}
	const NET_CLIENT_REMOTE* remote = &g_net.m_remote[p_index];
	if (!remote->m_used) {
		return 0;
	}
	bool playable = remote->m_map[0] && (remote->m_mapFlags & NET_MAPF_PLAYABLE);
	p_out->m_name = remote->m_name;
	p_out->m_map = remote->m_map[0] ? NetMapLeaf(remote->m_map) : "";
	p_out->m_local = 0;
	p_out->m_sameMap = remote->m_sameMap;
	p_out->m_away = !playable;
	return 1;
}

void Net_OnLocalAttack(SPRITE* p_sprite, float p_x, float p_y)
{
	if (g_net.m_state != NET_ONLINE) {
		return;
	}
	if (p_sprite == LocalFlagman()) {
		g_net.m_attackX = p_x;
		g_net.m_attackY = p_y;
	}
}

void Net_OnAmmoSpent(SPRITE* p_sprite, int p_amount)
{
	if (g_net.m_state != NET_ONLINE || p_amount >= 0 || p_amount < NET_AS1_SHOT_AMMO_FLOOR) {
		return;
	}
	const SPRITE* flagman = LocalFlagman();
	if (!flagman || (p_sprite != flagman && p_sprite->m_parent != flagman)) {
		return;
	}
	if (!g_net.m_mapAnnounced || !g_net.m_announcedPlayable || (Map->m_flag & NET_ENGINE_FLAG_DEMO_PLAYBACK)) {
		return;
	}
	unsigned int now = NetNow();
	if (g_net.m_shotSent && NetElapsedMs(now, g_net.m_lastShotSent) < NET_SHOT_DEDUP_MS) {
		return;
	}
	NetMsgShoot msg = {};
	msg.m_type = NET_MSG_SHOOT;
	msg.m_mapSeq = g_net.m_mapSeq;
	msg.m_x = g_net.m_attackX;
	msg.m_y = g_net.m_attackY;
	if (!NetShootValid(msg)) {
		return;
	}
	g_net.m_shotSent = 1;
	g_net.m_lastShotSent = now;
	SendPacket(&msg, sizeof(msg), NET_CHANNEL_EVENTS, true);
}

void Net_OnSpriteDeleted(SPRITE* p_sprite)
{
	if (!g_netProxyCount || !p_sprite) {
		return;
	}
	for (int i = 0; i < NET_MAX_PLAYERS; ++i) {
		if (g_net.m_remote[i].m_sprite == p_sprite) {
			g_net.m_remote[i].m_sprite = 0;
			--g_netProxyCount;
		}
	}
}

void Net_OnMapRelease()
{
	if (g_net.m_state == NET_IDLE) {
		return;
	}
	for (int i = 0; i < NET_MAX_PLAYERS; ++i) {
		NET_CLIENT_REMOTE* remote = &g_net.m_remote[i];
		DestroyProxy(remote);
		ClearSamples(remote);
		remote->m_sameMap = 0;
		remote->m_hereLogged = 0;
	}
	memset(g_net.m_mapKey, 0, sizeof(g_net.m_mapKey));
	g_net.m_announcedPlayable = 0;
	g_net.m_playingLogged = 0;
	g_net.m_following = NET_FOLLOW_IDLE;
	if (g_net.m_state == NET_ONLINE) {
		SendMap(0, g_net.m_mapKey);
		g_net.m_mapAnnounced = 1;
		if (g_net.m_host) {
			enet_host_flush(g_net.m_host);
		}
	}
	if (g_net.m_relay) {
		NetRelay_SetStartMap(g_net.m_relay, "");
		NetRelay_Service(g_net.m_relay, 0);
	}
}

static const NET_CLIENT_REMOTE* TeammateFurtherAhead(const char* p_requested)
{
	size_t directoryLength;
	int requested = CampaignLevelNumber(p_requested, &directoryLength);
	const NET_CLIENT_REMOTE* furthest = 0;
	int furthestLevel = requested;
	for (int i = 0; requested >= 1 && i < NET_MAX_PLAYERS; ++i) {
		const NET_CLIENT_REMOTE* remote = &g_net.m_remote[i];
		bool onLevel =
			remote->m_used && (remote->m_mapFlags & NET_MAPF_PLAYABLE) && NetMapNameIsGameplay(remote->m_map);
		int level = onLevel ? CampaignLevelNumber(remote->m_map, &directoryLength) : -1;
		if (level > furthestLevel && SameCampaignDirectory(remote->m_map, p_requested) &&
			StartMapLoadable(remote->m_map)) {
			furthest = remote;
			furthestLevel = level;
		}
	}
	return furthest;
}

void Net_OnMapChangeRequest(STRING& p_target)
{
	if (g_net.m_state != NET_ONLINE || !Map || !p_target.m_str || (Map->m_flag & NET_ENGINE_FLAG_DEMO_PLAYBACK)) {
		return;
	}
	g_net.m_following = NET_FOLLOW_IDLE;
	if (g_net.m_relay || NetMapNameIsGameplay(g_net.m_mapKey)) {
		return;
	}
	char requested[NET_MAP_LEN];
	if (!NetNormalizeMapName(p_target.m_str, strlen(p_target.m_str) + 1, requested) ||
		!NetMapNameIsGameplay(requested)) {
		return;
	}
	const NET_CLIENT_REMOTE* teammate = TeammateFurtherAhead(requested);
	if (teammate) {
		SetStatus("joining %s on %s", teammate->m_name, NetMapLeaf(teammate->m_map));
		p_target = STRING(teammate->m_map);
	}
}

void Net_OnMapLoaded()
{
	if (g_net.m_state == NET_IDLE || !Map) {
		return;
	}
	RefreshMapKey();
	g_net.m_mapAnnounced = 0;
	g_net.m_announcedPlayable = 0;
	g_net.m_playingLogged = 0;
	for (int i = 0; i < NET_MAX_PLAYERS; ++i) {
		if (g_net.m_remote[i].m_used) {
			RecomputeSameMap(&g_net.m_remote[i]);
		}
	}
	ResolvePendingStartMap();
	if (g_net.m_relay) {
		NetRelay_Service(g_net.m_relay, 0);
	}
}

void Net_OnBeforeSave()
{
	if (!g_netProxyCount) {
		return;
	}
	int excluded = 0;
	for (int i = 0; i < NET_MAX_PLAYERS; ++i) {
		if (g_net.m_remote[i].m_sprite) {
			DestroyProxy(&g_net.m_remote[i]);
			++excluded;
		}
	}
	if (excluded) {
		LogLine("save: excluded %d remote player(s)", excluded);
	}
}

int Net_IsProxySlow(const SPRITE* p_sprite)
{
	if (!p_sprite) {
		return 0;
	}
	for (int i = 0; i < NET_MAX_PLAYERS; ++i) {
		if (g_net.m_remote[i].m_sprite == p_sprite) {
			return 1;
		}
	}
	return 0;
}
