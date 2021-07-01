#include "g__lib.h"
#include "g__defs.h"
#include "c_bpm.h"
#include "resource.h"
#include <windows.h>
#include <commctrl.h>


int win32_dlgproc_custombpm(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
	C_THISCLASS* g_ConfigThis = (C_THISCLASS*)g_current_render;
switch (uMsg)
	{
	case WM_INITDIALOG:
		{
		char txt[40];
		g_ConfigThis->inInc = 1;
		g_ConfigThis->outInc = 1;
		g_ConfigThis->inSlide = 0;
		g_ConfigThis->outSlide = 0;
        if (g_ConfigThis->enabled) CheckDlgButton(hwndDlg,IDC_CHECK1,BST_CHECKED);
        if (g_ConfigThis->arbitrary) CheckDlgButton(hwndDlg,IDC_ARBITRARY,BST_CHECKED);
        if (g_ConfigThis->skip) CheckDlgButton(hwndDlg,IDC_SKIP,BST_CHECKED);
        if (g_ConfigThis->invert) CheckDlgButton(hwndDlg,IDC_INVERT,BST_CHECKED);
		SendDlgItemMessage(hwndDlg, IDC_ARBVAL, TBM_SETTICFREQ, 100, 0);
		SendDlgItemMessage(hwndDlg, IDC_SKIPVAL, TBM_SETTICFREQ, 1, 0);
		SendDlgItemMessage(hwndDlg, IDC_ARBVAL, TBM_SETRANGE, TRUE, MAKELONG(200, 10000));
		SendDlgItemMessage(hwndDlg, IDC_SKIPVAL, TBM_SETRANGE, TRUE, MAKELONG(1, 16));
		SendDlgItemMessage(hwndDlg, IDC_ARBVAL, TBM_SETPOS, TRUE, g_ConfigThis->arbVal);
		SendDlgItemMessage(hwndDlg, IDC_SKIPVAL, TBM_SETPOS, TRUE, g_ConfigThis->skipVal);
		SendDlgItemMessage(hwndDlg, IDC_IN, TBM_SETTICFREQ, 1, 0);
		SendDlgItemMessage(hwndDlg, IDC_IN, TBM_SETRANGE, TRUE, MAKELONG(0, 8));
		SendDlgItemMessage(hwndDlg, IDC_OUT, TBM_SETTICFREQ, 1, 0);
		SendDlgItemMessage(hwndDlg, IDC_OUT, TBM_SETRANGE, TRUE, MAKELONG(0, 8));
		SendDlgItemMessage(hwndDlg, IDC_SKIPFIRST, TBM_SETTICFREQ, 1, 0);
		SendDlgItemMessage(hwndDlg, IDC_SKIPFIRST, TBM_SETRANGE, TRUE, MAKELONG(0, 64));
		SendDlgItemMessage(hwndDlg, IDC_SKIPFIRST, TBM_SETPOS, TRUE, g_ConfigThis->skipfirst);
    wsprintf(txt, "%d bpm", 60000 / g_ConfigThis->arbVal);
		SetDlgItemText(hwndDlg, IDC_ARBTXT, txt);
		wsprintf(txt, "%d beat%s", g_ConfigThis->skipVal, g_ConfigThis->skipVal > 1 ? "s" : "");
		SetDlgItemText(hwndDlg, IDC_SKIPTXT, txt);
		wsprintf(txt, "%d beat%s", g_ConfigThis->skipfirst, g_ConfigThis->skipfirst > 1 ? "s" : "");
		SetDlgItemText(hwndDlg, IDC_SKIPFIRSTTXT, txt);
		SetTimer(hwndDlg, 0, 50, NULL);
		}
		return 1;
	case WM_TIMER:
		{
		if (g_ConfigThis->oldInSlide != g_ConfigThis->inSlide) {
			SendDlgItemMessage(hwndDlg, IDC_IN, TBM_SETPOS, TRUE, g_ConfigThis->inSlide); g_ConfigThis->oldInSlide=g_ConfigThis->inSlide; }
		if (g_ConfigThis->oldOutSlide != g_ConfigThis->outSlide) {
			SendDlgItemMessage(hwndDlg, IDC_OUT, TBM_SETPOS, TRUE, g_ConfigThis->outSlide); g_ConfigThis->oldOutSlide=g_ConfigThis->outSlide; }
		}
		return 0;
	case WM_NOTIFY:
		{
		char txt[40];
		if (LOWORD(wParam) == IDC_ARBVAL)
			g_ConfigThis->arbVal = SendDlgItemMessage(hwndDlg, IDC_ARBVAL, TBM_GETPOS, 0, 0);
		if (LOWORD(wParam) == IDC_SKIPVAL)
			g_ConfigThis->skipVal = SendDlgItemMessage(hwndDlg, IDC_SKIPVAL, TBM_GETPOS, 0, 0);
		if (LOWORD(wParam) == IDC_SKIPFIRST)
			g_ConfigThis->skipfirst = SendDlgItemMessage(hwndDlg, IDC_SKIPFIRST, TBM_GETPOS, 0, 0);
    wsprintf(txt, "%d bpm", 60000 / g_ConfigThis->arbVal);
		SetDlgItemText(hwndDlg, IDC_ARBTXT, txt);
		wsprintf(txt, "%d beat%s", g_ConfigThis->skipVal, g_ConfigThis->skipVal > 1 ? "s" : "");
		SetDlgItemText(hwndDlg, IDC_SKIPTXT, txt);
		wsprintf(txt, "%d beat%s", g_ConfigThis->skipfirst, g_ConfigThis->skipfirst > 1 ? "s" : "");
		SetDlgItemText(hwndDlg, IDC_SKIPFIRSTTXT, txt);
		return 0;
		}
	case WM_COMMAND:
      if ((LOWORD(wParam) == IDC_CHECK1) ||
		  (LOWORD(wParam) == IDC_ARBITRARY) ||
		  (LOWORD(wParam) == IDC_SKIP) ||
		  (LOWORD(wParam) == IDC_INVERT))
		  {
			g_ConfigThis->enabled=IsDlgButtonChecked(hwndDlg,IDC_CHECK1)?1:0;
			g_ConfigThis->arbitrary=IsDlgButtonChecked(hwndDlg,IDC_ARBITRARY)?1:0;
			g_ConfigThis->skip=IsDlgButtonChecked(hwndDlg,IDC_SKIP)?1:0;
			g_ConfigThis->invert=IsDlgButtonChecked(hwndDlg,IDC_INVERT)?1:0;
		  }
	  return 0;
	case WM_DESTROY:
		KillTimer(hwndDlg, 0);
		return 0;
	}
return 0;
}

