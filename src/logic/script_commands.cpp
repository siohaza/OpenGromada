#include "logic/script_commands.h"
#include "game/game_descriptor.h"

#include <climits>

namespace {

struct COMMAND_ADAPTER {
	unsigned short raw;
	unsigned short canonical;
	unsigned char arguments;
};










constexpr COMMAND_ADAPTER crazyLunchCommands[] = {
	{65, 65, 6},
	{70, 70, 4},
	{71, 71, 3},
	{72, 72, 5},
	{80, 79, 5},
	{81, 80, 3},
	{82, 82, 5},
	{83, 83, 1},
	{84, 84, 1},
	{85, 85, 1},
	{86, 86, 1},
	{87, 87, 1},
	{88, 88, 1},
	{89, 89, 1},
	{90, 90, 3},
	{91, 96, 1},
	{92, 97, 2},
	{100, 101, 2},
	{101, 102, 1},
	{102, 103, 1},
	{103, 104, 0},
	{104, 105, 0},
	{105, 106, 6},
	{106, 107, 5},
	{107, 108, 0},
	{108, 127, 0},
	{110, 98, 1},
	{113, 124, 1},
	{114, 152, 1},
	{115, 111, 0},
	{116, 112, 1},
	{118, 115, 0},
	{119, 116, 2},
	{120, 117, 1},
	{123, 149, 0},
	{125, 151, 0},
	{130, 109, 0},
	{131, 110, 0},
	{132, 113, 1},
	{134, 129, 2},
	{140, 130, 1},
	{141, 131, 1},
	{142, 132, 1},
	{145, 135, 3},
	{146, 136, 2},
	{147, 137, 4},
	{148, 138, 1},
	{150, 140, 1},
	{155, 145, 0},
	{158, 148, 1},
	{159, 119, 0},
	{160, 120, 0},
	{172, 155, 1},
	{174, 157, 0},
	{175, 158, 2},
	{177, 162, 2},
	{178, 163, 3},
	{183, 169, 1},
	{200, 174, 2},
	{201, 175, 1},
	{202, 176, 1},
	{203, 177, 1},
	{204, 178, 1},
	{205, 179, 1},
	{206, 182, 3},
	{207, 183, 3},
	{209, 185, 0},
	{215, 153, 2},
	{216, 164, 1},
	{217, 172, 2},
	{218, 205, 1},
	{219, 206, 1},
	{220, 207, 1},
	{221, 208, 2},
	{223, 123, 2},
};

const COMMAND_ADAPTER* FindCrazyLunchCommand(unsigned int p_raw)
{
	for (const COMMAND_ADAPTER& command : crazyLunchCommands) {
		if (command.raw == p_raw) {
			return &command;
		}
	}
	return nullptr;
}

bool IsCrazyLunchDialect()
{
	return GameDesc && GameDesc->m_scriptDialect == GAME_SCRIPT_CRAZY_LUNCH;
}







int LocolandMinimumArgs(unsigned int p_raw)
{
	switch (p_raw) {
	case 70: return 3;
	case 72: return 4;
	case 102: return 1;
	case 103: return 0;
	case 212: return 2;

	case 210: case 211: case 248: return -1;
	default: return -2;
	}
}

}

int ScriptCommandCanonical(unsigned int p_raw)
{
	if (GameDesc && GameDesc->m_scriptDialect == GAME_SCRIPT_LOCOLAND &&
		LocolandMinimumArgs(p_raw) == -1) {
		return -1;
	}
	if (!IsCrazyLunchDialect()) {
		return p_raw <= INT_MAX ? static_cast<int>(p_raw) : -1;
	}
	const COMMAND_ADAPTER* command = FindCrazyLunchCommand(p_raw);
	return command ? command->canonical : -1;
}

int ScriptCommandMinimumArgs(unsigned int p_raw)
{
	if (GameDesc && GameDesc->m_scriptDialect == GAME_SCRIPT_LOCOLAND) {
		return LocolandMinimumArgs(p_raw);
	}
	if (!IsCrazyLunchDialect()) {
		return -2;
	}
	const COMMAND_ADAPTER* command = FindCrazyLunchCommand(p_raw);
	return command ? command->arguments : -1;
}

int ScriptVidFieldCanonical(int p_field)
{
	if (!IsCrazyLunchDialect()) {
		return p_field;
	}



	switch (p_field) {
	case 49: return 28;
	case 112: return 48;
	default: return -1;
	}
}
