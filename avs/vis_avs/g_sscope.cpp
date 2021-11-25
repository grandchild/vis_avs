#include "c_sscope.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>

int win32_dlgproc_superscope(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    C_THISCLASS* g_this = (C_THISCLASS*)g_current_render;

    static int isstart;
    switch (uMsg) {
        case WM_INITDIALOG:
            isstart = 1;
#if 0  // syntax highlighting
      SendDlgItemMessage(hwndDlg,IDC_EDIT1,EM_SETEVENTMASK,0,ENM_CHANGE);
      SendDlgItemMessage(hwndDlg,IDC_EDIT2,EM_SETEVENTMASK,0,ENM_CHANGE);
      SendDlgItemMessage(hwndDlg,IDC_EDIT3,EM_SETEVENTMASK,0,ENM_CHANGE);
      SendDlgItemMessage(hwndDlg,IDC_EDIT4,EM_SETEVENTMASK,0,ENM_CHANGE);
#endif
            SetDlgItemInt(hwndDlg, IDC_NUMCOL, g_this->num_colors, FALSE);
            SetDlgItemText(hwndDlg, IDC_EDIT1, (char*)g_this->effect_exp[0].c_str());
            SetDlgItemText(hwndDlg, IDC_EDIT2, (char*)g_this->effect_exp[1].c_str());
            SetDlgItemText(hwndDlg, IDC_EDIT3, (char*)g_this->effect_exp[2].c_str());
            SetDlgItemText(hwndDlg, IDC_EDIT4, (char*)g_this->effect_exp[3].c_str());

#if 0  // syntax highlighting
      doAVSEvalHighLight(hwndDlg,IDC_EDIT1,(char*)g_this->effect_exp[0].c_str());
      doAVSEvalHighLight(hwndDlg,IDC_EDIT2,(char*)g_this->effect_exp[1].c_str());
      doAVSEvalHighLight(hwndDlg,IDC_EDIT3,(char*)g_this->effect_exp[2].c_str());
      doAVSEvalHighLight(hwndDlg,IDC_EDIT4,(char*)g_this->effect_exp[3].c_str());
#endif

            CheckDlgButton(hwndDlg, g_this->mode ? IDC_LINES : IDC_DOT, BST_CHECKED);
            if ((g_this->which_ch & 3) == 0)
                CheckDlgButton(hwndDlg, IDC_LEFTCH, BST_CHECKED);
            else if ((g_this->which_ch & 3) == 1)
                CheckDlgButton(hwndDlg, IDC_RIGHTCH, BST_CHECKED);
            else
                CheckDlgButton(hwndDlg, IDC_MIDCH, BST_CHECKED);
            if (g_this->which_ch & 4)
                CheckDlgButton(hwndDlg, IDC_SPEC, BST_CHECKED);
            else
                CheckDlgButton(hwndDlg, IDC_WAVE, BST_CHECKED);

            isstart = 0;
            return 1;
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
        case WM_COMMAND:
            if (!isstart) {
                if ((LOWORD(wParam) == IDC_EDIT1 || LOWORD(wParam) == IDC_EDIT2
                     || LOWORD(wParam) == IDC_EDIT3 || LOWORD(wParam) == IDC_EDIT4)
                    && HIWORD(wParam) == EN_CHANGE) {
                    lock(g_this->code_lock);
                    g_this->effect_exp[0] = string_from_dlgitem(hwndDlg, IDC_EDIT1);
                    g_this->effect_exp[1] = string_from_dlgitem(hwndDlg, IDC_EDIT2);
                    g_this->effect_exp[2] = string_from_dlgitem(hwndDlg, IDC_EDIT3);
                    g_this->effect_exp[3] = string_from_dlgitem(hwndDlg, IDC_EDIT4);
                    g_this->need_recompile = 1;
                    if (LOWORD(wParam) == IDC_EDIT4) g_this->inited = 0;
                    lock_unlock(g_this->code_lock);
#if 0  // syntax highlighting
          if (LOWORD(wParam) == IDC_EDIT1)
            doAVSEvalHighLight(hwndDlg,IDC_EDIT1,(char*)g_this->effect_exp[0].c_str());
          if (LOWORD(wParam) == IDC_EDIT2)
            doAVSEvalHighLight(hwndDlg,IDC_EDIT2,(char*)g_this->effect_exp[1].c_str());
          if (LOWORD(wParam) == IDC_EDIT3)
            doAVSEvalHighLight(hwndDlg,IDC_EDIT3,(char*)g_this->effect_exp[2].c_str());
          if (LOWORD(wParam) == IDC_EDIT4)
            doAVSEvalHighLight(hwndDlg,IDC_EDIT4,(char*)g_this->effect_exp[3].c_str());
#endif
                }
                if (LOWORD(wParam) == IDC_DOT || LOWORD(wParam) == IDC_LINES) {
                    g_this->mode = IsDlgButtonChecked(hwndDlg, IDC_LINES) ? 1 : 0;
                }
                if (LOWORD(wParam) == IDC_WAVE || LOWORD(wParam) == IDC_SPEC) {
                    if (IsDlgButtonChecked(hwndDlg, IDC_WAVE))
                        g_this->which_ch &= ~4;
                    else
                        g_this->which_ch |= 4;
                }
                if (LOWORD(wParam) == IDC_LEFTCH || LOWORD(wParam) == IDC_RIGHTCH
                    || LOWORD(wParam) == IDC_MIDCH) {
                    g_this->which_ch &= ~3;
                    if (IsDlgButtonChecked(hwndDlg, IDC_LEFTCH))
                        ;
                    else if (IsDlgButtonChecked(hwndDlg, IDC_RIGHTCH))
                        g_this->which_ch |= 1;
                    else
                        g_this->which_ch |= 2;
                }
            }
            if (LOWORD(wParam) == IDC_BUTTON2) {
                compilerfunctionlist(hwndDlg, g_this->help_text);
            }
            if (LOWORD(wParam) == IDC_BUTTON1) {
                RECT r;
                HMENU hMenu;
                MENUITEMINFO i = {};
                i.cbSize = sizeof(i);
                hMenu = CreatePopupMenu();
                unsigned int x;
                for (x = 0; x < sizeof(g_this->presets) / sizeof(g_this->presets[0]);
                     x++) {
                    i.fMask = MIIM_TYPE | MIIM_DATA | MIIM_ID;
                    i.fType = MFT_STRING;
                    i.wID = x + 16;
                    i.dwTypeData = g_this->presets[x].name;
                    i.cch = strlen(g_this->presets[x].name);
                    InsertMenuItem(hMenu, x, TRUE, &i);
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
                if (x >= 16
                    && x < 16 + sizeof(g_this->presets) / sizeof(g_this->presets[0])) {
                    SetDlgItemText(hwndDlg, IDC_EDIT1, g_this->presets[x - 16].point);
                    SetDlgItemText(hwndDlg, IDC_EDIT2, g_this->presets[x - 16].frame);
                    SetDlgItemText(hwndDlg, IDC_EDIT3, g_this->presets[x - 16].beat);
                    SetDlgItemText(hwndDlg, IDC_EDIT4, g_this->presets[x - 16].init);
                    SendMessage(
                        hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_EDIT4, EN_CHANGE), 0);
                }
                DestroyMenu(hMenu);
            }
            if (LOWORD(wParam) == IDC_NUMCOL) {
                int p;
                BOOL tr = FALSE;
                p = GetDlgItemInt(hwndDlg, IDC_NUMCOL, &tr, FALSE);
                if (tr) {
                    if (p > 16) p = 16;
                    g_this->num_colors = p;
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_DEFCOL), NULL, TRUE);
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
                    wc = (p.x * g_this->num_colors) / w;
                }
                if (wc >= 0) {
                    GR_SelectColor(hwndDlg, g_this->colors + wc);
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_DEFCOL), NULL, TRUE);
                }
            }
            return 0;
    }
    return 0;
}
