#include "e_bassspin.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>

int win32_dlgproc_bassspin(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    auto g_this = (E_BassSpin*)g_current_render;
    AVS_Parameter_Handle p_enabled_left = BassSpin_Info::parameters[0].handle;
    AVS_Parameter_Handle p_enabled_right = BassSpin_Info::parameters[1].handle;
    AVS_Parameter_Handle p_color_left = BassSpin_Info::parameters[2].handle;
    AVS_Parameter_Handle p_color_right = BassSpin_Info::parameters[3].handle;
    AVS_Parameter_Handle p_mode = BassSpin_Info::parameters[4].handle;

    switch (uMsg) {
        case WM_INITDIALOG:
            CheckDlgButton(hwndDlg, IDC_LEFT, g_this->get_bool(p_enabled_left));
            CheckDlgButton(hwndDlg, IDC_RIGHT, g_this->get_bool(p_enabled_right));
            CheckDlgButton(
                hwndDlg, IDC_LINES, g_this->get_int(p_mode) == BASSSPIN_MODE_OUTLINE);
            CheckDlgButton(
                hwndDlg, IDC_TRI, g_this->get_int(p_mode) == BASSSPIN_MODE_FILLED);
            return 1;
        case WM_DRAWITEM: {
            auto di = (DRAWITEMSTRUCT*)lParam;
            switch (di->CtlID) {
                case IDC_LC:
                    GR_DrawColoredButton(di, g_this->get_color(p_color_left));
                    break;
                case IDC_RC:
                    GR_DrawColoredButton(di, g_this->get_color(p_color_right));
                    break;
            }
            return 0;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_LINES:
                case IDC_TRI:
                    g_this->set_int(p_mode,
                                    IsDlgButtonChecked(hwndDlg, IDC_TRI)
                                        ? BASSSPIN_MODE_FILLED
                                        : BASSSPIN_MODE_OUTLINE);
                    return 0;
                case IDC_LEFT:
                    g_this->set_bool(p_enabled_left,
                                     IsDlgButtonChecked(hwndDlg, IDC_LEFT));
                    return 0;
                case IDC_RIGHT:
                    g_this->set_bool(p_enabled_right,
                                     IsDlgButtonChecked(hwndDlg, IDC_RIGHT));
                    return 0;
                case IDC_LC: {
                    auto color = (uint32_t)g_this->get_color(p_color_left);
                    GR_SelectColor(hwndDlg, (int32_t*)&color);
                    g_this->set_color(p_color_left, color);
                    InvalidateRect(GetDlgItem(hwndDlg, LOWORD(wParam)), NULL, FALSE);
                    return 0;
                }
                case IDC_RC: {
                    auto color = (uint32_t)g_this->get_color(p_color_right);
                    GR_SelectColor(hwndDlg, (int32_t*)&color);
                    g_this->set_color(p_color_right, color);
                    InvalidateRect(GetDlgItem(hwndDlg, LOWORD(wParam)), NULL, FALSE);
                    return 0;
                }
            }
            return 0;
    }
    return 0;
}
