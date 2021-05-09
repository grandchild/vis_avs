/*

Reconstructed from scratch, with the help of Ghidra and the original binary colormap.ape
(version 1.3.0.0). Most of the code deals with handling the UI interactions, and the
actual mapping code is fairly straightforward. All the Win32 UI API calls are thankfully
plainly visible in the decompiled binary and for that reason most of the credit still
goes to the original author Steven Wittens (a.k.a. "Unconed") @ https://acko.net -- I
mostly just transcribed what Ghidra gave me.

*/
#include <windows.h>
#include <commctrl.h>
#include <time.h>
#include "r_defs.h"
#include "resource.h"
#include <xmmintrin.h>
#include <emmintrin.h>
#include <tmmintrin.h>

#include <stdio.h>


#define MOD_NAME "Trans / ColorMap"
#define UNIQUEIDSTRING "Color Map"

#define NUM_COLOR_VALUES 256  // 2 ^ BITS_PER_CHANNEL (i.e. 8)
#define COLORMAP_NUM_MAPS 8
#define COLORMAP_MAP_FILENAME_MAXLEN 48
#define COLORMAP_SAVE_MAP_HEADER_SIZE (sizeof(map) - sizeof(map_color*))

#define COLORMAP_COLOR_KEY_RED          0
#define COLORMAP_COLOR_KEY_GREEN        1
#define COLORMAP_COLOR_KEY_BLUE         2
#define COLORMAP_COLOR_KEY_RGB_SUM_HALF 3
#define COLORMAP_COLOR_KEY_MAX          4
#define COLORMAP_COLOR_KEY_RGB_AVERAGE  5

#define COLORMAP_MAP_CYCLE_NONE            0
#define COLORMAP_MAP_CYCLE_BEAT_RANDOM     1
#define COLORMAP_MAP_CYCLE_BEAT_SEQUENTIAL 2

#define COLORMAP_MAP_CYCLE_TIMER_ID 1
#define COLORMAP_MAP_CYCLE_SPEED_MIN 1
#define COLORMAP_MAP_CYCLE_SPEED_MAX 64
#define COLORMAP_MAP_CYCLE_ANIMATION_STEPS NUM_COLOR_VALUES

#define COLORMAP_BLENDMODE_REPLACE    0
#define COLORMAP_BLENDMODE_ADDITIVE   1
#define COLORMAP_BLENDMODE_MAXIMUM    2
#define COLORMAP_BLENDMODE_MINIMUM    3
#define COLORMAP_BLENDMODE_5050       4
#define COLORMAP_BLENDMODE_SUB1       5
#define COLORMAP_BLENDMODE_SUB2       6
#define COLORMAP_BLENDMODE_MULTIPLY   7
#define COLORMAP_BLENDMODE_XOR        8
#define COLORMAP_BLENDMODE_ADJUSTABLE 9

#define COLORMAP_ADJUSTABLE_BLEND_MAX 255

#define RGB_TO_BGR(color) (((color) & 0xff0000) >> 16 | ((color) & 0xff00) | ((color) & 0xff) << 16)
#define BGR_TO_RGB(color) RGB_TO_BGR(color)  // is its own inverse
#define CLAMP(x, from, to) ((x) >= (to) ? (to) : ((x) <= (from) ? (from) : (x)))
#define GET_INT() (data[pos] | (data[pos+1] << 8) | (data[pos+2] << 16) | (data[pos+3] << 24))
#define PUT_INT(y) data[pos] = (y) & 0xff;         \
                   data[pos+1] = ((y) >> 8) & 0xff;  \
                   data[pos+2] = ((y) >> 16) & 0xff; \
                   data[pos+3] = ((y) >> 24) & 0xff

#define COLORMAP_NUM_CYCLEMODES 3
static char colormap_labels_map_cycle_mode[COLORMAP_NUM_CYCLEMODES][19] = {
    "None (single map)",
    "On-beat random",
    "On-beat sequential"
};
#define COLORMAP_NUM_KEYMODES 6
static char colormap_labels_color_key[COLORMAP_NUM_KEYMODES][16] = {
    "Red Channel",
    "Green Channel",
    "Blue Channel",
    "(R+G+B)/2",
    "Maximal Channel",
    "(R+G+B)/3"
};
#define COLORMAP_NUM_BLENDMODES 10
static char colormap_labels_blendmodes[COLORMAP_NUM_BLENDMODES][14] = {
    "Replace",
    "Additive",
    "Maximum",
    "Minimum",
    "50/50",
    "Subtractive 1",
    "Subtractive 2",
    "Multiply",
    "XOR",
    "Adjustable"
};

typedef struct {
    unsigned int position;
    unsigned int color;
    unsigned int color_id;
} map_color;

