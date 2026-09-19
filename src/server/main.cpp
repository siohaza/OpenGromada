#include "net/net_relay.h"

#include <chrono>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>

enum {
	SERVER_EXIT_OK = 0,
	SERVER_EXIT_FAILURE = 1,
	SERVER_SERVICE_WAIT_MS = 100,
	SERVER_ERROR_BACKOFF_MS = 10,
	SERVER_ERROR_TEXT_BYTES = 128
};

enum SERVER_ARGS_RESULT {
	SERVER_ARGS_RUN,
	SERVER_ARGS_HELP,
	SERVER_ARGS_BAD
};

namespace
{
volatile sig_atomic_t g_stop = 0;
}

static void OnSignal(int)
{
	g_stop = 1;
}

static void KeepRunningWhenLogPipeCloses()
{
#ifdef SIGPIPE
	signal(SIGPIPE, SIG_IGN);
#endif
}

static void PrintLine(void*, const char* p_line)
{
	printf("[server] %s\n", p_line);
	fflush(stdout);
}

static void PrintUsage(FILE* p_out)
{
	fprintf(
		p_out,
		"Usage: OpenGromadaServer [--port N] [--max-players N] [--rate MS] [--map NAME] [--no-follow]\n"
		"  --port N         udp port to listen on, 1..65535 (default %d)\n"
		"  --max-players N  player slots, 2..%d (default %d)\n"
		"  --rate MS        snapshot broadcast period, 10..1000 (default %d)\n"
		"  --map NAME       start map offered to joining players (default maps\\level_01.map)\n"
		"  --no-follow      always offer --map instead of the map the players are on\n"
		"  --help           show this text\n",
		NET_DEFAULT_PORT,
		NET_MAX_PLAYERS,
		NET_DEFAULT_MAX_PLAYERS,
		NET_DEFAULT_SNAPSHOT_MS
	);
}

static const char* OptionValue(int p_argc, char** p_argv, int* p_index, const char* p_name)
{
	const char* arg = p_argv[*p_index];
	size_t nameLength = strlen(p_name);
	if (strncmp(arg, p_name, nameLength)) {
		return 0;
	}
	if (arg[nameLength] == '=') {
		return arg + nameLength + 1;
	}
	if (arg[nameLength] || *p_index + 1 >= p_argc) {
		return 0;
	}
	++*p_index;
	return p_argv[*p_index];
}

static int ParseInt(const char* p_text, int* p_out)
{
	char* end = 0;
	errno = 0;
	long value = strtol(p_text, &end, 10);
	if (errno || end == p_text || *end || value < INT_MIN || value > INT_MAX) {
		return 0;
	}
	*p_out = (int) value;
	return 1;
}

#ifdef OPENGROMADA_NET_TESTHOOKS
static int ParseUnsigned(const char* p_text, unsigned int* p_out)
{
	char* end = 0;
	errno = 0;
	unsigned long value = strtoul(p_text, &end, 10);
	if (errno || end == p_text || *end || p_text[0] == '-' || value > UINT_MAX) {
		return 0;
	}
	*p_out = (unsigned int) value;
	return 1;
}
#endif

static int CopyMapOption(const char* p_text, NET_RELAY_CONFIG* p_config)
{
	size_t length = strlen(p_text);
	if (length >= sizeof(p_config->m_map)) {
		return 0;
	}
	memset(p_config->m_map, 0, sizeof(p_config->m_map));
	memcpy(p_config->m_map, p_text, length);
	return 1;
}

static SERVER_ARGS_RESULT ParseArgs(int p_argc, char** p_argv, NET_RELAY_CONFIG* p_config)
{
	for (int i = 1; i < p_argc; ++i) {
		const char* option = p_argv[i];
		const char* value = 0;
		int parsed = 1;
		if (!strcmp(option, "--help")) {
			return SERVER_ARGS_HELP;
		}
		if (!strcmp(option, "--no-follow")) {
			p_config->m_followPlayers = 0;
		}
		else if ((value = OptionValue(p_argc, p_argv, &i, "--port"))) {
			parsed = ParseInt(value, &p_config->m_port);
		}
		else if ((value = OptionValue(p_argc, p_argv, &i, "--max-players"))) {
			parsed = ParseInt(value, &p_config->m_maxPlayers);
		}
		else if ((value = OptionValue(p_argc, p_argv, &i, "--rate"))) {
			parsed = ParseInt(value, &p_config->m_rateMs);
		}
		else if ((value = OptionValue(p_argc, p_argv, &i, "--map"))) {
			parsed = CopyMapOption(value, p_config);
		}
#ifdef OPENGROMADA_NET_TESTHOOKS
		else if ((value = OptionValue(p_argc, p_argv, &i, "--tick-bias"))) {
			parsed = ParseUnsigned(value, &p_config->m_tickBias);
		}
		else if ((value = OptionValue(p_argc, p_argv, &i, "--hello-timeout"))) {
			parsed = ParseInt(value, &p_config->m_helloTimeoutMs);
		}
#endif
		else {
			fprintf(stderr, "[server] unknown or incomplete option %s\n", option);
			return SERVER_ARGS_BAD;
		}
		if (!parsed) {
			fprintf(stderr, "[server] invalid value for %s\n", option);
			return SERVER_ARGS_BAD;
		}
	}
	const char* problem = NetRelay_CheckConfig(p_config);
	if (problem) {
		fprintf(stderr, "[server] invalid configuration: %s\n", problem);
		return SERVER_ARGS_BAD;
	}
	return SERVER_ARGS_RUN;
}

int main(int p_argc, char** p_argv)
{
	NET_RELAY_CONFIG config;
	NetRelay_DefaultConfig(&config);
	config.m_log = PrintLine;

	SERVER_ARGS_RESULT args = ParseArgs(p_argc, p_argv, &config);
	if (args == SERVER_ARGS_HELP) {
		PrintUsage(stdout);
		return SERVER_EXIT_OK;
	}
	if (args == SERVER_ARGS_BAD) {
		PrintUsage(stderr);
		return SERVER_EXIT_FAILURE;
	}

	signal(SIGINT, OnSignal);
	signal(SIGTERM, OnSignal);
	KeepRunningWhenLogPipeCloses();

	char error[SERVER_ERROR_TEXT_BYTES];
	NET_RELAY* relay = NetRelay_Create(&config, error, sizeof(error));
	if (!relay) {
		fprintf(stderr, "[server] %s\n", error);
		return SERVER_EXIT_FAILURE;
	}
	while (!g_stop) {
		if (NetRelay_Service(relay, SERVER_SERVICE_WAIT_MS) < 0) {
			std::this_thread::sleep_for(std::chrono::milliseconds(SERVER_ERROR_BACKOFF_MS));
		}
	}
	NetRelay_Destroy(relay);
	return SERVER_EXIT_OK;
}
