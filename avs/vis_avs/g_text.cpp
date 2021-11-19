#include "c_text.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>
#include <commctrl.h>

int win32_dlgproc_text(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    C_THISCLASS* g_ConfigThis = (C_THISCLASS*)g_current_render;
    switch (uMsg) {
        case WM_INITDIALOG:
            SetDlgItemText(hwndDlg, IDC_EDIT, g_ConfigThis->text);
            SendDlgItemMessage(hwndDlg, IDC_SPEED, TBM_SETTICFREQ, 10, 0);
            SendDlgItemMessage(
                hwndDlg, IDC_SPEED, TBM_SETRANGE, TRUE, MAKELONG(1, 400));
            SendDlgItemMessage(hwndDlg, IDC_OUTLINESIZE, TBM_SETTICFREQ, 15, 0);
            SendDlgItemMessage(
                hwndDlg, IDC_OUTLINESIZE, TBM_SETRANGE, TRUE, MAKELONG(1, 16));
            SendDlgItemMessage(
                hwndDlg, IDC_OUTLINESIZE, TBM_SETPOS, TRUE, g_ConfigThis->outlinesize);
            SendDlgItemMessage(hwndDlg, IDC_VSHIFT, TBM_SETTICFREQ, 50, 0);
            SendDlgItemMessage(
                hwndDlg, IDC_VSHIFT, TBM_SETRANGE, TRUE, MAKELONG(0, 200));
            SendDlgItemMessage(
                hwndDlg, IDC_VSHIFT, TBM_SETPOS, TRUE, g_ConfigThis->yshift + 100);
            SendDlgItemMessage(hwndDlg, IDC_HSHIFT, TBM_SETTICFREQ, 50, 0);
            SendDlgItemMessage(
                hwndDlg, IDC_HSHIFT, TBM_SETRANGE, TRUE, MAKELONG(0, 200));
            SendDlgItemMessage(
                hwndDlg, IDC_HSHIFT, TBM_SETPOS, TRUE, g_ConfigThis->xshift + 100);
            if (g_ConfigThis->onbeat)
                SendDlgItemMessage(hwndDlg,
                                   IDC_SPEED,
                                   TBM_SETPOS,
                                   TRUE,
                                   (int)(g_ConfigThis->onbeatSpeed));
            else
                SendDlgItemMessage(hwndDlg,
                                   IDC_SPEED,
                                   TBM_SETPOS,
                                   TRUE,
                                   (int)(g_ConfigThis->normSpeed));
            if (g_ConfigThis->enabled) CheckDlgButton(hwndDlg, IDC_CHECK1, BST_CHECKED);
            if (g_ConfigThis->onbeat) CheckDlgButton(hwndDlg, IDC_ONBEAT, BST_CHECKED);
            if (g_ConfigThis->blend) CheckDlgButton(hwndDlg, IDC_ADDITIVE, BST_CHECKED);
            if (g_ConfigThis->blendavg) CheckDlgButton(hwndDlg, IDC_5050, BST_CHECKED);
            if (!g_ConfigThis->blend && !g_ConfigThis->blendavg)
                CheckDlgButton(hwndDlg, IDC_REPLACE, BST_CHECKED);
            if (g_ConfigThis->enabled) CheckDlgButton(hwndDlg, IDC_CHECK1, BST_CHECKED);
            if (g_ConfigThis->outline)
                CheckDlgButton(hwndDlg, IDC_OUTLINE, BST_CHECKED);
            if (g_ConfigThis->shadow) CheckDlgButton(hwndDlg, IDC_SHADOW, BST_CHECKED);
            if (!g_ConfigThis->outline && !g_ConfigThis->shadow)
                CheckDlgButton(hwndDlg, IDC_PLAIN, BST_CHECKED);
            if (g_ConfigThis->randomword)
                CheckDlgButton(hwndDlg, IDC_RANDWORD, BST_CHECKED);
            switch (g_ConfigThis->valign) {
                case DT_TOP:
                    CheckDlgButton(hwndDlg, IDC_VTOP, BST_CHECKED);
                    break;
                case DT_BOTTOM:
                    CheckDlgButton(hwndDlg, IDC_VBOTTOM, BST_CHECKED);
                    break;
                case DT_VCENTER:
                    CheckDlgButton(hwndDlg, IDC_VCENTER, BST_CHECKED);
                    break;
            }
            switch (g_ConfigThis->halign) {
                case DT_LEFT:
                    CheckDlgButton(hwndDlg, IDC_HLEFT, BST_CHECKED);
                    break;
                case DT_RIGHT:
                    CheckDlgButton(hwndDlg, IDC_HRIGHT, BST_CHECKED);
                    break;
                case DT_CENTER:
                    CheckDlgButton(hwndDlg, IDC_HCENTER, BST_CHECKED);
                    break;
            }
            if (g_ConfigThis->insertBlank)
                CheckDlgButton(hwndDlg, IDC_BLANKS, BST_CHECKED);
            if (g_ConfigThis->randomPos) {
                CheckDlgButton(hwndDlg, IDC_RANDOMPOS, BST_CHECKED);
                EnableWindow(GetDlgItem(hwndDlg, IDC_VTOP), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_VBOTTOM), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_VCENTER), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_HLEFT), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_HRIGHT), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_HCENTER), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_HSHIFT), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_VSHIFT), FALSE);
            }
            return 1;
        case WM_NOTIFY: {
            if ((LOWORD(wParam) == IDC_VSHIFT) || (LOWORD(wParam) == IDC_HSHIFT)) {
                g_ConfigThis->yshift =
                    SendDlgItemMessage(hwndDlg, IDC_VSHIFT, TBM_GETPOS, 0, 0) - 100;
                g_ConfigThis->xshift =
                    SendDlgItemMessage(hwndDlg, IDC_HSHIFT, TBM_GETPOS, 0, 0) - 100;
                g_ConfigThis->forceshift = 1;
                g_ConfigThis->forceredraw = 1;
                g_ConfigThis->shiftinit = 1;
            }
            if (LOWORD(wParam) == IDC_OUTLINESIZE) {
                g_ConfigThis->outlinesize =
                    SendDlgItemMessage(hwndDlg, IDC_OUTLINESIZE, TBM_GETPOS, 0, 0);
                g_ConfigThis->forceredraw = 1;
            }
            if (LOWORD(wParam) == IDC_SPEED) {
                if (g_ConfigThis->onbeat) {
                    g_ConfigThis->onbeatSpeed =
                        SendDlgItemMessage(hwndDlg, IDC_SPEED, TBM_GETPOS, 0, 0);
                    if (g_ConfigThis->nb > g_ConfigThis->onbeatSpeed)
                        g_ConfigThis->nb = g_ConfigThis->onbeatSpeed;
                } else
                    g_ConfigThis->normSpeed =
                        SendDlgItemMessage(hwndDlg, IDC_SPEED, TBM_GETPOS, 0, 0);
            }
            return 0;
        }
        case WM_DRAWITEM: {
            DRAWITEMSTRUCT* di = (DRAWITEMSTRUCT*)lParam;
            if (di->CtlID == IDC_DEFCOL || di->CtlID == IDC_DEFOUTCOL)  // paint nifty
                                                                        // color button
            {
                GR_DrawColoredButton(di,
                                     di->CtlID == IDC_DEFCOL
                                         ? g_ConfigThis->color
                                         : g_ConfigThis->outlinecolor);
            }
        }
            return 0;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_HRESET) {
                g_ConfigThis->xshift = 0;
                SendDlgItemMessage(hwndDlg, IDC_HSHIFT, TBM_SETPOS, TRUE, 100);
                g_ConfigThis->forceshift = 1;
                g_ConfigThis->forceredraw = 1;
                g_ConfigThis->shiftinit = 1;
            }
            if (LOWORD(wParam) == IDC_VRESET) {
                g_ConfigThis->yshift = 0;
                SendDlgItemMessage(hwndDlg, IDC_VSHIFT, TBM_SETPOS, TRUE, 100);
                g_ConfigThis->forceshift = 1;
                g_ConfigThis->forceredraw = 1;
                g_ConfigThis->shiftinit = 1;
            }
            if (LOWORD(wParam) == IDC_EDIT)
                if (HIWORD(wParam) == EN_CHANGE) {
                    int l;
                    if (g_ConfigThis->text) GlobalFree(g_ConfigThis->text);
                    l = SendDlgItemMessage(hwndDlg, IDC_EDIT, WM_GETTEXTLENGTH, 0, 0);
                    if (!l)
                        g_ConfigThis->text = NULL;
                    else {
                        g_ConfigThis->text = (char*)GlobalAlloc(GMEM_FIXED, l + 2);
                        GetDlgItemText(hwndDlg, IDC_EDIT, g_ConfigThis->text, l + 1);
                    }
                    g_ConfigThis->forceredraw = 1;
                }
            if (LOWORD(wParam) == IDC_CHOOSEFONT) {
                g_ConfigThis->cf.hwndOwner = hwndDlg;
                ChooseFont(&(g_ConfigThis->cf));
                g_ConfigThis->updating = true;
                g_ConfigThis->myFont = CreateFontIndirect(&(g_ConfigThis->lf));
                g_ConfigThis->forceredraw = 1;
                if (g_ConfigThis->hOldFont)
                    SelectObject(g_ConfigThis->hBitmapDC, g_ConfigThis->hOldFont);
                if (g_ConfigThis->myFont)
                    g_ConfigThis->hOldFont = (HFONT)SelectObject(
                        g_ConfigThis->hBitmapDC, g_ConfigThis->myFont);
                else
                    g_ConfigThis->hOldFont = NULL;
                g_ConfigThis->updating = false;
            }
            if ((LOWORD(wParam) == IDC_CHECK1) || (LOWORD(wParam) == IDC_ONBEAT)
                || (LOWORD(wParam) == IDC_ADDITIVE) || (LOWORD(wParam) == IDC_REPLACE)
                || (LOWORD(wParam) == IDC_BLANKS) || (LOWORD(wParam) == IDC_RANDOMPOS)
                || (LOWORD(wParam) == IDC_OUTLINE) || (LOWORD(wParam) == IDC_SHADOW)
                || (LOWORD(wParam) == IDC_PLAIN) || (LOWORD(wParam) == IDC_HLEFT)
                || (LOWORD(wParam) == IDC_HRIGHT) || (LOWORD(wParam) == IDC_HCENTER)
                || (LOWORD(wParam) == IDC_VTOP) || (LOWORD(wParam) == IDC_VBOTTOM)
                || (LOWORD(wParam) == IDC_VCENTER) || (LOWORD(wParam) == IDC_RANDWORD)
                || (LOWORD(wParam) == IDC_5050)) {
                g_ConfigThis->forceredraw = 1;
                g_ConfigThis->randomword =
                    IsDlgButtonChecked(hwndDlg, IDC_RANDWORD) ? 1 : 0;
                g_ConfigThis->enabled = IsDlgButtonChecked(hwndDlg, IDC_CHECK1) ? 1 : 0;
                g_ConfigThis->outline =
                    IsDlgButtonChecked(hwndDlg, IDC_OUTLINE) ? 1 : 0;
                g_ConfigThis->shadow = IsDlgButtonChecked(hwndDlg, IDC_SHADOW) ? 1 : 0;
                g_ConfigThis->onbeat = IsDlgButtonChecked(hwndDlg, IDC_ONBEAT) ? 1 : 0;
                g_ConfigThis->blend = IsDlgButtonChecked(hwndDlg, IDC_ADDITIVE) ? 1 : 0;
                g_ConfigThis->blendavg = IsDlgButtonChecked(hwndDlg, IDC_5050) ? 1 : 0;
                if (g_ConfigThis->onbeat)
                    SendDlgItemMessage(hwndDlg,
                                       IDC_SPEED,
                                       TBM_SETPOS,
                                       TRUE,
                                       (int)(g_ConfigThis->onbeatSpeed));
                else
                    SendDlgItemMessage(hwndDlg,
                                       IDC_SPEED,
                                       TBM_SETPOS,
                                       TRUE,
                                       (int)(g_ConfigThis->normSpeed));
                g_ConfigThis->insertBlank =
                    IsDlgButtonChecked(hwndDlg, IDC_BLANKS) ? 1 : 0;
                g_ConfigThis->randomPos =
                    IsDlgButtonChecked(hwndDlg, IDC_RANDOMPOS) ? 1 : 0;
                g_ConfigThis->valign = IsDlgButtonChecked(hwndDlg, IDC_VTOP)
                                           ? DT_TOP
                                           : g_ConfigThis->valign;
                g_ConfigThis->valign = IsDlgButtonChecked(hwndDlg, IDC_VCENTER)
                                           ? DT_VCENTER
                                           : g_ConfigThis->valign;
                g_ConfigThis->valign = IsDlgButtonChecked(hwndDlg, IDC_VBOTTOM)
                                           ? DT_BOTTOM
                                           : g_ConfigThis->valign;
                g_ConfigThis->halign = IsDlgButtonChecked(hwndDlg, IDC_HLEFT)
                                           ? DT_LEFT
                                           : g_ConfigThis->halign;
                g_ConfigThis->halign = IsDlgButtonChecked(hwndDlg, IDC_HRIGHT)
                                           ? DT_RIGHT
                                           : g_ConfigThis->halign;
                g_ConfigThis->halign = IsDlgButtonChecked(hwndDlg, IDC_HCENTER)
                                           ? DT_CENTER
                                           : g_ConfigThis->halign;
                EnableWindow(GetDlgItem(hwndDlg, IDC_VTOP),
                             g_ConfigThis->randomPos ? FALSE : TRUE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_VBOTTOM),
                             g_ConfigThis->randomPos ? FALSE : TRUE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_VCENTER),
                             g_ConfigThis->randomPos ? FALSE : TRUE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_HLEFT),
                             g_ConfigThis->randomPos ? FALSE : TRUE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_HRIGHT),
                             g_ConfigThis->randomPos ? FALSE : TRUE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_HCENTER),
                             g_ConfigThis->randomPos ? FALSE : TRUE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_HSHIFT),
                             g_ConfigThis->randomPos ? FALSE : TRUE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_VSHIFT),
                             g_ConfigThis->randomPos ? FALSE : TRUE);
                if ((LOWORD(wParam) == IDC_ONBEAT) && g_ConfigThis->onbeat)
                    g_ConfigThis->nb = g_ConfigThis->onbeatSpeed;
                if ((LOWORD(wParam) == IDC_RANDOMPOS) && g_ConfigThis->randomPos)
                    g_ConfigThis->forcealign = 1;
            }
            if (LOWORD(wParam) == IDC_DEFCOL
                || LOWORD(wParam) == IDC_DEFOUTCOL)  // handle clicks to nifty color
                                                     // button
            {
                int* a = &(LOWORD(wParam) == IDC_DEFCOL ? g_ConfigThis->color
                                                        : g_ConfigThis->outlinecolor);
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
                    if (LOWORD(wParam) == IDC_DEFCOL)
                        g_ConfigThis->color = *a;
                    else
                        g_ConfigThis->outlinecolor = *a;
                }
                InvalidateRect(GetDlgItem(hwndDlg, LOWORD(wParam)), NULL, TRUE);
                g_ConfigThis->updating = true;
                SetTextColor(g_ConfigThis->hBitmapDC,
                             ((g_ConfigThis->color & 0xFF0000) >> 16)
                                 | (g_ConfigThis->color & 0xFF00)
                                 | (g_ConfigThis->color & 0xFF) << 16);
                g_ConfigThis->forceredraw = 1;
                g_ConfigThis->updating = false;
            }
    }
    return 0;
}