typedef struct {
    int enabled;
    unsigned int length;
    int map_id;
    char filename[COLORMAP_MAP_FILENAME_MAXLEN];
    map_color* colors;
} map;

typedef struct {
    unsigned int color_key;
    unsigned int blendmode;
    unsigned int map_cycle_mode;
    unsigned char adjustable_alpha;
    unsigned char _unused;
    unsigned char dont_skip_fast_beats;
    unsigned char map_cycle_speed; // 1 to 64
} colormap_apeconfig;

typedef struct {
    int colors[NUM_COLOR_VALUES];
} map_cache;

typedef struct {
    RECT window_rect;
    HDC context;
    HBITMAP bitmap;
    LPRECT region;
} ui_map;

class C_ColorMap : public C_RBASE {
    public:
        C_ColorMap();
        virtual ~C_ColorMap();

        /* APE interface */
        virtual int render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int);
        virtual HWND conf(HINSTANCE hInstance, HWND hwndParent);
        virtual char *get_desc();
        virtual void load_config(unsigned char *data, int len);
        virtual int  save_config(unsigned char *data);

        /* Other utilities */
        void load_map_file(int map_index);
        void save_map_file(int map_index);
        void make_default_map(int map_index);
        void make_map_cache(int map_index);
        void add_map_color(int map_index, int position, int color);
        void remove_map_color(int map_index, int index);
        void sort_colors(int map_index);

        colormap_apeconfig config;
        map maps[COLORMAP_NUM_MAPS];
        map_cache map_caches[COLORMAP_NUM_MAPS];
        map_cache tween_map_cache;
        int current_map = 0;
        int next_map = 0;
        HWND dialog = NULL;
        int change_animation_step;
        int disable_map_change;

    protected:
        int next_id = 1337;
        int get_new_id();
        int get_key(int color);
        int get_key_ssse3(__m128i color4);
        void animation_step();
        bool load_map_header(unsigned char *data, int len, int map_index, int pos);
        bool load_map_colors(unsigned char *data, int len, int map_index, int pos);
};

static C_ColorMap* g_ColormapThis;
extern HINSTANCE g_hInstance;
static int g_ColorSetValue;
static int g_currently_selected_color_id = -1;

int compare_colors(const void* color1, const void* color2) {
    int diff = ((map_color*)color1)->position - ((map_color*)color2)->position;
    if (diff == 0) {
        // TODO [bugfix,feature]: If color positions are the same, he brighter color
        // gets sorted higher. This is somewhat arbitrary. We should try and sort by
        // previous position, so that when dragging a color, it does not flip until its
        // position actually becomes less than the other.
        diff = ((map_color*)color1)->color - ((map_color*)color2)->color;
    }
    return diff;
}

