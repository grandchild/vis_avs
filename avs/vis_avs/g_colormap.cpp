#include "c__defs.h"
#include "g__lib.h"
#include "g__defs.h"
#include "c_colormap.h"
#include "resource.h"
#include <windows.h>
#include <commctrl.h>

#define RGB_TO_BGR(color) (((color) & 0xff0000) >> 16 | ((color) & 0xff00) | ((color) & 0xff) << 16)
#define BGR_TO_RGB(color) RGB_TO_BGR(color)  // is its own inverse
#define CLAMP(x, from, to) ((x) >= (to) ? (to) : ((x) <= (from) ? (from) : (x)))
#define TRANSLATE_RANGE(a, a_min, a_max, b_min, b_max) \
    ( ( (a) - (a_min) ) * ( (b_max) - (b_min) ) / ( (a_max) - (a_min) ) )
#define TRANSLATE_DISTANCE(a, a_min, a_max, b, b_min, b_max) \
    ( ( (a_max) * (b) ) / ( (b_max) - (b_min) ) - (a) + (a_min) )


static int g_ColorSetValue;
static int g_currently_selected_color_id = -1;

void save_map_file(C_ColorMap* colormap, int map_index);
void load_map_file(C_ColorMap* colormap, int map_index);

int win32_dlgproc_colormap(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam) {
    C_ColorMap* g_ColormapThis = (C_ColorMap*)g_current_render;
    int choice;
    int current_map = SendDlgItemMessage(hwndDlg, IDC_COLORMAP_MAP_SELECT, CB_GETCURSEL, 0, 0);
    if(current_map < 0) {
        current_map = 0;
    }
    if(current_map >= COLORMAP_NUM_MAPS){
        current_map = 7;
    }
    switch(uMsg) {
        case WM_COMMAND: {  // 0x111
            if(g_ColormapThis == NULL) {
                return 0;
            }
            int wNotifyCode = HIWORD(wParam);
            if(wNotifyCode == CBN_SELCHANGE) {
                switch(LOWORD(wParam)) {
                    case IDC_COLORMAP_KEY_SELECT:
                        g_ColormapThis->config.color_key = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
                        return 0;
                    case IDC_COLORMAP_OUT_BLENDMODE:
                        g_ColormapThis->config.blendmode = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
                        EnableWindow(
                            GetDlgItem(hwndDlg, IDC_COLORMAP_ADJUSTABLE_SLIDER),
                            g_ColormapThis->config.blendmode == COLORMAP_BLENDMODE_ADJUSTABLE);
                        return 0;
                    case IDC_COLORMAP_MAP_SELECT:
                        current_map = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0) & 7;
                        g_ColormapThis->current_map = current_map >= 0 ? current_map : 0;
                        g_ColormapThis->next_map = g_ColormapThis->current_map;
                        CheckDlgButton(hwndDlg, IDC_COLORMAP_MAP_ENABLE, g_ColormapThis->maps[current_map].enabled != 0);
                        InvalidateRect(GetDlgItem(hwndDlg, IDC_COLORMAP_MAPVIEW), 0, 0);
                        SetDlgItemText(hwndDlg, IDC_COLORMAP_FILENAME_VIEW, g_ColormapThis->maps[current_map].filename);
                        return 0;
                    case IDC_COLORMAP_MAP_CYCLING_SELECT:
                        g_ColormapThis->config.map_cycle_mode = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
                        if (g_ColormapThis->config.map_cycle_mode == 0) {
                            g_ColormapThis->current_map = current_map;
                            g_ColormapThis->next_map = current_map;
                        } else {
                            g_ColormapThis->change_animation_step = 0;
                        }
                        EnableWindow(GetDlgItem(hwndDlg, IDC_COLORMAP_MAP_CYCLE_SPEED), g_ColormapThis->config.map_cycle_mode);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_COLORMAP_NO_SKIP_FAST_BEATS), g_ColormapThis->config.map_cycle_mode);
                        return 0;
                }
            } else if(wNotifyCode == BN_CLICKED) {
                switch(LOWORD(wParam)) {
                    case IDC_COLORMAP_HELP:
                        MessageBox(
                            hwndDlg,
                            "Color Map stores 8 different colormaps. For every point on the screen, a key is calculated"
                                " and used as an index (value 0-255) into the map. The point's color will be replaced"
                                " by the one in the map at that index.\n"
                                "If the on-beat cycling option is on, Color Map will cycle between all enabled maps. If"
                                " it's turned off, only Map 1 is used.\n\n"
                                "To edit the map, drag the arrows around with the left mouse button. You can add/edit"
                                "/remove colors by right-clicking, double-click an empty position to add a new point"
                                " there or double-click an existing point to edit it.",
                            "Color Map v1.3 - Help",
                            MB_ICONINFORMATION
                        );
                        return 0;
                    case IDC_COLORMAP_CYCLE_INFO:
                        MessageBox(
                            hwndDlg,
                            "Note: While editing maps, cycling is temporarily disabled. Switch to another component or"
                                " close the AVS editor to enable it again.",
                            "Color Map v1.3 - Help",
                            MB_ICONINFORMATION
                        );
                        return 0;
                    case IDC_COLORMAP_FILE_LOAD:
                        load_map_file(g_ColormapThis, current_map);
                        return 0;
                    case IDC_COLORMAP_FILE_SAVE:
                        save_map_file(g_ColormapThis, current_map);
                        return 0;
                    case IDC_COLORMAP_MAP_ENABLE:
                        g_ColormapThis->maps[current_map].enabled = IsDlgButtonChecked(hwndDlg, IDC_COLORMAP_MAP_ENABLE) == BST_CHECKED;
                        return 0;
                    case IDC_COLORMAP_FLIP_MAP:
                        for(unsigned int i = 0; i<g_ColormapThis->maps[current_map].length; i++) {
                            map_color* color = &g_ColormapThis->maps[current_map].colors[i];
                            color->position = max(0, NUM_COLOR_VALUES - 1 - color->position);
                        }
                        g_ColormapThis->bake_full_map(current_map);
                        InvalidateRect(GetDlgItem(hwndDlg,IDC_COLORMAP_MAPVIEW), 0, 0);
                        return 0;
                    case IDC_COLORMAP_CLEAR_MAP:
                        choice = MessageBox(hwndDlg, "Are you sure you want to clear this map?", "Color Map", MB_ICONWARNING | MB_YESNO);
                        if (choice != IDNO) {
                            g_ColormapThis->make_default_map(current_map);
                            InvalidateRect(GetDlgItem(hwndDlg,IDC_COLORMAP_MAPVIEW), 0, 0);
                            SetDlgItemText(hwndDlg, IDC_COLORMAP_FILENAME_VIEW, g_ColormapThis->maps[current_map].filename);
                        }
                        return 0;
                    case IDC_COLORMAP_NO_SKIP_FAST_BEATS:
                        g_ColormapThis->config.dont_skip_fast_beats = IsDlgButtonChecked(hwndDlg, IDC_COLORMAP_NO_SKIP_FAST_BEATS) == BST_CHECKED;
                        return 0;
                }
            }
            return 1;
        }
        case WM_DESTROY:  // 0x2
            KillTimer(hwndDlg, COLORMAP_MAP_CYCLE_TIMER_ID);
            if(g_ColormapThis != NULL) {
                g_ColormapThis->disable_map_change = 0;
            }
            g_ColormapThis = NULL;
            return 0;
        case WM_INITDIALOG:  // 0x110
            if(g_ColormapThis != NULL) {
                g_ColormapThis->dialog = hwndDlg;
                g_ColormapThis->disable_map_change = 1;
                SetTimer(hwndDlg, COLORMAP_MAP_CYCLE_TIMER_ID, 250, 0);
                for(unsigned int i = 0; i< COLORMAP_NUM_MAPS; i++) {
                    char map_dropdown_name_buf[6] = "Map X";
                    map_dropdown_name_buf[4] = (char)i + '1';
                    SendDlgItemMessage(hwndDlg, IDC_COLORMAP_MAP_SELECT, CB_ADDSTRING, 0, (LPARAM)map_dropdown_name_buf);
                }
                SendDlgItemMessage(hwndDlg, IDC_COLORMAP_MAP_SELECT, CB_SETCURSEL, g_ColormapThis->current_map, 0);
                CheckDlgButton(hwndDlg, IDC_COLORMAP_MAP_ENABLE, g_ColormapThis->maps[g_ColormapThis->current_map].enabled != 0);
                for(unsigned int i = 0; i < COLORMAP_NUM_CYCLEMODES; i++) {
                    SendDlgItemMessage(hwndDlg, IDC_COLORMAP_MAP_CYCLING_SELECT, CB_ADDSTRING, 0, (LPARAM)g_ColormapThis->colormap_labels_map_cycle_mode[i]);
                }
                SendDlgItemMessage(hwndDlg, IDC_COLORMAP_MAP_CYCLING_SELECT, CB_SETCURSEL, g_ColormapThis->config.map_cycle_mode, 0);
                for(unsigned int i = 0; i < COLORMAP_NUM_KEYMODES; i++) {
                    SendDlgItemMessage(hwndDlg, IDC_COLORMAP_KEY_SELECT, CB_ADDSTRING, 0, (LPARAM)g_ColormapThis->colormap_labels_color_key[i]);
                }
                SendDlgItemMessage(hwndDlg, IDC_COLORMAP_KEY_SELECT, CB_SETCURSEL, g_ColormapThis->config.color_key, 0);
                for(unsigned int i = 0; i < COLORMAP_NUM_BLENDMODES; i++) {
                    SendDlgItemMessage(hwndDlg, IDC_COLORMAP_OUT_BLENDMODE, CB_ADDSTRING, 0, (LPARAM)g_ColormapThis->colormap_labels_blendmodes[i]);
                }
                SendDlgItemMessage(hwndDlg, IDC_COLORMAP_OUT_BLENDMODE, CB_SETCURSEL, g_ColormapThis->config.blendmode, 0);
                CheckDlgButton(hwndDlg, IDC_COLORMAP_NO_SKIP_FAST_BEATS, g_ColormapThis->config.dont_skip_fast_beats != 0);
                EnableWindow(GetDlgItem(hwndDlg, IDC_COLORMAP_ADJUSTABLE_SLIDER),
                             g_ColormapThis->config.blendmode == COLORMAP_BLENDMODE_ADJUSTABLE);
                SendDlgItemMessage(hwndDlg, IDC_COLORMAP_ADJUSTABLE_SLIDER, TBM_SETRANGE, TRUE, MAKELONG(0, COLORMAP_ADJUSTABLE_BLEND_MAX));
                SendDlgItemMessage(hwndDlg, IDC_COLORMAP_ADJUSTABLE_SLIDER, TBM_SETPOS, TRUE, g_ColormapThis->config.adjustable_alpha);

                // This overrides the setting of the current map above. bug? quickfix?
                // TODO[feature]: This could be more convenient by saving and restoring the last-selected map.
                SendDlgItemMessage(hwndDlg, IDC_COLORMAP_MAP_SELECT, CB_SETCURSEL, 0, 0);
                SetDlgItemTextA(hwndDlg, IDC_COLORMAP_FILENAME_VIEW, g_ColormapThis->maps[g_ColormapThis->current_map].filename);
                EnableWindow(GetDlgItem(hwndDlg, IDC_COLORMAP_MAP_CYCLE_SPEED), g_ColormapThis->config.map_cycle_mode != 0);
                EnableWindow(GetDlgItem(hwndDlg, IDC_COLORMAP_NO_SKIP_FAST_BEATS), g_ColormapThis->config.map_cycle_mode != 0);
                SendDlgItemMessageA(hwndDlg, IDC_COLORMAP_MAP_CYCLE_SPEED, TBM_SETRANGE, TRUE, MAKELONG(COLORMAP_MAP_CYCLE_SPEED_MIN, COLORMAP_MAP_CYCLE_SPEED_MAX));
                SendDlgItemMessageA(hwndDlg, IDC_COLORMAP_MAP_CYCLE_SPEED, TBM_SETPOS, TRUE, g_ColormapThis->config.map_cycle_speed);
            }
            return 0;
        case WM_TIMER:  // 0x113
            if(g_ColormapThis != NULL) {
                g_ColormapThis->disable_map_change = IsWindowVisible(hwndDlg);
            }
            return 0;
        case WM_HSCROLL:
            if(g_ColormapThis != NULL) {
                if((HWND)lParam == GetDlgItem(hwndDlg, IDC_COLORMAP_ADJUSTABLE_SLIDER)) {
                    int adjustable_pos = SendDlgItemMessage(hwndDlg, IDC_COLORMAP_ADJUSTABLE_SLIDER, TBM_GETPOS, 0, 0);
                    g_ColormapThis->config.adjustable_alpha = CLAMP(adjustable_pos, 0, COLORMAP_ADJUSTABLE_BLEND_MAX);
                } else if((HWND)lParam == GetDlgItem(hwndDlg, IDC_COLORMAP_MAP_CYCLE_SPEED)) {
                    int cycle_speed_pos = SendDlgItemMessage(hwndDlg, IDC_COLORMAP_MAP_CYCLE_SPEED, TBM_GETPOS, 0, 0);
                    g_ColormapThis->config.map_cycle_speed = CLAMP(cycle_speed_pos, COLORMAP_MAP_CYCLE_SPEED_MIN, COLORMAP_MAP_CYCLE_SPEED_MAX);
                }
            }
            return 0;
    }
    return 0;
}

