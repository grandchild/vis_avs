#include "g__lib.h"
#include "g__defs.h"
#include "c_fadeout.h"
#include "resource.h"
#include <windows.h>
#include <commctrl.h>


int win32_dlgproc_fade(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
  C_THISCLASS* g_this = (C_THISCLASS*)g_current_render;

	int *a=NULL;
	switch (uMsg)
	{
		case WM_DRAWITEM:
			{
				DRAWITEMSTRUCT *di=(DRAWITEMSTRUCT *)lParam;
				switch (di->CtlID)
				{
					case IDC_LC:
						GR_DrawColoredButton(di,g_this->color);
					break;
				}
			}
		return 0;
		case WM_INITDIALOG:
			SendDlgItemMessage(hwndDlg,IDC_SLIDER1,TBM_SETRANGEMIN,0,0);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER1,TBM_SETRANGEMAX,0,92);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER1,TBM_SETPOS,1,g_this->fadelen);
			
			return 1;

		case WM_HSCROLL:
			{
				HWND swnd = (HWND) lParam;
				int t = (int) SendMessage(swnd,TBM_GETPOS,0,0);
				if (swnd == GetDlgItem(hwndDlg,IDC_SLIDER1))
				{
					g_this->fadelen=t;
          g_this->maketab();
				}
			}
			return 0;
    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
				case IDC_LC:
					GR_SelectColor(hwndDlg,&g_this->color);
					InvalidateRect(GetDlgItem(hwndDlg,LOWORD(wParam)),NULL,FALSE);
          g_this->maketab();
				return 0;
      }
    return 0;
	}
	return 0;
}

