#include "g__lib.h"
#include "g__defs.h"
#include "c_blur.h"
#include "resource.h"
#include <windows.h>


int win32_dlgproc_blur(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM)
{
  C_THISCLASS* g_this = (C_THISCLASS*)g_current_render;
	switch (uMsg)
	{
		case WM_INITDIALOG:
      if (g_this->enabled==2) CheckDlgButton(hwndDlg,IDC_RADIO3,BST_CHECKED);
      else if (g_this->enabled==3) CheckDlgButton(hwndDlg,IDC_RADIO4,BST_CHECKED);
      else if (g_this->enabled) CheckDlgButton(hwndDlg,IDC_RADIO2,BST_CHECKED);
      else CheckDlgButton(hwndDlg,IDC_RADIO1,BST_CHECKED);
      if (g_this->roundmode==0) CheckDlgButton(hwndDlg,IDC_ROUNDDOWN,BST_CHECKED);
      else CheckDlgButton(hwndDlg,IDC_ROUNDUP,BST_CHECKED);
			return 1;
    case WM_COMMAND:
      if (LOWORD(wParam) == IDC_RADIO1)
        if (IsDlgButtonChecked(hwndDlg,IDC_RADIO1))
          g_this->enabled=0;
      if (LOWORD(wParam) == IDC_RADIO2)
        if (IsDlgButtonChecked(hwndDlg,IDC_RADIO2))
          g_this->enabled=1;
      if (LOWORD(wParam) == IDC_RADIO3)
        if (IsDlgButtonChecked(hwndDlg,IDC_RADIO3))
          g_this->enabled=2;
      if (LOWORD(wParam) == IDC_RADIO4)
        if (IsDlgButtonChecked(hwndDlg,IDC_RADIO4))
          g_this->enabled=3;
      if (LOWORD(wParam) == IDC_ROUNDUP)
        if (IsDlgButtonChecked(hwndDlg,IDC_ROUNDUP))
          g_this->roundmode=1;
      if (LOWORD(wParam) == IDC_ROUNDDOWN)
        if (IsDlgButtonChecked(hwndDlg,IDC_ROUNDDOWN))
          g_this->roundmode=0;
      return 0;
    return 0;
	}
	return 0;
}

