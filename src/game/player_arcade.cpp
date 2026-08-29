#define DECOMP_INLINE_STRING_DTOR
#include "game/player_arcade.h"

#include <math.h>

#include "game/gametime.h"
#include "game/input_as.h"
#include "game/man.h"
#include "game/map.h"
#include "game/message.h"
#include "sprite/sprite.h"
#include "util/myerror.h"
#include "util/polar.h"
#include "world/group.h"
#include "world/groups.h"

extern int g_relativeControl;

// FUNCTION: ALIEN 0x43a8c0
unsigned int PLAYER_ARCADE::SetCleverAttack(int p_on)
{
	unsigned int result = (((p_on != 0) & 1) << 1) | (*(unsigned int*) &m_msg & 0xfffffffd);
	*(unsigned int*) &m_msg = result;
	return result;
}

#include "gfx/graph.h"

// FUNCTION: ALIEN 0x43e8c0
PLAYER_ARCADE::PLAYER_ARCADE(int p_control, int p_army)
	: PLAYER(p_control, p_army)
	, m_msg(4, -1, 398.0f, 388.0f, 5, 5000)
{
	MESSAGE& msg = m_msg;
	float height = Graph->m_height;
	msg.m_z = Graph->m_width - 242.0f;
	msg.m_y = height - 92.0f;
	m_msg.m_lineSpacing = 1;
}

// FUNCTION: ALIEN 0x43e930
void PLAYER_ARCADE::PutMessage(const STRING& p_msg, float p_x, float p_y)
{
	((MESSAGE*) &m_msg)->Put(p_msg, p_x, p_y);
}

// FUNCTION: ALIEN 0x43e9f0
void PLAYER_ARCADE::DeletePointerToSprite(SPRITE* p_sprite)
{
	((PLAYER_MSG*) &m_msg)->vf04(p_sprite);
	PLAYER::DeletePointerToSprite(p_sprite);
}

// GLOBAL: ALIEN 0x5da548
static int g_playerHeadAiming;

