#include "g__lib.h"
#include "g__defs.h"
#include "c_transition.h"
#include "resource.h"
#include <windows.h>
#include <commctrl.h>
#include "render.h"
#include "cfgwnd.h"


int win32_dlgproc_transition(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
      {
			int x;
      for (x = 0; x < sizeof(g_render_transition->transitionmodes)/sizeof(g_render_transition->transitionmodes[0]); x ++)
	      SendDlgItemMessage(hwndDlg,IDC_TRANSITION,CB_ADDSTRING,0,(LPARAM)g_render_transition->transitionmodes[x]);
      SendDlgItemMessage(hwndDlg,IDC_TRANSITION,CB_SETCURSEL,(WPARAM)cfg_transition_mode&0x7fff,0);
			SendDlgItemMessage(hwndDlg, IDC_SPEED, TBM_SETRANGE, TRUE, MAKELONG(1, 32));
			SendDlgItemMessage(hwndDlg, IDC_SPEED, TBM_SETPOS, TRUE, cfg_transitions_speed);
      if (cfg_transition_mode&0x8000) CheckDlgButton(hwndDlg,IDC_CHECK9,BST_CHECKED);
      if (cfg_transitions&1) CheckDlgButton(hwndDlg,IDC_CHECK2,BST_CHECKED);
      if (cfg_transitions&2) CheckDlgButton(hwndDlg,IDC_CHECK1,BST_CHECKED);
      if (cfg_transitions&4) CheckDlgButton(hwndDlg,IDC_CHECK8,BST_CHECKED);
      if (cfg_transitions2&1) CheckDlgButton(hwndDlg,IDC_CHECK10,BST_CHECKED);
      if (cfg_transitions2&2) CheckDlgButton(hwndDlg,IDC_CHECK11,BST_CHECKED);
      if (cfg_transitions2&4) CheckDlgButton(hwndDlg,IDC_CHECK3,BST_CHECKED);
      if (cfg_transitions2&32) CheckDlgButton(hwndDlg,IDC_CHECK4,BST_CHECKED);
      if (!(cfg_transitions2&128)) CheckDlgButton(hwndDlg,IDC_CHECK5,BST_CHECKED);

      }
 		return 1;
    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_TRANSITION:
          if (HIWORD(wParam) == CBN_SELCHANGE)
          {
            int r=SendDlgItemMessage(hwndDlg,IDC_TRANSITION,CB_GETCURSEL,0,0);
            if (r!=CB_ERR) 
            {
              cfg_transition_mode&=~0x7fff;
							cfg_transition_mode |= r;
            }
          }
	        break;
        case IDC_CHECK9:
          cfg_transition_mode&=0x7fff;
          cfg_transition_mode |= IsDlgButtonChecked(hwndDlg,IDC_CHECK9)?0x8000:0;
		      break;
        case IDC_CHECK2:
          cfg_transitions &= ~1;
          cfg_transitions |= IsDlgButtonChecked(hwndDlg,IDC_CHECK2)?1:0;
		      break;
        case IDC_CHECK1:
          cfg_transitions &= ~2;
          cfg_transitions |= IsDlgButtonChecked(hwndDlg,IDC_CHECK1)?2:0;
		      break;
        case IDC_CHECK8:
          cfg_transitions &= ~4;
          cfg_transitions |= IsDlgButtonChecked(hwndDlg,IDC_CHECK8)?4:0;
		      break;
        case IDC_CHECK10:
          cfg_transitions2 &= ~1;
          cfg_transitions2 |= IsDlgButtonChecked(hwndDlg,IDC_CHECK10)?1:0;
		      break;
        case IDC_CHECK11:
          cfg_transitions2 &= ~2;
          cfg_transitions2 |= IsDlgButtonChecked(hwndDlg,IDC_CHECK11)?2:0;
		      break;
        case IDC_CHECK3:
          cfg_transitions2 &= ~4;
          cfg_transitions2 |= IsDlgButtonChecked(hwndDlg,IDC_CHECK3)?4:0;
		      break;
        case IDC_CHECK4:
          cfg_transitions2 &= ~32;
          cfg_transitions2 |= IsDlgButtonChecked(hwndDlg,IDC_CHECK4)?32:0;
		      break;
        case IDC_CHECK5:
          cfg_transitions2 &= ~128;
          cfg_transitions2 |= IsDlgButtonChecked(hwndDlg,IDC_CHECK5)?0:128;
		      break;
			}
	    break;
	case WM_NOTIFY:
		if (LOWORD(wParam) == IDC_SPEED)
			cfg_transitions_speed = SendDlgItemMessage(hwndDlg, IDC_SPEED, TBM_GETPOS, 0, 0);
		break;
	}
	return 0;
}

