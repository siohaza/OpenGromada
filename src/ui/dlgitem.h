#ifndef DLGITEM_H
#define DLGITEM_H

#include "util/string.h"

class DLGITEM {
public:
	void* m_hDlg; // 0x00
	int m_id; // 0x04

	char** GetText(char** p_out);
	long SendMsg(unsigned int p_msg, unsigned int p_wParam, long p_lParam);
};

#endif
