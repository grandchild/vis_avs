#include "e_oscilloscopestar.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int win32_dlgproc_oscstar(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    auto g_this = (E_OscilloscopeStar*)g_current_render;
    AVS_Parameter_Handle p_audio_channel = OscilloscopeStar_Info::parameters[0].handle;
    AVS_Parameter_Handle p_position = OscilloscopeStar_Info::parameters[1].handle;
    const Parameter& p_size = OscilloscopeStar_Info::parameters[2];
    const Parameter& p_rotation = OscilloscopeStar_Info::parameters[3];
    AVS_Parameter_Handle p_colors = OscilloscopeStar_Info::parameters[4].handle;
    AVS_Parameter_Handle p_color = OscilloscopeStar_Info::color_params[0].handle;

    switch (uMsg) {
        case WM_INITDIALOG: {
            auto audio_channel = g_this->get_int(p_audio_channel);
            CheckDlgButton(hwndDlg, IDC_LEFTCH, audio_channel == AUDIO_LEFT);
            CheckDlgButton(hwndDlg, IDC_RIGHTCH, audio_channel == AUDIO_RIGHT);
            CheckDlgButton(hwndDlg, IDC_MIDCH, audio_channel == AUDIO_CENTER);
            auto position = g_this->get_int(p_position);
            CheckDlgButton(hwndDlg, IDC_LEFT, position == HPOS_LEFT);
            CheckDlgButton(hwndDlg, IDC_RIGHT, position == HPOS_RIGHT);
            CheckDlgButton(hwndDlg, IDC_CENTER, position == HPOS_CENTER);
            SetDlgItemInt(
                hwndDlg, IDC_NUMCOL, g_this->parameter_list_length(p_colors), false);
            auto size = g_this->get_int(p_size.handle);
            init_ranged_slider(p_size, size, hwndDlg, IDC_SLIDER1);
            auto rotation = g_this->get_int(p_size.handle);
            init_ranged_slider(p_rotation, rotation, hwndDlg, IDC_SLIDER2);
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
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case IDC_LEFTCH: g_this->set_int(p_audio_channel, AUDIO_LEFT); break;
                case IDC_RIGHTCH: g_this->set_int(p_audio_channel, AUDIO_RIGHT); break;
                case IDC_MIDCH: g_this->set_int(p_audio_channel, AUDIO_CENTER); break;
                case IDC_LEFT: g_this->set_int(p_position, HPOS_LEFT); break;
                case IDC_RIGHT: g_this->set_int(p_position, HPOS_RIGHT); break;
                case IDC_CENTER: g_this->set_int(p_position, HPOS_CENTER); break;
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
            return 0;
        }
        case WM_HSCROLL: {
            HWND swnd = (HWND)lParam;
            int t = (int)SendMessage(swnd, TBM_GETPOS, 0, 0);
            if (swnd == GetDlgItem(hwndDlg, IDC_SLIDER1)) {
                g_this->set_int(p_size.handle, t);
            }
            if (swnd == GetDlgItem(hwndDlg, IDC_SLIDER2)) {
                g_this->set_int(p_rotation.handle, t);
            }
            return 0;
        }
    }
    return 0;
}
