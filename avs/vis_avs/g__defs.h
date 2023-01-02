#pragma once

#include "windows.h"

#include "../platform.h"

// implemented in util.cpp
void GR_SelectColor(HWND hwnd, int* a);
void GR_DrawColoredButton(DRAWITEMSTRUCT* di, COLORREF color);
void loadComboBox(HWND dlg, char* ext, char* selectedName);
void compilerfunctionlist(HWND hwndDlg, const char* title, const char* text = NULL);
void init_ranged_slider(const Parameter& param,
                        int64_t value,
                        HWND hwndDlg,
                        uint32_t control_handle);
void init_ranged_slider_float(const Parameter& param,
                              double value,
                              double factor,
                              HWND hwndDlg,
                              uint32_t control_handle,
                              double tick_freq = 0);
void init_select(const Parameter& param,
                 int64_t value,
                 HWND hwndDlg,
                 uint32_t control_handle);
void init_select_radio(const Parameter& param,
                       int64_t value,
                       HWND hwndDlg,
                       uint32_t* control_handles,
                       size_t num_controls);
void init_resource(const Parameter& param,
                   const char* value,
                   HWND hwndDlg,
                   uint32_t control_handle,
                   size_t max_str_len);
