#include "g__lib.h"
#include "g__defs.h"
#include "c_colorfade.h"
#include "resource.h"
#include <windows.h>
#include <commctrl.h>


int win32_dlgproc_colorfade(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
  C_THISCLASS* g_this = (C_THISCLASS*)g_current_render;

	switch (uMsg)
	{
		case WM_INITDIALOG:
			SendDlgItemMessage(hwndDlg,IDC_SLIDER1,TBM_SETRANGEMIN,0,0);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER1,TBM_SETRANGEMAX,0,64);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER1,TBM_SETPOS,1,g_this->faders[0]+32);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER2,TBM_SETRANGEMIN,0,0);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER2,TBM_SETRANGEMAX,0,64);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER2,TBM_SETPOS,1,g_this->faders[1]+32);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER3,TBM_SETRANGEMIN,0,0);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER3,TBM_SETRANGEMAX,0,64);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER3,TBM_SETPOS,1,g_this->faders[2]+32);

			SendDlgItemMessage(hwndDlg,IDC_SLIDER4,TBM_SETRANGEMIN,0,0);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER4,TBM_SETRANGEMAX,0,64);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER4,TBM_SETPOS,1,g_this->beatfaders[0]+32);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER5,TBM_SETRANGEMIN,0,0);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER5,TBM_SETRANGEMAX,0,64);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER5,TBM_SETPOS,1,g_this->beatfaders[1]+32);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER6,TBM_SETRANGEMIN,0,0);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER6,TBM_SETRANGEMAX,0,64);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER6,TBM_SETPOS,1,g_this->beatfaders[2]+32);

      if (g_this->enabled&1) CheckDlgButton(hwndDlg,IDC_CHECK1,BST_CHECKED);
      if (g_this->enabled&2) CheckDlgButton(hwndDlg,IDC_CHECK2,BST_CHECKED);
      if (g_this->enabled&4) CheckDlgButton(hwndDlg,IDC_CHECK3,BST_CHECKED);
			return 1;
    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_CHECK1:
        case IDC_CHECK2:
        case IDC_CHECK3:
          g_this->enabled=(IsDlgButtonChecked(hwndDlg,IDC_CHECK1)?1:0)|
                          (IsDlgButtonChecked(hwndDlg,IDC_CHECK2)?2:0)|
                          (IsDlgButtonChecked(hwndDlg,IDC_CHECK3)?4:0);
          return 0;

      }
    return 0;
		case WM_HSCROLL:
			{
				HWND swnd = (HWND) lParam;
				int t = (int) SendMessage(swnd,TBM_GETPOS,0,0);
				if (swnd == GetDlgItem(hwndDlg,IDC_SLIDER1))
				{
					g_this->faders[0]=t-32;
				}
				if (swnd == GetDlgItem(hwndDlg,IDC_SLIDER2))
				{
					g_this->faders[1]=t-32;
				}
				if (swnd == GetDlgItem(hwndDlg,IDC_SLIDER3))
				{
					g_this->faders[2]=t-32;
				}
				if (swnd == GetDlgItem(hwndDlg,IDC_SLIDER4))
				{
					g_this->beatfaders[0]=t-32;
				}
				if (swnd == GetDlgItem(hwndDlg,IDC_SLIDER5))
				{
					g_this->beatfaders[1]=t-32;
				}
				if (swnd == GetDlgItem(hwndDlg,IDC_SLIDER6))
				{
					g_this->beatfaders[2]=t-32;
				}
			}
    return 0;
	}
	return 0;
}

