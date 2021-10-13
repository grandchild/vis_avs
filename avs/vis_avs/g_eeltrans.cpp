#include "c_eeltrans.h"

#include "c__defs.h"
#include "g__defs.h"
#include "g__lib.h"
#include "resource.h"

#include <windows.h>

class Chwnd {
   public:
    HWND hwnd;
    int width, height, left, top;
    Chwnd(HWND hw) { hwnd = hw; }
    void setpos() {
        if (hwnd) {
            WINDOWPLACEMENT wp;
            wp.length = sizeof(wp);
            GetWindowPlacement(hwnd, &wp);
            if ((GetWindowLong(hwnd, GWL_STYLE) & WS_VISIBLE) == 0) {
                // not visible
                wp.showCmd = SW_HIDE;
            } else {
                // visible
                wp.showCmd = SW_SHOW;
            }
            wp.rcNormalPosition.left = left;
            wp.rcNormalPosition.top = top;
            wp.rcNormalPosition.right = left + width;
            wp.rcNormalPosition.bottom = top + height;
            SetWindowPlacement(hwnd, &wp);
        }
    }
    void getpos() {
        if (hwnd) {
            WINDOWPLACEMENT wp;
            wp.length = sizeof(wp);
            GetWindowPlacement(hwnd, &wp);
            left = wp.rcNormalPosition.left;
            top = wp.rcNormalPosition.top;
            width = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
            height = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
        }
    }
    void refresh() {
        HDC hDC = GetWindowDC(hwnd);
        SendMessage(hwnd, WM_PAINT, (WPARAM)hDC, 0);
        ReleaseDC(hwnd, hDC);
    }

    HWND GetDlgItemNum(int DlgItemNum) {
        ECP_tmphwnd = 0;
        ECP_cnt = 0;
        ECP_DlgItemNum = DlgItemNum;
        EnumChildWindows(hwnd, &ECP_GetDlgItemNum_priv, (LPARAM)this);
        return ECP_tmphwnd;
    }
    char* GetWindowCaption() {
        int r;
        if ((hwnd) && (r = GetWindowTextLength(hwnd))) {
            LPTSTR buffer = new char[r + 1];
            r = GetWindowText(hwnd, buffer, r + 1);
            return buffer;
        } else {
            LPTSTR buffer = new char[1];
            *buffer = 0;
            return buffer;
        }
    }
    BOOL ECP_GetDlgItemNum(HWND childhwnd) {
        ECP_cnt++;
        if (ECP_DlgItemNum == ECP_cnt) {
            ECP_tmphwnd = childhwnd;
        }
        return true;
    }

   private:
    static BOOL CALLBACK ECP_GetDlgItemNum_priv(HWND hwnd, LPARAM lParam) {
        return ((Chwnd*)lParam)->ECP_GetDlgItemNum(hwnd);
    }
    int ECP_DlgItemNum;
    HWND ECP_tmphwnd;
    int ECP_cnt;
};

static BOOL CALLBACK win32_dlgproc_eeltrans(HWND hwndDlg,
                                            UINT uMsg,
                                            WPARAM wParam,
                                            LPARAM lParam) {
    C_EelTrans* g_EelTransThis = (C_EelTrans*)g_current_render;
    static WORD lastclwidth;
    static WORD lastclheight;
    static Chwnd* Txt_LogPath = NULL;
    static Chwnd* Txt_AutoPrefix = NULL;

    switch (uMsg) {
        case WM_INITDIALOG:
            HWND tmphwnd;
            tmphwnd = GetDlgItem(hwndDlg, IDC_EELTRANS_LOGPATH);
            Txt_LogPath = new Chwnd(tmphwnd);
            SetWindowText(tmphwnd, logpath);
            tmphwnd = GetDlgItem(hwndDlg, IDC_EELTRANS_CODE);
            Txt_AutoPrefix = new Chwnd(tmphwnd);
            SetWindowText(tmphwnd, autoprefix[g_EeltransThis].c_str());

            if (enabled_log) {
                CheckDlgButton(hwndDlg, IDC_EELTRANS_LOG, BST_CHECKED);
            }
            if (enabled_avstrans) {
                CheckDlgButton(hwndDlg, IDC_EELTRANS_ENABLED, BST_CHECKED);
            }
            if (defTransFirst) {
                CheckDlgButton(hwndDlg, IDC_EELTRANS_TRANSFIRST, BST_CHECKED);
            }
            if (ReadCommentCodes) {
                CheckDlgButton(hwndDlg, IDC_EELTRANS_READCOMMENTCODES, BST_CHECKED);
            }
            return 0;

        case WM_DESTROY:
            delete Txt_LogPath;
            Txt_LogPath = NULL;
            delete Txt_AutoPrefix;
            Txt_AutoPrefix = NULL;
            return 0;

        case WM_SIZE:
            lastclwidth = LOWORD(lParam);   // width of client area
            lastclheight = HIWORD(lParam);  // height of client area

            if (Txt_LogPath != NULL) {
                Txt_LogPath->getpos();
                Txt_LogPath->width = lastclwidth - 143;
                Txt_LogPath->setpos();
            }

            if (Txt_AutoPrefix != NULL) {
                Txt_AutoPrefix->getpos();
                Txt_AutoPrefix->width = lastclwidth - 9;
                Txt_AutoPrefix->height = lastclheight - 145;
                Txt_AutoPrefix->setpos();
            }

            return 0;

        case WM_COMMAND:
            if (HIWORD(wParam) == EN_CHANGE) {
                if ((LOWORD(wParam) == IDC_EELTRANS_LOGPATH) && (Txt_LogPath)) {
                    /*char *tmp = Txt_LogPath->GetWindowCaption();
                    logpath = new char[strlen(tmp)+1];
                    strcpy(logpath, tmp);*/
                    logpath = Txt_LogPath->GetWindowCaption();
                }

                if ((LOWORD(wParam) == IDC_EELTRANS_CODE) && (Txt_AutoPrefix)) {
                    autoprefix[g_EeltransThis] = Txt_AutoPrefix->GetWindowCaption();
                }
            }

            // see if the checkboxes are checked
            if (LOWORD(wParam) == IDC_EELTRANS_LOG) {
                enabled_log = (IsDlgButtonChecked(hwndDlg, IDC_EELTRANS_LOG) ? 1 : 0);
            }
            if (LOWORD(wParam) == IDC_EELTRANS_ENABLED) {
                enabled_avstrans =
                    (IsDlgButtonChecked(hwndDlg, IDC_EELTRANS_ENABLED) ? 1 : 0);
            }
            if (LOWORD(wParam) == IDC_EELTRANS_TRANSFIRST) {
                defTransFirst =
                    (IsDlgButtonChecked(hwndDlg, IDC_EELTRANS_TRANSFIRST) ? 1 : 0);
            }
            if (LOWORD(wParam) == IDC_EELTRANS_READCOMMENTCODES) {
                ReadCommentCodes =
                    (IsDlgButtonChecked(hwndDlg, IDC_EELTRANS_READCOMMENTCODES) ? 1
                                                                                : 0);
            }

            return 0;
    }
    return 0;
}
