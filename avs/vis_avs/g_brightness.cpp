#include "e_brightness.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int win32_dlgproc_brightness(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    auto g_this = (E_Brightness*)g_current_render;
    AVS_Parameter_Handle p_blend_mode = g_this->info.parameters[0].handle;
    const Parameter& p_red = g_this->info.parameters[1];
    const Parameter& p_green = g_this->info.parameters[2];
    const Parameter& p_blue = g_this->info.parameters[3];
    AVS_Parameter_Handle p_separate = g_this->info.parameters[4].handle;
    AVS_Parameter_Handle p_exclude = g_this->info.parameters[5].handle;
    AVS_Parameter_Handle p_exclude_color = g_this->info.parameters[6].handle;
    const Parameter& p_exclude_distance = g_this->info.parameters[7];

    switch (uMsg) {
        case WM_INITDIALOG: {
            auto red = g_this->get_int(p_red.handle);
            SendDlgItemMessage(hwndDlg, IDC_RED, TBM_SETRANGEMIN, TRUE, p_red.int_min);
            SendDlgItemMessage(hwndDlg, IDC_RED, TBM_SETRANGEMAX, TRUE, p_red.int_max);
            SendDlgItemMessage(hwndDlg, IDC_RED, TBM_SETPOS, TRUE, red);
            SendDlgItemMessage(hwndDlg, IDC_RED, TBM_SETTICFREQ, 256, 0);

            auto green = g_this->get_int(p_green.handle);
            SendDlgItemMessage(
                hwndDlg, IDC_GREEN, TBM_SETRANGEMIN, TRUE, p_green.int_min);
            SendDlgItemMessage(
                hwndDlg, IDC_GREEN, TBM_SETRANGEMAX, TRUE, p_green.int_max);
            SendDlgItemMessage(hwndDlg, IDC_GREEN, TBM_SETPOS, TRUE, green);
            SendDlgItemMessage(hwndDlg, IDC_GREEN, TBM_SETTICFREQ, 256, 0);

            auto blue = g_this->get_int(p_blue.handle);
            SendDlgItemMessage(
                hwndDlg, IDC_BLUE, TBM_SETRANGEMIN, TRUE, p_blue.int_min);
            SendDlgItemMessage(
                hwndDlg, IDC_BLUE, TBM_SETRANGEMAX, TRUE, p_blue.int_max);
            SendDlgItemMessage(hwndDlg, IDC_BLUE, TBM_SETPOS, TRUE, blue);
            SendDlgItemMessage(hwndDlg, IDC_BLUE, TBM_SETTICFREQ, 256, 0);

            auto distance = g_this->get_int(p_exclude_distance.handle);
            SendDlgItemMessage(hwndDlg,
                               IDC_DISTANCE,
                               TBM_SETRANGEMIN,
                               TRUE,
                               p_exclude_distance.int_min);
            SendDlgItemMessage(hwndDlg,
                               IDC_DISTANCE,
                               TBM_SETRANGEMAX,
                               TRUE,
                               p_exclude_distance.int_max);
            SendDlgItemMessage(hwndDlg, IDC_DISTANCE, TBM_SETPOS, TRUE, distance);
            SendDlgItemMessage(hwndDlg, IDC_DISTANCE, TBM_SETTICFREQ, 16, 0);

            CheckDlgButton(hwndDlg, IDC_CHECK1, g_this->enabled);
            CheckDlgButton(hwndDlg, IDC_EXCLUDE, g_this->get_bool(p_exclude));
            CheckDlgButton(hwndDlg, IDC_SEPARATE, g_this->get_bool(p_separate));

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
        case WM_NOTIFY: {
            if (LOWORD(wParam) == IDC_DISTANCE) {
                g_this->set_int(
                    p_exclude_distance.handle,
                    SendDlgItemMessage(hwndDlg, IDC_DISTANCE, TBM_GETPOS, 0, 0));
            }
            if (LOWORD(wParam) == IDC_RED) {
                g_this->set_int(p_red.handle,
                                SendDlgItemMessage(hwndDlg, IDC_RED, TBM_GETPOS, 0, 0));
                if (!g_this->get_bool(p_separate)) {
                    auto green = g_this->get_int(p_green.handle);
                    SendDlgItemMessage(hwndDlg, IDC_GREEN, TBM_SETPOS, TRUE, green);
                    auto blue = g_this->get_int(p_blue.handle);
                    SendDlgItemMessage(hwndDlg, IDC_BLUE, TBM_SETPOS, TRUE, blue);
                }
            }
            if (LOWORD(wParam) == IDC_GREEN) {
                g_this->set_int(
                    p_green.handle,
                    SendDlgItemMessage(hwndDlg, IDC_GREEN, TBM_GETPOS, 0, 0));
                if (!g_this->get_bool(p_separate)) {
                    auto red = g_this->get_int(p_red.handle);
                    SendDlgItemMessage(hwndDlg, IDC_RED, TBM_SETPOS, TRUE, red);
                    auto blue = g_this->get_int(p_blue.handle);
                    SendDlgItemMessage(hwndDlg, IDC_BLUE, TBM_SETPOS, TRUE, blue);
                }
            }
            if (LOWORD(wParam) == IDC_BLUE) {
                g_this->set_int(
                    p_blue.handle,
                    SendDlgItemMessage(hwndDlg, IDC_BLUE, TBM_GETPOS, 0, 0));
                if (!g_this->get_bool(p_separate)) {
                    auto red = g_this->get_int(p_red.handle);
                    SendDlgItemMessage(hwndDlg, IDC_RED, TBM_SETPOS, TRUE, red);
                    auto green = g_this->get_int(p_green.handle);
                    SendDlgItemMessage(hwndDlg, IDC_GREEN, TBM_SETPOS, TRUE, green);
                }
            }
            return 0;
        }
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case IDC_DEFCOL: {
                    unsigned int color = g_this->get_color(p_exclude_color);
                    static COLORREF custcolors[16];
                    CHOOSECOLOR cs;
                    cs.lStructSize = sizeof(cs);
                    cs.hwndOwner = hwndDlg;
                    cs.hInstance = 0;
                    cs.rgbResult = ((color >> 16) & 0xff) | (color & 0xff00)
                                   | ((color << 16) & 0xff0000);
                    cs.lpCustColors = custcolors;
                    cs.Flags = CC_RGBINIT | CC_FULLOPEN;
                    if (ChooseColor(&cs)) {
                        color = ((cs.rgbResult >> 16) & 0xff) | (cs.rgbResult & 0xff00)
                                | ((cs.rgbResult << 16) & 0xff0000);
                        g_this->set_color(p_exclude_color, color);
                    }
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_DEFCOL), NULL, TRUE);
                    break;
                }
                case IDC_BRED: {
                    g_this->set_int(p_red.handle, 0);
                    auto red = g_this->get_int(p_red.handle);
                    SendDlgItemMessage(hwndDlg, IDC_RED, TBM_SETPOS, TRUE, red);
                    if (!g_this->get_bool(p_separate)) {
                        auto green = g_this->get_int(p_green.handle);
                        SendDlgItemMessage(hwndDlg, IDC_GREEN, TBM_SETPOS, TRUE, green);
                        auto blue = g_this->get_int(p_blue.handle);
                        SendDlgItemMessage(hwndDlg, IDC_BLUE, TBM_SETPOS, TRUE, blue);
                    }
                    break;
                }
                case IDC_BGREEN: {
                    g_this->set_int(p_green.handle, 0);
                    auto green = g_this->get_int(p_green.handle);
                    SendDlgItemMessage(hwndDlg, IDC_GREEN, TBM_SETPOS, TRUE, green);
                    if (!g_this->get_bool(p_separate)) {
                        auto red = g_this->get_int(p_red.handle);
                        SendDlgItemMessage(hwndDlg, IDC_RED, TBM_SETPOS, TRUE, red);
                        auto blue = g_this->get_int(p_blue.handle);
                        SendDlgItemMessage(hwndDlg, IDC_BLUE, TBM_SETPOS, TRUE, blue);
                    }
                    break;
                }
                case IDC_BBLUE: {
                    g_this->set_int(p_blue.handle, 0);
                    auto blue = g_this->get_int(p_blue.handle);
                    SendDlgItemMessage(hwndDlg, IDC_BLUE, TBM_SETPOS, TRUE, blue);
                    if (!g_this->get_bool(p_separate)) {
                        auto red = g_this->get_int(p_red.handle);
                        SendDlgItemMessage(hwndDlg, IDC_RED, TBM_SETPOS, TRUE, red);
                        auto green = g_this->get_int(p_green.handle);
                        SendDlgItemMessage(hwndDlg, IDC_GREEN, TBM_SETPOS, TRUE, green);
                    }
                    break;
                }
                case IDC_CHECK1:
                case IDC_SEPARATE:
                case IDC_EXCLUDE: {
                    g_this->set_enabled(IsDlgButtonChecked(hwndDlg, IDC_CHECK1));
                    g_this->set_bool(p_separate,
                                     IsDlgButtonChecked(hwndDlg, IDC_SEPARATE));
                    g_this->set_bool(p_exclude,
                                     IsDlgButtonChecked(hwndDlg, IDC_EXCLUDE));
                    auto green = g_this->get_int(p_green.handle);
                    SendDlgItemMessage(hwndDlg, IDC_GREEN, TBM_SETPOS, TRUE, green);
                    auto blue = g_this->get_int(p_blue.handle);
                    SendDlgItemMessage(hwndDlg, IDC_BLUE, TBM_SETPOS, TRUE, blue);
                    break;
                }
                case IDC_REPLACE:
                case IDC_5050:
                case IDC_ADDITIVE: {
                    if (IsDlgButtonChecked(hwndDlg, IDC_REPLACE)) {
                        g_this->set_int(p_blend_mode, BLEND_SIMPLE_REPLACE);
                    } else if (IsDlgButtonChecked(hwndDlg, IDC_5050)) {
                        g_this->set_int(p_blend_mode, BLEND_SIMPLE_5050);
                    } else if (IsDlgButtonChecked(hwndDlg, IDC_ADDITIVE)) {
                        g_this->set_int(p_blend_mode, BLEND_SIMPLE_ADDITIVE);
                    }
                    break;
                }
            }
            return 0;
        }
        case WM_DRAWITEM: {
            DRAWITEMSTRUCT* di = (DRAWITEMSTRUCT*)lParam;
            if (di->CtlID == IDC_DEFCOL) {
                unsigned int color = g_this->get_color(p_exclude_color);
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
    }
    return 0;
}
