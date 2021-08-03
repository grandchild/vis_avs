#include "c__defs.h"
#include "g__lib.h"
#include "g__defs.h"
#include "c_dmove.h"
#include "resource.h"
#include <windows.h>


int win32_dlgproc_dynamicmovement(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM)
{
  C_THISCLASS* g_this = (C_THISCLASS*)g_current_render;

  static int isstart;
	switch (uMsg)
	{
		case WM_INITDIALOG:
      isstart=1;
      SetDlgItemText(hwndDlg,IDC_EDIT1,(char*)g_this->effect_exp[0].c_str());
      SetDlgItemText(hwndDlg,IDC_EDIT2,(char*)g_this->effect_exp[1].c_str());
      SetDlgItemText(hwndDlg,IDC_EDIT3,(char*)g_this->effect_exp[2].c_str());
      SetDlgItemText(hwndDlg,IDC_EDIT4,(char*)g_this->effect_exp[3].c_str());
      if (g_this->blend)
        CheckDlgButton(hwndDlg,IDC_CHECK1,BST_CHECKED);
      if (g_this->subpixel)
        CheckDlgButton(hwndDlg,IDC_CHECK2,BST_CHECKED);
      if (g_this->rectcoords)
        CheckDlgButton(hwndDlg,IDC_CHECK3,BST_CHECKED);
      if (g_this->wrap)
        CheckDlgButton(hwndDlg,IDC_WRAP,BST_CHECKED);
      if (g_this->nomove)
        CheckDlgButton(hwndDlg,IDC_NOMOVEMENT,BST_CHECKED);

      SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_ADDSTRING, 0, (int)"Current");
  		{
				int i=0;
				char txt[64];
				for (i=0;i<NBUF;i++)
        {
					 wsprintf(txt, "Buffer %d", i+1);
					 SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_ADDSTRING, 0, (int)txt);
				}
      }
		  SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_SETCURSEL, (WPARAM) g_this->buffern, 0);

      SetDlgItemInt(hwndDlg,IDC_EDIT5,g_this->m_xres,FALSE);
      SetDlgItemInt(hwndDlg,IDC_EDIT6,g_this->m_yres,FALSE);
      isstart=0;

    return 1;
    case WM_COMMAND:
      if (LOWORD(wParam) == IDC_BUTTON1) 
      {
        char *text="Dynamic Movement\0"
          "Dynamic movement help goes here (send me some :)";
        compilerfunctionlist(hwndDlg,text);
      }

      if (LOWORD(wParam)==IDC_CHECK1)
      {
        g_this->blend=IsDlgButtonChecked(hwndDlg,IDC_CHECK1)?1:0;
      }
      if (LOWORD(wParam)==IDC_CHECK2)
      {
        g_this->subpixel=IsDlgButtonChecked(hwndDlg,IDC_CHECK2)?1:0;
      }
      if (LOWORD(wParam)==IDC_CHECK3)
      {
        g_this->rectcoords=IsDlgButtonChecked(hwndDlg,IDC_CHECK3)?1:0;
      }
      if (LOWORD(wParam)==IDC_WRAP)
      {
        g_this->wrap=IsDlgButtonChecked(hwndDlg,IDC_WRAP)?1:0;
      }
      if (LOWORD(wParam)==IDC_NOMOVEMENT)
      {
        g_this->nomove=IsDlgButtonChecked(hwndDlg,IDC_NOMOVEMENT)?1:0;
      }
      // Load preset examples from the examples table.
      if (LOWORD(wParam) == IDC_BUTTON4)
      {
        RECT r;
        HMENU hMenu;
        MENUITEMINFO i={sizeof(i),};
        hMenu=CreatePopupMenu();
        int x;
        for (x = 0; x < sizeof(g_this->presets)/sizeof(g_this->presets[0]); x ++)
        {
            i.fMask=MIIM_TYPE|MIIM_DATA|MIIM_ID;
            i.fType=MFT_STRING;
            i.wID = x+16;
            i.dwTypeData=g_this->presets[x].name;
            i.cch=strlen(g_this->presets[x].name);
            InsertMenuItem(hMenu,x,TRUE,&i);
        }
        GetWindowRect(GetDlgItem(hwndDlg,IDC_BUTTON1),&r);
        x=TrackPopupMenu(hMenu,TPM_LEFTALIGN|TPM_TOPALIGN|TPM_RETURNCMD|TPM_RIGHTBUTTON|TPM_LEFTBUTTON|TPM_NONOTIFY,r.right,r.top,0,hwndDlg,NULL);
        if (x >= 16 && x < 16+sizeof(g_this->presets)/sizeof(g_this->presets[0]))
        {
          SetDlgItemText(hwndDlg,IDC_EDIT1,g_this->presets[x-16].point);
          SetDlgItemText(hwndDlg,IDC_EDIT2,g_this->presets[x-16].frame);
          SetDlgItemText(hwndDlg,IDC_EDIT3,g_this->presets[x-16].beat);
          SetDlgItemText(hwndDlg,IDC_EDIT4,g_this->presets[x-16].init);
          SetDlgItemInt(hwndDlg,IDC_EDIT5,g_this->presets[x-16].grid1,FALSE);
          SetDlgItemInt(hwndDlg,IDC_EDIT6,g_this->presets[x-16].grid2,FALSE);
          if (g_this->presets[x-16].rect)
          {
            g_this->rectcoords = 1;
            CheckDlgButton(hwndDlg,IDC_CHECK3,BST_CHECKED);
          }
          else
          {
            g_this->rectcoords = 0;
            CheckDlgButton(hwndDlg,IDC_CHECK3,0);
          }
          if (g_this->presets[x-16].wrap)
          {
            g_this->wrap = 1;
            CheckDlgButton(hwndDlg,IDC_WRAP,BST_CHECKED);
          }
          else
          {
            g_this->wrap = 0;
            CheckDlgButton(hwndDlg,IDC_WRAP,0);
          }
          SendMessage(hwndDlg,WM_COMMAND,MAKEWPARAM(IDC_EDIT4,EN_CHANGE),0);
        }
        DestroyMenu(hMenu);
      }

  	  if (!isstart && HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_COMBO1) // handle clicks to combo box
	  	  g_this->buffern = SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_GETCURSEL, 0, 0);
 
      if (!isstart && HIWORD(wParam) == EN_CHANGE)
      {
        if (LOWORD(wParam) == IDC_EDIT5 || LOWORD(wParam) == IDC_EDIT6)
        {
          BOOL t;
          g_this->m_xres=GetDlgItemInt(hwndDlg,IDC_EDIT5,&t,0);
          g_this->m_yres=GetDlgItemInt(hwndDlg,IDC_EDIT6,&t,0);
        }
      
        if (LOWORD(wParam) == IDC_EDIT1||LOWORD(wParam) == IDC_EDIT2||LOWORD(wParam) == IDC_EDIT3||LOWORD(wParam) == IDC_EDIT4)
        {
          EnterCriticalSection(&g_this->rcs);
          g_this->effect_exp[0] = string_from_dlgitem(hwndDlg,IDC_EDIT1);
          g_this->effect_exp[1] = string_from_dlgitem(hwndDlg,IDC_EDIT2);
          g_this->effect_exp[2] = string_from_dlgitem(hwndDlg,IDC_EDIT3);
          g_this->effect_exp[3] = string_from_dlgitem(hwndDlg,IDC_EDIT4);
          g_this->need_recompile=1;
				  if (LOWORD(wParam) == IDC_EDIT4) g_this->inited = 0;
          LeaveCriticalSection(&g_this->rcs);
        }
      }
    return 0;
  }
	return 0;
}

