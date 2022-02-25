#include "c_oscring.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int win32_dlgproc_oscring(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    C_THISCLASS* g_this = (C_THISCLASS*)g_current_render;

    switch (uMsg) {
        case WM_DRAWITEM: {
            DRAWITEMSTRUCT* di = (DRAWITEMSTRUCT*)lParam;
            if (di->CtlID == IDC_DEFCOL && g_this->num_colors > 0) {
                int x;
                int w = di->rcItem.right - di->rcItem.left;
                int l = 0, nl;
                for (x = 0; x < g_this->num_colors; x++) {
                    unsigned int color = g_this->colors[x];
                    nl = (w * (x + 1)) / g_this->num_colors;
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
        case WM_HSCROLL: {
            HWND swnd = (HWND)lParam;
            int t = (int)SendMessage(swnd, TBM_GETPOS, 0, 0);
            if (swnd == GetDlgItem(hwndDlg, IDC_SLIDER1)) {
                g_this->size = t;
            }
        }
            return 0;
        case WM_INITDIALOG:
            switch ((g_this->effect >> 2) & 3) {
                case 0: CheckDlgButton(hwndDlg, IDC_LEFTCH, BST_CHECKED); break;
                case 1: CheckDlgButton(hwndDlg, IDC_RIGHTCH, BST_CHECKED); break;
                case 2: CheckDlgButton(hwndDlg, IDC_MIDCH, BST_CHECKED); break;
            }
            switch ((g_this->effect >> 4) & 3) {
                case 0: CheckDlgButton(hwndDlg, IDC_TOP, BST_CHECKED); break;
                case 1: CheckDlgButton(hwndDlg, IDC_BOTTOM, BST_CHECKED); break;
                case 2: CheckDlgButton(hwndDlg, IDC_CENTER, BST_CHECKED); break;
            }
            if (g_this->source)
                CheckDlgButton(hwndDlg, IDC_SPEC, BST_CHECKED);
            else
                CheckDlgButton(hwndDlg, IDC_OSC, BST_CHECKED);
            SetDlgItemInt(hwndDlg, IDC_NUMCOL, g_this->num_colors, FALSE);
            SendDlgItemMessage(hwndDlg, IDC_SLIDER1, TBM_SETRANGEMIN, 0, 1);
            SendDlgItemMessage(hwndDlg, IDC_SLIDER1, TBM_SETRANGEMAX, 0, 64);
            SendDlgItemMessage(hwndDlg, IDC_SLIDER1, TBM_SETPOS, 1, g_this->size);
            return 1;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_OSC: g_this->source = 0; break;
                case IDC_SPEC: g_this->source = 1; break;
                case IDC_LEFTCH: g_this->effect &= ~12; break;
                case IDC_RIGHTCH:
                    g_this->effect &= ~12;
                    g_this->effect |= 4;
                    break;
                case IDC_MIDCH:
                    g_this->effect &= ~12;
                    g_this->effect |= 8;
                    break;
                case IDC_TOP: g_this->effect &= ~48; break;
                case IDC_BOTTOM:
                    g_this->effect &= ~48;
                    g_this->effect |= 16;
                    break;
                case IDC_CENTER:
                    g_this->effect &= ~48;
                    g_this->effect |= 32;
                    break;
                case IDC_NUMCOL: {
                    int p;
                    BOOL tr = FALSE;
                    p = GetDlgItemInt(hwndDlg, IDC_NUMCOL, &tr, FALSE);
                    if (tr) {
                        if (p > 16) p = 16;
                        g_this->num_colors = p;
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
                        wc = (p.x * g_this->num_colors) / w;
                    }
                    if (wc >= 0) {
                        GR_SelectColor(hwndDlg, g_this->colors + wc);
                        InvalidateRect(GetDlgItem(hwndDlg, IDC_DEFCOL), NULL, TRUE);
                    }
                }
            }
    }
    return 0;
}
