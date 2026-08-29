#ifndef SPRITE_H
#define SPRITE_H

#include "sprite/act.h"
#include "util/angle.h"
#include "util/decomp.h"
#include "util/myerror.h"
#include "video/vid.h"

class EX_SPRITE_DATA;
class STRING;
class SPRITE;

class PTR_SPRITE {
public:
	SPRITE* m_ptr; // 0x00

	PTR_SPRITE();
	~PTR_SPRITE();
	PTR_SPRITE& operator=(SPRITE* p_sprite);
	operator SPRITE*() const;
	SPRITE* operator->() const;
};

DECOMP_SIZE_ASSERT(PTR_SPRITE, 0x4)

enum {
	SPRITE_FLAG_INVISIBLE = 0x10000
};

// VTABLE: ALIEN 0x47a85c

class SPRITE {
public:
	SPRITE(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent);
	~SPRITE();
	virtual void* ScalarDeletingDestructor(unsigned int p_flags); // vtable+0x00
	virtual decomp_intptr Action(int p_action, int p_a, int p_b, int p_c); // vtable+0x04
	virtual void Tact(); // vtable+0x08
	virtual void MoveTact(); // vtable+0x0c
	virtual void DeletePointerToSprite(SPRITE* p_sprite); // vtable+0x10
	virtual VID* Draw(); // vtable+0x14
	virtual void DrawSecondaryInfo(); // vtable+0x18
	virtual void DrawGoalLine(); // vtable+0x1c

	int m_unk0x04; // 0x04
	int m_begCadr; // 0x08
	int m_noCadr; // 0x0c
	int m_endCadr; // 0x10
	unsigned int m_tactTime; // 0x14
	unsigned int m_createTime; // 0x18
	VID* m_vid; // 0x1c
	float m_speed; // 0x20
	float m_unk0x24; // 0x24
	unsigned int m_flag; // 0x28
	int m_noRef; // 0x2c
	float m_x; // 0x30
	float m_y; // 0x34
	float m_z; // 0x38
	SPRITE* m_goal; // 0x3c
	SPRITE* m_child; // 0x40
	SPRITE* m_parent; // 0x44
	int m_ani; // 0x48
	unsigned char m_dir; // 0x4c
	undefined m_unk0x4d[0x3]; // 0x4d
	undefined4 m_unk0x50; // 0x50
	int m_unk0x54; // 0x54
	LIST_ACT m_actions; // 0x58
	EX_SPRITE_DATA* m_exData; // 0x68
	PTR_SPRITE m_unk0x6c; // 0x6c

	void DrawRectangle();

	int IsInside(float p_x, float p_y) const
	{
		VID* vid = m_vid;
		float rx = vid->m_unk0x384;

		float x = m_x;
		if (p_x < x - rx || p_x > x + rx)
			return 0;
		float top = m_y - m_z;
		if (top - vid->m_unk0x24 - vid->m_unk0x388 >= p_y)
			return 0;
		if (top + vid->m_unk0x388 <= p_y)
			return 0;
		return 1;
	}
	float ScreenX();
	float ScreenY();
	float GetX();
	float GetY();
	float GetZ();
	GAMMA GetGamma();
	VID* GetVid();
	EX_SPRITE_DATA* ExData();

	float X() const { return m_x; }
	float Y() const { return m_y; }
	float Z() const { return m_z; }
	int IsClass(int p_class);
	int IsCommand(int p_command);
	SPRITE* Child();
	unsigned int Write(class STREAM* p_stream);
	SPRITE* Goal();
	int Remove();
	char* Insert();

	void InvisibleOn();
	void InvisibleOff();
	void ChangeAnimation(int p_ani);
	void SetGamma(const GAMMA& p_gamma);
	int AddLink(SPRITE* p_sprite);
	int AddLinkToLast(SPRITE* p_sprite);
	void BreakLink();
#ifdef DECOMP_INLINE_SPRITE_DIRECTIONTO

