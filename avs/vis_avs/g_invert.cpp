#include "g__lib.h"
#include "g__defs.h"
#include "c_invert.h"
#include "resource.h"
#include <windows.h>


int win32_dlgproc_invert(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM)
{
  C_THISCLASS* g_ConfigThis = (C_THISCLASS*)g_current_render;
  switch (uMsg) {
  	case WM_INITDIALOG:
      if (g_ConfigThis->enabled)
        CheckDlgButton(hwndDlg,IDC_CHECK1,BST_CHECKED);
  		return 1;
  	case WM_COMMAND:
      if (LOWORD(wParam) == IDC_CHECK1)
        g_ConfigThis->enabled=IsDlgButtonChecked(hwndDlg,IDC_CHECK1)?1:0;
	}
  return 0;
}

