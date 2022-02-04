#include "e_colormap.h"

#include "g__defs.h"
#include "g__lib.h"

#include "c__defs.h"
#include "resource.h"

#include "../util.h"

#include <windows.h>
#include <commctrl.h>
#include <unordered_map>

#define RGB_TO_BGR(color) \
    (((color)&0xff0000) >> 16 | ((color)&0xff00) | ((color)&0xff) << 16)
#define BGR_TO_RGB(color)  RGB_TO_BGR(color)  // is its own inverse
#define CLAMP(x, from, to) ((x) >= (to) ? (to) : ((x) <= (from) ? (from) : (x)))
#define TRANSLATE_RANGE(a, a_min, a_max, b_min, b_max) \
    (((a) - (a_min)) * ((b_max) - (b_min)) / ((a_max) - (a_min)))
#define TRANSLATE_DISTANCE(a, a_min, a_max, b, b_min, b_max) \
    (((a_max) * (b)) / ((b_max) - (b_min)) - (a) + (a_min))

typedef struct {
    RECT window_rect;
    HDC context;
    HBITMAP bitmap;
    LPRECT region;
} ui_map;

static int g_ColorSetValue;
static int g_currently_selected_color_id = -1;
static std::unordered_map<AVS_Component_Handle, int64_t> g_currently_selected_map;

extern HINSTANCE g_hInstance;

void save_map_file(E_ColorMap* colormap, int map_index, HWND hwndDlg);
void load_map_file(E_ColorMap* colormap, int map_index, HWND hwndDlg);

