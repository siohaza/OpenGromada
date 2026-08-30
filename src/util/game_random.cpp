#include "util/game_random.h"

#include <cstdint>

namespace
{

std::uint32_t g_gameRandomState = 1u;

} // namespace

int GameRand()
{
	g_gameRandomState = g_gameRandomState * 214013u + 2531011u;
	return (int) ((g_gameRandomState >> 16) & 0x7fffu);
}

void GameSrand(unsigned int p_seed)
{
	g_gameRandomState = (std::uint32_t) p_seed;
}