static int win32_dlgproc_colormap_color_position(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam) {
    int v;
    #define VALUE_BUF_LEN 64
    char value_buf[VALUE_BUF_LEN];

    switch(uMsg) {
        case WM_CLOSE:
            EndDialog(hwndDlg, 0);
            return 1;
        case WM_INITDIALOG:
            wsprintf(value_buf, "%d", g_ColorSetValue);
            SetDlgItemText(hwndDlg, IDC_COLORMAP_COLOR_POSITION, value_buf);
            SetFocus(GetDlgItem(hwndDlg, IDC_COLORMAP_COLOR_POSITION));
            SendDlgItemMessage(hwndDlg, IDC_COLORMAP_COLOR_POSITION, EM_SETSEL, 0, -1);
            return 0;
        case WM_COMMAND:
            switch(HIWORD(wParam)) {
                case WM_USER:
                    if(LOWORD(wParam) == IDC_COLORMAP_COLOR_POSITION) {
                        GetWindowText((HWND)lParam, value_buf, VALUE_BUF_LEN);
                        v = atoi(value_buf);
                        g_ColorSetValue = CLAMP(v, 0, NUM_COLOR_VALUES - 1);
                    }
                    return 0;
                case BN_CLICKED:
                    EndDialog(hwndDlg,0);
                    return 1;
            }
    }
    return 0;
}

