#include "c_interleave.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int win32_dlgproc_interleave(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    C_THISCLASS* g_ConfigThis = (C_THISCLASS*)g_current_render;
    switch (uMsg) {
        case WM_INITDIALOG:
            SendDlgItemMessage(hwndDlg, IDC_X, TBM_SETTICFREQ, 1, 0);
            SendDlgItemMessage(hwndDlg, IDC_X, TBM_SETRANGE, TRUE, MAKELONG(0, 64));
            SendDlgItemMessage(hwndDlg, IDC_X, TBM_SETPOS, TRUE, g_ConfigThis->x);
            SendDlgItemMessage(hwndDlg, IDC_Y, TBM_SETTICFREQ, 1, 0);
            SendDlgItemMessage(hwndDlg, IDC_Y, TBM_SETRANGE, TRUE, MAKELONG(0, 64));
            SendDlgItemMessage(hwndDlg, IDC_Y, TBM_SETPOS, TRUE, g_ConfigThis->y);
            SendDlgItemMessage(hwndDlg, IDC_X2, TBM_SETTICFREQ, 1, 0);
            SendDlgItemMessage(hwndDlg, IDC_X2, TBM_SETRANGE, TRUE, MAKELONG(0, 64));
            SendDlgItemMessage(hwndDlg, IDC_X2, TBM_SETPOS, TRUE, g_ConfigThis->x2);
            SendDlgItemMessage(hwndDlg, IDC_Y2, TBM_SETTICFREQ, 1, 0);
            SendDlgItemMessage(hwndDlg, IDC_Y2, TBM_SETRANGE, TRUE, MAKELONG(0, 64));
            SendDlgItemMessage(hwndDlg, IDC_Y2, TBM_SETPOS, TRUE, g_ConfigThis->y2);
            SendDlgItemMessage(hwndDlg, IDC_X3, TBM_SETTICFREQ, 1, 0);
            SendDlgItemMessage(hwndDlg, IDC_X3, TBM_SETRANGE, TRUE, MAKELONG(1, 64));
            SendDlgItemMessage(
                hwndDlg, IDC_X3, TBM_SETPOS, TRUE, g_ConfigThis->beatdur);
            if (g_ConfigThis->enabled) CheckDlgButton(hwndDlg, IDC_CHECK1, BST_CHECKED);
            if (g_ConfigThis->onbeat) CheckDlgButton(hwndDlg, IDC_CHECK8, BST_CHECKED);
            if (g_ConfigThis->blend) CheckDlgButton(hwndDlg, IDC_ADDITIVE, BST_CHECKED);
            if (g_ConfigThis->blendavg) CheckDlgButton(hwndDlg, IDC_5050, BST_CHECKED);
            if (!g_ConfigThis->blend && !g_ConfigThis->blendavg)
                CheckDlgButton(hwndDlg, IDC_REPLACE, BST_CHECKED);
            return 1;
        case WM_HSCROLL: {
            HWND swnd = (HWND)lParam;
            int t = (int)SendMessage(swnd, TBM_GETPOS, 0, 0);
            if (swnd == GetDlgItem(hwndDlg, IDC_X)) {
                g_ConfigThis->cur_x = (double)(g_ConfigThis->x = t);
            } else if (swnd == GetDlgItem(hwndDlg, IDC_Y)) {
                g_ConfigThis->cur_y = (double)(g_ConfigThis->y = t);
            } else if (swnd == GetDlgItem(hwndDlg, IDC_X2)) {
                g_ConfigThis->cur_x = (double)(g_ConfigThis->x2 = t);
            } else if (swnd == GetDlgItem(hwndDlg, IDC_Y2)) {
                g_ConfigThis->cur_y = (double)(g_ConfigThis->y2 = t);
            } else if (swnd == GetDlgItem(hwndDlg, IDC_X3)) {
                g_ConfigThis->beatdur = t;
            }
        }
            return 0;
        case WM_DRAWITEM: {
            DRAWITEMSTRUCT* di = (DRAWITEMSTRUCT*)lParam;
            if (di->CtlID == IDC_DEFCOL)  // paint nifty color button
            {
                unsigned int _color = g_ConfigThis->color;
                _color = ((_color >> 16) & 0xff) | (_color & 0xff00)
                         | ((_color << 16) & 0xff0000);

                HPEN hPen, hOldPen;
                HBRUSH hBrush, hOldBrush;
                LOGBRUSH lb = {BS_SOLID, _color, 0};
                hPen = (HPEN)CreatePen(PS_SOLID, 0, _color);
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
        }
            return 0;
        case WM_COMMAND:
            if ((LOWORD(wParam) == IDC_CHECK1) || (LOWORD(wParam) == IDC_CHECK8)
                || (LOWORD(wParam) == IDC_ADDITIVE) || (LOWORD(wParam) == IDC_REPLACE)
                || (LOWORD(wParam) == IDC_5050)) {
                g_ConfigThis->enabled = IsDlgButtonChecked(hwndDlg, IDC_CHECK1) ? 1 : 0;
                g_ConfigThis->onbeat = IsDlgButtonChecked(hwndDlg, IDC_CHECK8) ? 1 : 0;
                g_ConfigThis->blend = IsDlgButtonChecked(hwndDlg, IDC_ADDITIVE) ? 1 : 0;
                g_ConfigThis->blendavg = IsDlgButtonChecked(hwndDlg, IDC_5050) ? 1 : 0;
            }
            if (LOWORD(wParam) == IDC_DEFCOL)  // handle clicks to nifty color button
            {
                int* a = &(g_ConfigThis->color);
                static COLORREF custcolors[16];
                CHOOSECOLOR cs;
                cs.lStructSize = sizeof(cs);
                cs.hwndOwner = hwndDlg;
                cs.hInstance = 0;
                cs.rgbResult =
                    ((*a >> 16) & 0xff) | (*a & 0xff00) | ((*a << 16) & 0xff0000);
                cs.lpCustColors = custcolors;
                cs.Flags = CC_RGBINIT | CC_FULLOPEN;
                if (ChooseColor(&cs)) {
                    *a = ((cs.rgbResult >> 16) & 0xff) | (cs.rgbResult & 0xff00)
                         | ((cs.rgbResult << 16) & 0xff0000);
                    g_ConfigThis->color = *a;
                }
                InvalidateRect(GetDlgItem(hwndDlg, IDC_DEFCOL), NULL, TRUE);
            }
    }
    return 0;
}
