#include "g__lib.h"
#include "g__defs.h"
#include "c_parts.h"
#include "resource.h"
#include <windows.h>
#include <commctrl.h>


int win32_dlgproc_movingparticles(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
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
						GR_DrawColoredButton(di,g_this->colors);
					break;
				}
			}
		return 0;
		case WM_INITDIALOG:
			SendDlgItemMessage(hwndDlg,IDC_SLIDER1,TBM_SETRANGEMIN,0,1);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER1,TBM_SETRANGEMAX,0,32);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER1,TBM_SETPOS,1,g_this->maxdist);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER3,TBM_SETRANGEMIN,0,1);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER3,TBM_SETRANGEMAX,0,128);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER3,TBM_SETPOS,1,g_this->size);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER4,TBM_SETRANGEMIN,0,1);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER4,TBM_SETRANGEMAX,0,128);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER4,TBM_SETPOS,1,g_this->size2);
			if (g_this->enabled&1) CheckDlgButton(hwndDlg,IDC_LEFT,BST_CHECKED);
			if (g_this->enabled&2) CheckDlgButton(hwndDlg,IDC_CHECK1,BST_CHECKED);

      if (g_this->blend == 1) CheckDlgButton(hwndDlg,IDC_RADIO2,BST_CHECKED);
      else if (g_this->blend == 2) CheckDlgButton(hwndDlg,IDC_RADIO3,BST_CHECKED);
      else if (g_this->blend == 3) CheckDlgButton(hwndDlg,IDC_RADIO4,BST_CHECKED);
      else CheckDlgButton(hwndDlg,IDC_RADIO1,BST_CHECKED);

			return 1;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
        case IDC_RADIO1:
        case IDC_RADIO2:
        case IDC_RADIO3:
        case IDC_RADIO4:
          if (IsDlgButtonChecked(hwndDlg,IDC_RADIO1)) g_this->blend=0;
          else if (IsDlgButtonChecked(hwndDlg,IDC_RADIO2)) g_this->blend=1;
          else if (IsDlgButtonChecked(hwndDlg,IDC_RADIO3)) g_this->blend=2;
          else if (IsDlgButtonChecked(hwndDlg,IDC_RADIO4)) g_this->blend=3;
        break;
				case IDC_LEFT:
					g_this->enabled&=~1;
					g_this->enabled|=IsDlgButtonChecked(hwndDlg,IDC_LEFT)?1:0;
				return 0;
				case IDC_CHECK1:
					g_this->enabled&=~2;
					g_this->enabled|=IsDlgButtonChecked(hwndDlg,IDC_CHECK1)?2:0;
				return 0;
				case IDC_LC:
					if (!a) a=&g_this->colors;
					GR_SelectColor(hwndDlg,a);
					InvalidateRect(GetDlgItem(hwndDlg,LOWORD(wParam)),NULL,FALSE);
				return 0;


			}
		return 0;
		case WM_HSCROLL:
			{
				HWND swnd = (HWND) lParam;
				int t = (int) SendMessage(swnd,TBM_GETPOS,0,0);
				if (swnd == GetDlgItem(hwndDlg,IDC_SLIDER1))
				{
					g_this->maxdist=t;
				}
				if (swnd == GetDlgItem(hwndDlg,IDC_SLIDER3))
				{
					g_this->s_pos=g_this->size=t;
				}
				if (swnd == GetDlgItem(hwndDlg,IDC_SLIDER4))
				{
					g_this->s_pos=g_this->size2=t;
				}
			}
		return 0;


	}
	return 0;
}