static int win32_dlgproc_colormap_edit(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    C_ColorMap* g_ColormapThis = (C_ColorMap*)g_current_render;
    if (g_ColormapThis == NULL) {
        return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
    }
    int current_map = SendDlgItemMessage(g_ColormapThis->dialog, IDC_COLORMAP_MAP_SELECT, CB_GETCURSEL, 0, 0);
    if(current_map < 0) {
        current_map = 0;
    }
    if(current_map > 8) {
        current_map = 7;
    }
    ui_map* map = (ui_map*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    HDC desktop_dc;
    PAINTSTRUCT painter;
    RECT map_rect;
    RECT draw_rect;

    HDC paint_context;
    HBRUSH brush;
    HGDIOBJ brush_sel;
    HPEN pen;
    HGDIOBJ pen_sel;

    short x, y;
    int selected_color_index;
    int selected_menu_item;
    CHOOSECOLOR choosecolor;
    bool has_chosen_color;

    switch (uMsg) {
        case WM_CREATE:  // 0x1
            desktop_dc = GetDC(hwndDlg);
            map = new ui_map;
            GetClientRect(hwndDlg, &map->window_rect);
            map->context = CreateCompatibleDC(0);
            map->bitmap = CreateCompatibleBitmap(
                desktop_dc, map->window_rect.right, map->window_rect.bottom);
            map->region = (LPRECT)SelectObject(map->context, map->bitmap);
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)map);
            ReleaseDC(hwndDlg, desktop_dc);
            return 0;
        case WM_DESTROY:  // 0x2
            SelectObject(map->context, map->region);
            DeleteObject(map->bitmap);
            DeleteDC(map->context);
            return 0;
        case WM_PAINT:  // 0xf
            paint_context = BeginPaint(hwndDlg, &painter);
            GetClientRect(hwndDlg, &map_rect);
            brush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
            FillRect(map->context, &map_rect, brush);
            DeleteObject(brush);

            map_rect.bottom -= 15;  // make space for color handles

            pen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));
            pen_sel = SelectObject(map->context, pen);
            MoveToEx(map->context, map_rect.right - 1, 0, 0);
            LineTo(map->context, 0, 0);
            LineTo(map->context, 0, map_rect.bottom - 1);
            SelectObject(map->context, pen_sel);
            DeleteObject(pen);

            pen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DHILIGHT));
            pen_sel = SelectObject(map->context, pen);
            MoveToEx(map->context, map_rect.right - 1, 0, 0);
            LineTo(map->context, map_rect.right - 1, map_rect.bottom - 1);
            LineTo(map->context, -1, map_rect.bottom - 1);
            SelectObject(map->context, pen_sel);
            DeleteObject(pen);

            map_rect.top++;
            map_rect.left++;
            map_rect.bottom--;
            map_rect.right--;

            draw_rect.top = map_rect.top;
            draw_rect.bottom = map_rect.bottom;
            for(unsigned int i = 0; i < NUM_COLOR_VALUES; i++) {
                int w = map_rect.right - map_rect.left;
                // The disassembled code from the original reads:
                //   CDQ  (i * w)              ; all bits in edx are (i * w)'s sign-bit
                //   AND  edx, 0xff            ; offset = (i * w) < 0 ? 255 : 0
                //   ADD  eax, edx             ; i * w + offset
                // This seems like what MSVC might emit for modulo, which seems
                // unneccessary unless you'd expect (map_rect.right - map_rect.left) to
                // be negative. The following should be sufficient:
                draw_rect.left = i * w / NUM_COLOR_VALUES + map_rect.left;
                draw_rect.right = (i + 1) * w / NUM_COLOR_VALUES + map_rect.left;
                int color = g_ColormapThis->baked_maps[current_map].colors[i];
                brush = CreateSolidBrush(RGB_TO_BGR(color));
                FillRect(map->context, &draw_rect, brush);
                DeleteObject(brush);
            }
            for(unsigned int i = 0; i < g_ColormapThis->maps[current_map].length; i++) {
                unsigned int color = g_ColormapThis->maps[current_map].colors[i].color;
                brush = CreateSolidBrush(RGB_TO_BGR(color));
                brush_sel = SelectObject(map->context, brush);
                bool is_selected = g_ColormapThis->maps[current_map].colors[i].color_id == g_currently_selected_color_id;
                SelectObject(map->context, GetStockObject(is_selected ? WHITE_PEN : BLACK_PEN));
                POINT triangle[3];
                triangle[0].x = g_ColormapThis->maps[current_map].colors[i].position * map_rect.right / 0xff;
                triangle[0].y = map_rect.bottom + 3;
                triangle[1].x = triangle[0].x + 6;
                triangle[1].y = map_rect.bottom + 15;
                triangle[2].x = triangle[0].x - 6;
                triangle[2].y = map_rect.bottom + 15;
                Polygon(map->context, triangle, 3);
                SelectObject(map->context, brush_sel);
                DeleteObject(brush);
            }
            GetClientRect(hwndDlg, &map_rect);
            BitBlt(paint_context, map_rect.left, map_rect.top, map_rect.right, map_rect.bottom, map->context, 0, 0, SRCCOPY);
            EndPaint(hwndDlg, &painter);
            return 0;
        case WM_LBUTTONDOWN:  // 0x0201
            x = LOWORD(lParam);
            y = HIWORD(lParam);
            GetClientRect(hwndDlg, &draw_rect);
            draw_rect.top++;
            draw_rect.left++;
            draw_rect.right--;
            draw_rect.bottom--;
            g_currently_selected_color_id = -1;
            if(y > draw_rect.bottom - 14) {
                for(unsigned int i = 0; i < g_ColormapThis->maps[current_map].length; i++) {
                    map_color color = g_ColormapThis->maps[current_map].colors[i];
                    int distance = TRANSLATE_DISTANCE(x, draw_rect.left, draw_rect.right, color.position, 0, NUM_COLOR_VALUES - 1);
                    if(abs(distance) < 6) {
                        g_currently_selected_color_id = color.color_id;
                    }
                }
            }
            InvalidateRect(hwndDlg, 0, 0);
            SetCapture(hwndDlg);
            return 0;
        case WM_MOUSEMOVE:  // 0x0200
            if(GetCapture() == hwndDlg) {
                x = LOWORD(lParam);
                GetClientRect(hwndDlg, &draw_rect);
                draw_rect.top++;
                draw_rect.left++;
                draw_rect.right--;
                draw_rect.bottom--;
                x = CLAMP(x, draw_rect.left, draw_rect.right);
                for(unsigned int i = 0; i < g_ColormapThis->maps[current_map].length; i++) {
                    map_color* color = &g_ColormapThis->maps[current_map].colors[i];
                    if(color->color_id == g_currently_selected_color_id) {
                        color->position = TRANSLATE_RANGE(x, draw_rect.left, draw_rect.right, 0, NUM_COLOR_VALUES - 1);
                        g_ColormapThis->bake_full_map(current_map);
                    }
                }
                InvalidateRect(hwndDlg, 0, 0);
            }
            return 0;
        case WM_LBUTTONUP:  // 0x0202
            InvalidateRect(hwndDlg, 0, 0);
            ReleaseCapture();
            return 0;
        case WM_LBUTTONDBLCLK:  // 0x0203
        case WM_RBUTTONDOWN:  // 0x0204
            x = LOWORD(lParam);
            y = HIWORD(lParam);
            GetClientRect(hwndDlg, &draw_rect);
            draw_rect.top++;
            draw_rect.left++;
            draw_rect.right--;
            draw_rect.bottom--;
            x = CLAMP(x, draw_rect.left, draw_rect.right);
            g_currently_selected_color_id = -1;
            if(y >= draw_rect.bottom - 14) {
                for(unsigned int i = 0; i < g_ColormapThis->maps[current_map].length; i++) {
                    map_color color = g_ColormapThis->maps[current_map].colors[i];
                    int distance = TRANSLATE_DISTANCE(x, draw_rect.left, draw_rect.right, color.position, 0, NUM_COLOR_VALUES - 1);
                    if(abs(distance) < 6) {
                        g_currently_selected_color_id = color.color_id;
                    }
                }
            }
            selected_color_index = 0;
            for(unsigned int i = 0; i < g_ColormapThis->maps[current_map].length; i++) {
                if(g_ColormapThis->maps[current_map].colors[i].color_id == g_currently_selected_color_id) {
                    selected_color_index = i;
                }
            }
            if(uMsg == WM_LBUTTONDBLCLK) {
                selected_menu_item = g_currently_selected_color_id != -1 ?
                                     IDM_COLORMAP_MENU_EDIT : IDM_COLORMAP_MENU_ADD;
            } else {
                HMENU menu = CreatePopupMenu();
                AppendMenu(menu, MF_ENABLED, IDM_COLORMAP_MENU_ADD, "Add Color");
                if (g_currently_selected_color_id != -1) {
                    AppendMenu(menu, MF_ENABLED, IDM_COLORMAP_MENU_EDIT, "Edit Color");
                    if (g_ColormapThis->maps[current_map].length > 1) {
                        AppendMenu(menu, MF_ENABLED, IDM_COLORMAP_MENU_DELETE, "Delete Color");
                    }
                    AppendMenu(menu, MF_SEPARATOR, 0, NULL);
                    char set_position_str[19];
                    wsprintf(set_position_str, "Set Position (%d)",
                             g_ColormapThis->maps[current_map].colors[selected_color_index].position);
                    AppendMenu(menu, MF_ENABLED, IDM_COLORMAP_MENU_SETPOS, set_position_str);
                }
                POINT popup_coords = {x + 1, y};
                ClientToScreen(hwndDlg, &popup_coords);
                selected_menu_item = TrackPopupMenuEx(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, popup_coords.x, popup_coords.y, hwndDlg, NULL);
            }
            if(selected_menu_item) {
                int color_position = TRANSLATE_RANGE(x, draw_rect.left, draw_rect.right, 0, NUM_COLOR_VALUES - 1);
                static COLORREF custcolors[16];
                switch(selected_menu_item) {
                    case IDM_COLORMAP_MENU_ADD:
                        choosecolor.lStructSize = sizeof(CHOOSECOLOR);
                        choosecolor.hwndOwner = hwndDlg;
                        choosecolor.rgbResult = RGB_TO_BGR(g_ColormapThis->baked_maps[current_map].colors[color_position]);
                        choosecolor.lpCustColors = custcolors;
                        choosecolor.Flags = CC_RGBINIT | CC_FULLOPEN | CC_ANYCOLOR;
                        has_chosen_color = ChooseColor(&choosecolor);
                        if(has_chosen_color) {
                            g_ColormapThis->add_map_color(current_map, color_position, BGR_TO_RGB(choosecolor.rgbResult));
                        }
                        break;
                    case IDM_COLORMAP_MENU_EDIT:
                        choosecolor.lStructSize = sizeof(CHOOSECOLOR);
                        choosecolor.hwndOwner = hwndDlg;
                        choosecolor.rgbResult = RGB_TO_BGR(g_ColormapThis->maps[current_map].colors[selected_color_index].color);
                        choosecolor.lpCustColors = custcolors;
                        choosecolor.Flags = CC_RGBINIT | CC_FULLOPEN | CC_ANYCOLOR;
                        has_chosen_color = ChooseColor(&choosecolor);
                        if(has_chosen_color) {
                            g_ColormapThis->maps[current_map].colors[selected_color_index].color = BGR_TO_RGB(choosecolor.rgbResult);
                        }
                        break;
                    case IDM_COLORMAP_MENU_DELETE:
                        g_ColormapThis->remove_map_color(current_map, g_currently_selected_color_id);
                        break;
                    case IDM_COLORMAP_MENU_SETPOS:
                        g_ColorSetValue = g_ColormapThis->maps[current_map].colors[selected_color_index].position;
                        DialogBoxParam(NULL, MAKEINTRESOURCE(IDD_CFG_COLORMAP_COLOR_POSITION), hwndDlg, (DLGPROC)win32_dlgproc_colormap_color_position, 0);
                        g_ColormapThis->maps[current_map].colors[selected_color_index].position = g_ColorSetValue;
                        break;
                }
                g_ColormapThis->bake_full_map(current_map);
            }
            InvalidateRect(hwndDlg, NULL, 0);
            return 0;
    }
    return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
}

