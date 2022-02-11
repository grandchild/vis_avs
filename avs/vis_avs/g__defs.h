#pragma once

#include "windows.h"

#include "../platform.h"

// implemented in util.cpp
void GR_SelectColor(HWND hwnd, int* a);
void GR_DrawColoredButton(DRAWITEMSTRUCT* di, COLORREF color);
void loadComboBox(HWND dlg, char* ext, char* selectedName);
void compilerfunctionlist(HWND hwndDlg, const char* title, const char* text = NULL);
