#include "e_multiplier.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>

int win32_dlgproc_multiplier(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    E_Multiplier* g_this = (E_Multiplier*)g_current_render;
    int id = 0;
    switch (uMsg) {
        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED) {
                id = LOWORD(wParam);
                switch (id) {
                    case IDC_XI: g_this->config.multiply = MULTIPLY_INF_ROOT; break;
                    case IDC_X8: g_this->config.multiply = MULTIPLY_X8; break;
                    case IDC_X4: g_this->config.multiply = MULTIPLY_X4; break;
                    case IDC_X2: g_this->config.multiply = MULTIPLY_X2; break;
                    case IDC_X05: g_this->config.multiply = MULTIPLY_X05; break;
                    case IDC_X025: g_this->config.multiply = MULTIPLY_X025; break;
                    case IDC_X0125: g_this->config.multiply = MULTIPLY_X0125; break;
                    case IDC_XS: g_this->config.multiply = MULTIPLY_INF_SQUARE; break;
                }
            }
            return 0;

        case WM_INITDIALOG:
            switch (g_this->config.multiply) {
                case MULTIPLY_INF_ROOT: id = IDC_XI; break;
                case MULTIPLY_X8: id = IDC_X8; break;
                case MULTIPLY_X4: id = IDC_X4; break;
                case MULTIPLY_X2: id = IDC_X2; break;
                case MULTIPLY_X05: id = IDC_X05; break;
                case MULTIPLY_X025: id = IDC_X025; break;
                case MULTIPLY_X0125: id = IDC_X0125; break;
                case MULTIPLY_INF_SQUARE: id = IDC_XS; break;
            }
            if (id) {
                SendMessage(GetDlgItem(hwndDlg, id), BM_SETCHECK, BST_CHECKED, 0);
            }
            return 1;

        case WM_DESTROY: return 1;
    }
    return 0;
}