void win32_uiprep_colormap(HINSTANCE hInstance) {
    WNDCLASSEX mapview;
    mapview.cbSize = sizeof(WNDCLASSEX);
    if(GetClassInfoEx(hInstance, "ColormapEdit", &mapview)) {
        return;
    }
    mapview.style = CS_PARENTDC | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    mapview.lpfnWndProc = (WNDPROC)win32_dlgproc_colormap_edit;
    mapview.cbClsExtra = 0;
    mapview.cbWndExtra = 0;
    mapview.hInstance = hInstance;
    mapview.hIcon = NULL;
    mapview.hCursor = LoadCursor(0, MAKEINTRESOURCE(IDC_ARROW));
    mapview.hbrBackground = NULL;
    mapview.lpszMenuName = NULL;
    mapview.lpszClassName = "ColormapEdit";
    RegisterClassEx(&mapview);
}

bool endswithn(char* str, char* suffix, size_t n) {
    size_t str_len = strnlen(str, n);
    size_t suffix_len = strnlen(suffix, n);
    if(str_len < suffix_len) {
        return false;
    }
    char* str_end = str + (str_len - suffix_len);
    bool result = strncmp(str_end, suffix, suffix_len) == 0;
    return result;
}

void save_map_file(C_ColorMap* colormap, int map_index) {
    char filepath[MAX_PATH];
    OPENFILENAME openfilename;
    if(strnlen(colormap->maps[map_index].filename, COLORMAP_MAP_FILENAME_MAXLEN) > 0) {
        strncpy(filepath, colormap->maps[map_index].filename, COLORMAP_MAP_FILENAME_MAXLEN);
    } else {
        strncpy(filepath, "New Map.clm", COLORMAP_MAP_FILENAME_MAXLEN);
    }
    openfilename.lpstrInitialDir = g_path;
    openfilename.hwndOwner = NULL;
    openfilename.lpstrFileTitle = NULL;
    openfilename.nMaxFileTitle = 0;
    openfilename.lpstrFile = filepath;
    openfilename.lStructSize = sizeof(OPENFILENAME);
    openfilename.nMaxFile = MAX_PATH;
    openfilename.lpstrFilter = "Color Map (*.clm)\0*.clm\0All Files\0*\0";
    openfilename.nFilterIndex = 1;
    openfilename.lpstrTitle = "Save Color Map";
    openfilename.Flags = OFN_EXPLORER;
    if(GetSaveFileName(&openfilename) == 0) {
        return;
    }
    strncpy(filepath, openfilename.lpstrFile, MAX_PATH - 1);
    filepath[MAX_PATH - 1] = '\0';
    if(!endswithn(openfilename.lpstrFile, ".clm", MAX_PATH)) {
        size_t len_selected_path = strnlen(openfilename.lpstrFile, MAX_PATH);
        if(len_selected_path >= MAX_PATH - 4) {
            MessageBox(colormap->dialog, "Filename too long!", "Error", MB_ICONERROR);
            return;
        }
        strncpy(filepath + len_selected_path, ".clm", 5);
    }
    FILE* file = fopen(openfilename.lpstrFile, "wb");
    if(file == NULL) {
        MessageBox(colormap->dialog, "Unable to open file for writing!", "Error", MB_ICONERROR);
        return;
    }
    fwrite("CLM1", 4, 1, file);
    fwrite(&colormap->maps[map_index].length, sizeof(int), 1, file);
    fwrite(colormap->maps[map_index].colors, sizeof(map_color), colormap->maps[map_index].length, file);
    fclose(file);
    // TODO [cleanup][bugfix]: Make OS-portable. May return NULL.
    char* basename = strrchr(filepath, '\\') + 1;
    strncpy(colormap->maps[map_index].filename, basename, COLORMAP_MAP_FILENAME_MAXLEN);
    colormap->maps[map_index].filename[COLORMAP_MAP_FILENAME_MAXLEN - 1] = '\0';
    SetDlgItemText(colormap->dialog, IDC_COLORMAP_FILENAME_VIEW, colormap->maps[map_index].filename);
}

