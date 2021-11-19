#pragma once

#include "windows.h"

#include "../gcc-hacks.h"

// implemented in util.cpp
void GR_SelectColor(HWND hwnd, int* a);
void GR_DrawColoredButton(DRAWITEMSTRUCT* di, COLORREF color);
void loadComboBox(HWND dlg, char* ext, char* selectedName);
void compilerfunctionlist(HWND hwndDlg, char* localinfo = NULL);
