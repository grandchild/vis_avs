#include "g__lib.h"
#include "g__defs.h"
#include "c_rotblit.h"
#include "resource.h"
#include <windows.h>
#include <commctrl.h>


int win32_dlgproc_rotoblitter(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
  C_THISCLASS* g_this = (C_THISCLASS*)g_current_render;

	switch (uMsg)
	{
		case WM_INITDIALOG:
			SendDlgItemMessage(hwndDlg,IDC_SLIDER1,TBM_SETRANGEMIN,0,0);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER1,TBM_SETRANGEMAX,0,256);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER1,TBM_SETPOS,1,g_this->zoom_scale);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER6,TBM_SETRANGEMIN,0,0);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER6,TBM_SETRANGEMAX,0,256);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER6,TBM_SETPOS,1,g_this->zoom_scale2);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER5,TBM_SETRANGEMIN,0,0);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER5,TBM_SETRANGEMAX,0,8);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER5,TBM_SETPOS,1,g_this->beatch_speed);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER2,TBM_SETRANGEMIN,0,0);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER2,TBM_SETRANGEMAX,0,64);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER2,TBM_SETPOS,1,g_this->rot_dir);
			if (g_this->subpixel) CheckDlgButton(hwndDlg,IDC_CHECK2,BST_CHECKED);			
			else if (g_this->blend==1) CheckDlgButton(hwndDlg,IDC_BLEND,BST_CHECKED);			
			if (g_this->beatch==1) CheckDlgButton(hwndDlg,IDC_CHECK1,BST_CHECKED);			
      if (g_this->beatch_scale==1) CheckDlgButton(hwndDlg,IDC_CHECK6,BST_CHECKED);			
			return 1;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_BLEND:
					if (IsDlgButtonChecked(hwndDlg,IDC_BLEND))
          {
						g_this->blend=1;
          }
					else g_this->blend=0;
				break;
				case IDC_CHECK1:
					if (IsDlgButtonChecked(hwndDlg,IDC_CHECK1))
						g_this->beatch=1;
					else g_this->beatch=0;
				break;
				case IDC_CHECK2:
					if (IsDlgButtonChecked(hwndDlg,IDC_CHECK2))
          {
						g_this->subpixel=1;
          }
					else g_this->subpixel=0;
				break;
				case IDC_CHECK6:
					if (IsDlgButtonChecked(hwndDlg,IDC_CHECK6))
						g_this->beatch_scale=1;
					else g_this->beatch_scale=0;
				break;
			}
		return 0;

		case WM_HSCROLL:
			{
				HWND swnd = (HWND) lParam;
				int t = (int) SendMessage(swnd,TBM_GETPOS,0,0);
				if (swnd == GetDlgItem(hwndDlg,IDC_SLIDER1))
				{
					g_this->zoom_scale=t;
				}
				if (swnd == GetDlgItem(hwndDlg,IDC_SLIDER6))
				{
					g_this->zoom_scale2=t;
          g_this->scale_fpos=t;
				}
				if (swnd == GetDlgItem(hwndDlg,IDC_SLIDER2))
				{
					g_this->rot_dir=t;
				}
        if (swnd == GetDlgItem(hwndDlg,IDC_SLIDER5))
        {
          g_this->beatch_speed=t;
        }
			}
	}
	return 0;
}

