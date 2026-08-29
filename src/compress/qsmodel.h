#ifndef QSMODEL_H
#define QSMODEL_H

#include "util/decomp.h"

class QSMODEL {
public:
	int m_n; // 0x00
	int m_left; // 0x04
	int m_nextLeft; // 0x08
	int m_rescaleInterval; // 0x0c
	int m_targetRescale; // 0x10
	int m_incr; // 0x14
	int m_shift; // 0x18
	unsigned short* m_cumFreq; // 0x1c
	unsigned short* m_freq; // 0x20
	unsigned short* m_lookup; // 0x24

	QSMODEL();
	~QSMODEL();

	void dorescale();
	void Init(int p_symbols, int p_shift, int p_rescale, int* p_initial);
	void Reset(int* p_initial);
	int GetSym(int p_lcount);
	void GetFreq(int p_sym, int* p_freq, int* p_cum);
	void Update(int p_sym);
};

DECOMP_SIZE_ASSERT(QSMODEL, 0x28)

#endif