static LRESULT CALLBACK dialog_handler(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam) {
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
                        MessageBox(hwndDlg, "", "", MB_ICONINFORMATION);
                        return 0;
                    case IDC_COLORMAP_CYCLE_INFO:
                        MessageBox(hwndDlg, "", "", MB_ICONINFORMATION);
                        return 0;
                    case IDC_COLORMAP_FILE_LOAD:
                        g_ColormapThis->load_map_file(current_map);
                        return 0;
                    case IDC_COLORMAP_FILE_SAVE:
                        g_ColormapThis->save_map_file(current_map);
                        return 0;
                    case IDC_COLORMAP_MAP_ENABLE:
                        g_ColormapThis->maps[current_map].enabled = IsDlgButtonChecked(hwndDlg, IDC_COLORMAP_MAP_ENABLE) == BST_CHECKED;
                        return 0;
                    case IDC_COLORMAP_FLIP_MAP:
                        if (g_ColormapThis->maps[current_map].length > 0) {
                            for(int i=0; i<g_ColormapThis->maps[current_map].length; i++) {
                                map_color color = g_ColormapThis->maps[current_map].colors[i];
                                color.position = NUM_COLOR_VALUES - 1 - color.position;
                            }
                        }
                        g_ColormapThis->make_map_cache(current_map);
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
                        g_ColormapThis->config.dont_skip_fast_beats = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
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
                for(int i=0; i< COLORMAP_NUM_MAPS; i++) {
                    char map_dropdown_name_buf[6] = "Map X";
                    map_dropdown_name_buf[4] = (char)i + '1';
                    SendDlgItemMessage(hwndDlg, IDC_COLORMAP_MAP_SELECT, CB_ADDSTRING, 0, (LPARAM)map_dropdown_name_buf);
                }
                SendDlgItemMessage(hwndDlg, IDC_COLORMAP_MAP_SELECT, CB_SETCURSEL, g_ColormapThis->current_map, 0);
                CheckDlgButton(hwndDlg, IDC_COLORMAP_MAP_ENABLE, g_ColormapThis->maps[g_ColormapThis->current_map].enabled != 0);
                for(int i=0; i < COLORMAP_NUM_CYCLEMODES; i++) {
                    SendDlgItemMessage(hwndDlg, IDC_COLORMAP_MAP_CYCLING_SELECT, CB_ADDSTRING, NULL, (LPARAM)colormap_labels_map_cycle_mode[i]);
                }
                SendDlgItemMessage(hwndDlg, IDC_COLORMAP_MAP_CYCLING_SELECT, CB_SETCURSEL, g_ColormapThis->config.map_cycle_mode, NULL);
                for(int i=0; i < COLORMAP_NUM_KEYMODES; i++) {
                    SendDlgItemMessage(hwndDlg, IDC_COLORMAP_KEY_SELECT, CB_ADDSTRING, NULL, (LPARAM)colormap_labels_color_key[i]);
                }
                SendDlgItemMessage(hwndDlg, IDC_COLORMAP_KEY_SELECT, CB_SETCURSEL, g_ColormapThis->config.color_key, NULL);
                for(int i=0; i < COLORMAP_NUM_BLENDMODES; i++) {
                    SendDlgItemMessage(hwndDlg, IDC_COLORMAP_OUT_BLENDMODE, CB_ADDSTRING, NULL, (LPARAM)colormap_labels_blendmodes[i]);
                }
                SendDlgItemMessage(hwndDlg, IDC_COLORMAP_OUT_BLENDMODE, CB_SETCURSEL, g_ColormapThis->config.blendmode, NULL);
                CheckDlgButton(hwndDlg, IDC_COLORMAP_NO_SKIP_FAST_BEATS, g_ColormapThis->config.dont_skip_fast_beats != 0);
                EnableWindow(GetDlgItem(hwndDlg, IDC_COLORMAP_ADJUSTABLE_SLIDER),
                             g_ColormapThis->config.blendmode == COLORMAP_BLENDMODE_ADJUSTABLE);
                SendDlgItemMessage(hwndDlg, IDC_COLORMAP_ADJUSTABLE_SLIDER, TBM_SETRANGE, TRUE, MAKELONG(0, COLORMAP_ADJUSTABLE_BLEND_MAX));
                SendDlgItemMessage(hwndDlg, IDC_COLORMAP_ADJUSTABLE_SLIDER, TBM_SETPOS, TRUE, g_ColormapThis->config.adjustable_alpha);
                
                // This overrides the setting of the current map above. bug? quickfix?
                SendDlgItemMessage(hwndDlg, IDC_COLORMAP_MAP_SELECT, CB_SETCURSEL, 0, NULL);  // TODO[feature]: This could be more convenient by saving and restoring the last-selected map.
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
                    int adjustable_pos = SendDlgItemMessage(hwndDlg, IDC_COLORMAP_ADJUSTABLE_SLIDER, TBM_GETPOS, NULL, NULL);
                    g_ColormapThis->config.adjustable_alpha = CLAMP(adjustable_pos, 0, COLORMAP_ADJUSTABLE_BLEND_MAX);
                } else if((HWND)lParam == GetDlgItem(hwndDlg, IDC_COLORMAP_MAP_CYCLE_SPEED)) {
                    int cycle_speed_pos = SendDlgItemMessage(hwndDlg, IDC_COLORMAP_MAP_CYCLE_SPEED, TBM_GETPOS, NULL, NULL);
                    g_ColormapThis->config.map_cycle_speed = CLAMP(cycle_speed_pos, COLORMAP_MAP_CYCLE_SPEED_MIN, COLORMAP_MAP_CYCLE_SPEED_MAX);
                }
            }
            return 0;
    }
    return 0;
}

static LRESULT CALLBACK set_color_position_handler(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam) {
    int v;
    HWND hWnd;
    #define VALUE_BUF_LEN 64
    char value_buf[VALUE_BUF_LEN];

    switch(uMsg) {
        case WM_CLOSE:
            EndDialog(hwndDlg, 0);
            return 0;
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
                        g_ColorSetValue = CLAMP(v, 0, NUM_COLOR_VALUES);
                    }
                    return 0;
                case BN_CLICKED:
                    EndDialog(hwndDlg,0);
                    return 0;
            }
    }
    return 1;
}

