#include "g__lib.h"
#include "g__defs.h"
#include "c_grain.h"
#include "resource.h"
#include <windows.h>
#include <commctrl.h>


int win32_dlgproc_grain(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM)
{
	C_THISCLASS* g_ConfigThis = (C_THISCLASS*)g_current_render;
switch (uMsg)
	{
	case WM_INITDIALOG:
		SendDlgItemMessage(hwndDlg, IDC_MAX, TBM_SETTICFREQ, 10, 0);
		SendDlgItemMessage(hwndDlg, IDC_MAX, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
		SendDlgItemMessage(hwndDlg, IDC_MAX, TBM_SETPOS, TRUE, g_ConfigThis->smax);
        if (g_ConfigThis->enabled) CheckDlgButton(hwndDlg,IDC_CHECK1,BST_CHECKED);
        if (g_ConfigThis->staticgrain) CheckDlgButton(hwndDlg,IDC_STATGRAIN,BST_CHECKED);
        if (g_ConfigThis->blend) CheckDlgButton(hwndDlg,IDC_ADDITIVE,BST_CHECKED);
        if (g_ConfigThis->blendavg) CheckDlgButton(hwndDlg,IDC_5050,BST_CHECKED);
        if (!g_ConfigThis->blend && !g_ConfigThis->blendavg)
		   CheckDlgButton(hwndDlg,IDC_REPLACE,BST_CHECKED);
		return 1;
	case WM_NOTIFY:
		{
		if (LOWORD(wParam) == IDC_MAX)
			g_ConfigThis->smax = SendDlgItemMessage(hwndDlg, IDC_MAX, TBM_GETPOS, 0, 0);
			return 0;
		}
	case WM_COMMAND:
	  if ((LOWORD(wParam) == IDC_CHECK1) ||
	     (LOWORD(wParam) == IDC_STATGRAIN) ||
	     (LOWORD(wParam) == IDC_ADDITIVE) ||
	     (LOWORD(wParam) == IDC_REPLACE) ||
	     (LOWORD(wParam) == IDC_5050) )
			{
			g_ConfigThis->enabled=IsDlgButtonChecked(hwndDlg,IDC_CHECK1)?1:0;
			g_ConfigThis->blend=IsDlgButtonChecked(hwndDlg,IDC_ADDITIVE)?1:0;
			g_ConfigThis->blendavg=IsDlgButtonChecked(hwndDlg,IDC_5050)?1:0;
			g_ConfigThis->staticgrain=IsDlgButtonChecked(hwndDlg,IDC_STATGRAIN)?1:0;
			}
	  return 0;
	}
return 0;
}

