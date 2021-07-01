#include "g__lib.h"
#include "g__defs.h"
#include "c_avi.h"
#include "resource.h"
#include <windows.h>
#include <commctrl.h>


static void EnableWindows(HWND hwndDlg, C_THISCLASS* config)
{
	EnableWindow(GetDlgItem(hwndDlg,IDC_PERSIST),config->adapt);
	EnableWindow(GetDlgItem(hwndDlg,IDC_PERSIST_TITLE),config->adapt);
}

int win32_dlgproc_avi(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
	C_THISCLASS* g_ConfigThis = (C_THISCLASS*)g_current_render;
switch (uMsg)
	{
	case WM_INITDIALOG:
		SendDlgItemMessage(hwndDlg, IDC_PERSIST, TBM_SETRANGE, TRUE, MAKELONG(0, 32));
		SendDlgItemMessage(hwndDlg, IDC_PERSIST, TBM_SETPOS, TRUE, g_ConfigThis->persist);
		SendDlgItemMessage(hwndDlg, IDC_SPEED, TBM_SETTICFREQ, 50, 0);
		SendDlgItemMessage(hwndDlg, IDC_SPEED, TBM_SETRANGE, TRUE, MAKELONG(0, 1000));
		SendDlgItemMessage(hwndDlg, IDC_SPEED, TBM_SETPOS, TRUE, g_ConfigThis->speed);
    if (g_ConfigThis->enabled) CheckDlgButton(hwndDlg,IDC_CHECK1,BST_CHECKED);
    if (g_ConfigThis->blend) CheckDlgButton(hwndDlg,IDC_ADDITIVE,BST_CHECKED);
    if (g_ConfigThis->blendavg) CheckDlgButton(hwndDlg,IDC_5050,BST_CHECKED);
    if (g_ConfigThis->adapt) CheckDlgButton(hwndDlg,IDC_ADAPT,BST_CHECKED);
    if (!g_ConfigThis->adapt && !g_ConfigThis->blend && !g_ConfigThis->blendavg)
		CheckDlgButton(hwndDlg,IDC_REPLACE,BST_CHECKED);
		EnableWindows(hwndDlg, g_ConfigThis);
	  loadComboBox(GetDlgItem(hwndDlg,OBJ_COMBO),"*.AVI",g_ConfigThis->ascName);
		return 1;
	case WM_NOTIFY:
		{
		if (LOWORD(wParam) == IDC_PERSIST)
			g_ConfigThis->persist = SendDlgItemMessage(hwndDlg, IDC_PERSIST, TBM_GETPOS, 0, 0);
		if (LOWORD(wParam) == IDC_SPEED)
			g_ConfigThis->speed = SendDlgItemMessage(hwndDlg, IDC_SPEED, TBM_GETPOS, 0, 0);
		}
	case WM_COMMAND:
	  if ((LOWORD(wParam) == IDC_CHECK1) ||
	     (LOWORD(wParam) == IDC_ADDITIVE) ||
	     (LOWORD(wParam) == IDC_REPLACE) ||
	     (LOWORD(wParam) == IDC_ADAPT) ||
	     (LOWORD(wParam) == IDC_5050) )
			{
			g_ConfigThis->enabled=IsDlgButtonChecked(hwndDlg,IDC_CHECK1)?1:0;
			g_ConfigThis->blend=IsDlgButtonChecked(hwndDlg,IDC_ADDITIVE)?1:0;
			g_ConfigThis->blendavg=IsDlgButtonChecked(hwndDlg,IDC_5050)?1:0;
			g_ConfigThis->adapt=IsDlgButtonChecked(hwndDlg,IDC_ADAPT)?1:0;
			EnableWindows(hwndDlg, g_ConfigThis);
			}
	  if (HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == OBJ_COMBO) // handle clicks to combo box
		  {
		  int sel = SendDlgItemMessage(hwndDlg, OBJ_COMBO, CB_GETCURSEL, 0, 0);
		  if (sel != -1)
				{
				SendDlgItemMessage(hwndDlg, OBJ_COMBO, CB_GETLBTEXT, sel, (LPARAM)g_ConfigThis->ascName);
				if (*(g_ConfigThis->ascName))
					g_ConfigThis->loadAvi(g_ConfigThis->ascName);
				}
		  }
	  return 0;
	}
return 0;
}