static LRESULT CALLBACK colormap_edit_handler(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    printf("> Colormap: palette dialog 0x%x\n", uMsg);
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
    
    int x, y;
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
            for(int i = 0; i < NUM_COLOR_VALUES; i++) {
                int w = map_rect.right - map_rect.left;
                // The disassembled code from the original reads:
                //   CDQ  (i * w)              ; all bits in edx are x's sign-bit
                //   AND  edx, 0xff            ; offset = (i * w) < 0 ? 255 : 0
                //   ADD  eax, edx             ; i * w + offset
                //   SAR  eax, 8               ; / 256
                //   ADD  eax, map_rect.left
                //   MOV  draw_rect.left, eax
                // This seems like some sort of weird branchless negative modulo catch
                // for negative x's (but with an off-by-one?), which seems unneccessary
                // unless you'd expect (map_rect.right - map_rect.left) to be negative.
                // The following should be sufficient:
                draw_rect.left = i * w / NUM_COLOR_VALUES + map_rect.left;
                draw_rect.right = (i + 1) * w / NUM_COLOR_VALUES + map_rect.left;
                int color = g_ColormapThis->map_caches[current_map].colors[i];
                brush = CreateSolidBrush(RGB_TO_BGR(color));
                FillRect(map->context, &draw_rect, brush);
                DeleteObject(brush);
            }
            for(int i = 0; i < g_ColormapThis->maps[current_map].length; i++) {
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
                for(int i = 0; i < g_ColormapThis->maps[current_map].length; i++) {
                    map_color color = g_ColormapThis->maps[current_map].colors[i];
                    int distance = (draw_rect.right * color.position) / NUM_COLOR_VALUES - (x - draw_rect.left);
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
                for(int i = 0; i < g_ColormapThis->maps[current_map].length; i++) {
                    map_color* color = &g_ColormapThis->maps[current_map].colors[i];
                    if(color->color_id == g_currently_selected_color_id) {
                        color->position = (x - draw_rect.left) * NUM_COLOR_VALUES / (draw_rect.right - draw_rect.left);
                        g_ColormapThis->sort_colors(current_map);
                        
                    }
                }
                g_ColormapThis->make_map_cache(current_map);
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
                for(int i = 0; i < g_ColormapThis->maps[current_map].length; i++) {
                    map_color color = g_ColormapThis->maps[current_map].colors[i];
                    int distance = ((draw_rect.right * color.position) / NUM_COLOR_VALUES - x) + draw_rect.left;
                    if(abs(distance) < 6) {
                        g_currently_selected_color_id = color.color_id;
                    }
                }
            }
            selected_color_index = 0;
            for(int i = 0; i < g_ColormapThis->maps[current_map].length; i++) {
                if(g_ColormapThis->maps[current_map].colors[i].color_id == g_currently_selected_color_id) {
                    selected_color_index = i;
                }
            }
            if(uMsg == WM_LBUTTONDBLCLK) {
                selected_menu_item = g_currently_selected_color_id != -1 ?
                                     IDM_COLORMAP_MENU_EDIT : IDM_COLORMAP_MENU_ADD;
            } else {
                HMENU menu = CreatePopupMenu();
                AppendMenu(menu, NULL, IDM_COLORMAP_MENU_ADD, "Add Color");
                if (g_currently_selected_color_id != -1) {
                    AppendMenu(menu, NULL, IDM_COLORMAP_MENU_EDIT, "Edit Color");
                    if (g_ColormapThis->maps[current_map].length > 1) {
                        AppendMenu(menu, NULL, IDM_COLORMAP_MENU_DELETE, "Delete Color");
                    }
                    AppendMenu(menu, MF_SEPARATOR, NULL, NULL);
                    char set_position_str[19];
                    wsprintf(set_position_str, "Set Position (%d)",
                             g_ColormapThis->maps[current_map].colors[selected_color_index].position);
                    AppendMenu(menu, NULL, IDM_COLORMAP_MENU_SETPOS, set_position_str);
                }
                POINT popup_coords = {x + 1, y};
                ClientToScreen(hwndDlg, &popup_coords);
                selected_menu_item = TrackPopupMenuEx(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, popup_coords.x, popup_coords.y, hwndDlg, NULL);
            }
            if(selected_menu_item != NULL) {
                int color_position = (x - draw_rect.left) * NUM_COLOR_VALUES / (draw_rect.right - draw_rect.left);
                static COLORREF custcolors[16];
                switch(selected_menu_item) {
                    case IDM_COLORMAP_MENU_ADD:
                        choosecolor.lStructSize = sizeof(CHOOSECOLOR);
                        choosecolor.hwndOwner = hwndDlg;
                        choosecolor.rgbResult = RGB_TO_BGR(g_ColormapThis->map_caches[current_map].colors[color_position]);
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
                        DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_CFG_COLORMAP_COLOR_POSITION), hwndDlg, (DLGPROC)set_color_position_handler, NULL);
                        g_ColormapThis->maps[current_map].colors[selected_color_index].position = g_ColorSetValue;
                        break;
                }
                g_ColormapThis->make_map_cache(current_map);
            }
            InvalidateRect(hwndDlg, NULL, 0);
            return 0;
    }
    return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
}

C_ColorMap::C_ColorMap() {
    for(int i = 0; i < COLORMAP_NUM_MAPS; i++) {
        this->maps[i].enabled = i == 0;
        this->maps[i].length = 2;
        this->maps[i].map_id = this->get_new_id();
        this->make_default_map(i);
    }
}

