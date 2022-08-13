#include "e_interleave.h"

#include "g__lib.h"

#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int win32_dlgproc_interleave(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    auto* g_this = (E_Interleave*)g_current_render;
    const Parameter& p_x = Interleave_Info::parameters[0];
    const Parameter& p_y = Interleave_Info::parameters[1];
    AVS_Parameter_Handle p_color = Interleave_Info::parameters[2].handle;
    AVS_Parameter_Handle p_on_beat = Interleave_Info::parameters[3].handle;
    const Parameter& p_on_beat_x = Interleave_Info::parameters[4];
    const Parameter& p_on_beat_y = Interleave_Info::parameters[5];
    const Parameter& p_on_beat_duration = Interleave_Info::parameters[6];
    AVS_Parameter_Handle p_blend_mode = Interleave_Info::parameters[7].handle;

    switch (uMsg) {
        case WM_INITDIALOG: {
            auto x_range = MAKELONG(p_x.int_min, p_x.int_max);
            SendDlgItemMessage(hwndDlg, IDC_X, TBM_SETRANGE, TRUE, x_range);
            auto x = g_this->get_int(p_x.handle);
            SendDlgItemMessage(hwndDlg, IDC_X, TBM_SETPOS, TRUE, (int32_t)x);
            SendDlgItemMessage(hwndDlg, IDC_X, TBM_SETTICFREQ, 1, 0);

            auto y_range = MAKELONG(p_y.int_min, p_y.int_max);
            SendDlgItemMessage(hwndDlg, IDC_Y, TBM_SETRANGE, TRUE, y_range);
            auto y = g_this->get_int(p_y.handle);
            SendDlgItemMessage(hwndDlg, IDC_Y, TBM_SETPOS, TRUE, (int32_t)y);
            SendDlgItemMessage(hwndDlg, IDC_Y, TBM_SETTICFREQ, 1, 0);

            auto on_beat_x_range = MAKELONG(p_on_beat_x.int_min, p_on_beat_x.int_max);
            SendDlgItemMessage(hwndDlg, IDC_X2, TBM_SETRANGE, TRUE, on_beat_x_range);
            auto on_beat_x = g_this->get_int(p_on_beat_x.handle);
            SendDlgItemMessage(hwndDlg, IDC_X2, TBM_SETPOS, TRUE, (int32_t)on_beat_x);
            SendDlgItemMessage(hwndDlg, IDC_X2, TBM_SETTICFREQ, 1, 0);

            auto on_beat_y_range = MAKELONG(p_on_beat_y.int_min, p_on_beat_y.int_max);
            SendDlgItemMessage(hwndDlg, IDC_Y2, TBM_SETRANGE, TRUE, on_beat_y_range);
            auto on_beat_y = g_this->get_int(p_on_beat_y.handle);
            SendDlgItemMessage(hwndDlg, IDC_Y2, TBM_SETPOS, TRUE, (int32_t)on_beat_y);
            SendDlgItemMessage(hwndDlg, IDC_Y2, TBM_SETTICFREQ, 1, 0);

            auto on_beat_duration_range =
                MAKELONG(p_on_beat_duration.int_min, p_on_beat_duration.int_max);
            SendDlgItemMessage(
                hwndDlg, IDC_X3, TBM_SETRANGE, TRUE, on_beat_duration_range);
            auto on_beat_duration = g_this->get_int(p_on_beat_duration.handle);
            SendDlgItemMessage(
                hwndDlg, IDC_X3, TBM_SETPOS, TRUE, (int32_t)on_beat_duration);
            SendDlgItemMessage(hwndDlg, IDC_X3, TBM_SETTICFREQ, 1, 0);

            CheckDlgButton(hwndDlg, IDC_CHECK1, g_this->enabled);
            CheckDlgButton(hwndDlg, IDC_CHECK8, g_this->get_bool(p_on_beat));
            switch (g_this->get_int(p_blend_mode)) {
                default:
                case BLEND_SIMPLE_REPLACE:
                    CheckDlgButton(hwndDlg, IDC_REPLACE, BST_CHECKED);
                    break;
                case BLEND_SIMPLE_ADDITIVE:
                    CheckDlgButton(hwndDlg, IDC_ADDITIVE, BST_CHECKED);
                    break;
                case BLEND_SIMPLE_5050:
                    CheckDlgButton(hwndDlg, IDC_5050, BST_CHECKED);
                    break;
            }
            return 1;
        }
        case WM_HSCROLL: {
            if (LOWORD(wParam) == TB_THUMBTRACK) {
                int64_t slider_value = HIWORD(wParam);
                if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_X)) {
                    g_this->set_int(p_x.handle, slider_value);
                } else if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_Y)) {
                    g_this->set_int(p_y.handle, slider_value);
                } else if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_X2)) {
                    g_this->set_int(p_on_beat_x.handle, slider_value);
                } else if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_Y2)) {
                    g_this->set_int(p_on_beat_y.handle, slider_value);
                } else if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_X3)) {
                    g_this->set_int(p_on_beat_duration.handle, slider_value);
                }
            }
            return 0;
        }
        case WM_DRAWITEM: {
            auto* di = (DRAWITEMSTRUCT*)lParam;
            if (di->CtlID == IDC_DEFCOL) {
                unsigned int color = g_this->get_color(p_color);
                color = ((color >> 16) & 0xff) | (color & 0xff00)
                        | ((color << 16) & 0xff0000);

                HPEN hPen, hOldPen;
                HBRUSH hBrush, hOldBrush;
                LOGBRUSH lb = {BS_SOLID, color, 0};
                hPen = (HPEN)CreatePen(PS_SOLID, 0, color);
                hBrush = CreateBrushIndirect(&lb);
                hOldPen = (HPEN)SelectObject(di->hDC, hPen);
                hOldBrush = (HBRUSH)SelectObject(di->hDC, hBrush);
                Rectangle(di->hDC,
                          di->rcItem.left,
                          di->rcItem.top,
                          di->rcItem.right,
                          di->rcItem.bottom);
                SelectObject(di->hDC, hOldPen);
                SelectObject(di->hDC, hOldBrush);
                DeleteObject(hBrush);
                DeleteObject(hPen);
            }
            return 0;
        }
        case WM_COMMAND: {
            auto element = LOWORD(wParam);
            if ((element == IDC_CHECK1) || (element == IDC_CHECK8)
                || (element == IDC_ADDITIVE) || (element == IDC_REPLACE)
                || (element == IDC_5050)) {
                g_this->set_enabled(IsDlgButtonChecked(hwndDlg, IDC_CHECK1));
                g_this->set_bool(p_on_beat, IsDlgButtonChecked(hwndDlg, IDC_CHECK8));
                if (element == IDC_REPLACE || element == IDC_ADDITIVE
                    || element == IDC_5050) {
                    if (IsDlgButtonChecked(hwndDlg, IDC_REPLACE)) {
                        g_this->set_int(p_blend_mode, BLEND_SIMPLE_REPLACE);
                    } else if (IsDlgButtonChecked(hwndDlg, IDC_ADDITIVE)) {
                        g_this->set_int(p_blend_mode, BLEND_SIMPLE_ADDITIVE);
                    } else if (IsDlgButtonChecked(hwndDlg, IDC_5050)) {
                        g_this->set_int(p_blend_mode, BLEND_SIMPLE_5050);
                    }
                }
            }
            if (LOWORD(wParam) == IDC_DEFCOL) {
                unsigned int color = g_this->get_color(p_color);
                static COLORREF custcolors[16];
                CHOOSECOLOR cs;
                cs.lStructSize = sizeof(cs);
                cs.hwndOwner = hwndDlg;
                cs.hInstance = nullptr;
                cs.rgbResult = ((color >> 16) & 0xff) | (color & 0xff00)
                               | ((color << 16) & 0xff0000);
                cs.lpCustColors = custcolors;
                cs.Flags = CC_RGBINIT | CC_FULLOPEN;
                if (ChooseColor(&cs)) {
                    color = ((cs.rgbResult >> 16) & 0xff) | (cs.rgbResult & 0xff00)
                            | ((cs.rgbResult << 16) & 0xff0000);
                    g_this->set_color(p_color, color);
                }
                InvalidateRect(GetDlgItem(hwndDlg, IDC_DEFCOL), nullptr, TRUE);
            }
            return 0;
        }
        default: break;
    }
    return 0;
}