	ANGLE DirectionTo(const SPRITE* p_other) const
	{
		float dy = p_other->m_y - m_y;
		float dx = p_other->m_x - m_x;
		return ANGLE(dx, dy);
	}
#else
	ANGLE DirectionTo(const SPRITE* p_other) const;
#endif
	ANGLE DirectionTo(const SPRITE* p_other, int* p_dist) const;
	void PlaySFX(int p_sfx) const;
	void CreateChild();
	void CreateChildAndPlaySFX(int p_ani);
	STRING GetTextItems();
	void SetTextItems(const STRING& p_text);
	int GetFireDamage();
	int PrimitiveTact();
	int HaveFightLink();
	STRING GetTextActions() const;
	void SetTextActions(const STRING& p_text);
	void Stop();
	int AskLine(float* p_x, float* p_y, float* p_z);
	int SetCommandWithoutLink(int p_cmd, SPRITE* p_goal);
	int StartMove();
	int InsertUniqueItem(int p_vidIdx);
	void InsertItem(int p_vidIdx);
#ifdef DECOMP_INLINE_SPRITE_RELEASE

	int Release()
	{
		int refs = m_noRef - 1;
		m_noRef = refs;
		if (refs <= 0) {
			if (refs < 0) {
				MYERROR::Error(::Error, "SPRITE %i", 4, "noRef at Release",
					refs, m_vid ? m_vid->m_idx : -1);
				return 0;
			}
			if (this)
				ScalarDeletingDestructor(1);
			return 0;
		}
		return refs;
	}
#else
	int Release();
#endif
	void ResetActionStack();
	int ReleaseRef();
	void SetGoal(SPRITE* p_sprite);
	int PercentHp();
	int SetCommand(int p_cmd, SPRITE* p_goal);

	int SetCommand(int p_cmd, float p_x, float p_y, float p_z)
	{
		return SetCommand(p_cmd,
			new SPRITE(EmptyVid, p_x, p_y, p_z, ANGLE(0), 0));
	}
	int AttackTact(int p_time);
	ANGLE Rotate(ANGLE p_dir, int p_time);
	ANGLE GlideDirection(ANGLE p_dir);
	void MoveTactCalcCoor(float* p_x, float* p_y, float* p_z);
	int CanPlaceWithCrushAndGlide(float* p_x, float* p_y, float* p_z);
	void ChangeCoor(float p_x, float p_y, float p_z);

	void ChangeZ(float p_z) { ChangeCoor(m_x, m_y, p_z); }
	ANGLE Direction();
	int CanPlace(float p_x, float p_y, float p_z);
	int CanPlaceWithCrush(float p_x, float p_y, float p_z);

	int IsXYCross(const VID* p_vid, float p_x, float p_y) const;
	ANGLE RotateToGoal(int p_time);
	int MoveTactMapLimit(float p_x, float p_y);
	int ActionStackHaveCommand(int p_cmd);
	void AddActionAfterStop(int p_cmd, int p_a, int p_b, int p_c);
	int ChangeDirection(ANGLE p_dir);
	void ChangeArmy(int p_army);
	void CreateLink();
	void ChangeHp(int p_hp);
	int DestroyLink(VID* p_vid);
	SPRITE* SeekEnemy();
	int IsBetterEnemy(float p_dist, float p_bestDist, SPRITE* p_cand, SPRITE* p_best);
	static float NearDistanceTo(float p_x, float p_y);
	int CanSeekEnemy(SPRITE* p_sprite);
	int CanAttackThisSprite(SPRITE* p_sprite);
	int Attack(SPRITE* p_target);
	void Move(SPRITE* p_goal);
};

DECOMP_SIZE_ASSERT(SPRITE, 0x70)

inline int SPRITE::ReleaseRef()
{
	int refs = --m_noRef;
	if (refs <= 0) {
		if (refs < 0) {
			MYERROR::Error(::Error, "SPRITE %i", 4, "noRef at Release", refs, m_vid ? m_vid->m_idx : -1);
			return 0;
		}
		if (this)
			ScalarDeletingDestructor(1);
		return 0;
	}
	return refs;
}

inline PTR_SPRITE::PTR_SPRITE() { m_ptr = 0; }

inline PTR_SPRITE::~PTR_SPRITE()
{
	if (m_ptr)
		MYERROR::Error(::Error, "SPRITE %i", 10,
			"PTR_SPRITE with this sprite not clear", 0,
			m_ptr->m_vid ? m_ptr->m_vid->m_idx : -1);
}

inline PTR_SPRITE& PTR_SPRITE::operator=(SPRITE* p_sprite)
{
	if (m_ptr)
		m_ptr->ReleaseRef();
	m_ptr = p_sprite;
	return *this;
}

inline PTR_SPRITE::operator SPRITE*() const { return m_ptr; }
inline SPRITE* PTR_SPRITE::operator->() const { return m_ptr; }

#endif
