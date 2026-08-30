#include "game/player.h"

#include "game/map.h"
#include "sprite/sprite.h"
#include "util/myerror.h"
#include "util/profile.h"
#include "video/vid.h"

#include <stdlib.h>

// FUNCTION: ALIEN 0x413060
PLAYER::PLAYER(int p_control, int p_army) : m_army(p_army)
{
	m_flagman = 0;
	m_underCursor = 0;
	m_control = p_control;
	m_money = 1000;
}

// FUNCTION: ALIEN 0x413140
void PLAYER::SetFlagman(SPRITE* p_sprite)
{
	if (p_sprite) {
		++p_sprite->m_noRef;
	}
	if (m_flagman) {
		((SPRITE*) m_flagman)->ReleaseRef();
	}
	m_flagman.m_ptr = p_sprite;
}

// FUNCTION: ALIEN 0x4131b0
void PLAYER::PutMessage(const STRING&, float, float)
{
}

// FUNCTION: ALIEN 0x4131e0
PLAYER::~PLAYER()
{
	Release();
	if (m_underCursor) {
		MYERROR::Error(
			::Error,
			"SPRITE %i",
			10,
			// STRING: ALIEN 0x482a90
			"PTR_SPRITE with this sprite not clear",
			0,
			m_underCursor->m_vid ? m_underCursor->m_vid->m_idx : -1
		);
	}
}

// FUNCTION: ALIEN 0x413280
void PLAYER::DeletePointerToSprite(SPRITE* p_sprite)
{
	m_stateBar.Delete(p_sprite);
	if (m_flagman == p_sprite) {
		m_flagman = 0;
	}
	if (m_underCursor == p_sprite) {
		m_underCursor = 0;
	}
}

// FUNCTION: ALIEN 0x413360
void PLAYER::Release()
{
	m_flagman = 0;
	m_underCursor = 0;
	m_money = 1000;
}

// FUNCTION: ALIEN 0x413420
int PLAYER::Save(STREAM* p_stream) const
{
	return p_stream->Write(&m_flagman, 4);
}

// FUNCTION: ALIEN 0x413440
void PLAYER::Load(STREAM* p_stream)
{
	MAN* man = (MAN*) Map->ReadPointer(p_stream);
	if (man) {
		++man->m_noRef;
	}
	if (m_flagman) {
		((SPRITE*) m_flagman)->ReleaseRef();
	}
	m_flagman.m_ptr = man;
}

// FUNCTION: ALIEN 0x4134c0
STRING PLAYER::GetMouseTipsString() const
{
	if (m_underCursor) {
		STRING dflt(empty_str);
		STRING app(
			// STRING: ALIEN 0x482ab8
			"Units"
		);
		char buffer[128];
		int idx = m_underCursor->m_vid->m_idx;
		STRING key(_itoa(idx, buffer, 10));
		return Strings->GetString(app, key, dflt);
	}
	return STRING(empty_str);
}

unsigned int PLAYER::GetMoney()
{
	return m_money;
}

// FUNCTION: ALIEN 0x43a8b0
unsigned int PLAYER::SetMoney(unsigned int p_money)
{
	m_money = p_money;
	return p_money;
}

// FUNCTION: ALIEN 0x43f190
MAN* PLAYER::Flagman()
{
	return (MAN*) (SPRITE*) m_flagman;
}
