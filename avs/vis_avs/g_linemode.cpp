#include "g__lib.h"
#include "g__defs.h"
#include "c_linemode.h"
#include "resource.h"
#include <windows.h>
#include <commctrl.h>


int win32_dlgproc_setrendermode(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM)
{
  C_THISCLASS* g_this = (C_THISCLASS*)g_current_render;

	switch (uMsg)
	{
		case WM_INITDIALOG:
      {
        int x;
        for (x = 0; x<sizeof(g_this->line_blendmodes)/sizeof(g_this->line_blendmodes[0]); x ++)
        {
          SendDlgItemMessage(hwndDlg,IDC_COMBO1,CB_ADDSTRING,0,(LPARAM)g_this->line_blendmodes[x]);
        }
      }
      if (g_this->newmode&0x80000000)
        CheckDlgButton(hwndDlg,IDC_CHECK1,BST_CHECKED);
      SendDlgItemMessage(hwndDlg, IDC_ALPHASLIDE, TBM_SETRANGE, TRUE, MAKELONG(0, 255));
  		SendDlgItemMessage(hwndDlg, IDC_ALPHASLIDE, TBM_SETPOS, TRUE, (int)(g_this->newmode>>8)&0xff);      
      SendDlgItemMessage(hwndDlg,IDC_COMBO1,CB_SETCURSEL,(WPARAM)g_this->newmode&0xff,0);
      EnableWindow(GetDlgItem(hwndDlg, IDC_OUTSLIDE), (g_this->newmode&0xff) == 7);
      SetDlgItemInt(hwndDlg,IDC_EDIT1,(g_this->newmode>>16)&0xff,FALSE);
			return 1;
    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_EDIT1:
          {
            int r,t;
            r=GetDlgItemInt(hwndDlg,IDC_EDIT1,&t,FALSE);
            if (t)
            {
              g_this->newmode&=~0xff0000;
              g_this->newmode|=(r&0xff)<<16;
            }
          }
        break;
        case IDC_CHECK1:
          g_this->newmode&=0x7fffffff;
          g_this->newmode|=IsDlgButtonChecked(hwndDlg,IDC_CHECK1)?0x80000000:0;
        break;
        case IDC_COMBO1:
          if (HIWORD(wParam) == CBN_SELCHANGE)
          {
            int r=SendDlgItemMessage(hwndDlg,IDC_COMBO1,CB_GETCURSEL,0,0);
            if (r!=CB_ERR) 
						{
							g_this->newmode&=~0xff;
              g_this->newmode|=r;
							EnableWindow(GetDlgItem(hwndDlg, IDC_OUTSLIDE), r == 7);
						}
          }
        break;
      }
    return 0;
	  case WM_NOTIFY:
		  if (LOWORD(wParam) == IDC_ALPHASLIDE)
      {
        g_this->newmode &= ~0xff00;
			  g_this->newmode |= (SendDlgItemMessage(hwndDlg, IDC_ALPHASLIDE, TBM_GETPOS, 0, 0)&0xff)<<8;
      }
		break;
	}
	return 0;
}

