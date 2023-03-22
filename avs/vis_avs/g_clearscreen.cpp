#include "e_clearscreen.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>

int win32_dlgproc_clearscreen(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    auto g_this = (E_ClearScreen*)g_current_render;
    AVS_Parameter_Handle p_color = ClearScreen_Info::parameters[0].handle;
    AVS_Parameter_Handle p_blend_mode = ClearScreen_Info::parameters[1].handle;
    AVS_Parameter_Handle p_only_first = ClearScreen_Info::parameters[2].handle;

    switch (uMsg) {
        case WM_INITDIALOG: {
            CheckDlgButton(hwndDlg, IDC_CHECK1, g_this->enabled);
            CheckDlgButton(
                hwndDlg, IDC_CLEARFIRSTFRAME, g_this->get_bool(p_only_first));
            auto blend_mode = g_this->get_int(p_blend_mode);
            CheckDlgButton(hwndDlg, IDC_REPLACE, blend_mode == CLEAR_BLEND_REPLACE);
            CheckDlgButton(hwndDlg, IDC_ADDITIVE, blend_mode == CLEAR_BLEND_ADDITIVE);
            CheckDlgButton(
                hwndDlg, IDC_DEFRENDBLEND, blend_mode == CLEAR_BLEND_DEFAULT);
            CheckDlgButton(hwndDlg, IDC_5050, blend_mode == CLEAR_BLEND_5050);
            return 1;
        }
        case WM_DRAWITEM: {
            auto di = (DRAWITEMSTRUCT*)lParam;
            if (di->CtlID == IDC_DEFCOL) {
                uint32_t color = g_this->get_color(p_color);
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
        case WM_COMMAND:
            if ((LOWORD(wParam) == IDC_CHECK1) || (LOWORD(wParam) == IDC_ADDITIVE)
                || (LOWORD(wParam) == IDC_REPLACE) || (LOWORD(wParam) == IDC_5050)
                || (LOWORD(wParam) == IDC_DEFRENDBLEND)) {
                g_this->set_enabled(IsDlgButtonChecked(hwndDlg, IDC_CHECK1));
                if (IsDlgButtonChecked(hwndDlg, IDC_REPLACE)) {
                    g_this->set_int(p_blend_mode, CLEAR_BLEND_REPLACE);
                } else if (IsDlgButtonChecked(hwndDlg, IDC_ADDITIVE)) {
                    g_this->set_int(p_blend_mode, CLEAR_BLEND_ADDITIVE);
                } else if (IsDlgButtonChecked(hwndDlg, IDC_5050)) {
                    g_this->set_int(p_blend_mode, CLEAR_BLEND_5050);
                } else if (IsDlgButtonChecked(hwndDlg, IDC_DEFRENDBLEND)) {
                    g_this->set_int(p_blend_mode, CLEAR_BLEND_DEFAULT);
                }
            }
            if (LOWORD(wParam) == IDC_CLEARFIRSTFRAME) {
                g_this->set_bool(p_only_first,
                                 IsDlgButtonChecked(hwndDlg, IDC_CLEARFIRSTFRAME));
            }

            if (LOWORD(wParam) == IDC_DEFCOL) {
                uint32_t color = g_this->get_color(p_color);
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
                InvalidateRect(GetDlgItem(hwndDlg, IDC_DEFCOL), nullptr, true);
            }
    }
    return 0;
}