C_ColorMap::~C_ColorMap() {
    for(int i = 0; i < COLORMAP_NUM_MAPS; i++) {
        delete[] this->maps[i].colors;
    }
}

int C_ColorMap::get_new_id() {
    return this->next_id++;
}

void C_ColorMap::make_default_map(int map_index) {
    delete[] this->maps[map_index].colors;
    this->maps[map_index].colors = new map_color[2];
    this->maps[map_index].colors[0].position = 0;
    this->maps[map_index].colors[0].color = 0x000000;
    this->maps[map_index].colors[0].color_id = this->get_new_id();
    this->maps[map_index].colors[1].position = 255;
    this->maps[map_index].colors[1].color = 0xffffff;
    this->maps[map_index].colors[1].color_id = this->get_new_id();
    this->make_map_cache(map_index);
}

void C_ColorMap::make_map_cache(int map_index) {
    int cache_index = 0, color_index = 0;
    if(this->maps[map_index].length <= 1) {
        return;
    }
    map_color first = this->maps[map_index].colors[0];
    for(; cache_index < first.position; cache_index++) {
        this->map_caches[map_index].colors[cache_index] = first.color;
    }
    for(; color_index < this->maps[map_index].length - 1; color_index++) {
        map_color from = this->maps[map_index].colors[color_index];
        map_color to = this->maps[map_index].colors[color_index + 1];
        for(; cache_index < to.position; cache_index++) {
            int rel_i = cache_index - from.position;
            int w = to.position - from.position;
            int lerp = NUM_COLOR_VALUES * (float)(cache_index - from.position) / (float)((to.position - from.position));
            this->map_caches[map_index].colors[cache_index] = BLEND_ADJ_NOMMX(to.color, from.color, lerp);
        }
    }
    map_color last = this->maps[map_index].colors[color_index];
    for(; cache_index < NUM_COLOR_VALUES; cache_index++) {
        this->map_caches[map_index].colors[cache_index] = last.color;
    }
}

void C_ColorMap::add_map_color(int map_index, int position, int color) {
    if(this->maps[map_index].length >= NUM_COLOR_VALUES) {
        return;
    }
    map_color* new_map_colors = new map_color[this->maps[map_index].length + 1];
    for(int i=0, a=0; i < this->maps[map_index].length + 1; i++) {
        if(a == 0 && position >= this->maps[map_index].colors[i].position) {
            new_map_colors[i].position = position;
            new_map_colors[i].color = color;
            new_map_colors[i].color_id = this->get_new_id();
            a = 1;
        }
        new_map_colors[i + a] = this->maps[map_index].colors[i];
    }
    map_color* old_map_colors = this->maps[map_index].colors;
    this->maps[map_index].colors = new_map_colors;
    this->maps[map_index].length += 1;
    this->sort_colors(map_index);
    delete[] old_map_colors;
}

void C_ColorMap::remove_map_color(int map_index, int index) {
    if(this->maps[map_index].length <= 1) {
        return;
    }
    map_color* new_map_colors = new map_color[this->maps[map_index].length];
    for(int i=0, a=0; i < this->maps[map_index].length + 1; i++) {
        if(i == index) {
            a = 1;
        }
        new_map_colors[i] = this->maps[map_index].colors[i + a];
    }
    map_color* old_map_colors = this->maps[map_index].colors;
    this->maps[map_index].length -= 1;
    this->maps[map_index].colors = new_map_colors;
    this->sort_colors(map_index);
    delete[] old_map_colors;
}

void C_ColorMap::sort_colors(int map_index) {
    qsort(this->maps[map_index].colors, this->maps[map_index].length, sizeof(map_color), compare_colors);
}

// TODO [performance]: Maybe redirecting a function pointer on-frame, instead of
// branching according to the key config on every pixel could increase performance? Is a
// 6-fold switch-case slower than a function call?
inline int C_ColorMap::get_key(int color) {
    int r, g, b;
    switch(this->config.color_key) {
        case COLORMAP_COLOR_KEY_RED:
            return color & 0xff;
        case COLORMAP_COLOR_KEY_GREEN:
            return color >> 8 & 0xff;
        case COLORMAP_COLOR_KEY_BLUE:
            return color >> 16 & 0xff;
        case COLORMAP_COLOR_KEY_RGB_SUM_HALF:
            return ((color & 0xff) + (color >> 8 & 0xff) + (color >> 16 & 0xff)) / 2;
        case COLORMAP_COLOR_KEY_MAX:
            r = color & 0xff;
            g = (color & 0xff00) >> 8;
            b = (color & 0xff0000) >> 16;
            return r > g ? (r > b ? r : b) : (g > b ? g : b);
        default:  // COLORMAP_COLOR_KEY_RGB_AVERAGE:
            return ((color & 0xff) + (color >> 8 & 0xff00) + (color >> 16 & 0xff)) / 3;
    }
}

