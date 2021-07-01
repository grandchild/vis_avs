#include "g__lib.h"
#include "g__defs.h"
#include "c_stars.h"
#include "resource.h"
#include <windows.h>
#include <commctrl.h>


int win32_dlgproc_starfield(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
	C_THISCLASS* g_ConfigThis = (C_THISCLASS*)g_current_render;
switch (uMsg)
	{
	case WM_INITDIALOG:
#ifdef LASER
    ShowWindow(GetDlgItem(hwndDlg,IDC_ADDITIVE),SW_HIDE);
    ShowWindow(GetDlgItem(hwndDlg,IDC_5050),SW_HIDE);
    ShowWindow(GetDlgItem(hwndDlg,IDC_REPLACE),SW_HIDE);
#endif
		SendDlgItemMessage(hwndDlg, IDC_SPEED, TBM_SETTICFREQ, 10, 0);
		SendDlgItemMessage(hwndDlg, IDC_SPEED, TBM_SETRANGE, TRUE, MAKELONG(1, 5000));
		SendDlgItemMessage(hwndDlg, IDC_SPEED, TBM_SETPOS, TRUE, (int)(g_ConfigThis->WarpSpeed*100));
		SendDlgItemMessage(hwndDlg, IDC_NUMSTARS, TBM_SETTICFREQ, 100, 0);
		SendDlgItemMessage(hwndDlg, IDC_NUMSTARS, TBM_SETRANGE, TRUE, MAKELONG(100, 4095));
		SendDlgItemMessage(hwndDlg, IDC_NUMSTARS, TBM_SETPOS, TRUE, g_ConfigThis->MaxStars_set);
		SendDlgItemMessage(hwndDlg, IDC_SPDCHG, TBM_SETTICFREQ, 10, 0);
		SendDlgItemMessage(hwndDlg, IDC_SPDCHG, TBM_SETRANGE, TRUE, MAKELONG(1, 5000));
		SendDlgItemMessage(hwndDlg, IDC_SPDCHG, TBM_SETPOS, TRUE, (int)(g_ConfigThis->spdBeat*100));
		SendDlgItemMessage(hwndDlg, IDC_SPDDUR, TBM_SETTICFREQ, 10, 0);
		SendDlgItemMessage(hwndDlg, IDC_SPDDUR, TBM_SETRANGE, TRUE, MAKELONG(1, 100));
		SendDlgItemMessage(hwndDlg, IDC_SPDDUR, TBM_SETPOS, TRUE, (int)(g_ConfigThis->durFrames));
        if (g_ConfigThis->enabled) CheckDlgButton(hwndDlg,IDC_CHECK1,BST_CHECKED);
        if (g_ConfigThis->onbeat) CheckDlgButton(hwndDlg,IDC_ONBEAT2,BST_CHECKED);
        if (g_ConfigThis->blend) CheckDlgButton(hwndDlg,IDC_ADDITIVE,BST_CHECKED);
        if (g_ConfigThis->blendavg) CheckDlgButton(hwndDlg,IDC_5050,BST_CHECKED);
        if (!g_ConfigThis->blend && !g_ConfigThis->blendavg)
		   CheckDlgButton(hwndDlg,IDC_REPLACE,BST_CHECKED);
		return 1;
	case WM_NOTIFY:
		{
		if (LOWORD(wParam) == IDC_SPDCHG)
			g_ConfigThis->spdBeat = (float)SendDlgItemMessage(hwndDlg, IDC_SPDCHG, TBM_GETPOS, 0, 0)/100;
		if (LOWORD(wParam) == IDC_SPDDUR)
			g_ConfigThis->durFrames = SendDlgItemMessage(hwndDlg, IDC_SPDDUR, TBM_GETPOS, 0, 0);
		if (LOWORD(wParam) == IDC_SPEED)
			{
			g_ConfigThis->WarpSpeed = (float)SendDlgItemMessage(hwndDlg, IDC_SPEED, TBM_GETPOS, 0, 0)/100;
			g_ConfigThis->CurrentSpeed = (float)SendDlgItemMessage(hwndDlg, IDC_SPEED, TBM_GETPOS, 0, 0)/100;
			}

		if (LOWORD(wParam) == IDC_NUMSTARS)
			{
#ifndef LASER
  		int a = g_ConfigThis->MaxStars_set;
#endif
			g_ConfigThis->MaxStars_set = SendDlgItemMessage(hwndDlg, IDC_NUMSTARS, TBM_GETPOS, 0, 0);
#ifndef LASER
			if (g_ConfigThis->MaxStars_set > a)
#endif
				if (g_ConfigThis->Width && g_ConfigThis->Height) g_ConfigThis->InitializeStars();
      }
			return 0;
		}
	case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT *di=(DRAWITEMSTRUCT *)lParam;
			if (di->CtlID == IDC_DEFCOL) // paint nifty color button
			{
        int w=di->rcItem.right-di->rcItem.left;
        unsigned int _color=g_ConfigThis->color;
          _color = ((_color>>16)&0xff)|(_color&0xff00)|((_color<<16)&0xff0000);

	        HPEN hPen,hOldPen;
	        HBRUSH hBrush,hOldBrush;
	        LOGBRUSH lb={BS_SOLID,_color,0};
	        hPen = (HPEN)CreatePen(PS_SOLID,0,_color);
	        hBrush = CreateBrushIndirect(&lb);
	        hOldPen=(HPEN)SelectObject(di->hDC,hPen);
	        hOldBrush=(HBRUSH)SelectObject(di->hDC,hBrush);
	        Rectangle(di->hDC,di->rcItem.left,di->rcItem.top,di->rcItem.right,di->rcItem.bottom);
	        SelectObject(di->hDC,hOldPen);
	        SelectObject(di->hDC,hOldBrush);
	        DeleteObject(hBrush);
	        DeleteObject(hPen);
      }
	  }
		return 0;
	case WM_COMMAND:
	  if ((LOWORD(wParam) == IDC_CHECK1) ||
	     (LOWORD(wParam) == IDC_ONBEAT2) ||
	     (LOWORD(wParam) == IDC_ADDITIVE) ||
	     (LOWORD(wParam) == IDC_REPLACE) ||
	     (LOWORD(wParam) == IDC_5050) )
			{
			g_ConfigThis->enabled=IsDlgButtonChecked(hwndDlg,IDC_CHECK1)?1:0;
			g_ConfigThis->onbeat=IsDlgButtonChecked(hwndDlg,IDC_ONBEAT2)?1:0;
			g_ConfigThis->blend=IsDlgButtonChecked(hwndDlg,IDC_ADDITIVE)?1:0;
			g_ConfigThis->blendavg=IsDlgButtonChecked(hwndDlg,IDC_5050)?1:0;
			}
      if (LOWORD(wParam) == IDC_DEFCOL) // handle clicks to nifty color button
      {
      int *a=&(g_ConfigThis->color);
      static COLORREF custcolors[16];
      CHOOSECOLOR cs;
      cs.lStructSize = sizeof(cs);
      cs.hwndOwner = hwndDlg;
      cs.hInstance = 0;
      cs.rgbResult=((*a>>16)&0xff)|(*a&0xff00)|((*a<<16)&0xff0000);
      cs.lpCustColors = custcolors;
      cs.Flags = CC_RGBINIT|CC_FULLOPEN;
      if (ChooseColor(&cs))
	        {
		        *a = ((cs.rgbResult>>16)&0xff)|(cs.rgbResult&0xff00)|((cs.rgbResult<<16)&0xff0000);
	        	g_ConfigThis->color = *a;
			}
      InvalidateRect(GetDlgItem(hwndDlg,IDC_DEFCOL),NULL,TRUE);            
      }
	}
return 0;
}

