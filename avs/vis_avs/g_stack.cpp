#include "c__defs.h"
#include "g__lib.h"
#include "g__defs.h"
#include "c_stack.h"
#include "resource.h"
#include <windows.h>
#include <commctrl.h>


int win32_dlgproc_buffersave(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM)
{
  C_THISCLASS* g_this = (C_THISCLASS*)g_current_render;

	switch (uMsg)
	{
		case WM_INITDIALOG:
      {
        int x;
        for (x = 1; x <= NBUF; x ++)
        {
          char s[32];
          wsprintf(s,"Buffer %d",x);
          SendDlgItemMessage(hwndDlg,IDC_COMBO1,CB_ADDSTRING,0,(LPARAM)s);
        }
      }
      SendDlgItemMessage(hwndDlg,IDC_COMBO1,CB_SETCURSEL,g_this->which,0);
      if (g_this->dir==1) CheckDlgButton(hwndDlg,IDC_RESTFB,BST_CHECKED);
      else if (g_this->dir == 2) CheckDlgButton(hwndDlg,IDC_RADIO1,BST_CHECKED);
      else if (g_this->dir == 3) CheckDlgButton(hwndDlg,IDC_RADIO2,BST_CHECKED);
      else CheckDlgButton(hwndDlg,IDC_SAVEFB,BST_CHECKED);
			SendDlgItemMessage(hwndDlg, IDC_BLENDSLIDE, TBM_SETRANGE, TRUE, MAKELONG(0, 255));
			SendDlgItemMessage(hwndDlg, IDC_BLENDSLIDE, TBM_SETPOS, TRUE, (int)(g_this->adjblend_val));
      if (g_this->blend==0) CheckDlgButton(hwndDlg,IDC_RSTACK_BLEND1,BST_CHECKED);
      if (g_this->blend==1) CheckDlgButton(hwndDlg,IDC_RSTACK_BLEND2,BST_CHECKED);
      if (g_this->blend==2) CheckDlgButton(hwndDlg,IDC_RSTACK_BLEND3,BST_CHECKED);
      if (g_this->blend==3) CheckDlgButton(hwndDlg,IDC_RSTACK_BLEND4,BST_CHECKED);
      if (g_this->blend==4) CheckDlgButton(hwndDlg,IDC_RSTACK_BLEND5,BST_CHECKED);
      if (g_this->blend==5) CheckDlgButton(hwndDlg,IDC_RSTACK_BLEND6,BST_CHECKED);
      if (g_this->blend==6) CheckDlgButton(hwndDlg,IDC_RSTACK_BLEND7,BST_CHECKED);
      if (g_this->blend==7) CheckDlgButton(hwndDlg,IDC_RSTACK_BLEND8,BST_CHECKED);
      if (g_this->blend==8) CheckDlgButton(hwndDlg,IDC_RSTACK_BLEND9,BST_CHECKED);
      if (g_this->blend==9) CheckDlgButton(hwndDlg,IDC_RSTACK_BLEND10,BST_CHECKED);
      if (g_this->blend==10) CheckDlgButton(hwndDlg,IDC_RSTACK_BLEND11,BST_CHECKED);
      if (g_this->blend==11) CheckDlgButton(hwndDlg,IDC_RSTACK_BLEND12,BST_CHECKED);
			return 1;
    case WM_COMMAND:
      if (LOWORD(wParam) == IDC_BUTTON2)
      {
        g_this->dir_ch^=1;
      }
      if (LOWORD(wParam) == IDC_BUTTON1)
      {
        g_this->clear=1;
      }
      if (LOWORD(wParam) == IDC_SAVEFB || LOWORD(wParam) == IDC_RESTFB || LOWORD(wParam) == IDC_RADIO1 || LOWORD(wParam) == IDC_RADIO2)
      {
        if (IsDlgButtonChecked(hwndDlg,IDC_SAVEFB)) g_this->dir=0;
        else if (IsDlgButtonChecked(hwndDlg,IDC_RADIO2)) g_this->dir=3;
        else if (IsDlgButtonChecked(hwndDlg,IDC_RADIO1)) g_this->dir=2;
        else if (IsDlgButtonChecked(hwndDlg,IDC_RESTFB)) g_this->dir=1;
      }
      if (IsDlgButtonChecked(hwndDlg,LOWORD(wParam)))
      {
        if (LOWORD(wParam) == IDC_RSTACK_BLEND1) g_this->blend=0;
        if (LOWORD(wParam) == IDC_RSTACK_BLEND2) g_this->blend=1;
        if (LOWORD(wParam) == IDC_RSTACK_BLEND3) g_this->blend=2;
        if (LOWORD(wParam) == IDC_RSTACK_BLEND4) g_this->blend=3;
        if (LOWORD(wParam) == IDC_RSTACK_BLEND5) g_this->blend=4;
        if (LOWORD(wParam) == IDC_RSTACK_BLEND6) g_this->blend=5;
        if (LOWORD(wParam) == IDC_RSTACK_BLEND7) g_this->blend=6;
        if (LOWORD(wParam) == IDC_RSTACK_BLEND8) g_this->blend=7;
        if (LOWORD(wParam) == IDC_RSTACK_BLEND9) g_this->blend=8;
        if (LOWORD(wParam) == IDC_RSTACK_BLEND10) g_this->blend=9;
        if (LOWORD(wParam) == IDC_RSTACK_BLEND11) g_this->blend=10;
        if (LOWORD(wParam) == IDC_RSTACK_BLEND12) g_this->blend=11;
      }
      if (LOWORD(wParam) == IDC_COMBO1 && HIWORD(wParam) == CBN_SELCHANGE)
      {
        int i=SendDlgItemMessage(hwndDlg,IDC_COMBO1,CB_GETCURSEL,0,0);
        if (i != CB_ERR)
          g_this->which=i;
      }
      return 0;
    return 0;
	  case WM_NOTIFY:
		  if (LOWORD(wParam) == IDC_BLENDSLIDE)
			  g_this->adjblend_val = SendDlgItemMessage(hwndDlg, IDC_BLENDSLIDE, TBM_GETPOS, 0, 0);
    return 0;
	}
	return 0;
}