inline int C_ColorMap::get_key_ssse3(__m128i color4) {
    int output;
    int avg_tmp[4];
    // Gather uint8s from the certain source locations. (0xff => dest will be zero.)
    // Collect the respective channels into the lower 32bits, so they can be returned as
    // a single 32bit (4-packed-uint8) int value.
    __m128i gather_red_into_p8 =    _mm_set_epi32(0xffffffff, 0xffffffff, 0xffffffff, 0x0c080400);
    __m128i gather_green_into_p8 =  _mm_set_epi32(0xffffffff, 0xffffffff, 0xffffffff, 0x0d090501);
    __m128i gather_blue_into_p8 =   _mm_set_epi32(0xffffffff, 0xffffffff, 0xffffffff, 0x0e0a0602);
    // For the (r+g+b)/2 and /3 operations we need overflow capabilities.
    __m128i gather_red_into_p16 =   _mm_set_epi32(0xffffffff, 0xffffffff, 0xff0cff08, 0xff04ff00);
    __m128i gather_green_into_p16 = _mm_set_epi32(0xffffffff, 0xffffffff, 0xff0dff09, 0xff05ff01);
    __m128i gather_blue_into_p16 =  _mm_set_epi32(0xffffffff, 0xffffffff, 0xff0eff0a, 0xff06ff02);
    __m128i max_channel_value_p16 = _mm_set_epi32(0xffffffff, 0xffffffff, 0x00ff00ff, 0x00ff00ff);
    __m128i gather_p16_into_p8 = _mm_set_epi32(0xffffffff, 0xffffffff, 0xffffffff, 0x06040200);
    __m128i r, g;
    switch(this->config.color_key) {
        case COLORMAP_COLOR_KEY_RED:
            color4 = _mm_shuffle_epi8(color4, gather_red_into_p8);
            break;
        case COLORMAP_COLOR_KEY_GREEN:
            color4 = _mm_shuffle_epi8(color4, gather_green_into_p8);
            break;
        case COLORMAP_COLOR_KEY_BLUE:
            color4 = _mm_shuffle_epi8(color4, gather_blue_into_p8);
            break;
        case COLORMAP_COLOR_KEY_RGB_SUM_HALF:
            // ((color & 0xff) + ((color & 0xff00) >> 8) + ((color & 0xff0000) >> 16)) / 2;
            r = _mm_shuffle_epi8(color4, gather_red_into_p16);
            g = _mm_shuffle_epi8(color4, gather_green_into_p16);
            color4 = _mm_shuffle_epi8(color4, gather_blue_into_p16);
            color4 = _mm_add_epi16(color4, r);
            // Correct average, round up on odd sum, i.e.: (a+b+c + 1) >> 1:
            //color4 = _mm_avg_epi16(color4, g);
            // Original Colormap behavior, round down on .5, i.e. (a+b+c) >> 1:
            color4 = _mm_add_epi16(color4, g);
            color4 = _mm_srli_epi16(color4, 1);
            color4 = _mm_min_epi16(color4, max_channel_value_p16);
            color4 = _mm_shuffle_epi8(color4, gather_p16_into_p8);
            break;
        case COLORMAP_COLOR_KEY_MAX:
            r = _mm_shuffle_epi8(color4, gather_red_into_p8);
            g = _mm_shuffle_epi8(color4, gather_green_into_p8);
            color4 = _mm_shuffle_epi8(color4, gather_blue_into_p8);
            color4 = _mm_max_epu8(color4, r);
            color4 = _mm_max_epu8(color4, g);
            break;
        default:  // COLORMAP_COLOR_KEY_RGB_AVERAGE:
            r = _mm_shuffle_epi8(color4, gather_red_into_p16);
            g = _mm_shuffle_epi8(color4, gather_green_into_p16);
            color4 = _mm_shuffle_epi8(color4, gather_blue_into_p16);
            color4 = _mm_add_epi16(color4, r);
            color4 = _mm_avg_epi16(color4, g);
            // ((color & 0xff) + ((color & 0xff00) >> 8) + ((color & 0xff0000) >> 16)) / 3;
            _mm_storeu_si32();
            
            return output;
    }
    _mm_storeu_si32(&output, color4);
    return output;
}

void C_ColorMap::animation_step() {
    return;
}

