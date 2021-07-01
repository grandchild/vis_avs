#include "g__lib.h"
#include "g__defs.h"
#include "c_interf.h"
#include "resource.h"
#include <windows.h>
#include <commctrl.h>


int win32_dlgproc_interferences(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
	C_THISCLASS* g_ConfigThis = (C_THISCLASS*)g_current_render;
switch (uMsg)
	{
	case WM_INITDIALOG:
				SendDlgItemMessage(hwndDlg, IDC_NPOINTS, TBM_SETTICFREQ, 1, 0);
				SendDlgItemMessage(hwndDlg, IDC_NPOINTS, TBM_SETRANGE, TRUE, MAKELONG(0, 8));
				SendDlgItemMessage(hwndDlg, IDC_NPOINTS, TBM_SETPOS, TRUE, g_ConfigThis->nPoints);
				SendDlgItemMessage(hwndDlg, IDC_ALPHA, TBM_SETTICFREQ, 16, 0);
				SendDlgItemMessage(hwndDlg, IDC_ALPHA, TBM_SETRANGE, TRUE, MAKELONG(1, 255));
				SendDlgItemMessage(hwndDlg, IDC_ALPHA, TBM_SETPOS, TRUE, g_ConfigThis->alpha);
				SendDlgItemMessage(hwndDlg, IDC_DISTANCE, TBM_SETTICFREQ, 8, 0);
				SendDlgItemMessage(hwndDlg, IDC_DISTANCE, TBM_SETRANGE, TRUE, MAKELONG(1, 64));
				SendDlgItemMessage(hwndDlg, IDC_DISTANCE, TBM_SETPOS, TRUE, g_ConfigThis->distance);
				SendDlgItemMessage(hwndDlg, IDC_ROTATE, TBM_SETTICFREQ, 2, 0);
				SendDlgItemMessage(hwndDlg, IDC_ROTATE, TBM_SETRANGE, TRUE, MAKELONG(0, 64));
				SendDlgItemMessage(hwndDlg, IDC_ROTATE, TBM_SETPOS, TRUE, g_ConfigThis->rotationinc*-1+32);
				SendDlgItemMessage(hwndDlg, IDC_ALPHA2, TBM_SETTICFREQ, 16, 0);
				SendDlgItemMessage(hwndDlg, IDC_ALPHA2, TBM_SETRANGE, TRUE, MAKELONG(1, 255));
				SendDlgItemMessage(hwndDlg, IDC_ALPHA2, TBM_SETPOS, TRUE, g_ConfigThis->alpha2);
				SendDlgItemMessage(hwndDlg, IDC_DISTANCE2, TBM_SETTICFREQ, 8, 0);
				SendDlgItemMessage(hwndDlg, IDC_DISTANCE2, TBM_SETRANGE, TRUE, MAKELONG(1, 64));
				SendDlgItemMessage(hwndDlg, IDC_DISTANCE2, TBM_SETPOS, TRUE, g_ConfigThis->distance2);
				SendDlgItemMessage(hwndDlg, IDC_ROTATE2, TBM_SETTICFREQ, 2, 0);
				SendDlgItemMessage(hwndDlg, IDC_ROTATE2, TBM_SETRANGE, TRUE, MAKELONG(0, 64));
				SendDlgItemMessage(hwndDlg, IDC_ROTATE2, TBM_SETPOS, TRUE, g_ConfigThis->rotationinc2*-1+32);
				SendDlgItemMessage(hwndDlg, IDC_INITROT, TBM_SETTICFREQ, 16, 0);
				SendDlgItemMessage(hwndDlg, IDC_INITROT, TBM_SETRANGE, TRUE, MAKELONG(0, 255));
				SendDlgItemMessage(hwndDlg, IDC_INITROT, TBM_SETPOS, TRUE, 255-g_ConfigThis->rotation);
				SendDlgItemMessage(hwndDlg, IDC_SPEED, TBM_SETTICFREQ, 1000, 0);
				SendDlgItemMessage(hwndDlg, IDC_SPEED, TBM_SETRANGE, TRUE, MAKELONG(1, 128));
				SendDlgItemMessage(hwndDlg, IDC_SPEED, TBM_SETPOS, TRUE, (int)(g_ConfigThis->speed*100));
        if (g_ConfigThis->enabled) CheckDlgButton(hwndDlg,IDC_CHECK1,BST_CHECKED);
        if (g_ConfigThis->blend) CheckDlgButton(hwndDlg,IDC_ADDITIVE,BST_CHECKED);
        if (g_ConfigThis->blendavg) CheckDlgButton(hwndDlg,IDC_5050,BST_CHECKED);
        if (!g_ConfigThis->blend && !g_ConfigThis->blendavg)
			   CheckDlgButton(hwndDlg,IDC_REPLACE,BST_CHECKED);
			  EnableWindow(GetDlgItem(hwndDlg, IDC_RGB), (g_ConfigThis->nPoints == 3 || g_ConfigThis->nPoints==6));
		    CheckDlgButton(hwndDlg,IDC_RGB,g_ConfigThis->rgb ? BST_CHECKED: BST_UNCHECKED);
		    CheckDlgButton(hwndDlg,IDC_ONBEAT,g_ConfigThis->onbeat ? BST_CHECKED: BST_UNCHECKED);
		return 1;
	case WM_COMMAND:
	  if ((LOWORD(wParam) == IDC_CHECK1) ||
	     (LOWORD(wParam) == IDC_RGB) ||
	     (LOWORD(wParam) == IDC_ONBEAT) ||
	     (LOWORD(wParam) == IDC_ADDITIVE) ||
	     (LOWORD(wParam) == IDC_REPLACE) ||
	     (LOWORD(wParam) == IDC_5050) )
			{
			g_ConfigThis->enabled=IsDlgButtonChecked(hwndDlg,IDC_CHECK1)?1:0;
			g_ConfigThis->blend=IsDlgButtonChecked(hwndDlg,IDC_ADDITIVE)?1:0;
			g_ConfigThis->blendavg=IsDlgButtonChecked(hwndDlg,IDC_5050)?1:0;
			g_ConfigThis->rgb=IsDlgButtonChecked(hwndDlg,IDC_RGB)?1:0;
			g_ConfigThis->onbeat=IsDlgButtonChecked(hwndDlg,IDC_ONBEAT)?1:0;
			}
		return 0;
	case WM_NOTIFY:
		if (LOWORD(wParam) == IDC_NPOINTS)
		  {
			g_ConfigThis->nPoints = SendDlgItemMessage(hwndDlg, IDC_NPOINTS, TBM_GETPOS, 0, 0);
			EnableWindow(GetDlgItem(hwndDlg, IDC_RGB), (g_ConfigThis->nPoints == 3 || g_ConfigThis->nPoints==6));
			}
		if (LOWORD(wParam) == IDC_ALPHA)
			g_ConfigThis->alpha = SendDlgItemMessage(hwndDlg, IDC_ALPHA, TBM_GETPOS, 0, 0);
		if (LOWORD(wParam) == IDC_DISTANCE)
			g_ConfigThis->distance = SendDlgItemMessage(hwndDlg, IDC_DISTANCE, TBM_GETPOS, 0, 0);
		if (LOWORD(wParam) == IDC_ROTATE)
			g_ConfigThis->rotationinc= -1*(SendDlgItemMessage(hwndDlg, IDC_ROTATE, TBM_GETPOS, 0, 0)-32);
		if (LOWORD(wParam) == IDC_INITROT)
			g_ConfigThis->rotation = 255-SendDlgItemMessage(hwndDlg, IDC_INITROT, TBM_GETPOS, 0, 0);
		if (LOWORD(wParam) == IDC_SPEED)
			g_ConfigThis->speed = (float)SendDlgItemMessage(hwndDlg, IDC_SPEED, TBM_GETPOS, 0, 0)/100;
		if (LOWORD(wParam) == IDC_ALPHA2)
			g_ConfigThis->alpha2 = SendDlgItemMessage(hwndDlg, IDC_ALPHA2, TBM_GETPOS, 0, 0);
		if (LOWORD(wParam) == IDC_DISTANCE2)
			g_ConfigThis->distance2 = SendDlgItemMessage(hwndDlg, IDC_DISTANCE2, TBM_GETPOS, 0, 0);
		if (LOWORD(wParam) == IDC_ROTATE2)
			g_ConfigThis->rotationinc2= -1*(SendDlgItemMessage(hwndDlg, IDC_ROTATE2, TBM_GETPOS, 0, 0)-32);
		return 0;
	}
return 0;
}

