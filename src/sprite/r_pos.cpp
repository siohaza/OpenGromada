#include "sprite/r_pos.h"

#include <math.h>

#include "game/engine.h"
#include "sprite/r_dot.h"
#include "game/map.h"
#include "sprite/r_map.h"

extern int dots_num;
extern int dots_finded_for_return[95];
#include "util/myerror.h"
#include "util/stream.h"

inline ANGLE PosAngle(const R_POS* p_pos)
{
	return p_pos->m_dot ? p_pos->m_dot->m_links[p_pos->m_link].m_dir : ANGLE(0);
}

// FUNCTION: ALIEN 0x43b8e0
int R_POS::DoStep(R_DOT* p_goal, SPRITE* p_target, ENGINE* p_engine)
{
	R_DOT* prev = m_dot;
	ANGLE dir = m_dot ? m_dot->m_links[m_link].m_dir : ANGLE(0);
	int segLen = m_dot ? m_dot->m_links[m_link].m_dist : 0;
	if (*(int*) &m_unk0x04[0] > segLen) {
		int len = m_dot ? m_dot->m_links[m_link].m_dist : 0;
		*(int*) &m_unk0x04[0] -= len;
	}
	m_dot = m_dot ? m_dot->m_links[m_link].m_dot : 0;

	if (p_goal || p_target) {
		RailMap.PrepareForFindDot(p_goal, p_target, (p_engine->m_flag >> 2) & 0x1f, p_engine);
		float dx;
		float dy;
		if (p_goal) {
			dx = p_engine->m_x;
			dx -= p_goal->m_x;
			dx = (float) fabs(dx);
			dy = p_engine->m_y;
			dy -= p_goal->m_y;
		}
		else {
			dx = p_engine->m_x;
			dx -= p_target->m_x;
			dx = (float) fabs(dx);
			dy = p_engine->m_y;
			dy -= p_target->m_y;
		}
		dy = (float) fabs(dy);
		float dist = (dx > dy) ? dx + dy * 0.5f : dx * 0.5f + dy;
		R_DOT::MaxPathDots = (int) (dist * 0.1f);
		m_link = m_dot->FindNewDot(m_dot->GetLink_idx(prev),
			prev->m_links[prev->GetLink_idx(m_dot)].m_dir);
		if (R_DOT::NoStep == 0xffff) {
			RailMap.PrepareForFindDot(p_goal, p_target, (p_engine->m_flag >> 2) & 0x1f, p_engine);
			m_link = m_dot->FindNewDot(m_dot->GetLink_idx(prev),
				prev->m_links[prev->GetLink_idx(m_dot)].m_dir);
		}
	}
	else {
		m_link = -1;
	}

	int link = m_link;
	if (link >= 0) {
		unsigned char newDir = (m_dot ? m_dot->m_links[link].m_dir : ANGLE(0)).m_dir;
		unsigned char d1 = (unsigned char) (dir.m_dir - newDir);
		unsigned char d2 = (unsigned char) (newDir - dir.m_dir);
		unsigned char delta = (d1 < d2) ? d1 : d2;
		if (delta > 0x1f) {
			R_DOT* avoid = m_dot ? m_dot->m_links[link].m_dot : 0;
			m_link = m_dot->GetLink_dir(dir);
			if (R_DOT::train_length_in_rails) {
				dots_num = 0;
				if (!m_dot->TryToFindRailsForReturn(R_DOT::train_length_in_rails, prev, avoid,
						p_engine, dir)) {
					R_DOT::NoStep = 0xffff;
				}
				else {
					m_link = dots_finded_for_return[0];
				}
			}
			if (p_engine && R_DOT::FindedDot && p_engine->IsTailInFindedPath(m_dot))
				R_DOT::NoStep = -R_DOT::NoStep;
		}
	}
	else {
		m_link = m_dot->GetLink_dir(dir);
	}

	R_DOT* nd = m_dot;
	int newSeg = nd ? nd->m_links[m_link].m_dist : 0;
	if (*(int*) &m_unk0x04[0] > newSeg)
		*(int*) &m_unk0x04[0] = nd ? nd->m_links[m_link].m_dist : 0;
	return R_DOT::NoStep;
}

// FUNCTION: ALIEN 0x43d750
int R_POS::NoStepToTarget(R_DOT* p_goal, SPRITE* p_target, int p_command, ENGINE* p_eng)
{
	if (!p_eng)
		return 0x10000;
	if (!p_goal && !p_target)
		return 0x10000;
	RailMap.PrepareForFindDot(p_goal, p_target, p_command, p_eng);
	int step = GetLinkedDot()->FindNewDot(GetLinkedDot()->GetLink_idx(m_dot),
										  PosAngle(this));
	if (step >= 0) {
		R_DOT* linked = m_dot ? m_dot->m_links[m_link].m_dot : 0;
		unsigned char cur = PosAngle(this).m_dir;
		const ANGLE* newDir = &linked->m_links[step].m_dir;
		unsigned char d1 = cur - newDir->m_dir;
		unsigned char d2 = newDir->m_dir - cur;
		if (d1 < d2)
			d2 = d1;
		if (d2 > 0x1f) {
			if (R_DOT::FindedDot) {
				if (p_eng->IsTailInFindedPath(m_dot ? m_dot->m_links[m_link].m_dot : 0)) {
					R_DOT::NoStep = -R_DOT::NoStep;
					return R_DOT::NoStep;
				}
			}
		}
	}
	return R_DOT::NoStep;
}

// FUNCTION: ALIEN 0x43d870
R_DOT* R_POS::GetLinkedDot()
{
	if (m_dot)
		return m_dot->m_links[m_link].m_dot;
	return 0;
}

// FUNCTION: ALIEN 0x43d890
int R_POS::Write(STREAM* p_stream) const
{
	p_stream->Write(&m_dot->m_x, 2);
	p_stream->Write(&m_dot->m_y, 2);
	p_stream->Write(&m_dot->m_z, 2);
	p_stream->Write(&(m_dot ? m_dot->m_links[m_link].m_dot : 0)->m_x, 2);
	p_stream->Write(&(m_dot ? m_dot->m_links[m_link].m_dot : 0)->m_y, 2);
	p_stream->Write(&(m_dot ? m_dot->m_links[m_link].m_dot : 0)->m_z, 2);
	int buf = *(int*) m_unk0x04 << 16;
	return p_stream->Write(&buf, 4);
}

// FUNCTION: ALIEN 0x43d960
int R_POS::Read(STREAM* p_stream)
{
	short x;
	short y;
	short z;
	p_stream->Read(&x, 2);
	p_stream->Read(&y, 2);
	p_stream->Read(&z, 2);
	m_dot = RailMap.GetDot(x, y, z);
	p_stream->Read(&x, 2);
	p_stream->Read(&y, 2);
	p_stream->Read(&z, 2);
	if (m_dot)
		m_link = m_dot->GetLink_idx(RailMap.GetDot(x, y, z));
	if (m_link < 0)
		MYERROR::Log(::Error,
			// STRING: ALIEN 0x484324
			"!!!ERROR!!!RAIL: Read error");
	int result = p_stream->Read((int*) m_unk0x04, 4);
	*(int*) m_unk0x04 >>= 16;
	return result;
}

// FUNCTION: ALIEN 0x4517d0
ANGLE R_POS::GetAngle() const
{
	return m_dot ? m_dot->m_links[m_link].m_dir : ANGLE(0);
}
