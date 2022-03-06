#include "e_rotstar.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>

int win32_dlgproc_rotstar(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    E_RotStar* g_this = (E_RotStar*)g_current_render;
    const Parameter& p_colors = g_this->info.parameters[0];

    switch (uMsg) {
        case WM_DRAWITEM: {
            DRAWITEMSTRUCT* di = (DRAWITEMSTRUCT*)lParam;
            if (di->CtlID == IDC_DEFCOL && g_this->config.colors.size() > 0) {
                uint32_t x;
                int w = di->rcItem.right - di->rcItem.left;
                int l = 0, nl;
                for (x = 0; x < g_this->config.colors.size(); x++) {
                    unsigned int color = g_this->config.colors[x].color;
                    nl = (w * (x + 1)) / g_this->config.colors.size();
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
        }
            return 0;
        case WM_INITDIALOG:
            SetDlgItemInt(hwndDlg, IDC_NUMCOL, g_this->config.colors.size(), false);
            return 1;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_NUMCOL: {
                    int p;
                    WINBOOL success = FALSE;
                    bool check_for_negative = false;
                    p = GetDlgItemInt(
                        hwndDlg, IDC_NUMCOL, &success, check_for_negative);
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
                        InvalidateRect(GetDlgItem(hwndDlg, IDC_DEFCOL), NULL, TRUE);
                    }
                } break;
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
                        wc = (p.x * g_this->config.colors.size()) / w;
                    }
                    if (wc >= 0) {
                        GR_SelectColor(hwndDlg,
                                       (int*)&(g_this->config.colors[wc].color));
                        InvalidateRect(GetDlgItem(hwndDlg, IDC_DEFCOL), NULL, TRUE);
                    }
                }
            }
    }
    return 0;
}
