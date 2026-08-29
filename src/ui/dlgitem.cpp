#include "ui/dlgitem.h"

#include <string.h>
#include <windows.h>

// FUNCTION: ALIEN 0x404b70
char** DLGITEM::GetText(char** p_out)
{
	char buffer[512];
	GetDlgItemTextA((HWND) m_hDlg, m_id, buffer, 512);
	if (buffer && buffer[0]) {
		unsigned int len = strlen(buffer);
		char* buf = (char*) operator new((len & 0xfffffff0) + 16);
		*p_out = buf;
		memcpy(buf, buffer, len);
		(*p_out)[len] = 0;
		return p_out;
	}
	*p_out = STRING::EMPTY;
	return p_out;
}

// FUNCTION: ALIEN 0x42f490
long DLGITEM::SendMsg(unsigned int p_msg, unsigned int p_wParam, long p_lParam)
{
	return SendDlgItemMessageA((HWND) m_hDlg, m_id, p_msg, p_wParam, p_lParam);
}