int C_ColorMap::render(char visdata[2][2][576], int is_beat, int *framebuffer, int *fbout, int w, int h) {
    map_cache* blend_map_cache;
    this->next_map %= COLORMAP_NUM_MAPS;
    if(this->config.map_cycle_mode == COLORMAP_MAP_CYCLE_NONE || this->disable_map_change) {
        this->change_animation_step = 0;
        blend_map_cache = &this->map_caches[this->current_map];
    } else {
        this->change_animation_step = max(this->change_animation_step + this->config.map_cycle_speed, COLORMAP_MAP_CYCLE_ANIMATION_STEPS);
        if(is_beat && (!this->config.dont_skip_fast_beats || this->change_animation_step == COLORMAP_MAP_CYCLE_ANIMATION_STEPS)) {
            int i;
            bool any_maps_enabled = false;
            for(i = 0; i < COLORMAP_NUM_MAPS; i++) {
                if(this->maps[i].enabled) {
                    any_maps_enabled = true;
                    break;
                }
            }
            while(any_maps_enabled && !this->maps[this->next_map].enabled) {
                if(this->config.map_cycle_mode == COLORMAP_MAP_CYCLE_BEAT_RANDOM) {
                    this->next_map = rand() % COLORMAP_NUM_MAPS;
                } else{
                    this->next_map = ++this->next_map % COLORMAP_NUM_MAPS;
                }
            }
            this->change_animation_step = 0;
        }
        if(this->change_animation_step == COLORMAP_MAP_CYCLE_ANIMATION_STEPS) {
            this->current_map = this->next_map;
        } else {
            if(this->change_animation_step == 0) {
                memcpy(&this->tween_map_cache, &this->map_caches[this->current_map], sizeof(map_cache));
                blend_map_cache = &this->tween_map_cache;
            } else {
                if(this->current_map != this->next_map) {
                    this->animation_step();
                } else {
                    memcpy(&this->tween_map_cache, &this->map_caches[this->current_map], sizeof(map_cache));
                    blend_map_cache = &this->tween_map_cache;
                }
            }
        }
    }
    switch(this->config.blendmode) {
        case COLORMAP_BLENDMODE_REPLACE:
            for(int i = 0; i < w * h; i++) {
                int key = this->get_key(framebuffer[i]);
                framebuffer[i] = this->map_caches[this->current_map].colors[key];
            }
            break;
        case COLORMAP_BLENDMODE_ADDITIVE:
            for(int i = 0; i < w * h; i++) {
                int key = this->get_key(framebuffer[i]);
                framebuffer[i] = this->map_caches[this->current_map].colors[key];
            }
            break;
        case COLORMAP_BLENDMODE_MAXIMUM:
            for(int i = 0; i < w * h; i++) {
                int key = this->get_key(framebuffer[i]);
                framebuffer[i] = this->map_caches[this->current_map].colors[key];
            }
            break;
        case COLORMAP_BLENDMODE_MINIMUM:
            for(int i = 0; i < w * h; i++) {
                int key = this->get_key(framebuffer[i]);
                framebuffer[i] = this->map_caches[this->current_map].colors[key];
            }
            break;
        case COLORMAP_BLENDMODE_5050:
            for(int i = 0; i < w * h; i++) {
                int key = this->get_key(framebuffer[i]);
                framebuffer[i] = this->map_caches[this->current_map].colors[key];
            }
            break;
        case COLORMAP_BLENDMODE_SUB1:
            for(int i = 0; i < w * h; i++) {
                int key = this->get_key(framebuffer[i]);
                framebuffer[i] = this->map_caches[this->current_map].colors[key];
            }
            break;
        case COLORMAP_BLENDMODE_SUB2:
            for(int i = 0; i < w * h; i++) {
                int key = this->get_key(framebuffer[i]);
                framebuffer[i] = this->map_caches[this->current_map].colors[key];
            }
            break;
        case COLORMAP_BLENDMODE_MULTIPLY:
            for(int i = 0; i < w * h; i++) {
                int key = this->get_key(framebuffer[i]);
                framebuffer[i] = this->map_caches[this->current_map].colors[key];
            }
            break;
        case COLORMAP_BLENDMODE_XOR:
            for(int i = 0; i < w * h; i++) {
                int key = this->get_key(framebuffer[i]);
                framebuffer[i] = this->map_caches[this->current_map].colors[key];
            }
            break;
        case COLORMAP_BLENDMODE_ADJUSTABLE:
            for(int i = 0; i < w * h; i++) {
                int key = this->get_key(framebuffer[i]);
                framebuffer[i] = this->map_caches[this->current_map].colors[key];
            }
            break;
        
    }
    return 0;
}

