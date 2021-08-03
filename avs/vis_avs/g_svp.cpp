#include "g__lib.h"
#include "g__defs.h"
#include "c_svp.h"
#include "resource.h"
#include <windows.h>


int win32_dlgproc_svp(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM)
{
  C_THISCLASS* g_this = (C_THISCLASS*)g_current_render;

	switch (uMsg)
	{
		case WM_INITDIALOG:
      loadComboBox(GetDlgItem(hwndDlg,IDC_COMBO1), "*.SVP", g_this->m_library);
      loadComboBox(GetDlgItem(hwndDlg,IDC_COMBO1), "*.UVS", g_this->m_library);
 			return 1;
    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_COMBO1:
          {
            int a=SendDlgItemMessage(hwndDlg,IDC_COMBO1,CB_GETCURSEL,0,0);
            if (a != CB_ERR)
            {
              if (SendDlgItemMessage(hwndDlg,IDC_COMBO1,CB_GETLBTEXT,a,(LPARAM)g_this->m_library) == CB_ERR)
              {
                g_this->m_library[0]=0;
              }
            }
            else 
              g_this->m_library[0]=0;
            g_this->SetLibrary();
          }
          return 0;
      }
    return 0;
	}
	return 0;
}

