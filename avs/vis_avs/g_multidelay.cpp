#include "e_multidelay.h"

#include "g__defs.h"
#include "g__lib.h"

#include "resource.h"

#include <windows.h>

int win32_dlgproc_multidelay(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    E_MultiDelay* g_this = (E_MultiDelay*)g_current_render;
    const AVS_Parameter_Handle p_mode = g_this->info.parameters[0].handle;
    const AVS_Parameter_Handle p_active_buffer = g_this->info.parameters[1].handle;

    const AVS_Parameter_Handle p_use_beats = g_this->info.buffer_parameters[0].handle;
    const AVS_Parameter_Handle p_delay = g_this->info.buffer_parameters[1].handle;

    static bool init = false;
    switch (uMsg) {
        case WM_INITDIALOG: {
            init = true;
            if (!g_this->enabled) {
                CheckDlgButton(hwndDlg, IDC_MULTIDELAY_MODE_DISABLED, true);
            } else {
                CheckDlgButton(
                    hwndDlg, IDC_MULTIDELAY_MODE_INPUT + g_this->get_int(p_mode), true);
            }
            int64_t active = g_this->get_int(p_active_buffer);
            for (int64_t i = 0; i < MULTIDELAY_NUM_BUFFERS; i++) {
                CheckDlgButton(hwndDlg, IDC_MULTIDELAY_BUFFER_A + i, active == i);
                bool use_beats = g_this->get_bool(p_use_beats, {i});
                CheckDlgButton(hwndDlg, IDC_MULTIDELAY_A_BEATS + i, use_beats);
                CheckDlgButton(hwndDlg, IDC_MULTIDELAY_A_FRAMES + i, !use_beats);
                int64_t delay = g_this->get_int(p_delay, {i});
                SetDlgItemInt(hwndDlg, IDC_MULTIDELAY_A_DELAY + i, delay, false);
            }
            init = false;
            return 1;
        }
        case WM_COMMAND:
            uint16_t element = LOWORD(wParam);
            if (element == IDC_MULTIDELAY_MODE_DISABLED) {
                g_this->set_enabled(
                    !IsDlgButtonChecked(hwndDlg, IDC_MULTIDELAY_MODE_DISABLED));
            }

            else if (element == IDC_MULTIDELAY_MODE_INPUT
                     || element == IDC_MULTIDELAY_MODE_OUTPUT) {
                if (IsDlgButtonChecked(hwndDlg, element)) {
                    g_this->set_enabled(true);
                    g_this->set_int(p_mode, element - IDC_MULTIDELAY_MODE_INPUT);
                }
            } else if ((element == IDC_MULTIDELAY_BUFFER_A
                        || element == IDC_MULTIDELAY_BUFFER_B
                        || element == IDC_MULTIDELAY_BUFFER_C
                        || element == IDC_MULTIDELAY_BUFFER_D
                        || element == IDC_MULTIDELAY_BUFFER_E
                        || element == IDC_MULTIDELAY_BUFFER_F)
                       && IsDlgButtonChecked(hwndDlg, element)) {
                g_this->set_int(p_active_buffer, element - IDC_MULTIDELAY_BUFFER_A);
            } else if (element == IDC_MULTIDELAY_A_DELAY
                       || element == IDC_MULTIDELAY_B_DELAY
                       || element == IDC_MULTIDELAY_C_DELAY
                       || element == IDC_MULTIDELAY_D_DELAY
                       || element == IDC_MULTIDELAY_E_DELAY
                       || element == IDC_MULTIDELAY_F_DELAY) {
                if (init) {
                    return 1;
                }
                uint16_t message = HIWORD(wParam);
                if (message == EN_CHANGE) {
                    int success = false;
                    uint32_t delay_value;
                    delay_value = GetDlgItemInt(hwndDlg, element, &success, false);
                    if (success) {
                        g_this->set_int(
                            p_delay, delay_value, {element - IDC_MULTIDELAY_A_DELAY});
                    }
                } else if (message == EN_KILLFOCUS) {
                    int64_t delay =
                        g_this->get_int(p_delay, {element - IDC_MULTIDELAY_A_DELAY});
                    SetDlgItemInt(hwndDlg, element, delay, false);
                }
            } else if ((element == IDC_MULTIDELAY_A_BEATS
                        || element == IDC_MULTIDELAY_B_BEATS
                        || element == IDC_MULTIDELAY_C_BEATS
                        || element == IDC_MULTIDELAY_D_BEATS
                        || element == IDC_MULTIDELAY_E_BEATS
                        || element == IDC_MULTIDELAY_F_BEATS)
                       && IsDlgButtonChecked(hwndDlg, element)) {
                g_this->set_bool(p_use_beats, true, {element - IDC_MULTIDELAY_A_BEATS});
            } else if ((element == IDC_MULTIDELAY_A_FRAMES
                        || element == IDC_MULTIDELAY_B_FRAMES
                        || element == IDC_MULTIDELAY_C_FRAMES
                        || element == IDC_MULTIDELAY_D_FRAMES
                        || element == IDC_MULTIDELAY_E_FRAMES
                        || element == IDC_MULTIDELAY_F_FRAMES)
                       && IsDlgButtonChecked(hwndDlg, element)) {
                g_this->set_bool(
                    p_use_beats, false, {element - IDC_MULTIDELAY_A_FRAMES});
            }
            return 0;
    }
    return 0;
}
