#include "g__lib.h"
#include "g__defs.h"
#include "c_picture.h"
#include "resource.h"
#include <windows.h>
#include <commctrl.h>


static void EnableWindows(HWND hwndDlg, C_THISCLASS* config)
{
	EnableWindow(GetDlgItem(hwndDlg,IDC_PERSIST),config->adapt);
	EnableWindow(GetDlgItem(hwndDlg,IDC_PERSIST_TITLE),config->adapt);
	EnableWindow(GetDlgItem(hwndDlg,IDC_PERSIST_TEXT1),config->adapt);
	EnableWindow(GetDlgItem(hwndDlg,IDC_PERSIST_TEXT2),config->adapt);
	EnableWindow(GetDlgItem(hwndDlg,IDC_X_RATIO),config->ratio);
	EnableWindow(GetDlgItem(hwndDlg,IDC_Y_RATIO),config->ratio);
}

int win32_dlgproc_picture(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM)
{
	C_THISCLASS* g_ConfigThis = (C_THISCLASS*)g_current_render;
	switch (uMsg)
	{
		case WM_INITDIALOG:
			SendDlgItemMessage(hwndDlg, IDC_PERSIST, TBM_SETRANGE, TRUE, MAKELONG(0, 32));
			SendDlgItemMessage(hwndDlg, IDC_PERSIST, TBM_SETPOS, TRUE, g_ConfigThis->persist);
      if (g_ConfigThis->enabled) CheckDlgButton(hwndDlg,IDC_ENABLED,BST_CHECKED);
      if (g_ConfigThis->blend) CheckDlgButton(hwndDlg,IDC_ADDITIVE,BST_CHECKED);
      if (g_ConfigThis->blendavg) CheckDlgButton(hwndDlg,IDC_5050,BST_CHECKED);
      if (g_ConfigThis->adapt) CheckDlgButton(hwndDlg,IDC_ADAPT,BST_CHECKED);
      if (!g_ConfigThis->adapt && !g_ConfigThis->blend && !g_ConfigThis->blendavg)
			CheckDlgButton(hwndDlg,IDC_REPLACE,BST_CHECKED);
      if (g_ConfigThis->ratio) CheckDlgButton(hwndDlg,IDC_RATIO,BST_CHECKED);
      if (!g_ConfigThis->axis_ratio) CheckDlgButton(hwndDlg,IDC_X_RATIO,BST_CHECKED);
      if (g_ConfigThis->axis_ratio) CheckDlgButton(hwndDlg,IDC_Y_RATIO,BST_CHECKED);
			EnableWindows(hwndDlg, g_ConfigThis);
	    loadComboBox(GetDlgItem(hwndDlg,OBJ_COMBO),"*.BMP",g_ConfigThis->ascName);
			return 1;
		case WM_NOTIFY:
			if (LOWORD(wParam) == IDC_PERSIST)
				g_ConfigThis->persist = SendDlgItemMessage(hwndDlg, IDC_PERSIST, TBM_GETPOS, 0, 0);
			return 0;
    case WM_COMMAND:
		  if ((LOWORD(wParam) == IDC_ENABLED) ||
	     (LOWORD(wParam) == IDC_ADDITIVE) ||
	     (LOWORD(wParam) == IDC_REPLACE) ||
	     (LOWORD(wParam) == IDC_ADAPT) ||
			 (LOWORD(wParam) == IDC_5050) ) {
				g_ConfigThis->enabled=IsDlgButtonChecked(hwndDlg,IDC_ENABLED)?1:0;
				g_ConfigThis->blend=IsDlgButtonChecked(hwndDlg,IDC_ADDITIVE)?1:0;
				g_ConfigThis->blendavg=IsDlgButtonChecked(hwndDlg,IDC_5050)?1:0;
				g_ConfigThis->adapt=IsDlgButtonChecked(hwndDlg,IDC_ADAPT)?1:0;
				EnableWindows(hwndDlg, g_ConfigThis);
			}
			if(LOWORD(wParam) == IDC_RATIO || LOWORD(wParam) == IDC_X_RATIO || LOWORD(wParam) == IDC_Y_RATIO) {
				g_ConfigThis->ratio=IsDlgButtonChecked(hwndDlg,IDC_RATIO)?1:0;
				g_ConfigThis->axis_ratio=IsDlgButtonChecked(hwndDlg,IDC_Y_RATIO)?1:0;
				g_ConfigThis->lastWidth=-1; g_ConfigThis->lastHeight=-1;
				EnableWindows(hwndDlg, g_ConfigThis);
			}
		  if (HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == OBJ_COMBO) // handle clicks to combo box
			  {
				int sel = SendDlgItemMessage(hwndDlg, OBJ_COMBO, CB_GETCURSEL, 0, 0);
				if (sel != -1)
					{
					SendDlgItemMessage(hwndDlg, OBJ_COMBO, CB_GETLBTEXT, sel, (LPARAM)g_ConfigThis->ascName);
					if (*(g_ConfigThis->ascName))
						g_ConfigThis->loadPicture(g_ConfigThis->ascName);
					}
				}
			return 0;
		case WM_HSCROLL:
			return 0;
	}
	return 0;
}

