#ifndef ENGINE_H
#define ENGINE_H

#include "util/decomp.h"

class VID;

#include "sprite/list_sprite.h"
#include "sprite/r_dot.h"
#include "sprite/r_pos.h"
#include "game/unit.h"
#include "sprite/sprite.h"

class R_DOT_REF {
public:
	R_DOT_REF()
	{
		m_dot = 0;
		m_unk0x08 = 0;
	}

	R_DOT* m_dot; // 0x00
	int m_pos; // 0x04
	int m_unk0x08; // 0x08
	int m_link; // 0x0c

	R_DOT* LinkedDot() const { return m_dot ? m_dot->m_links[m_link].m_dot : 0; }
	int LinkDist() const { return m_dot ? m_dot->m_links[m_link].m_dist : 0; }
	int LinkBackLink() const { return m_dot ? m_dot->m_links[m_link].m_backLink : 0; }
	ANGLE LinkedDotAngle() const { return ANGLE(m_dot ? m_dot->m_links[m_link].m_dir.m_dir : 0); }
};

// VTABLE: ALIEN 0x47aabc

class ENGINE : public UNIT {
public:
	~ENGINE();
	virtual void* ScalarDeletingDestructor(unsigned int p_flags);

	ENGINE(VID* p_vid, float p_x, float p_y, float p_z, ANGLE p_dir, SPRITE* p_parent);

	int Action(int p_cmd, int p_a, int p_b, int p_c);
	void SetRDot();
	void ActNextCommandFighter();
	void DeleteAttackToEngine();
	void ReverseTrain();
	float Clash(ENGINE* p_other, int p_dir);
	int MT_IntersectingProcessing(const R_POS& p_pos, float* p_speed);
	void MoveTact();
	void DrawSecondaryInfo();
	void DrawGoalLine();

	int m_unk0x90; // 0x90
	ENGINE* m_prevEngine; // 0x94
	ENGINE* m_nextEngine; // 0x98
	SPRITE* m_commandOwner; // 0x9c
	R_DOT* m_commandDot; // 0xa0
	R_DOT* m_secondaryCommandDot; // 0xa4
	R_DOT* m_commandPathDot; // 0xa8
	int m_unk0xac; // 0xac
	float m_unk0xb0; // 0xb0
	int m_unk0xb4; // 0xb4
	R_DOT_REF m_curDotRef; // 0xb8
	R_DOT_REF m_lastDotRef; // 0xc8
	int m_unk0xd8; // 0xd8
	int m_unk0xdc; // 0xdc
	float m_unk0xe0; // 0xe0
	float m_unk0xe4; // 0xe4
	float m_unk0xe8; // 0xe8
	int m_unk0xec; // 0xec
	int m_unk0xf0; // 0xf0
	undefined4 m_unk0xf4; // 0xf4
	unsigned char m_pathLinks[0x9c4]; // 0xf8
	int m_noPathLinks; // 0xabc

	static SPRITE_LIST PathDots;

	static int NoStepForNotFound;
	static int NoStep;
	static int globaldeleting;

	ENGINE* NextEngine();
	void SetCommandToTrain(int p_cmd, int p_x, int p_y);
	void SetCommandToTrain(int p_cmd, SPRITE* p_goal, R_DOT* p_dot, R_DOT* p_secondaryDot);
	void DeletePointerToSprite(SPRITE* p_sprite);
	void Move(float p_x, float p_y, float p_z, int p_a5, int p_a6);
	void ReCalcMoveParameters();
	void BreakTrain(float p_x, float p_y);
	void MT_SpeedProcessing(float* p_speed);
	void CalcCoor();
	int ForceLink(ENGINE* p_other);
	void PullTail(R_DOT_REF* p_ref);
	void MT_PullCoordinates(R_POS* p_ref, float p_speed, int p_deceleration);
	void MoveEngineTact();
	void AddAmmoTact();
	int RepairByRepair(ENGINE* p_engine);
	void RepairTact();
	int HaveArmy(int p_army) const;
	int NeedRepairByRepair();
	int NeedAddAmmo();
	void ForceStop();
	int IsTailInFindedPath(R_DOT* p_dot);
	ENGINE* GetTrain();
	void Stop();
	ENGINE* FirstEngine();
	int ReachTheTarget();
	ENGINE* LastEngine();
	int InTrain(const SPRITE* p_sprite) const;
	int IsTouch(const ENGINE* p_engine, int p_flag);
	ENGINE* GetForwardIntersecting(int* p_touch);
	ENGINE* GetIntersecting();
	ENGINE* GetBadIntersecting();
	int CanLinkWithEngine(ENGINE* p_other);
	void CreatePathDots(R_DOT* p_dot, LIST_SPRITE* p_list, int p_vid);
	void CreatePathDots(R_DOT* p_dot);
	void ClearDotBusy();
	void SetDotBusy();
	int GetTrainLengthInRails();
	int TrainWeaponRange();
	void CheckPrevNextEngine();
};

DECOMP_SIZE_ASSERT(R_DOT_REF, 0x10)
DECOMP_SIZE_ASSERT(ENGINE, 0xac0)

#endif