int win32_dlgproc_colormap(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    E_ColorMap* g_this = (E_ColorMap*)g_current_render;

    const Parameter& p_color_key = g_this->info.parameters[0];
    const Parameter& p_blendmode = g_this->info.parameters[1];
    const Parameter& p_map_cycle_mode = g_this->info.parameters[2];
    const Parameter& p_map_cycle_speed = g_this->info.parameters[5];
    const Parameter& p_maps = g_this->info.parameters[6];

    const Parameter& p_flip_map = g_this->info.map_params[4];
    const Parameter& p_clear_map = g_this->info.map_params[5];

    int64_t options_length;
    const char* const* options;
    int choice;
    int current_map =
        SendDlgItemMessage(hwndDlg, IDC_COLORMAP_MAP_SELECT, CB_GETCURSEL, 0, 0);
    if (current_map < 0) {
        current_map = 0;
    }
    int64_t num_maps = 0;
    if (g_this != NULL) {
        num_maps = g_this->parameter_list_length(&p_maps);
        if (current_map >= num_maps) {
            current_map = num_maps - 1;
        }
    }
    switch (uMsg) {
        case WM_SHOWWINDOW: {
            g_this->config.disable_map_change = wParam;
            if (wParam) {
                g_this->config.current_map = g_currently_selected_map[g_this->handle];
            }
            break;
        }
        case WM_COMMAND: {  // 0x111
            if (g_this == NULL) {
                return 0;
            }
            int wNotifyCode = HIWORD(wParam);
            if (wNotifyCode == CBN_SELCHANGE) {
                switch (LOWORD(wParam)) {
                    case IDC_COLORMAP_KEY_SELECT:
                        g_this->set_int(p_color_key.handle,
                                        SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0));
                        return 0;
                    case IDC_COLORMAP_OUT_BLENDMODE:
                        g_this->set_int(p_blendmode.handle,
                                        SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0));
                        EnableWindow(
                            GetDlgItem(hwndDlg, IDC_COLORMAP_ADJUSTABLE_SLIDER),
                            g_this->config.blendmode == COLORMAP_BLENDMODE_ADJUSTABLE);
                        return 0;
                    case IDC_COLORMAP_MAP_SELECT:
                        current_map = CLAMP(
                            SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0), 0, num_maps);
                        g_currently_selected_map[g_this->handle] = current_map;
                        g_this->config.current_map = current_map;
                        g_this->config.next_map = g_this->config.current_map;
                        CheckDlgButton(hwndDlg,
                                       IDC_COLORMAP_MAP_ENABLE,
                                       g_this->config.maps[current_map].enabled != 0);
                        InvalidateRect(GetDlgItem(hwndDlg, IDC_COLORMAP_MAPVIEW), 0, 0);
                        SetDlgItemText(
                            hwndDlg,
                            IDC_COLORMAP_FILENAME_VIEW,
                            g_this->config.maps[current_map].filepath.c_str());
                        return 0;
                    case IDC_COLORMAP_MAP_CYCLING_SELECT:
                        g_this->set_int(p_map_cycle_mode.handle,
                                        SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0));
                        EnableWindow(GetDlgItem(hwndDlg, IDC_COLORMAP_MAP_CYCLE_SPEED),
                                     g_this->config.map_cycle_mode);
                        EnableWindow(
                            GetDlgItem(hwndDlg, IDC_COLORMAP_NO_SKIP_FAST_BEATS),
                            g_this->config.map_cycle_mode);
                        return 0;
                }
            } else if (wNotifyCode == BN_CLICKED) {
                switch (LOWORD(wParam)) {
                    case IDC_COLORMAP_HELP:
                        MessageBox(hwndDlg,
                                   g_this->info.get_help(),
                                   "Color Map v1.3 - Help",
                                   MB_ICONINFORMATION);
                        return 0;
                    case IDC_COLORMAP_CYCLE_INFO:
                        MessageBox(hwndDlg,
                                   "Note: While editing maps, cycling is temporarily "
                                   "disabled. Switch to another component or"
                                   " close the AVS editor to enable it again.",
                                   "Color Map v1.3 - Help",
                                   MB_ICONINFORMATION);
                        return 0;
                    case IDC_COLORMAP_FILE_LOAD:
                        load_map_file(g_this, current_map, hwndDlg);
                        return 0;
                    case IDC_COLORMAP_FILE_SAVE:
                        save_map_file(g_this, current_map, hwndDlg);
                        return 0;
                    case IDC_COLORMAP_MAP_ENABLE:
                        g_this->config.maps[current_map].enabled =
                            IsDlgButtonChecked(hwndDlg, IDC_COLORMAP_MAP_ENABLE)
                            == BST_CHECKED;
                        return 0;
                    case IDC_COLORMAP_FLIP_MAP:
                        g_this->run_action(p_flip_map.handle, {current_map});
                        InvalidateRect(GetDlgItem(hwndDlg, IDC_COLORMAP_MAPVIEW), 0, 0);
                        return 0;
                    case IDC_COLORMAP_CLEAR_MAP:
                        choice = MessageBox(hwndDlg,
                                            "Are you sure you want to clear this map?",
                                            "Color Map",
                                            MB_ICONWARNING | MB_YESNO);
                        if (choice != IDNO) {
                            g_this->run_action(p_clear_map.handle, {current_map});
                            InvalidateRect(
                                GetDlgItem(hwndDlg, IDC_COLORMAP_MAPVIEW), 0, 0);
                            SetDlgItemText(
                                hwndDlg,
                                IDC_COLORMAP_FILENAME_VIEW,
                                g_this->config.maps[current_map].filepath.c_str());
                        }
                        return 0;
                    case IDC_COLORMAP_NO_SKIP_FAST_BEATS:
                        g_this->config.dont_skip_fast_beats =
                            IsDlgButtonChecked(hwndDlg, IDC_COLORMAP_NO_SKIP_FAST_BEATS)
                            == BST_CHECKED;
                        return 0;
                }
            }
            return 1;
        }
        case WM_DESTROY:  // 0x2
            if (g_this != NULL) {
                g_this->config.disable_map_change = false;
            }
            g_this = NULL;
            return 0;
        case WM_INITDIALOG:  // 0x110
            if (g_this != NULL) {
                g_this->config.disable_map_change = true;
                for (unsigned int i = 0; i < COLORMAP_NUM_MAPS; i++) {
                    char map_select_name[6] = "Map X";
                    map_select_name[4] = (char)i + '1';
                    SendDlgItemMessage(hwndDlg,
                                       IDC_COLORMAP_MAP_SELECT,
                                       CB_ADDSTRING,
                                       0,
                                       (LPARAM)map_select_name);
                }
                // TODO[feature]: This could be more convenient by saving and restoring
                // the last-selected map.
                g_this->config.current_map = g_currently_selected_map[g_this->handle];
                SendDlgItemMessage(hwndDlg,
                                   IDC_COLORMAP_MAP_SELECT,
                                   CB_SETCURSEL,
                                   g_this->config.current_map,
                                   0);
                CheckDlgButton(
                    hwndDlg,
                    IDC_COLORMAP_MAP_ENABLE,
                    g_this->config.maps[g_this->config.current_map].enabled != 0);
                options = p_map_cycle_mode.get_options(&options_length);
                for (unsigned int i = 0; i < options_length; i++) {
                    SendDlgItemMessage(hwndDlg,
                                       IDC_COLORMAP_MAP_CYCLING_SELECT,
                                       CB_ADDSTRING,
                                       0,
                                       (LPARAM)options[i]);
                }
                SendDlgItemMessage(hwndDlg,
                                   IDC_COLORMAP_MAP_CYCLING_SELECT,
                                   CB_SETCURSEL,
                                   g_this->config.map_cycle_mode,
                                   0);
                options = p_color_key.get_options(&options_length);
                for (unsigned int i = 0; i < options_length; i++) {
                    SendDlgItemMessage(hwndDlg,
                                       IDC_COLORMAP_KEY_SELECT,
                                       CB_ADDSTRING,
                                       0,
                                       (LPARAM)options[i]);
                }
                SendDlgItemMessage(hwndDlg,
                                   IDC_COLORMAP_KEY_SELECT,
                                   CB_SETCURSEL,
                                   g_this->config.color_key,
                                   0);
                options = p_blendmode.get_options(&options_length);
                for (unsigned int i = 0; i < options_length; i++) {
                    SendDlgItemMessage(hwndDlg,
                                       IDC_COLORMAP_OUT_BLENDMODE,
                                       CB_ADDSTRING,
                                       0,
                                       (LPARAM)options[i]);
                }
                SendDlgItemMessage(hwndDlg,
                                   IDC_COLORMAP_OUT_BLENDMODE,
                                   CB_SETCURSEL,
                                   g_this->config.blendmode,
                                   0);
                CheckDlgButton(hwndDlg,
                               IDC_COLORMAP_NO_SKIP_FAST_BEATS,
                               g_this->config.dont_skip_fast_beats != 0);
                EnableWindow(GetDlgItem(hwndDlg, IDC_COLORMAP_ADJUSTABLE_SLIDER),
                             g_this->config.blendmode == COLORMAP_BLENDMODE_ADJUSTABLE);
                SendDlgItemMessage(hwndDlg,
                                   IDC_COLORMAP_ADJUSTABLE_SLIDER,
                                   TBM_SETRANGE,
                                   TRUE,
                                   MAKELONG(0, COLORMAP_ADJUSTABLE_BLEND_MAX));
                SendDlgItemMessage(hwndDlg,
                                   IDC_COLORMAP_ADJUSTABLE_SLIDER,
                                   TBM_SETPOS,
                                   TRUE,
                                   g_this->config.adjustable_alpha);

                SetDlgItemTextA(hwndDlg,
                                IDC_COLORMAP_FILENAME_VIEW,
                                g_this->config.maps[current_map].filepath.c_str());
                EnableWindow(GetDlgItem(hwndDlg, IDC_COLORMAP_MAP_CYCLE_SPEED),
                             g_this->config.map_cycle_mode != 0);
                EnableWindow(GetDlgItem(hwndDlg, IDC_COLORMAP_NO_SKIP_FAST_BEATS),
                             g_this->config.map_cycle_mode != 0);
                SendDlgItemMessageA(
                    hwndDlg,
                    IDC_COLORMAP_MAP_CYCLE_SPEED,
                    TBM_SETRANGE,
                    TRUE,
                    MAKELONG(p_map_cycle_speed.int_min, p_map_cycle_speed.int_max));
                SendDlgItemMessageA(hwndDlg,
                                    IDC_COLORMAP_MAP_CYCLE_SPEED,
                                    TBM_SETPOS,
                                    TRUE,
                                    g_this->config.map_cycle_speed);
            }
            return 0;
        case WM_HSCROLL:
            if (g_this != NULL) {
                if ((HWND)lParam
                    == GetDlgItem(hwndDlg, IDC_COLORMAP_ADJUSTABLE_SLIDER)) {
                    int adjustable_pos = SendDlgItemMessage(
                        hwndDlg, IDC_COLORMAP_ADJUSTABLE_SLIDER, TBM_GETPOS, 0, 0);
                    g_this->config.adjustable_alpha =
                        CLAMP(adjustable_pos, 0, COLORMAP_ADJUSTABLE_BLEND_MAX);
                } else if ((HWND)lParam
                           == GetDlgItem(hwndDlg, IDC_COLORMAP_MAP_CYCLE_SPEED)) {
                    int cycle_speed_pos = SendDlgItemMessage(
                        hwndDlg, IDC_COLORMAP_MAP_CYCLE_SPEED, TBM_GETPOS, 0, 0);
                    g_this->config.map_cycle_speed =
                        CLAMP(cycle_speed_pos,
                              COLORMAP_MAP_CYCLE_SPEED_MIN,
                              COLORMAP_MAP_CYCLE_SPEED_MAX);
                }
            }
            return 0;
    }
    return 0;
}