HWND C_ColorMap::conf(HINSTANCE hInstance, HWND hwndParent) {
    WNDCLASSEX mapview;
    mapview.cbSize = sizeof(WNDCLASSEX);
    GetClassInfoEx(hInstance, "ColormapEdit", &mapview);
    mapview.style = CS_PARENTDC | CS_HREDRAW | CS_VREDRAW;
    mapview.lpfnWndProc = (WNDPROC)colormap_edit_handler;
    mapview.cbClsExtra = 0;
    mapview.cbWndExtra = 0;
    mapview.hInstance = hInstance;
    mapview.hCursor = LoadCursor(0, MAKEINTRESOURCE(IDC_ARROW));
    mapview.lpszClassName = "ColormapEdit";
    if(!RegisterClassEx(&mapview)) {
        printf("RegisterClassEx failed with %ld\n", GetLastError());
    }
    g_ColormapThis = this;
    return CreateDialog(hInstance, MAKEINTRESOURCE(IDD_CFG_COLORMAP), hwndParent, (DLGPROC)dialog_handler);
}

char *C_ColorMap::get_desc(void) {
    return MOD_NAME;
}

bool C_ColorMap::load_map_header(unsigned char *data, int len, int map_index, int pos) {
    if(len - pos < COLORMAP_SAVE_MAP_HEADER_SIZE) {
        printf("len: %d, pos: %d\n", len, pos);
        return false;
    }
    int start = pos;
    this->maps[map_index].enabled = GET_INT();
    pos += 4;
    this->maps[map_index].length = GET_INT();
    pos += 4;
    this->maps[map_index].map_id = GET_INT();
    pos += 4;
    strncpy(this->maps[map_index].filename, (char*)&data[pos], COLORMAP_MAP_FILENAME_MAXLEN - 1);
    this->maps[map_index].filename[COLORMAP_MAP_FILENAME_MAXLEN - 1] = '\0';
    return true;
}

bool C_ColorMap::load_map_colors(unsigned char *data, int len, int map_index, int pos) {
    delete[] this->maps[map_index].colors;
    this->maps[map_index].colors = new map_color[this->maps[map_index].length];
    int i;
    for(i = 0; i < this->maps[map_index].length; i++) {
        if(len - pos < sizeof(map_color)) {
            return false;
        }
        this->maps[map_index].colors[i].position = GET_INT();
        pos += 4;
        this->maps[map_index].colors[i].color = GET_INT();
        pos += 4;
        this->maps[map_index].colors[i].color_id = GET_INT();
        pos += 4;
    }
    this->sort_colors(map_index);
    return true;
}

void C_ColorMap::load_config(unsigned char *data, int len) {
    if (len >= sizeof(colormap_apeconfig))
        memcpy(&this->config, data, sizeof(colormap_apeconfig));

    bool success = true;
    unsigned int pos = sizeof(colormap_apeconfig);
    for(int map_index = 0; map_index < COLORMAP_NUM_MAPS; map_index++) {
        success = this->load_map_header(data, len, map_index, pos);
        if(!success) {
            printf("Saved Colormap config header is incomplete.\n");
            break;
        }
        pos += COLORMAP_SAVE_MAP_HEADER_SIZE;
    }
    for(int map_index = 0; map_index < COLORMAP_NUM_MAPS; map_index++) {
        success = this->load_map_colors(data, len, map_index, pos);
        if(!success) {
            printf("Saved Colormap config color list is incomplete.\n");
            break;
        }
        pos += this->maps[map_index].length * sizeof(map_color);
    }
    for(int map_index = 0; map_index < COLORMAP_NUM_MAPS; map_index++) {
        this->make_map_cache(map_index);
    }
}

int C_ColorMap::save_config(unsigned char *data) {
    memcpy(data, &this->config, sizeof(colormap_apeconfig));
    int pos = sizeof(colormap_apeconfig);

    for(int map_index = 0; map_index < COLORMAP_NUM_MAPS; map_index++) {
        PUT_INT(this->maps[map_index].enabled);
        pos += 4;
        PUT_INT(this->maps[map_index].length);
        pos += 4;
        PUT_INT(this->maps[map_index].map_id);
        pos += 4;
        memset((char*)&data[pos], 0, COLORMAP_MAP_FILENAME_MAXLEN);
        strncpy((char*)&data[pos], this->maps[map_index].filename, COLORMAP_MAP_FILENAME_MAXLEN - 1);
        pos += COLORMAP_MAP_FILENAME_MAXLEN;
    }
    for(int map_index = 0; map_index < COLORMAP_NUM_MAPS; map_index++) {
        for(int i=0; i < this->maps[map_index].length; i++) {
            PUT_INT(this->maps[map_index].colors[i].position);
            pos += 4;
            PUT_INT(this->maps[map_index].colors[i].color);
            pos += 4;
            PUT_INT(this->maps[map_index].colors[i].color_id);
            pos += 4;
        }
    }
    return pos;
}

void C_ColorMap::save_map_file(int map_index) {}
void C_ColorMap::load_map_file(int map_index) {}

C_RBASE *R_ColorMap(char *desc) {
    srand(time(NULL));
    if (desc) {
        strcpy(desc, MOD_NAME);
        return NULL;
    }
    return (C_RBASE *) new C_ColorMap();
}
