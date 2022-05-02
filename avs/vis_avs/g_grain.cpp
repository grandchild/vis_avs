#include "c_grain.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int win32_dlgproc_grain(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {

	E_Grain* g_this = (E_Grain*)g_current_render;
	AVS_Parameter_Handle p_blend_mode = g_this->info.parameters[0].handle;
	const Parameter& p_amount = g_this->info.parameters[1];
	AVS_Parameter_Handle p_static = g_this->info.parameters[2].handle;

	switch (uMsg) {
		case WM_INITDIALOG:
			SendDlgItemMessage(hwndDlg, IDC_MAX, TBM_SETTICFREQ, 10, 0);
			SendDlgItemMessage(hwndDlg, IDC_MAX, TBM_SETRANGE, TRUE, MAKELONG(p_amount.int_min, p_amount.int_max));
			SendDlgItemMessage(hwndDlg, IDC_MAX, TBM_SETPOS, TRUE, g_this->get_int(p_amount.handle));
			if (g_this->enabled) CheckDlgButton(hwndDlg, IDC_CHECK1, BST_CHECKED);
			if (g_this->get_bool(p_static))
				CheckDlgButton(hwndDlg, IDC_STATGRAIN, BST_CHECKED);
			switch (g_this->get_int(p_blend_mode)) {
				default: 
				case 0: CheckDlgButton(hwndDlg, IDC_REPLACE, BST_CHECKED); break;
				case 1: CheckDlgButton(hwndDlg, IDC_ADDITIVE, BST_CHECKED); break;
				case 2: CheckDlgButton(hwndDlg, IDC_5050, BST_CHECKED); break;
			}
			return 1;
		case WM_NOTIFY: {
			if (LOWORD(wParam) == IDC_MAX) {
				g_this->set_int(
					p_amount.handle,
					SendDlgItemMessage(hwndDlg, IDC_MAX, TBM_GETPOS, 0, 0)
				);
			}
			return 0;
		}
		case WM_COMMAND:
			if ((LOWORD(wParam) == IDC_CHECK1) || (LOWORD(wParam) == IDC_STATGRAIN)
				|| (LOWORD(wParam) == IDC_ADDITIVE) || (LOWORD(wParam) == IDC_REPLACE)
				|| (LOWORD(wParam) == IDC_5050)) {
				g_this->set_enabled(IsDlgButtonChecked(hwndDlg, IDC_CHECK1));
				
				int64_t blend_mode = 0;
				if (IsDlgButtonChecked(hwndDlg, IDC_ADDITIVE)) {
					blend_mode = 1;
				}

				if (IsDlgButtonChecked(hwndDlg, IDC_5050)) {
					blend_mode = 2;
				}
				g_this->set_int(p_blend_mode, blend_mode);
				g_this->set_bool(p_static, IsDlgButtonChecked(hwndDlg, IDC_STATGRAIN));
			}
			return 0;
	}
	return 0;
}
