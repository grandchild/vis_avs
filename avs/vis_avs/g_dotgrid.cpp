#include "e_dotgrid.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int win32_dlgproc_dotgrid(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    auto g_this = (E_DotGrid*)g_current_render;
    AVS_Parameter_Handle p_color = DotGrid_Info::color_params[0].handle;
    AVS_Parameter_Handle p_colors = DotGrid_Info::parameters[0].handle;
    AVS_Parameter_Handle p_spacing = DotGrid_Info::parameters[1].handle;
    const Parameter& p_speed_x = DotGrid_Info::parameters[2];
    const Parameter& p_speed_y = DotGrid_Info::parameters[3];
    AVS_Parameter_Handle p_blend_mode = DotGrid_Info::parameters[4].handle;
    AVS_Parameter_Handle p_zero_speed_x = DotGrid_Info::parameters[5].handle;
    AVS_Parameter_Handle p_zero_speed_y = DotGrid_Info::parameters[6].handle;

    switch (uMsg) {
        case WM_INITDIALOG: {
            auto speed_x = g_this->get_int(p_speed_x.handle);
            init_ranged_slider(p_speed_x, speed_x, hwndDlg, IDC_SLIDER1, 32);
            auto speed_y = g_this->get_int(p_speed_y.handle);
            init_ranged_slider(p_speed_y, speed_y, hwndDlg, IDC_SLIDER2, 32);
            SetDlgItemInt(
                hwndDlg, IDC_NUMCOL, g_this->parameter_list_length(p_colors), false);
            SetDlgItemInt(hwndDlg, IDC_EDIT1, g_this->get_int(p_spacing), false);
            auto blend_mode = g_this->get_int(p_blend_mode);
            CheckDlgButton(hwndDlg, IDC_RADIO2, blend_mode == DOTGRID_BLEND_ADDITIVE);
            CheckDlgButton(hwndDlg, IDC_RADIO3, blend_mode == DOTGRID_BLEND_5050);
            CheckDlgButton(hwndDlg, IDC_RADIO4, blend_mode == DOTGRID_BLEND_DEFAULT);
            CheckDlgButton(hwndDlg, IDC_RADIO1, blend_mode == DOTGRID_BLEND_REPLACE);
            return 1;
        }
        case WM_DRAWITEM: {
            auto di = (DRAWITEMSTRUCT*)lParam;
            auto num_colors = g_this->parameter_list_length(p_colors);
            if (di->CtlID == IDC_DEFCOL && num_colors > 0) {
                uint32_t x;
                int32_t w = di->rcItem.right - di->rcItem.left;
                int32_t l = 0, nl;
                for (x = 0; x < num_colors; x++) {
                    uint32_t color = g_this->get_color(p_color, {x});
                    nl = (w * (x + 1)) / num_colors;
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
                              di->rcItem.left + l,
                              di->rcItem.top,
                              di->rcItem.left + nl,
                              di->rcItem.bottom);
                    SelectObject(di->hDC, hOldPen);
                    SelectObject(di->hDC, hOldBrush);
                    DeleteObject(hBrush);
                    DeleteObject(hPen);
                    l = nl;
                }
            }
            return 0;
        }
        case WM_HSCROLL: {
            HWND swnd = (HWND)lParam;
            auto t = (int)SendMessage(swnd, TBM_GETPOS, 0, 0);
            if (swnd == GetDlgItem(hwndDlg, IDC_SLIDER1)) {
                g_this->set_int(p_speed_x.handle, t);
            }
            if (swnd == GetDlgItem(hwndDlg, IDC_SLIDER2)) {
                g_this->set_int(p_speed_y.handle, t);
            }
            return 0;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_BUTTON1:
                    g_this->run_action(p_zero_speed_x);
                    SendDlgItemMessage(hwndDlg,
                                       IDC_SLIDER1,
                                       TBM_SETPOS,
                                       1,
                                       g_this->get_int(p_speed_x.handle));
                    return 0;
                case IDC_BUTTON3:
                    g_this->run_action(p_zero_speed_y);
                    SendDlgItemMessage(hwndDlg,
                                       IDC_SLIDER2,
                                       TBM_SETPOS,
                                       1,
                                       g_this->get_int(p_speed_y.handle));
                    return 0;
                case IDC_RADIO1:
                case IDC_RADIO2:
                case IDC_RADIO3:
                case IDC_RADIO4:
                    if (IsDlgButtonChecked(hwndDlg, IDC_RADIO1)) {
                        g_this->set_int(p_blend_mode, DOTGRID_BLEND_REPLACE);
                    } else if (IsDlgButtonChecked(hwndDlg, IDC_RADIO2)) {
                        g_this->set_int(p_blend_mode, DOTGRID_BLEND_ADDITIVE);
                    } else if (IsDlgButtonChecked(hwndDlg, IDC_RADIO3)) {
                        g_this->set_int(p_blend_mode, DOTGRID_BLEND_5050);
                    } else if (IsDlgButtonChecked(hwndDlg, IDC_RADIO4)) {
                        g_this->set_int(p_blend_mode, DOTGRID_BLEND_DEFAULT);
                    }
                    break;
                case IDC_NUMCOL: {
                    uint32_t p;
                    BOOL success = false;
                    bool check_for_negative = false;
                    p = GetDlgItemInt(
                        hwndDlg, IDC_NUMCOL, &success, check_for_negative);
                    if (success) {
                        int64_t length = g_this->parameter_list_length(p_colors);
                        if (length < p) {
                            for (; length < p; length++) {
                                g_this->parameter_list_entry_add(p_colors, -1);
                            }
                        } else {
                            for (; length > p; length--) {
                                g_this->parameter_list_entry_remove(p_colors, -1);
                            }
                        }
                        InvalidateRect(GetDlgItem(hwndDlg, IDC_DEFCOL), nullptr, true);
                    }
                    break;
                }
                case IDC_EDIT1: {
                    BOOL success = false;
                    uint32_t p = GetDlgItemInt(hwndDlg, IDC_EDIT1, &success, false);
                    if (success) {
                        if (p < 2) {
                            p = 2;
                        }
                        g_this->set_int(p_spacing, p);
                    }
                    break;
                }
                case IDC_DEFCOL: {
                    int32_t wc = -1, w, h;
                    POINT p;
                    RECT r;
                    GetCursorPos(&p);
                    GetWindowRect(GetDlgItem(hwndDlg, IDC_DEFCOL), &r);
                    p.x -= r.left;
                    p.y -= r.top;
                    w = r.right - r.left;
                    h = r.bottom - r.top;
                    if (p.x >= 0 && p.x < w && p.y >= 0 && p.y < h) {
                        wc = (p.x * g_this->parameter_list_length(p_colors)) / w;
                    }
                    if (wc >= 0) {
                        uint32_t color = g_this->get_color(p_color, {wc});
                        GR_SelectColor(hwndDlg, (int32_t*)&color);
                        g_this->set_color(p_color, color, {wc});
                        InvalidateRect(GetDlgItem(hwndDlg, IDC_DEFCOL), nullptr, true);
                    }
                }
            }
    }
    return 0;
}
