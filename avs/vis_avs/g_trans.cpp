#include "g__lib.h"
#include "g__defs.h"
#include "c_trans.h"
#include "resource.h"
#include <windows.h>


int win32_dlgproc_movement(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM)
{
  C_THISCLASS* g_this = (C_THISCLASS*)g_current_render;

  static int isstart;
	switch (uMsg)
	{
		case WM_INITDIALOG:
      {
        unsigned int x;
        for (x = 0; x < sizeof(g_this->descriptions)/sizeof(g_this->descriptions[0]); x ++) 
        {
          SendDlgItemMessage(hwndDlg,IDC_LIST1,LB_ADDSTRING,0,(long)(g_this->descriptions[x].list_desc));
        }

        isstart=1;
        SetDlgItemText(hwndDlg,IDC_EDIT1,(char*)g_this->effect_exp.c_str());
        // After we set whatever value into the edit box, that's the new saved value (ie: don't change the save format)
        isstart=0;

        if (g_this->blend)
          CheckDlgButton(hwndDlg,IDC_CHECK1,BST_CHECKED);
        if (g_this->subpixel)
          CheckDlgButton(hwndDlg,IDC_CHECK4,BST_CHECKED);
        if (g_this->wrap)
          CheckDlgButton(hwndDlg,IDC_WRAP,BST_CHECKED);
        if (g_this->rectangular)
          CheckDlgButton(hwndDlg,IDC_CHECK3,BST_CHECKED);
        if (g_this->sourcemapped&2)
          CheckDlgButton(hwndDlg,IDC_CHECK2,BST_INDETERMINATE);
        else if (g_this->sourcemapped&1)
          CheckDlgButton(hwndDlg,IDC_CHECK2,BST_CHECKED);

        SendDlgItemMessage(hwndDlg,IDC_LIST1,LB_ADDSTRING,0,(long)"(user defined)");
        SendDlgItemMessage(hwndDlg,IDC_LIST1,LB_SETCURSEL,(g_this->effect==32767)?sizeof(g_this->descriptions)/sizeof(g_this->descriptions[0]):g_this->effect,0);
        if (g_this->effect == 32767)
        {
          EnableWindow(GetDlgItem(hwndDlg,IDC_EDIT1),1);
          EnableWindow(GetDlgItem(hwndDlg,IDC_CHECK3),1);
          EnableWindow(GetDlgItem(hwndDlg,IDC_LABEL1),1);          
        }
        else if (g_this->effect >= 0 && g_this->effect <= REFFECT_MAX)
        {
          if (strlen(g_this->descriptions[g_this->effect].eval_desc) > 0)
          {
            SetDlgItemText(hwndDlg,IDC_EDIT1,g_this->descriptions[g_this->effect].eval_desc);
            CheckDlgButton(hwndDlg,IDC_CHECK3,g_this->descriptions[g_this->effect].uses_rect?BST_CHECKED:0);
            EnableWindow(GetDlgItem(hwndDlg,IDC_EDIT1),1);
            EnableWindow(GetDlgItem(hwndDlg,IDC_CHECK3),1);
            EnableWindow(GetDlgItem(hwndDlg,IDC_LABEL1),1);
          }
          else
          {
            EnableWindow(GetDlgItem(hwndDlg,IDC_EDIT1),0);
            EnableWindow(GetDlgItem(hwndDlg,IDC_CHECK3),0);
            EnableWindow(GetDlgItem(hwndDlg,IDC_LABEL1),0);
          }
        }
        else
        {
          EnableWindow(GetDlgItem(hwndDlg,IDC_EDIT1),0);
          EnableWindow(GetDlgItem(hwndDlg,IDC_CHECK3),0);
          EnableWindow(GetDlgItem(hwndDlg,IDC_LABEL1),0);
        }
      }
			return 1;
    case WM_TIMER:
      if (wParam == 1)
      {
        KillTimer(hwndDlg,1);
        EnterCriticalSection(&g_this->rcs);
        g_this->effect=32767;
        g_this->effect_exp = string_from_dlgitem(hwndDlg,IDC_EDIT1);
        g_this->effect_exp_ch=1;
        LeaveCriticalSection(&g_this->rcs);
      }
    return 0;
    case WM_COMMAND:
      if (LOWORD(wParam) == IDC_EDIT1 && HIWORD(wParam) == EN_CHANGE)
      {
        KillTimer(hwndDlg,1);
        if (!isstart) SetTimer(hwndDlg,1,1000,NULL);

        // If someone edits the editwnd, force the "(user defined)" to be the new selection.
        if (SendDlgItemMessage(hwndDlg,IDC_LIST1,LB_GETCURSEL,0,0) != sizeof(g_this->descriptions)/sizeof(g_this->descriptions[0]))
        {
          g_this->rectangular=IsDlgButtonChecked(hwndDlg,IDC_CHECK3)?1:0;
          SendDlgItemMessage(hwndDlg,IDC_LIST1,LB_SETCURSEL,sizeof(g_this->descriptions)/sizeof(g_this->descriptions[0]),0);
        }
      }
      if (LOWORD(wParam)==IDC_LIST1 && HIWORD(wParam)==LBN_SELCHANGE)
      {
        int t;
        t=SendDlgItemMessage(hwndDlg,IDC_LIST1,LB_GETCURSEL,0,0);
        if (t == sizeof(g_this->descriptions)/sizeof(g_this->descriptions[0]))
        {
          g_this->effect=32767;
          EnableWindow(GetDlgItem(hwndDlg,IDC_EDIT1),1);
          EnableWindow(GetDlgItem(hwndDlg,IDC_CHECK3),1);
          EnableWindow(GetDlgItem(hwndDlg,IDC_LABEL1),1);

          SetDlgItemText(hwndDlg,IDC_EDIT1,(char*)g_this->effect_exp.c_str());
          CheckDlgButton(hwndDlg,IDC_CHECK3,g_this->rectangular);

          // always reinit =)
          {
            EnterCriticalSection(&g_this->rcs);
            g_this->effect_exp = string_from_dlgitem(hwndDlg,IDC_EDIT1);
            g_this->effect_exp_ch=1;
            LeaveCriticalSection(&g_this->rcs);
          }
        }
        else 
        {
          g_this->effect=t;

          // If there is a string to stuff in the eval box,
          if (strlen(g_this->descriptions[t].eval_desc) > 0)
          {
            // save the value to be able to restore it later
            // stuff it and make sure the boxes are editable
            SetDlgItemText(hwndDlg,IDC_EDIT1,g_this->descriptions[t].eval_desc);
            EnableWindow(GetDlgItem(hwndDlg,IDC_EDIT1),1);
            EnableWindow(GetDlgItem(hwndDlg,IDC_CHECK3),1);
            EnableWindow(GetDlgItem(hwndDlg,IDC_LABEL1),1);
            CheckDlgButton(hwndDlg,IDC_CHECK3,g_this->descriptions[t].uses_rect?BST_CHECKED:0);
          }
          else
          {
            // otherwise, they're not editable.
            CheckDlgButton(hwndDlg,IDC_CHECK3,g_this->rectangular?BST_CHECKED:0);
            SetDlgItemText(hwndDlg,IDC_EDIT1,"");
            EnableWindow(GetDlgItem(hwndDlg,IDC_EDIT1),0);
            EnableWindow(GetDlgItem(hwndDlg,IDC_CHECK3),0);
            EnableWindow(GetDlgItem(hwndDlg,IDC_LABEL1),0);
          }
        }

      }
      if (LOWORD(wParam)==IDC_CHECK1)
      {
        g_this->blend=IsDlgButtonChecked(hwndDlg,IDC_CHECK1)?1:0;
      }
      if (LOWORD(wParam)==IDC_CHECK4)
      {
        g_this->subpixel=IsDlgButtonChecked(hwndDlg,IDC_CHECK4)?1:0;
        g_this->effect_exp_ch=1;
      }
      if (LOWORD(wParam)==IDC_WRAP)
      {
        g_this->wrap=IsDlgButtonChecked(hwndDlg,IDC_WRAP)?1:0;
        g_this->effect_exp_ch=1;
      }
      if (LOWORD(wParam)==IDC_CHECK2)
      {
        int a=IsDlgButtonChecked(hwndDlg,IDC_CHECK2);
        if (a == BST_INDETERMINATE) 
          g_this->sourcemapped=2;
        else if (a == BST_CHECKED) 
          g_this->sourcemapped=1;
        else
          g_this->sourcemapped=0;
      }
      if (LOWORD(wParam) == IDC_CHECK3)
      {
          g_this->rectangular=IsDlgButtonChecked(hwndDlg,IDC_CHECK3)?1:0;
          if (SendDlgItemMessage(hwndDlg,IDC_LIST1,LB_GETCURSEL,0,0) != sizeof(g_this->descriptions)/sizeof(g_this->descriptions[0]))
          {
            EnterCriticalSection(&g_this->rcs);
            g_this->effect=32767;
            g_this->effect_exp = string_from_dlgitem(hwndDlg,IDC_EDIT1);
            g_this->effect_exp_ch=1;
            LeaveCriticalSection(&g_this->rcs);
            SendDlgItemMessage(hwndDlg,IDC_LIST1,LB_SETCURSEL,sizeof(g_this->descriptions)/sizeof(g_this->descriptions[0]),0);
          }
          else
            g_this->effect_exp_ch=1;
      }
      if (LOWORD(wParam) == IDC_BUTTON2)
      {
        compilerfunctionlist(hwndDlg, g_this->help_text);
      }
      return 0;
    return 0;
	}
	return 0;
}

