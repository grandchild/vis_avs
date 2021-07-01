#include "g__lib.h"
#include "g__defs.h"
#include "c_bright.h"
#include "resource.h"
#include <windows.h>
#include <commctrl.h>


int win32_dlgproc_brightness(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
	C_THISCLASS* g_ConfigThis = (C_THISCLASS*)g_current_render;
switch (uMsg)
	{
	case WM_INITDIALOG:
		SendDlgItemMessage(hwndDlg, IDC_RED, TBM_SETRANGEMIN, TRUE, 0);
		SendDlgItemMessage(hwndDlg, IDC_GREEN, TBM_SETRANGEMIN, TRUE, 0);
		SendDlgItemMessage(hwndDlg, IDC_BLUE, TBM_SETRANGEMIN, TRUE, 0);
		SendDlgItemMessage(hwndDlg, IDC_RED, TBM_SETRANGEMAX, TRUE, 8192);
		SendDlgItemMessage(hwndDlg, IDC_GREEN, TBM_SETRANGEMAX, TRUE, 8192);
		SendDlgItemMessage(hwndDlg, IDC_BLUE, TBM_SETRANGEMAX, TRUE, 8192);
		SendDlgItemMessage(hwndDlg, IDC_RED, TBM_SETTICFREQ, 256, 0);
		SendDlgItemMessage(hwndDlg, IDC_GREEN, TBM_SETTICFREQ, 256, 0);
		SendDlgItemMessage(hwndDlg, IDC_BLUE, TBM_SETTICFREQ, 256, 0);
		SendDlgItemMessage(hwndDlg, IDC_RED, TBM_SETPOS, TRUE, g_ConfigThis->redp+4096);
		SendDlgItemMessage(hwndDlg, IDC_GREEN, TBM_SETPOS, TRUE, g_ConfigThis->greenp+4096);
		SendDlgItemMessage(hwndDlg, IDC_BLUE, TBM_SETPOS, TRUE, g_ConfigThis->bluep+4096);
		SendDlgItemMessage(hwndDlg, IDC_DISTANCE, TBM_SETRANGE, TRUE, MAKELONG(0, 255));
		SendDlgItemMessage(hwndDlg, IDC_DISTANCE, TBM_SETPOS, TRUE, g_ConfigThis->distance);
		SendDlgItemMessage(hwndDlg, IDC_DISTANCE, TBM_SETTICFREQ, 16, 0);
        if (g_ConfigThis->enabled) CheckDlgButton(hwndDlg,IDC_CHECK1,BST_CHECKED);
        if (g_ConfigThis->exclude) CheckDlgButton(hwndDlg,IDC_EXCLUDE,BST_CHECKED);
        if (g_ConfigThis->blend) CheckDlgButton(hwndDlg,IDC_ADDITIVE,BST_CHECKED);
        if (g_ConfigThis->blendavg) CheckDlgButton(hwndDlg,IDC_5050,BST_CHECKED);
        if (g_ConfigThis->dissoc) CheckDlgButton(hwndDlg,IDC_DISSOC,BST_CHECKED);
        if (!g_ConfigThis->blend && !g_ConfigThis->blendavg)
		   CheckDlgButton(hwndDlg,IDC_REPLACE,BST_CHECKED);
		return 1;
	case WM_NOTIFY:
		{
		if (LOWORD(wParam) == IDC_DISTANCE)
			{
			g_ConfigThis->distance = SendDlgItemMessage(hwndDlg, IDC_DISTANCE, TBM_GETPOS, 0, 0);
			}
		if (LOWORD(wParam) == IDC_RED)
			{
			g_ConfigThis->redp = SendDlgItemMessage(hwndDlg, IDC_RED, TBM_GETPOS, 0, 0)-4096;
			rred:
      g_ConfigThis->tabs_needinit=1;
			if (!g_ConfigThis->dissoc)
				{
				g_ConfigThis->greenp = g_ConfigThis->redp;
				SendDlgItemMessage(hwndDlg, IDC_GREEN, TBM_SETPOS, TRUE, g_ConfigThis->greenp+4096);
				g_ConfigThis->bluep = g_ConfigThis->redp;
				SendDlgItemMessage(hwndDlg, IDC_BLUE, TBM_SETPOS, TRUE, g_ConfigThis->bluep+4096);
				}
			}
		if (LOWORD(wParam) == IDC_GREEN)
			{
			g_ConfigThis->greenp = SendDlgItemMessage(hwndDlg, IDC_GREEN, TBM_GETPOS, 0, 0)-4096;
			rgreen:
      g_ConfigThis->tabs_needinit=1;
			if (!g_ConfigThis->dissoc)
				{
				g_ConfigThis->redp = g_ConfigThis->greenp;
				SendDlgItemMessage(hwndDlg, IDC_RED, TBM_SETPOS, TRUE, g_ConfigThis->redp+4096);
				g_ConfigThis->bluep = g_ConfigThis->greenp;
				SendDlgItemMessage(hwndDlg, IDC_BLUE, TBM_SETPOS, TRUE, g_ConfigThis->bluep+4096);
				}
			}
		if (LOWORD(wParam) == IDC_BLUE)
			{
			g_ConfigThis->bluep = SendDlgItemMessage(hwndDlg, IDC_BLUE, TBM_GETPOS, 0, 0)-4096;
			rblue:
      g_ConfigThis->tabs_needinit=1;
			if (!g_ConfigThis->dissoc)
				{
				g_ConfigThis->redp = g_ConfigThis->bluep;
				SendDlgItemMessage(hwndDlg, IDC_RED, TBM_SETPOS, TRUE, g_ConfigThis->redp+4096);
				g_ConfigThis->greenp = g_ConfigThis->bluep;
				SendDlgItemMessage(hwndDlg, IDC_GREEN, TBM_SETPOS, TRUE, g_ConfigThis->greenp+4096);
				}
			}
			return 0;
		}
	case WM_COMMAND:
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
	  if (LOWORD(wParam) == IDC_BRED)
		{
		g_ConfigThis->redp = 0;
		SendDlgItemMessage(hwndDlg, IDC_RED, TBM_SETPOS, TRUE, g_ConfigThis->redp+4096);
		goto rred; // gotos are so sweet ;) 
		}
	  if (LOWORD(wParam) == IDC_BGREEN)
		{
		g_ConfigThis->greenp = 0;
		SendDlgItemMessage(hwndDlg, IDC_GREEN, TBM_SETPOS, TRUE, g_ConfigThis->greenp+4096);
		goto rgreen; 
		}
	  if (LOWORD(wParam) == IDC_BBLUE)
		{
		g_ConfigThis->bluep = 0;
		SendDlgItemMessage(hwndDlg, IDC_BLUE, TBM_SETPOS, TRUE, g_ConfigThis->bluep+4096);
		goto rblue; 
		}
	  if ((LOWORD(wParam) == IDC_CHECK1) ||
	     (LOWORD(wParam) == IDC_ADDITIVE) ||
	     (LOWORD(wParam) == IDC_REPLACE) ||
	     (LOWORD(wParam) == IDC_EXCLUDE) ||
	     (LOWORD(wParam) == IDC_DISSOC) ||
	     (LOWORD(wParam) == IDC_5050) )
			{
			g_ConfigThis->enabled=IsDlgButtonChecked(hwndDlg,IDC_CHECK1)?1:0;
			g_ConfigThis->exclude=IsDlgButtonChecked(hwndDlg,IDC_EXCLUDE)?1:0;
			g_ConfigThis->blend=IsDlgButtonChecked(hwndDlg,IDC_ADDITIVE)?1:0;
			g_ConfigThis->blendavg=IsDlgButtonChecked(hwndDlg,IDC_5050)?1:0;
			g_ConfigThis->dissoc=IsDlgButtonChecked(hwndDlg,IDC_DISSOC)?1:0;
			if (!g_ConfigThis->dissoc)
				{
				g_ConfigThis->greenp = g_ConfigThis->redp;
				SendDlgItemMessage(hwndDlg, IDC_GREEN, TBM_SETPOS, TRUE, g_ConfigThis->greenp+4096);
				g_ConfigThis->bluep = g_ConfigThis->redp;
				SendDlgItemMessage(hwndDlg, IDC_BLUE, TBM_SETPOS, TRUE, g_ConfigThis->bluep+4096);
				}
			}
	  return 0;
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
	}
return 0;
}