void load_map_file(C_ColorMap* colormap, int map_index) {
    char filename[MAX_PATH];
    OPENFILENAME openfilename;
    HANDLE file;
    unsigned char* contents;
    size_t filesize;
    long unsigned int bytes_read;
    int length;
    int colors_offset;

    strncpy(filename, colormap->maps[map_index].filename, COLORMAP_MAP_FILENAME_MAXLEN);
    openfilename.lpstrInitialDir = g_path;
    openfilename.hwndOwner = NULL;
    openfilename.lpstrFileTitle = NULL;
    openfilename.nMaxFileTitle = 0;
    openfilename.lpstrFile = filename;
    openfilename.lStructSize = sizeof(OPENFILENAME);
    openfilename.nMaxFile = MAX_PATH;
    openfilename.lpstrFilter = "Color Map (*.clm)\0*.clm\0All Files\0*\0";
    openfilename.nFilterIndex = 1;
    openfilename.lpstrTitle = "Load Color Map";
    openfilename.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    if(GetOpenFileName(&openfilename) == 0) {
        return;
    }
    file = CreateFile(
        openfilename.lpstrFile,
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
        NULL
    );
    if (file == INVALID_HANDLE_VALUE) {
        MessageBox(colormap->dialog, "Unable to open file for reading!", "Error", MB_ICONERROR);
        return;
    }
    filesize = GetFileSize(file, NULL);
    contents = new unsigned char[filesize];
    ReadFile(file, contents, filesize, &bytes_read, NULL);
    CloseHandle(file);
    if(strncmp((char*)contents, "CLM1", 4) != 0) {
        delete[] contents;
        MessageBox(colormap->dialog, "Invalid CLM file!", "Error", MB_ICONERROR);
        return;
    }
    length = *(int*)(contents + 4);
    colors_offset = 4/*CLM1*/ + 4/*length*/;
    if(length < 0
        || length > COLORMAP_MAX_COLORS
        || length != (bytes_read - colors_offset) / sizeof(map_color)
    ) {
        MessageBox(colormap->dialog, "Corrupt CLM file!", "Warning", MB_ICONWARNING);
        length = (bytes_read - colors_offset) / sizeof(map_color);
    }
    colormap->maps[map_index].length = length;
    colormap->load_map_colors(contents, bytes_read, map_index, colors_offset);
    delete[] contents;
    strncpy(colormap->maps[map_index].filename, strrchr(openfilename.lpstrFile, '\\') + 1, COLORMAP_MAP_FILENAME_MAXLEN);
    InvalidateRect(GetDlgItem(colormap->dialog, IDC_COLORMAP_MAPVIEW), NULL, 0);
    SetDlgItemText(colormap->dialog, IDC_COLORMAP_FILENAME_VIEW, colormap->maps[map_index].filename);
    colormap->bake_full_map(map_index);
}


