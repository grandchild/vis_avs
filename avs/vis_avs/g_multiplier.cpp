#include "g__lib.h"
#include "g__defs.h"
#include "c_multiplier.h"
#include "resource.h"
#include <windows.h>


int win32_dlgproc_multiplier(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM)
{
	C_THISCLASS* g_ConfigThis = (C_THISCLASS*)g_current_render;
	int id = 0;
	switch (uMsg)
	{
		case WM_COMMAND:
			if (HIWORD(wParam) == BN_CLICKED) {
				id = LOWORD(wParam);
				switch (id) {
				case IDC_XI:
					g_ConfigThis->config.ml = MD_XI;
					break;
				case IDC_X8:
					g_ConfigThis->config.ml = MD_X8;
					break;
				case IDC_X4:
					g_ConfigThis->config.ml = MD_X4;
					break;
				case IDC_X2:
					g_ConfigThis->config.ml = MD_X2;
					break;
				case IDC_X05:
					g_ConfigThis->config.ml = MD_X05;
					break;
				case IDC_X025:
					g_ConfigThis->config.ml = MD_X025;
					break;
				case IDC_X0125:
					g_ConfigThis->config.ml = MD_X0125;
					break;
				case IDC_XS:
					g_ConfigThis->config.ml = MD_XS;
					break;
				}
			}
			return 0;

		case WM_INITDIALOG:
			switch (g_ConfigThis->config.ml) {
			case MD_XI:
				id = IDC_XI;
				break;
			case MD_X8:
				id = IDC_X8;
				break;
			case MD_X4:
				id = IDC_X4;
				break;
			case MD_X2:
				id = IDC_X2;
				break;
			case MD_X05:
				id = IDC_X05;
				break;
			case MD_X025:
				id = IDC_X025;
				break;
			case MD_X0125:
				id = IDC_X0125;
				break;
			case MD_XS:
				id = IDC_XS;
				break;
			}
			if(id) {
				SendMessage(GetDlgItem(hwndDlg, id), BM_SETCHECK, BST_CHECKED, 0);
			}

			return 1;

		case WM_DESTROY:
			return 1;
	}
	return 0;
}

