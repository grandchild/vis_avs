#include "e_superscope.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>

int win32_dlgproc_superscope(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    E_SuperScope* g_this = (E_SuperScope*)g_current_render;
    const Parameter& p_init = g_this->info.parameters[0];
    const Parameter& p_frame = g_this->info.parameters[1];
    const Parameter& p_beat = g_this->info.parameters[2];
    const Parameter& p_point = g_this->info.parameters[3];
    const Parameter& p_colors = g_this->info.parameters[4];
    const Parameter& p_example = g_this->info.parameters[8];
    const Parameter& p_load_example = g_this->info.parameters[9];

    static int isstart;
    switch (uMsg) {
        case WM_INITDIALOG:
            isstart = 1;

            SetDlgItemInt(hwndDlg, IDC_NUMCOL, g_this->config.colors.size(), false);
            SetDlgItemText(hwndDlg, IDC_EDIT1, (char*)g_this->config.point.c_str());
            SetDlgItemText(hwndDlg, IDC_EDIT2, (char*)g_this->config.frame.c_str());
            SetDlgItemText(hwndDlg, IDC_EDIT3, (char*)g_this->config.beat.c_str());
            SetDlgItemText(hwndDlg, IDC_EDIT4, (char*)g_this->config.init.c_str());

            CheckDlgButton(
                hwndDlg, g_this->config.draw_mode ? IDC_LINES : IDC_DOT, BST_CHECKED);
            switch (g_this->config.audio_channel) {
                default:
                case 0: CheckDlgButton(hwndDlg, IDC_MIDCH, BST_CHECKED); break;
                case 1: CheckDlgButton(hwndDlg, IDC_LEFTCH, BST_CHECKED); break;
                case 2: CheckDlgButton(hwndDlg, IDC_RIGHTCH, BST_CHECKED); break;
            }
            if (g_this->config.audio_source == 1) {
                CheckDlgButton(hwndDlg, IDC_SPEC, BST_CHECKED);
            } else {
                CheckDlgButton(hwndDlg, IDC_WAVE, BST_CHECKED);
            }

            isstart = 0;
            return 1;
        case WM_DRAWITEM: {
            DRAWITEMSTRUCT* di = (DRAWITEMSTRUCT*)lParam;
            if (di->CtlID == IDC_DEFCOL && g_this->config.colors.size() > 0) {
                uint64_t x;
                int w = di->rcItem.right - di->rcItem.left;
                int l = 0, nl;
                for (x = 0; x < g_this->config.colors.size(); x++) {
                    auto color = g_this->config.colors[x].color;
                    nl = (w * (x + 1)) / g_this->config.colors.size();
                    color = ((color >> 16) & 0xff) | (color & 0xff00)
                            | ((color << 16) & 0xff0000);

                    HPEN hPen, hOldPen;
                    HBRUSH hBrush, hOldBrush;
                    LOGBRUSH lb = {BS_SOLID, (uint32_t)color, 0};
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
        case WM_COMMAND:
            if (!isstart && HIWORD(wParam) == EN_CHANGE) {
                int length = 0;
                char* buf = NULL;
                switch (LOWORD(wParam)) {
                    case IDC_EDIT1:
                        length = GetWindowTextLength((HWND)lParam) + 1;
                        buf = new char[length];
                        GetDlgItemText(hwndDlg, IDC_EDIT1, buf, length);
                        g_this->set_string(p_point.handle, buf);
                        delete[] buf;
                        break;
                    case IDC_EDIT2:
                        length = GetWindowTextLength((HWND)lParam) + 1;
                        buf = new char[length];
                        GetDlgItemText(hwndDlg, IDC_EDIT2, buf, length);
                        g_this->set_string(p_frame.handle, buf);
                        delete[] buf;
                        break;
                    case IDC_EDIT3:
                        length = GetWindowTextLength((HWND)lParam) + 1;
                        buf = new char[length];
                        GetDlgItemText(hwndDlg, IDC_EDIT3, buf, length);
                        g_this->set_string(p_beat.handle, buf);
                        delete[] buf;
                        break;
                    case IDC_EDIT4:
                        length = GetWindowTextLength((HWND)lParam) + 1;
                        buf = new char[length];
                        GetDlgItemText(hwndDlg, IDC_EDIT4, buf, length);
                        g_this->set_string(p_init.handle, buf);
                        delete[] buf;
                        break;
                }
            }
            if (LOWORD(wParam) == IDC_DOT || LOWORD(wParam) == IDC_LINES) {
                g_this->config.draw_mode =
                    IsDlgButtonChecked(hwndDlg, IDC_LINES) ? 1 : 0;
            }
            if (LOWORD(wParam) == IDC_WAVE || LOWORD(wParam) == IDC_SPEC) {
                if (IsDlgButtonChecked(hwndDlg, IDC_WAVE)) {
                    g_this->config.audio_source = 0;
                } else {
                    g_this->config.audio_source = 1;
                }
            }
            if (LOWORD(wParam) == IDC_LEFTCH || LOWORD(wParam) == IDC_RIGHTCH
                || LOWORD(wParam) == IDC_MIDCH) {
                if (IsDlgButtonChecked(hwndDlg, IDC_LEFTCH)) {
                    g_this->config.audio_channel = 1;
                } else if (IsDlgButtonChecked(hwndDlg, IDC_RIGHTCH)) {
                    g_this->config.audio_channel = 2;
                } else {
                    g_this->config.audio_channel = 0;
                }
            }
            if (LOWORD(wParam) == IDC_BUTTON2) {
                compilerfunctionlist(
                    hwndDlg, g_this->info.get_name(), g_this->info.get_help());
            }
            if (LOWORD(wParam) == IDC_BUTTON1) {
                RECT r;
                HMENU hMenu;
                MENUITEMINFO i = {};
                i.cbSize = sizeof(i);
                hMenu = CreatePopupMenu();
                int x;

                int64_t options_length;
                const char* const* options = p_example.get_options(&options_length);
                const int index_offset = 16;
                for (x = 0; x < options_length; x++) {
                    i.fMask = MIIM_TYPE | MIIM_DATA | MIIM_ID;
                    i.fType = MFT_STRING;
                    i.wID = x + index_offset;
                    i.dwTypeData = (char*)options[x];
                    i.cch = strnlen(options[x], 128);
                    InsertMenuItem(hMenu, x, true, &i);
                }
                GetWindowRect(GetDlgItem(hwndDlg, IDC_BUTTON1), &r);
                x = TrackPopupMenu(hMenu,
                                   TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD
                                       | TPM_RIGHTBUTTON | TPM_LEFTBUTTON
                                       | TPM_NONOTIFY,
                                   r.right,
                                   r.top,
                                   0,
                                   hwndDlg,
                                   NULL);
                if (x >= index_offset && x < index_offset + options_length) {
                    g_this->set_int(p_example.handle, x - index_offset);
                    g_this->run_action(p_load_example.handle);
                    SetDlgItemText(
                        hwndDlg, IDC_EDIT1, (char*)g_this->config.point.c_str());
                    SetDlgItemText(
                        hwndDlg, IDC_EDIT2, (char*)g_this->config.frame.c_str());
                    SetDlgItemText(
                        hwndDlg, IDC_EDIT3, (char*)g_this->config.beat.c_str());
                    SetDlgItemText(
                        hwndDlg, IDC_EDIT4, (char*)g_this->config.init.c_str());
                }
                DestroyMenu(hMenu);
            }
            if (LOWORD(wParam) == IDC_NUMCOL) {
                int p;
                WINBOOL success = false;
                bool check_for_negative = false;
                p = GetDlgItemInt(hwndDlg, IDC_NUMCOL, &success, check_for_negative);
                if (success) {
                    int64_t length = g_this->parameter_list_length(&p_colors);
                    if (length < p) {
                        for (; length < p; length++) {
                            g_this->parameter_list_entry_add(&p_colors, -1, {});
                        }
                    } else {
                        for (; length > p; length--) {
                            g_this->parameter_list_entry_remove(&p_colors, -1);
                        }
                    }
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_DEFCOL), NULL, true);
                }
            }
            if (LOWORD(wParam) == IDC_DEFCOL) {
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
                    wc = (p.x * g_this->config.colors.size()) / w;
                }
                if (wc >= 0) {
                    GR_SelectColor(hwndDlg, (int*)&(g_this->config.colors[wc].color));
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_DEFCOL), NULL, true);
                }
            }
            return 0;
    }
    return 0;
}
