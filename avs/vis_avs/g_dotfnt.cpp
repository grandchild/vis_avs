#include "g__lib.h"
#include "g__defs.h"
#include "c_dotfnt.h"
#include "resource.h"
#include <windows.h>
#include <commctrl.h>


int win32_dlgproc_dotplane(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
  C_THISCLASS* g_this = (C_THISCLASS*)g_current_render;

	switch (uMsg)
	{
		case WM_COMMAND:
      if (LOWORD(wParam) == IDC_BUTTON1)
      {
        g_this->rotvel=0;
   			SendDlgItemMessage(hwndDlg,IDC_SLIDER1,TBM_SETPOS,1,50);
      }
			if (LOWORD(wParam) >= IDC_C1 && LOWORD(wParam) <= IDC_C5) {
				GR_SelectColor(hwndDlg,&g_this->colors[IDC_C5-LOWORD(wParam)]);
				InvalidateRect(GetDlgItem(hwndDlg,LOWORD(wParam)),NULL,FALSE);
				g_this->initcolortab();
			}
		return 0;
		case WM_DRAWITEM:
			{
				DRAWITEMSTRUCT *di=(DRAWITEMSTRUCT *)lParam;
				if (di->CtlID >= IDC_C1 && di->CtlID <= IDC_C5)
				{
					GR_DrawColoredButton(di,g_this->colors[IDC_C5-di->CtlID]);
				}
			}
		return 0;
		case WM_INITDIALOG:
			SendDlgItemMessage(hwndDlg,IDC_SLIDER1,TBM_SETRANGEMIN,0,0);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER1,TBM_SETRANGEMAX,0,101);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER1,TBM_SETPOS,1,g_this->rotvel+50);
			SendDlgItemMessage(hwndDlg,IDC_ANGLE,TBM_SETRANGEMIN,0,0);
			SendDlgItemMessage(hwndDlg,IDC_ANGLE,TBM_SETRANGEMAX,0,181);
			SendDlgItemMessage(hwndDlg,IDC_ANGLE,TBM_SETPOS,1,g_this->angle+90);
			
			return 1;

		case WM_HSCROLL:
			{
				HWND swnd = (HWND) lParam;
				int t = (int) SendMessage(swnd,TBM_GETPOS,0,0);
				if (swnd == GetDlgItem(hwndDlg,IDC_SLIDER1))
				{
					g_this->rotvel=t-50;
				}
        if (swnd == GetDlgItem(hwndDlg,IDC_ANGLE))
        {
          g_this->angle=t-90;
        }
			}
	}
	return 0;
}

