
#include "game/game_descriptor.h"

#include "platform/paths.h"

#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

namespace
{

struct RES_SIGNATURE {
	int m_weap;
	int m_obj;
	int m_sfx;
	bool m_readable;
};

const GAME_DESCRIPTOR g_gameAS1 = {
	GAME_AS1,
	"AlienShooter",
	"Alien Shooter",
	"com.sigmateam.alienshooter",
	612,
	0x800,
	640,
	480,
	0,
	0,
};

const GAME_DESCRIPTOR g_gameZS1 = {
	GAME_ZS1,
	"ZombieShooter",
	"Zombie Shooter",
	"com.sigmateam.zombieshooter",
	640,
	0x1000,
	1024,
	768,
	1280,
	720,
};

int g_cliOverride = -1;

RES_SIGNATURE FingerprintRes()
{
	RES_SIGNATURE signature = {0, 0, 0, false};
	FILE* file = Platform_FOpen("objects.res", "rb");
	if (!file) {
		return signature;
	}

	unsigned int header[3];
	if (fread(header, 4, 3, file) == 3 && header[0] == 0x20534552) {
		signature.m_readable = true;
		long start = ftell(file);
		for (int section = 0; section < 24; ++section) {
			unsigned int head[5];
			if (fseek(file, start, SEEK_SET) || fread(head, 4, 5, file) != 5 || head[2] != 0x80000000u) {
				break;
			}
			unsigned int first = 0;
			if (head[4] > 0 && fread(&first, 4, 1, file) == 1) {
				if (head[0] == 0x50414557) {
					signature.m_weap = (int) first;
				}
				else if (head[0] == 0x204a424f) {
					signature.m_obj = (int) first;
				}
				else if (head[0] == 0x20584653) {
					signature.m_sfx = (int) first;
				}
			}
			start += 8 + (long) head[1] + ((head[1] & 1) ? 1 : 0);
		}
	}
	fclose(file);
	return signature;
}

// AS1 and ZS1 are told apart by the first record size of three sections. WEAP
// alone is not enough: it is 612 for AS1, Theseus and Chacks Temple, and 640
// for ZS1, the whole AS2/ZS2 family and Alien Shooter: Revisited. SFX splits
// the 612 group, and OBJ splits ZS1 (725) from AS2 (738+) and ZS2 (746). The
// OBJ bound leaves room for a mod that renames the first object; a mod that
// overruns it is refused rather than mis-loaded, and --game= forces the issue.
const GAME_DESCRIPTOR* MatchSignature(const RES_SIGNATURE& p_signature)
{
	if (p_signature.m_weap == g_gameAS1.m_weapRecordBytes && p_signature.m_sfx == 21) {
		return &g_gameAS1;
	}
	if (p_signature.m_weap == g_gameZS1.m_weapRecordBytes && p_signature.m_sfx == 41 &&
		p_signature.m_obj > 0 && p_signature.m_obj < 732) {
		return &g_gameZS1;
	}
	return 0;
}

}

const GAME_DESCRIPTOR* GameDesc = &g_gameAS1;

void Game_SetCliOverride(GAME_ID p_id)
{
	g_cliOverride = p_id;
}

bool Game_Detect()
{
	if (g_cliOverride == GAME_AS1 || g_cliOverride == GAME_ZS1) {
		GameDesc = g_cliOverride == GAME_ZS1 ? &g_gameZS1 : &g_gameAS1;
		Platform_SetPrefApp(GameDesc->m_className);
		return true;
	}

	RES_SIGNATURE signature = FingerprintRes();
	const GAME_DESCRIPTOR* pick = MatchSignature(signature);
	if (!pick) {
		if (!signature.m_readable) {
			// No readable objects.res at all: leave the default in place and
			// let the game-data check report the missing installation.
			Platform_SetPrefApp(GameDesc->m_className);
			return true;
		}

		char message[1024];
		snprintf(
			message,
			sizeof(message),
			"This objects.res is not Alien Shooter or Zombie Shooter data.\n\n"
			"Record signature: WEAP=%i OBJ=%i SFX=%i\n"
			"Supported: WEAP=612 OBJ=717 SFX=21 (Alien Shooter)\n"
			"           WEAP=640 OBJ=725 SFX=41 (Zombie Shooter)\n\n"
			"Alien Shooter 2, Zombie Shooter 2, Alien Shooter: Revisited, Theseus\n"
			"and Chacks Temple use different record layouts and are not supported.\n"
			"Force a game anyway with --game=as1 or --game=zs1.\n",
			signature.m_weap,
			signature.m_obj,
			signature.m_sfx
		);
		fprintf(stderr, "%s\n", message);
		fflush(stderr);
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Unsupported Game Data", message, 0);
		return false;
	}

	GameDesc = pick;
	Platform_SetPrefApp(pick->m_className);
	return true;
}
