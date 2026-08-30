#include "sprite/act.h"

#include "util/myerror.h"

void LIST_ACT::SetNumber(int p_n)
{
	m_n = p_n;
	if (p_n > m_max) {
		Expand(p_n);
	}
}

// FUNCTION: ALIEN 0x405ef0
void LIST_ACT::InsertFirst(ACT p_act)
{
	int max = m_max;
	if (m_n >= max) {
		int newMax = 2 * max + 4;
		if (newMax > max) {
			ACT* oldData = m_data;
			m_data = new ACT[newMax];
			if (!m_data) {
				MYERROR::LogExit(::Error, "!!!ERROR!!!::LIST: Not enough memory %i", newMax);
			}
			if (oldData) {
				for (int i = 0; i < m_max; ++i) {
					m_data[i] = oldData[i];
				}
				delete[] oldData;
			}
			m_max = newMax;
		}
	}
	int n = m_n++;
	while (n) {
		m_data[n] = m_data[n - 1];
		--n;
	}
	m_data[0] = p_act;
}

// FUNCTION: ALIEN 0x446930
void LIST_ACT::Insert(ACT p_act)
{
	int max = m_max;
	int n = m_n;
	if (n >= max) {
		int newMax = 2 * max + 4;
		if (newMax > max) {
			ACT* oldData = m_data;
			m_data = new ACT[newMax];
			if (!m_data) {
				MYERROR::LogExit(::Error, "!!!ERROR!!!::LIST: Not enough memory %i", newMax);
			}
			if (oldData) {
				for (int i = 0; i < m_max; ++i) {
					m_data[i] = oldData[i];
				}
				delete[] oldData;
			}
			m_max = newMax;
		}
	}
	ACT* item = &m_data[m_n];
	*item = p_act;
	++m_n;
}

// FUNCTION: ALIEN 0x446a10
void LIST_ACT::InsertBefore(int p_idx, ACT p_act)
{
	int max = m_max;
	if (m_n >= max) {
		int newMax = 2 * max + 4;
		if (newMax > max) {
			ACT* oldData = m_data;
			m_data = new ACT[newMax];
			if (!m_data) {
				MYERROR::LogExit(::Error, "!!!ERROR!!!::LIST: Not enough memory %i", newMax);
			}
			if (oldData) {
				for (int i = 0; i < m_max; ++i) {
					m_data[i] = oldData[i];
				}
				delete[] oldData;
			}
			m_max = newMax;
		}
	}
	int n = m_n++;
	while (n > p_idx) {
		m_data[n] = m_data[n - 1];
		--n;
	}
	m_data[p_idx] = p_act;
}

// FUNCTION: ALIEN 0x446be0
void LIST_ACT::Expand(int p_max)
{
	int max = p_max;
	if (p_max > m_max) {
		ACT* oldData = m_data;
		ACT* newData = new ACT[p_max];
		m_data = newData;
		if (!newData) {
			MYERROR::LogExit(::Error, "!!!ERROR!!!::LIST: Not enough memory %i", p_max);
		}
		if (oldData) {
			int i = 0;
			if (m_max > 0) {
				do {
					m_data[i] = oldData[i];
					++i;
				} while (i < m_max);
			}
			delete[] oldData;
		}
		m_max = max;
	}
}
