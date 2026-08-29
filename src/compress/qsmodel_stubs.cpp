#include "compress/qsmodel.h"

#include <stdio.h>
#include <stdlib.h>

// FUNCTION: ALIEN 0x425710
void QSMODEL::dorescale()
{
	int i;
	int cf;
	int missing;

	if (m_nextLeft) {
		++m_incr;
		m_left = m_nextLeft;
		m_nextLeft = 0;
		return;
	}
	if (m_rescaleInterval < m_targetRescale) {
		m_rescaleInterval *= 2;
		if (m_rescaleInterval > m_targetRescale)
			m_rescaleInterval = m_targetRescale;
	}
	cf = missing = m_cumFreq[m_n];
	for (i = m_n - 1; i; --i) {
		int tmp = m_freq[i];
		cf -= tmp;
		m_cumFreq[i] = (unsigned short) cf;
		tmp = (tmp | 2) >> 1;
		missing -= tmp;
		m_freq[i] = (unsigned short) tmp;
	}
	if (cf != m_freq[0]) {

		fprintf(stderr,
			// STRING: ALIEN 0x48366c
			"BUG: rescaling left %d total frequency\n", m_cumFreq);
		exit(1);
	}
	m_freq[0] = (m_freq[0] | 2) >> 1;
	missing -= m_freq[0];
	m_incr = missing / m_rescaleInterval;
	m_nextLeft = missing % m_rescaleInterval;
	m_left = m_rescaleInterval - m_nextLeft;
	if (m_lookup) {
		i = m_n;
		while (i) {
			int start;
			int end;
			end = (m_cumFreq[i] - 1) >> m_shift;
			--i;
			start = m_cumFreq[i] >> m_shift;
			while (start <= end) {
				m_lookup[start] = (unsigned short) i;
				++start;
			}
		}
	}
}

// FUNCTION: ALIEN 0x425820
void QSMODEL::Init(int p_symbols, int p_shift, int p_rescale, int* p_initial)
{
	m_targetRescale = p_rescale;
	m_n = p_symbols;
	m_shift = p_shift - 7;
	if (m_shift < 0)
		m_shift = 0;
	if (m_cumFreq)
		operator delete(m_cumFreq);
	m_cumFreq = (unsigned short*) operator new(2 * p_symbols + 2);
	if (m_freq)
		operator delete(m_freq);
	m_freq = (unsigned short*) operator new(2 * p_symbols + 2);
	if (m_lookup)
		operator delete(m_lookup);
	m_lookup = (unsigned short*) operator new(0x102);
	m_cumFreq[p_symbols] = 1 << p_shift;
	m_cumFreq[0] = 0;
	if (m_lookup)
		m_lookup[128] = p_symbols - 1;
	Reset(p_initial);
}

// FUNCTION: ALIEN 0x4258e0
void QSMODEL::Reset(int* p_initial)
{
	int i;
	int end;
	int initval;

	m_rescaleInterval = (m_n | 0x20) >> 4;
	m_nextLeft = 0;
	if (!p_initial) {
		initval = m_cumFreq[m_n] / m_n;
		end = m_cumFreq[m_n] % m_n;
		for (i = 0; i < end; ++i)
			m_freq[i] = initval + 1;
		for (; i < m_n; ++i)
			m_freq[i] = initval;
	}
	else {
		for (i = 0; i < m_n; ++i)
			m_freq[i] = p_initial[i];
	}
	dorescale();
}