static int win32_dlgproc_colormap_color_position(HWND hwndDlg,
                                                 UINT uMsg,
                                                 WPARAM wParam,
                                                 LPARAM) {
    int v;

    switch (uMsg) {
        case WM_CLOSE:
            EndDialog(hwndDlg, 0);
            return 1;
        case WM_INITDIALOG: {
            SetDlgItemInt(hwndDlg, IDC_COLORMAP_COLOR_POSITION, g_ColorSetValue, true);
            auto edit = GetDlgItem(hwndDlg, IDC_COLORMAP_COLOR_POSITION);
            SetFocus(edit);
            SendDlgItemMessage(hwndDlg, IDC_COLORMAP_COLOR_POSITION, EM_SETSEL, 0, -1);
            return 0;
        }
        case WM_COMMAND:
            switch (HIWORD(wParam)) {
                case EN_CHANGE:
                    if (LOWORD(wParam) == IDC_COLORMAP_COLOR_POSITION) {
                        int success = false;
                        v = GetDlgItemInt(
                            hwndDlg, IDC_COLORMAP_COLOR_POSITION, &success, true);
                        if (success) {
                            g_ColorSetValue = CLAMP(v, 0, NUM_COLOR_VALUES - 1);
                        }
                    }
                    return 0;
                case BN_CLICKED:
                    EndDialog(hwndDlg, 0);
                    return 1;
            }
    }
    return 0;
}

