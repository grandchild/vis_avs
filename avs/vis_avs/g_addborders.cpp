#include "e_addborders.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>
#include <commctrl.h>

#define RGB_TO_BGR(color) \
    (((color)&0xff0000) >> 16 | ((color)&0xff00) | ((color)&0xff) << 16)
#define BGR_TO_RGB(color) RGB_TO_BGR(color)  // is its own inverse

static COLORREF g_cust_colors[16];

int win32_dlgproc_addborders(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    E_AddBorders* g_this = (E_AddBorders*)g_current_render;
    const AVS_Parameter_Handle p_color = g_this->info.parameters[0].handle;
    const Parameter& p_size = g_this->info.parameters[1];

    DRAWITEMSTRUCT* drawitem;
    HWND width_slider;

    switch (uMsg) {
        case WM_DRAWITEM:  // 0x02b
            drawitem = (DRAWITEMSTRUCT*)lParam;
            if (drawitem->CtlID == IDC_ADDBORDERS_COLOR) {
                LOGBRUSH logbrush;
                logbrush.lbStyle = BS_SOLID;
                logbrush.lbColor = RGB_TO_BGR(g_this->get_color(p_color));
                logbrush.lbHatch = 0;
                HGDIOBJ brush = CreateBrushIndirect(&logbrush);
                HGDIOBJ obj = SelectObject(drawitem->hDC, brush);
                Rectangle(drawitem->hDC,
                          drawitem->rcItem.left,
                          drawitem->rcItem.top,
                          drawitem->rcItem.right,
                          drawitem->rcItem.bottom);
                SelectObject(drawitem->hDC, obj);
                DeleteObject(brush);
            }
            break;
        case WM_INITDIALOG:  // 0x110
            width_slider = GetDlgItem(hwndDlg, IDC_ADDBORDERS_SIZE);
            SendMessage(width_slider,
                        TBM_SETRANGE,
                        0,
                        MAKELONG(p_size.int_min, p_size.int_max));
            SendMessage(width_slider, TBM_SETPOS, 1, g_this->get_int(p_size.handle));
            if (g_this->enabled) {
                CheckDlgButton(hwndDlg, IDC_ADDBORDERS_ENABLE, 1);
            }
            break;
        case WM_HSCROLL:  // 0x114
            if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_ADDBORDERS_SIZE)) {
                g_this->set_int(p_size.handle,
                                SendMessage((HWND)lParam, TBM_GETPOS, 0, 0));
            }
            break;
        case WM_COMMAND:  // 0x111
            if (LOWORD(wParam) == IDC_ADDBORDERS_ENABLE) {
                g_this->set_enabled(IsDlgButtonChecked(hwndDlg, IDC_ADDBORDERS_ENABLE));
            } else if (LOWORD(wParam) == IDC_ADDBORDERS_COLOR) {
                CHOOSECOLOR choosecolor;
                choosecolor.lStructSize = sizeof(CHOOSECOLOR);
                choosecolor.hwndOwner = hwndDlg;
                choosecolor.hInstance = NULL;
                choosecolor.rgbResult = RGB_TO_BGR(g_this->get_color(p_color));
                choosecolor.lpCustColors = g_cust_colors;
                choosecolor.Flags = CC_RGBINIT | CC_FULLOPEN;
                if (ChooseColor(&choosecolor) != 0) {
                    g_this->set_color(p_color, BGR_TO_RGB(choosecolor.rgbResult));
                }
                InvalidateRect(GetDlgItem(hwndDlg, IDC_ADDBORDERS_COLOR), NULL, 1);
            }
            break;
    }
    return 0;
}
