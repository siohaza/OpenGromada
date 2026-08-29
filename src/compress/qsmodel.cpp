#include "compress/qsmodel.h"

// FUNCTION: ALIEN 0x425690
QSMODEL::QSMODEL()
{
	m_cumFreq = 0;
	m_freq = 0;
	m_lookup = 0;
	Init(257, 12, 2000, 0);
}

// FUNCTION: ALIEN 0x4256c0
QSMODEL::~QSMODEL()
{
	if (m_cumFreq)
		operator delete(m_cumFreq);
	m_cumFreq = 0;
	if (m_freq)
		operator delete(m_freq);
	m_freq = 0;
	if (m_lookup)
		operator delete(m_lookup);
	m_lookup = 0;
}

// FUNCTION: ALIEN 0x425970
void QSMODEL::GetFreq(int p_sym, int* p_freq, int* p_cum)
{
	*p_freq = m_cumFreq[p_sym + 1] - (*p_cum = m_cumFreq[p_sym]);
}

// FUNCTION: ALIEN 0x4259a0
int QSMODEL::GetSym(int p_lcount)
{
	int lo;
	int hi;
	unsigned short* tmp;

	tmp = m_lookup + (p_lcount >> m_shift);
	lo = *tmp;
	hi = *(tmp + 1) + 1;
	while (lo + 1 < hi) {
		int mid = (hi + lo) >> 1;
		if (p_lcount < m_cumFreq[mid])
			hi = mid;
		else
			lo = mid;
	}
	return lo;
}

// FUNCTION: ALIEN 0x4259f0
void QSMODEL::Update(int p_sym)
{
	if (m_left <= 0)
		dorescale();
	--m_left;
	m_freq[p_sym] += m_incr;
}