static int win32_dlgproc_colormap_edit(HWND hwndDlg,
                                       UINT uMsg,
                                       WPARAM wParam,
                                       LPARAM lParam) {
    E_ColorMap* g_this = (E_ColorMap*)g_current_render;
    const Parameter& p_maps = g_this->info.parameters[6];

    const Parameter& p_colors = g_this->info.map_params[3];

    const Parameter& p_position = g_this->info.map_color_params[0];
    const Parameter& p_color = g_this->info.map_color_params[1];

    if (g_this == NULL) {
        return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
    }
    int current_map = SendDlgItemMessage(
        GetParent(hwndDlg), IDC_COLORMAP_MAP_SELECT, CB_GETCURSEL, 0, 0);
    if (current_map < 0) {
        current_map = 0;
    }
    int64_t num_maps = g_this->parameter_list_length(&p_maps);
    if (current_map >= num_maps) {
        current_map = num_maps - 1;
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
            for (unsigned int i = 0; i < NUM_COLOR_VALUES; i++) {
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
                int color = g_this->config.maps[current_map].baked_map[i];
                brush = CreateSolidBrush(RGB_TO_BGR(color));
                FillRect(map->context, &draw_rect, brush);
                DeleteObject(brush);
            }
            for (unsigned int i = 0; i < g_this->config.maps[current_map].colors.size();
                 i++) {
                unsigned int color = g_this->config.maps[current_map].colors[i].color;
                brush = CreateSolidBrush(RGB_TO_BGR(color));
                brush_sel = SelectObject(map->context, brush);
                bool is_selected = g_this->config.maps[current_map].colors[i].color_id
                                   == g_currently_selected_color_id;
                SelectObject(map->context,
                             GetStockObject(is_selected ? WHITE_PEN : BLACK_PEN));
                POINT triangle[3];
                triangle[0].x = g_this->config.maps[current_map].colors[i].position
                                * map_rect.right / 0xff;
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
            BitBlt(paint_context,
                   map_rect.left,
                   map_rect.top,
                   map_rect.right,
                   map_rect.bottom,
                   map->context,
                   0,
                   0,
                   SRCCOPY);
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
            if (y > draw_rect.bottom - 14) {
                for (unsigned int i = 0;
                     i < g_this->config.maps[current_map].colors.size();
                     i++) {
                    int distance = TRANSLATE_DISTANCE(
                        x,
                        draw_rect.left,
                        draw_rect.right,
                        g_this->config.maps[current_map].colors[i].position,
                        0,
                        NUM_COLOR_VALUES - 1);
                    if (abs(distance) < 6) {
                        g_currently_selected_color_id =
                            g_this->config.maps[current_map].colors[i].color_id;
                    }
                }
            }
            InvalidateRect(hwndDlg, 0, 0);
            SetCapture(hwndDlg);
            return 0;
        case WM_MOUSEMOVE:  // 0x0200
            if (GetCapture() == hwndDlg) {
                x = LOWORD(lParam);
                GetClientRect(hwndDlg, &draw_rect);
                draw_rect.top++;
                draw_rect.left++;
                draw_rect.right--;
                draw_rect.bottom--;
                x = CLAMP(x, draw_rect.left, draw_rect.right);
                for (unsigned int i = 0;
                     i < g_this->config.maps[current_map].colors.size();
                     i++) {
                    if (g_this->config.maps[current_map].colors[i].color_id
                        == g_currently_selected_color_id) {
                        g_this->set_int(p_position.handle,
                                        TRANSLATE_RANGE(x,
                                                        draw_rect.left,
                                                        draw_rect.right,
                                                        0,
                                                        NUM_COLOR_VALUES - 1),
                                        {current_map, i});
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
        case WM_RBUTTONDOWN:    // 0x0204
            x = LOWORD(lParam);
            y = HIWORD(lParam);
            GetClientRect(hwndDlg, &draw_rect);
            draw_rect.top++;
            draw_rect.left++;
            draw_rect.right--;
            draw_rect.bottom--;
            x = CLAMP(x, draw_rect.left, draw_rect.right);
            g_currently_selected_color_id = -1;
            if (y >= draw_rect.bottom - 14) {
                for (unsigned int i = 0;
                     i < g_this->config.maps[current_map].colors.size();
                     i++) {
                    int distance = TRANSLATE_DISTANCE(
                        x,
                        draw_rect.left,
                        draw_rect.right,
                        g_this->config.maps[current_map].colors[i].position,
                        0,
                        NUM_COLOR_VALUES - 1);
                    if (abs(distance) < 6) {
                        g_currently_selected_color_id =
                            g_this->config.maps[current_map].colors[i].color_id;
                    }
                }
            }
            selected_color_index = 0;
            for (unsigned int i = 0; i < g_this->config.maps[current_map].colors.size();
                 i++) {
                if (g_this->config.maps[current_map].colors[i].color_id
                    == g_currently_selected_color_id) {
                    selected_color_index = i;
                }
            }
            if (uMsg == WM_LBUTTONDBLCLK) {
                selected_menu_item = g_currently_selected_color_id != -1
                                         ? IDM_COLORMAP_MENU_EDIT
                                         : IDM_COLORMAP_MENU_ADD;
            } else {
                HMENU menu = CreatePopupMenu();
                AppendMenu(menu, MF_ENABLED, IDM_COLORMAP_MENU_ADD, "Add Color");
                if (g_currently_selected_color_id != -1) {
                    AppendMenu(menu, MF_ENABLED, IDM_COLORMAP_MENU_EDIT, "Edit Color");
                    if (g_this->config.maps[current_map].colors.size() > 1) {
                        AppendMenu(
                            menu, MF_ENABLED, IDM_COLORMAP_MENU_DELETE, "Delete Color");
                    }
                    AppendMenu(menu, MF_SEPARATOR, 0, NULL);
                    char set_position_str[19];
                    wsprintf(set_position_str,
                             "Set Position (%d)",
                             g_this->config.maps[current_map]
                                 .colors[selected_color_index]
                                 .position);
                    AppendMenu(
                        menu, MF_ENABLED, IDM_COLORMAP_MENU_SETPOS, set_position_str);
                }
                POINT popup_coords = {x + 1, y};
                ClientToScreen(hwndDlg, &popup_coords);
                selected_menu_item = TrackPopupMenuEx(menu,
                                                      TPM_RETURNCMD | TPM_RIGHTBUTTON,
                                                      popup_coords.x,
                                                      popup_coords.y,
                                                      hwndDlg,
                                                      NULL);
            }
            if (!selected_menu_item) {
                InvalidateRect(hwndDlg, NULL, 0);
                return 0;
            }
            int color_position = TRANSLATE_RANGE(
                x, draw_rect.left, draw_rect.right, 0, NUM_COLOR_VALUES - 1);
            static COLORREF custcolors[16];
            uint64_t color;
            switch (selected_menu_item) {
                case IDM_COLORMAP_MENU_ADD:
                    choosecolor.lStructSize = sizeof(CHOOSECOLOR);
                    choosecolor.hwndOwner = hwndDlg;
                    color = g_this->config.maps[current_map].baked_map[color_position];
                    choosecolor.rgbResult = RGB_TO_BGR(color);
                    choosecolor.lpCustColors = custcolors;
                    choosecolor.Flags = CC_RGBINIT | CC_FULLOPEN | CC_ANYCOLOR;
                    has_chosen_color = ChooseColor(&choosecolor);
                    if (has_chosen_color) {
                        AVS_Value position_value;
                        position_value.i = color_position;
                        AVS_Value color_value;
                        color_value.c = BGR_TO_RGB(choosecolor.rgbResult);
                        std::vector<AVS_Parameter_Value> new_color = {
                            {p_position.handle, position_value},
                            {p_color.handle, color_value},
                        };
                        g_this->parameter_list_entry_add(
                            &p_colors, -1, new_color, {current_map});
                        // c.config["Maps"][current_map]["Colors"].add_child(
                        //     -1,
                        //     {{"Position", position_value}, {"Color", color_value}});
                    }
                    break;
                case IDM_COLORMAP_MENU_EDIT:
                    choosecolor.lStructSize = sizeof(CHOOSECOLOR);
                    choosecolor.hwndOwner = hwndDlg;
                    choosecolor.rgbResult = RGB_TO_BGR(g_this->config.maps[current_map]
                                                           .colors[selected_color_index]
                                                           .color);
                    choosecolor.lpCustColors = custcolors;
                    choosecolor.Flags = CC_RGBINIT | CC_FULLOPEN | CC_ANYCOLOR;
                    has_chosen_color = ChooseColor(&choosecolor);
                    if (has_chosen_color) {
                        g_this->set_color(p_color.handle,
                                          BGR_TO_RGB(choosecolor.rgbResult),
                                          {current_map, selected_color_index});
                    }
                    break;
                case IDM_COLORMAP_MENU_DELETE:
                    for (int64_t i = 0;
                         i < g_this->config.maps[current_map].colors.size();
                         i++) {
                        if (g_this->config.maps[current_map].colors[i].color_id
                            == g_currently_selected_color_id) {
                            g_this->parameter_list_entry_remove(
                                &p_colors, i, {current_map});
                        }
                    }
                    break;
                case IDM_COLORMAP_MENU_SETPOS:
                    g_ColorSetValue = g_this->config.maps[current_map]
                                          .colors[selected_color_index]
                                          .position;
                    DialogBoxParam(g_hInstance,
                                   MAKEINTRESOURCE(IDD_CFG_COLORMAP_COLOR_POSITION),
                                   hwndDlg,
                                   (DLGPROC)win32_dlgproc_colormap_color_position,
                                   0);
                    g_this->set_int(p_position.handle,
                                    g_ColorSetValue,
                                    {current_map, selected_color_index});
                    break;
            }
            InvalidateRect(hwndDlg, NULL, 0);
            return 0;
    }
    return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
}

void win32_uiprep_colormap(HINSTANCE hInstance) {
    WNDCLASSEX mapview;
    mapview.cbSize = sizeof(WNDCLASSEX);
    if (GetClassInfoEx(hInstance, "ColormapEdit", &mapview)) {
        return;
    }
    mapview.style = CS_PARENTDC | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    mapview.lpfnWndProc = (WNDPROC)win32_dlgproc_colormap_edit;
    mapview.cbClsExtra = 0;
    mapview.cbWndExtra = 0;
    mapview.hInstance = hInstance;
    mapview.hIcon = NULL;
    mapview.hCursor = LoadCursor(0, IDC_ARROW);
    mapview.hbrBackground = NULL;
    mapview.lpszMenuName = NULL;
    mapview.lpszClassName = "ColormapEdit";
    RegisterClassEx(&mapview);
}

void save_map_file(E_ColorMap* colormap, int map_index, HWND hwndDlg) {
    std::string filename;
    char filename_c[MAX_PATH];
    std::string initial_dir;
    size_t filenamepos;
    OPENFILENAME openfilename;
    if (!colormap->config.maps[map_index].filepath.empty()) {
        // Note: Overflow `string::npos + 1 = 0` is by design.
        filenamepos = colormap->config.maps[map_index].filepath.rfind('\\') + 1;
        filename = colormap->config.maps[map_index].filepath.substr(filenamepos);
        initial_dir = colormap->config.maps[map_index].filepath.substr(0, filenamepos);
    } else {
        filename = "New Map.clm";
    }
    if (initial_dir.empty()) {
        initial_dir = g_path;
    }
    strncpy(filename_c, filename.c_str(), MAX_PATH - 1);
    filename_c[MAX_PATH - 1] = '\0';
    openfilename.lpstrInitialDir = initial_dir.c_str();
    openfilename.hwndOwner = NULL;
    openfilename.lpstrFileTitle = NULL;
    openfilename.nMaxFileTitle = 0;
    openfilename.lpstrFile = filename_c;
    openfilename.lStructSize = sizeof(OPENFILENAME);
    openfilename.nMaxFile = MAX_PATH;
    openfilename.lpstrFilter = "Color Map (*.clm)\0*.clm\0All Files\0*\0";
    openfilename.nFilterIndex = 1;
    openfilename.lpstrTitle = "Save Color Map";
    openfilename.Flags = OFN_EXPLORER;
    if (GetSaveFileName(&openfilename) == 0) {
        return;
    }
    std::string filepath = openfilename.lpstrFile;
    if (filepath.substr(filepath.length() - 4) != ".clm") {
        filepath += ".clm";
    }
    if (filepath.length() >= MAX_PATH) {
        MessageBox(hwndDlg, "Filename too long!", "Error", MB_ICONERROR);
        return;
    }
    colormap->config.maps[map_index].filepath = filepath;
    const Parameter& p_save_map = colormap->info.map_params[6];
    colormap->run_action(p_save_map.handle, {map_index});
    filenamepos = filepath.rfind('\\') + 1;
    filename = filepath.substr(filenamepos);
    SetDlgItemText(hwndDlg, IDC_COLORMAP_FILENAME_VIEW, filename.c_str());
}

void load_map_file(E_ColorMap* colormap, int map_index, HWND hwndDlg) {
    std::string filename;
    char filename_c[MAX_PATH];
    std::string initial_dir;
    size_t filenamepos;
    OPENFILENAME openfilename;
    if (!colormap->config.maps[map_index].filepath.empty()) {
        // Note: Overflow `string::npos + 1 = 0` is by design.
        filenamepos = colormap->config.maps[map_index].filepath.rfind('\\') + 1;
        filename = colormap->config.maps[map_index].filepath.substr(filenamepos);
        initial_dir = colormap->config.maps[map_index].filepath.substr(0, filenamepos);
    } else {
        filename = "New Map.clm";
    }
    if (initial_dir.empty()) {
        initial_dir = g_path;
    }
    strncpy(filename_c, filename.c_str(), MAX_PATH - 1);
    openfilename.lpstrInitialDir = initial_dir.c_str();
    openfilename.hwndOwner = NULL;
    openfilename.lpstrFileTitle = NULL;
    openfilename.nMaxFileTitle = 0;
    openfilename.lpstrFile = filename_c;
    openfilename.lStructSize = sizeof(OPENFILENAME);
    openfilename.nMaxFile = MAX_PATH;
    openfilename.lpstrFilter = "Color Map (*.clm)\0*.clm\0All Files\0*\0";
    openfilename.nFilterIndex = 1;
    openfilename.lpstrTitle = "Load Color Map";
    openfilename.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    if (GetOpenFileName(&openfilename) == 0) {
        return;
    }
    colormap->config.maps[map_index].filepath = openfilename.lpstrFile;
    const Parameter& p_load_map = colormap->info.map_params[7];
    colormap->run_action(p_load_map.handle, {map_index});
    InvalidateRect(GetDlgItem(hwndDlg, IDC_COLORMAP_MAPVIEW), NULL, 0);
    filenamepos = colormap->config.maps[map_index].filepath.rfind('\\') + 1;
    filename = colormap->config.maps[map_index].filepath.substr(filenamepos);
    SetDlgItemText(hwndDlg, IDC_COLORMAP_FILENAME_VIEW, filename.c_str());
}