// STUB: ALIEN 0x43ea10
void PLAYER_ARCADE::Control(INPUT_AS* p_input)
{
	GROUP* base = Map->m_groups.First();
	GROUP* grp = base;
	if (grp) {
		do {
			int n = grp->m_n;
			SPRITE* goal = 0;
			int cmd = 0;
			int hasIdle = 0;
			SPRITE* lastGoal = 0;
			if (n > 0) {
				SPRITE** data = (SPRITE**) grp->m_data;
				for (int i = 0; i < n; ++i) {
					SPRITE* s = *data;
					unsigned int f = s->m_flag & 0x7c;
					if (f == 0) {
						goal = lastGoal;
						hasIdle = 1;
					}
					if (f == 12 || f == 16) {
						goal = s->m_goal;
						cmd = (s->m_flag >> 2) & 0x1f;
						lastGoal = goal;
					}
					++data;
				}
				if (goal && cmd && hasIdle) {
					int j = 0;
					do {
						SPRITE* s = ((SPRITE**) grp->m_data)[j];
						if ((s->m_flag & 0x7c) == 0)
							s->SetCommand(cmd, lastGoal);
						++j;
					} while (j < grp->m_n);
				}
			}
			grp = Map->m_groups.Next(grp);
		} while (grp);
	}

	m_msg.Shift();
	MAN* flagman = (MAN*) (SPRITE*) m_flagman;
	if (!flagman || (int) flagman->m_vid->m_sprClass != 7)
		return;

	if (p_input->m_button & 0x8000) {
		float gz;
		flagman->Action(33, (int) p_input->m_worldX,
			(int) (Map->GetGroundZScr(p_input->m_worldX, p_input->m_worldY)
				+ p_input->m_worldY), 0);
	}
	if ((p_input->m_button & 0x700) || (p_input->m_button & 0x80)) {
		MAN* f = (MAN*) (SPRITE*) m_flagman;
		if ((f->m_flag & 0x7c) == 4)
			f->SetCommandWithoutLink(0, 0);
	}
	if (p_input->m_button & 0x4000) {
		MAN* f = (MAN*) (SPRITE*) m_flagman;
		VID* vid = f->m_vid;
		if ((int) vid->m_sprClass == 7
			&& f->UNIT::m_ammo / 64 < vid->m_aniFireCount[8]) {
			int weapon = f->m_child->m_vid->m_idx - 11;
			if (!f->ChangeWeapon(weapon)) {
				do
					--weapon;
				while (!((MAN*) (SPRITE*) m_flagman)->ChangeWeapon(weapon));
			}
		}
		((MAN*) (SPRITE*) m_flagman)->Action(37,
			(int) p_input->m_worldX, (int) p_input->m_worldY, 0);
	}

	unsigned int key = p_input->m_key;
	if (key >= 0x30 && key <= 0x39) {
		MAN* f = (MAN*) (SPRITE*) m_flagman;
		if ((int) f->m_vid->m_sprClass == 7)
			f->ChangeWeapon(key - 48);
	}

	int weapon = ((MAN*) (SPRITE*) m_flagman)->m_vid->m_linkVid->m_idx - 10;
	int wheel = p_input->m_wheel;
	if (wheel == 0) {
		int k = p_input->m_unk0x1c;
		if (k == INPUT_AS::nextKey1)
			wheel = 1;
		else if (k == INPUT_AS::prevKey1)
			wheel = -1;
	}
	while (wheel > 0) {
		if (weapon < 10) {
			do
				++weapon;
			while (!((MAN*) (SPRITE*) m_flagman)->ChangeWeapon(weapon) && weapon < 10);
		}
		--wheel;
	}
	if (wheel < 0) {
		if (weapon > 0) {
			do
				--weapon;
			while (!((MAN*) (SPRITE*) m_flagman)->ChangeWeapon(weapon) && weapon > 0);
		}
	}

	MAN* f = (MAN*) (SPRITE*) m_flagman;
	float dy = f->m_z + p_input->m_worldY - f->m_y;
	float dx = p_input->m_worldX - f->m_x;
	ANGLE cursor;
	cursor = Decart2Polar_f(dx, dy);
	ANGLE aim(0);
	if (g_relativeControl)
		aim.m_dir = cursor.m_dir;

	if ((p_input->m_button & 0x400) && (p_input->m_button & 0x100)) {
		Flagman()->Rotate(aim + (unsigned char) 32 + (unsigned char) 8, CurrentTime - PrevCurrentTime);
	}
	else if ((p_input->m_button & 0x400) && (p_input->m_button & 0x80)) {
		Flagman()->Rotate(aim + (unsigned char) -32 - (unsigned char) 8, CurrentTime - PrevCurrentTime);
	}
	else if ((p_input->m_button & 0x200) && (p_input->m_button & 0x100)) {
		Flagman()->Rotate(aim + (unsigned char) 96 - (unsigned char) 8, CurrentTime - PrevCurrentTime);
	}
	else if ((p_input->m_button & 0x200) && (p_input->m_button & 0x80)) {
		Flagman()->Rotate(aim + (unsigned char) -96 + (unsigned char) 8, CurrentTime - PrevCurrentTime);
	}
	else if (p_input->m_button & 0x80) {
		Flagman()->Rotate(Flagman()->GlideDirection(aim + (unsigned char) -64), CurrentTime - PrevCurrentTime);
	}
	else if (p_input->m_button & 0x100) {
		Flagman()->Rotate(Flagman()->GlideDirection(aim + (unsigned char) 64), CurrentTime - PrevCurrentTime);
	}
	else if (p_input->m_button & 0x200) {
		Flagman()->Rotate(Flagman()->GlideDirection(aim + (unsigned char) 0x80), CurrentTime - PrevCurrentTime);
	}
	else if (p_input->m_button & 0x400) {
		Flagman()->Rotate(Flagman()->GlideDirection(aim + (unsigned char) 0), CurrentTime - PrevCurrentTime);
	}
	else {
		MAN* fm = (MAN*) (SPRITE*) m_flagman;
		if ((fm->m_flag & 0x7c) != 4)
			fm->Stop();
	}

	if ((p_input->m_button & 0x700) || (p_input->m_button & 0x80))
		((MAN*) (SPRITE*) m_flagman)->StartMove();

	MAN* fm = (MAN*) (SPRITE*) m_flagman;
	SPRITE* child = fm->m_child;
	if (child && child->m_vid == fm->m_vid->m_linkVid) {
		if (fm->m_speed == 0.0f && fm->m_vid->m_idx == 9) {
			if (g_playerHeadAiming) {
				int hdt;
				ANGLE childDir;
				if (!Flagman()->Rotate(Flagman()->Child()->Direction(),
						CurrentTime - PrevCurrentTime).m_dir)
					g_playerHeadAiming = 0;
			}
			else {
				unsigned char cd = child->m_dir;
				unsigned char fd = fm->m_dir;
				unsigned char d1 = (unsigned char) (fd - cd);
				unsigned char d2 = (unsigned char) (cd - fd);
				unsigned char turn = (d1 < d2) ? d1 : d2;
				g_playerHeadAiming = turn > 0x3f;
			}
		}
		int cdt;
		Flagman()->Child()->Rotate(cursor, CurrentTime - PrevCurrentTime);
	}

	SPRITE* under = m_underCursor;
	if (under && (((SPRITE*) under)->m_vid->m_unk0x0c & 2)) {
		SPRITE* u = m_underCursor;
		if (u) {
			int refs;
		}
		(PTR_SPRITE&) m_underCursor = 0;
	}
}
