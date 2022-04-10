#include "e_channelshift.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>

int win32_dlgproc_chanshift(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    E_ChannelShift* g_this = (E_ChannelShift*)g_current_render;
    const Parameter& p_mode = g_this->info.parameters[0];
    const AVS_Parameter_Handle p_onbeat = g_this->info.parameters[1].handle;

    int64_t options_length;
    const int ids_length = 6;
    int ids[ids_length] = {
        IDC_RGB,
        IDC_GBR,
        IDC_BRG,
        IDC_RBG,
        IDC_BGR,
        IDC_GRB,
    };
    switch (uMsg) {
        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED) {
                p_mode.get_options(&options_length);
                for (unsigned int i = 0; i < options_length && i < ids_length; i++) {
                    if (IsDlgButtonChecked(hwndDlg, ids[i])) {
                        g_this->set_int(p_mode.handle, i);
                    }
                }
                g_this->set_bool(p_onbeat, IsDlgButtonChecked(hwndDlg, IDC_ONBEAT));
            }
            return 1;

        case WM_INITDIALOG:
            auto selected = g_this->get_int(p_mode.handle);
            if (selected >= ids_length) {
                selected = ids_length - 1;
            }
            CheckDlgButton(hwndDlg, ids[selected], 1);
            CheckDlgButton(hwndDlg, IDC_ONBEAT, g_this->get_bool(p_onbeat));
            return 1;
    }
    return 0;
}
