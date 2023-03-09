#include "e_simple.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>

int win32_dlgproc_simple(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    auto g_this = (E_Simple*)g_current_render;
    const AVS_Parameter_Handle p_audio_source = Simple_Info::parameters[0].handle;
    const AVS_Parameter_Handle p_draw_mode = Simple_Info::parameters[1].handle;
    const AVS_Parameter_Handle p_audio_channel = Simple_Info::parameters[2].handle;
    const AVS_Parameter_Handle p_position = Simple_Info::parameters[3].handle;
    const AVS_Parameter_Handle p_colors = Simple_Info::parameters[4].handle;
    const AVS_Parameter_Handle p_color = Simple_Info::color_params[0].handle;

    switch (uMsg) {
        case WM_DRAWITEM: {
            DRAWITEMSTRUCT* di = (DRAWITEMSTRUCT*)lParam;
            auto num_colors = g_this->parameter_list_length(p_colors);
            if (di->CtlID == IDC_DEFCOL && num_colors > 0) {
                size_t x;
                int w = di->rcItem.right - di->rcItem.left;
                int l = 0, nl;
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
        case WM_INITDIALOG:
            switch (g_this->get_int(p_audio_source)) {
                default:
                case AUDIO_WAVEFORM: CheckDlgButton(hwndDlg, IDC_OSC, true); break;
                case AUDIO_SPECTRUM: CheckDlgButton(hwndDlg, IDC_SPEC, true); break;
            }
            switch (g_this->get_int(p_draw_mode)) {
                default:
                case DRAW_SOLID: CheckDlgButton(hwndDlg, IDC_SOLID, true); break;
                case DRAW_LINES: CheckDlgButton(hwndDlg, IDC_LINES, true); break;
                case DRAW_DOTS: CheckDlgButton(hwndDlg, IDC_DOT, true); break;
            }
            switch (g_this->get_int(p_audio_channel)) {
                case AUDIO_LEFT: CheckDlgButton(hwndDlg, IDC_LEFTCH, true); break;
                case AUDIO_RIGHT: CheckDlgButton(hwndDlg, IDC_RIGHTCH, true); break;
                default:
                case AUDIO_CENTER: CheckDlgButton(hwndDlg, IDC_MIDCH, true); break;
            }
            switch (g_this->get_int(p_position)) {
                case VPOS_TOP: CheckDlgButton(hwndDlg, IDC_TOP, true); break;
                case VPOS_BOTTOM: CheckDlgButton(hwndDlg, IDC_BOTTOM, true); break;
                default:
                case VPOS_CENTER: CheckDlgButton(hwndDlg, IDC_CENTER, true); break;
            }
            SetDlgItemInt(
                hwndDlg, IDC_NUMCOL, g_this->parameter_list_length(p_colors), false);
            return 1;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_SOLID: g_this->set_int(p_draw_mode, DRAW_SOLID); break;
                case IDC_LINES: g_this->set_int(p_draw_mode, DRAW_LINES); break;
                case IDC_DOT: g_this->set_int(p_draw_mode, DRAW_DOTS); break;

                case IDC_OSC: g_this->set_int(p_audio_source, AUDIO_WAVEFORM); break;
                case IDC_SPEC: g_this->set_int(p_audio_source, AUDIO_SPECTRUM); break;

                case IDC_LEFTCH: g_this->set_int(p_audio_channel, AUDIO_LEFT); break;
                case IDC_RIGHTCH: g_this->set_int(p_audio_channel, AUDIO_RIGHT); break;
                case IDC_MIDCH: g_this->set_int(p_audio_channel, AUDIO_CENTER); break;

                case IDC_TOP: g_this->set_int(p_position, VPOS_TOP); break;
                case IDC_BOTTOM: g_this->set_int(p_position, VPOS_BOTTOM); break;
                case IDC_CENTER: g_this->set_int(p_position, VPOS_CENTER); break;

                case IDC_NUMCOL: {
                    BOOL success = false;
                    bool check_for_negative = false;
                    uint32_t p = GetDlgItemInt(
                        hwndDlg, IDC_NUMCOL, &success, check_for_negative);
                    if (success) {
                        int64_t length = g_this->parameter_list_length(p_colors);
                        if (length < p) {
                            for (; length < p; length++) {
                                g_this->parameter_list_entry_add(p_colors, -1, {});
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
                    int wc = -1, w, h;
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
                        int32_t color;
                        GR_SelectColor(hwndDlg, &color);
                        g_this->set_color(p_color, (uint64_t)color, {wc});
                        InvalidateRect(GetDlgItem(hwndDlg, IDC_DEFCOL), nullptr, true);
                    }
                    break;
                }
            }
    }
    return 0;
}
