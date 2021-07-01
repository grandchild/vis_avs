#include "g__lib.h"
#include "g__defs.h"
#include "c_comment.h"
#include "resource.h"
#include <windows.h>


int win32_dlgproc_comment(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
  C_THISCLASS* g_this = (C_THISCLASS*)g_current_render;

	switch (uMsg)
	{
		case WM_INITDIALOG:
      SetDlgItemText(hwndDlg,IDC_EDIT1,(char*)g_this->msgdata.c_str());
    return 1;
    case WM_COMMAND:
      if (LOWORD(wParam) == IDC_EDIT1 && HIWORD(wParam) == EN_CHANGE)
      {
        g_this->msgdata = string_from_dlgitem(hwndDlg,IDC_EDIT1);
      }
      return 0;
	}
	return 0;
}

