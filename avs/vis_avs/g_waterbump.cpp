#include "g__lib.h"
#include "g__defs.h"
#include "c_waterbump.h"
#include "resource.h"
#include <windows.h>
#include <commctrl.h>


int win32_dlgproc_waterbump(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
  C_THISCLASS* g_ConfigThis = (C_THISCLASS*)g_current_render;
	switch (uMsg)
	{
		case WM_INITDIALOG:
			if (g_ConfigThis->enabled) CheckDlgButton(hwndDlg,IDC_CHECK1,BST_CHECKED);
			SendDlgItemMessage(hwndDlg,IDC_DAMP,TBM_SETRANGEMIN,0,2);
			SendDlgItemMessage(hwndDlg,IDC_DAMP,TBM_SETRANGEMAX,0,10);
			SendDlgItemMessage(hwndDlg,IDC_DAMP,TBM_SETPOS,1,g_ConfigThis->density);
			SendDlgItemMessage(hwndDlg,IDC_DEPTH,TBM_SETRANGEMIN,0,100);
			SendDlgItemMessage(hwndDlg,IDC_DEPTH,TBM_SETRANGEMAX,0,2000);
			SendDlgItemMessage(hwndDlg,IDC_DEPTH,TBM_SETPOS,1,g_ConfigThis->depth);
			SendDlgItemMessage(hwndDlg,IDC_RADIUS,TBM_SETRANGEMIN,0,10);
			SendDlgItemMessage(hwndDlg,IDC_RADIUS,TBM_SETRANGEMAX,0,100);
			SendDlgItemMessage(hwndDlg,IDC_RADIUS,TBM_SETPOS,1,g_ConfigThis->drop_radius);
			CheckDlgButton(hwndDlg,IDC_RANDOM_DROP,g_ConfigThis->random_drop);
			CheckDlgButton(hwndDlg,IDC_DROP_LEFT,g_ConfigThis->drop_position_x==0);
			CheckDlgButton(hwndDlg,IDC_DROP_CENTER,g_ConfigThis->drop_position_x==1);
			CheckDlgButton(hwndDlg,IDC_DROP_RIGHT,g_ConfigThis->drop_position_x==2);
			CheckDlgButton(hwndDlg,IDC_DROP_TOP,g_ConfigThis->drop_position_y==0);
			CheckDlgButton(hwndDlg,IDC_DROP_MIDDLE,g_ConfigThis->drop_position_y==1);
			CheckDlgButton(hwndDlg,IDC_DROP_BOTTOM,g_ConfigThis->drop_position_y==2);
			return 1;
		case WM_DRAWITEM:
		return 0;
    case WM_COMMAND:
      if (LOWORD(wParam) == IDC_CHECK1)
        g_ConfigThis->enabled=IsDlgButtonChecked(hwndDlg,IDC_CHECK1)?1:0;
      if (LOWORD(wParam) == IDC_RANDOM_DROP)
        g_ConfigThis->random_drop=IsDlgButtonChecked(hwndDlg,IDC_RANDOM_DROP);
      if (LOWORD(wParam) == IDC_DROP_LEFT)
        g_ConfigThis->drop_position_x=0;
      if (LOWORD(wParam) == IDC_DROP_CENTER)
        g_ConfigThis->drop_position_x=1;
      if (LOWORD(wParam) == IDC_DROP_RIGHT)
        g_ConfigThis->drop_position_x=2;
      if (LOWORD(wParam) == IDC_DROP_TOP)
        g_ConfigThis->drop_position_y=0;
      if (LOWORD(wParam) == IDC_DROP_MIDDLE)
        g_ConfigThis->drop_position_y=1;
      if (LOWORD(wParam) == IDC_DROP_BOTTOM)
        g_ConfigThis->drop_position_y=2;
      return 0;
	case WM_HSCROLL:
		{
			HWND swnd = (HWND) lParam;
			int t = (int) SendMessage(swnd,TBM_GETPOS,0,0);
			if (swnd == GetDlgItem(hwndDlg,IDC_DAMP))
				g_ConfigThis->density=t;
			if (swnd == GetDlgItem(hwndDlg,IDC_DEPTH))
				g_ConfigThis->depth=t;
			if (swnd == GetDlgItem(hwndDlg,IDC_RADIUS))
				g_ConfigThis->drop_radius=t;
		}
		return 0;
	}
	return 0;
}

