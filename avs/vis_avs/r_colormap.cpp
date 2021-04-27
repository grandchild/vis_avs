#include <windows.h>
#include "r_defs.h"
#include "resource.h"
#include <xmmintrin.h>
#include <emmintrin.h>


#define MOD_NAME "Trans / ColorMap"
#define UNIQUEIDSTRING "Color Map"

#define NUM_COLOR_VALUES 256  // 2 ^ BITS_PER_CHANNEL (i.e. 8)
#define COLORMAP_NUM_MAPS 8
#define COLORMAP_MAP_FILENAME_MAXLEN 48

#define COLORMAP_COLOR_KEY_RED 1
#define COLORMAP_COLOR_KEY_GREEN 2
#define COLORMAP_COLOR_KEY_BLUE 3
#define COLORMAP_COLOR_KEY_RGB_SUM_HALF 4
#define COLORMAP_COLOR_KEY_MAX 5
#define COLORMAP_COLOR_KEY_RGB_AVERAGE 6

#define COLORMAP_MAP_SWITCH_SINGLE 1
#define COLORMAP_MAP_SWITCH_BEAT_RANDOM 2
#define COLORMAP_MAP_SWITCH_BEAT_SEQUENTIAL 3


typedef struct {
    unsigned int position;
    unsigned int color;
    unsigned int color_id;
} map_color;

typedef struct {
    int enabled;
    unsigned int num_colors;
    int map_id;
    char filename[COLORMAP_MAP_FILENAME_MAXLEN];
    map_color* colors;
} map;

typedef struct {
    unsigned int color_key;
    unsigned int blend_mode;
    unsigned int map_cycle_mode;
    unsigned char adjustable_alpha;
    unsigned char _unused;
    unsigned char dontSkipFastBeats;
    unsigned char cycleSpeed; // 1 to 64
} colormap_apeconfig;

typedef struct {
    RECT window_rect;
    HDC context;
    HBITMAP bitmap;
    RECT region;
} ui_map;

class C_ColorMap : public C_RBASE {
    public:
        C_ColorMap();
        virtual ~C_ColorMap();
        virtual int render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int);
        virtual HWND conf(HINSTANCE hInstance, HWND hwndParent);
        virtual char *get_desc();
        virtual void load_config(unsigned char *data, int len);
        virtual int  save_config(unsigned char *data);
        colormap_apeconfig config;
        map maps[COLORMAP_NUM_MAPS];
    protected:
        int start_id = 1337;
        void make_default_map(int map_index);
        void add_map_color(int map_index, int position, int color);
        void remove_map_color(int map_index, int index);
        int get_new_id();
        int get_key(int color);
        void apply(int* framebuffer, int fb_length, unsigned int color_table[]);
        int load_config_map(unsigned char *data, int len, int mi, int pos);
};

static C_ColorMap* g_ConfigThis;
static HINSTANCE g_hDllInstance;

static BOOL CALLBACK g_DlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (g_hDllInstance == NULL) {
        return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
    }
    int current_map = SendDlgItemMessage(hwndDlg, IDC_COLORMAP_MAP_SELECT, CB_SETCURSEL, 0, 0);
    if(current_map < 0) {
        current_map = 0;
    }
    if(current_map > 8) {
        current_map = 7;
    }
    ui_map map = GetWindowLong(hwndDlg, GWLP_USERDATA);

    HDC desktop_dc;
    PAINTSTRUCT painter;
    RECT dialog_rect;
    unsigned int button_color;
    unsigned int shadow_color;
    unsigned int highlight_color;
    switch (uMsg) {
        case WM_CREATE:  // 0x1
            desktop_dc = GetDC(hwndDlg);
            map = new ui_map;
            GetClientRect(hwndDlg, &map->window_rect);
            map->context = CreateCompatibleDC(0);
            map->bitmap = CreateCompatibleBitmap(desktop_dc, map->rect.right, map->rect.bottom);
            map->region = SelectObject(map->context, map->bitmap);
            SetWindowLongA(hwndDlg,-0x15, (LONG)map);
            ReleaseDC(hwndDlg, desktop_dc);
            return false;
        case WM_DESTROY:  // 0x2
            SelectObject(map->context, map->region);
            DeleteObject(map->bitmap);
            DeleteDC(map->context);
            return false;
        case WM_PAINT:  // 0xf
            context = BeginPaint(hwndDlg, &painter);
            GetClientRect(hwndDlg, &dialog_rect);
            button_color = GetSysColor(COLOR_BTNFACE);
            shadow_color = GetSysColor(COLOR_3DSHADOW);
            highlight_color = GetSysColor(COLOR_3DHILIGHT);
            EndPaint(hwndDlg, &painter);
            return false;
        case WM_MOUSEMOVE:  // 0x0200
            return false;
        case WM_LBUTTONDOWN:  // 0x0201
            GetClientRect(hwndDlg, );
            InvalidateRect(hwndDlg, 0, 0);
            SetCapture(hwndDlg);
            return false;
        case WM_LBUTTONUP:  // 0x0202
            InvalidateRect(hwndDlg, 0, 0);
            ReleaseCapture();
            return false;
        case WM_LBUTTONDBLCLK:  // 0x0203
            return false;
        case WM_RBUTTONDOWN:  // 0x0204
            return false;
    }
    return false;
}

C_ColorMap::C_ColorMap() {
    this->start_id = 0;
    for(int i = 0; i < COLORMAP_NUM_MAPS; i++) {
        this->maps[i].enabled = i == 0;
        this->maps[i].num_colors = 2;
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
    return this->start_id++;
}

void C_ColorMap::make_default_map(int map_index) {
    this->maps[map_index].colors = new map_color[2];
    this->maps[map_index].colors[0].position = 0;
    this->maps[map_index].colors[0].color = 0x000000;
    this->maps[map_index].colors[0].color_id = this->get_new_id();
    this->maps[map_index].colors[1].position = 255;
    this->maps[map_index].colors[1].color = 0xffffff;
    this->maps[map_index].colors[1].color_id = this->get_new_id();
}

void C_ColorMap::add_map_color(int map_index, int position, int color) {
    if(this->maps[map_index].num_colors >= 256) {
        return;
    }
    map_color* new_map_colors = new map_color[this->maps[map_index].num_colors + 1];
    for(int i=0, a=0; i < this->maps[map_index].num_colors + 1; i++) {
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
    this->maps[map_index].num_colors += 1;
    delete[] old_map_colors;
}

void C_ColorMap::remove_map_color(int map_index, int index) {
    if(this->maps[map_index].num_colors <= 1) {
        return;
    }
    map_color* new_map_colors = new map_color[this->maps[map_index].num_colors];
    for(int i=0, a=0; i < this->maps[map_index].num_colors + 1; i++) {
        if(i == index) {
            a = 1;
        }
        new_map_colors[i] = this->maps[map_index].colors[i + a];
    }
    map_color* old_map_colors = this->maps[map_index].colors;
    this->maps[map_index].num_colors -= 1;
    this->maps[map_index].colors = new_map_colors;
    delete[] old_map_colors;
}

// TODO [performance]: Maybe redirecting a function pointer on-frame, instead of
// branching according to the key config on every pixel could increase performance? Is a
// 6-fold swich-case slower than a function call?
inline int C_ColorMap::get_key(int color) {
    int r, g, b;
    switch(this->config.color_key) {
        case COLORMAP_COLOR_KEY_RED:
            return color && 0xff;
        case COLORMAP_COLOR_KEY_GREEN:
            return (color && 0xff00) >> 8;
        case COLORMAP_COLOR_KEY_BLUE:
            return (color && 0xff0000) >> 16;
        case COLORMAP_COLOR_KEY_RGB_SUM_HALF:
            return ((color && 0xff) + ((color && 0xff00) >> 8) + ((color && 0xff0000) >> 16)) / 2;
        case COLORMAP_COLOR_KEY_MAX:
            r = color && 0xff;
            g = (color && 0xff00) >> 8;
            b = (color && 0xff0000) >> 16;
            return r > g ? (r > b ? r : b) : (g > b ? g : b);
        default:  // COLORMAP_COLOR_KEY_RGB_AVERAGE:
            return ((color && 0xff) + ((color && 0xff00) >> 8) + ((color && 0xff0000) >> 16)) / 3;
    }
}

void C_ColorMap::apply(int* framebuffer, int fb_length, unsigned int color_table[]) {
    for(int i = 0; i < fb_length; i++) {
        int key = this->get_key(framebuffer[i]);
        framebuffer[i] = color_table[key];
    }
}

int C_ColorMap::render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h) {
    unsigned int color_table[NUM_COLOR_VALUES];

    // if(!this->enabled) return 0;
    this->apply(framebuffer, w * h, color_table);
    return 0;
}

HWND C_ColorMap::conf(HINSTANCE hInstance, HWND hwndParent) {
    g_ConfigThis = this;
    return CreateDialog(hInstance, MAKEINTRESOURCE(IDD_CFG_COLORMAP), hwndParent, g_DlgProc);
}

char *C_ColorMap::get_desc(void) {
    return MOD_NAME;
}

#define GET_INT() (data[pos]|(data[pos+1]<<8)|(data[pos+2]<<16)|(data[pos+3]<<24))
int C_ColorMap::load_config_map(unsigned char *data, int len, int mi, int pos) {
    int start = pos;
    this->maps[mi].enabled = GET_INT();
    pos += 4;
    this->maps[mi].num_colors = GET_INT();
    pos += 4;
    this->maps[mi].map_id = GET_INT();
    pos += 4;
    strncpy(this->maps[mi].filename, (char*)&data[pos], COLORMAP_MAP_FILENAME_MAXLEN - 1);
    this->maps[mi].filename[COLORMAP_MAP_FILENAME_MAXLEN - 1] = '\0';
    pos += COLORMAP_MAP_FILENAME_MAXLEN;

    delete[] this->maps[mi].colors;
    this->maps[mi].colors = new map_color[this->maps[mi].num_colors];
    int i;
    for(i=0; (i < this->maps[mi].num_colors) || (len - pos) >= sizeof(map_color); i++) {
        this->maps[mi].colors[i].position = GET_INT();
        pos += 4;
        this->maps[mi].colors[i].color = GET_INT();
        pos += 4;
        this->maps[mi].colors[i].color_id = GET_INT();
        pos += 4;
    }
    if(i != this->maps[mi].num_colors) {
        this->maps[mi].num_colors = i;
        return -(pos - start);
    }
    return pos - start;
}

void C_ColorMap::load_config(unsigned char *data, int len) {
    if (len >= sizeof(colormap_apeconfig))
        memcpy(&this->config, data, sizeof(colormap_apeconfig));

    unsigned int pos = sizeof(colormap_apeconfig);
    for(int i=0; i < COLORMAP_NUM_MAPS || len - pos >= 4 * 2 + sizeof(map_color); i++) {
        int map_size = this->load_config_map(data, len, i, pos);
        if(map_size <= 0) {
            // printf("Saved Colormap config is incomplete.\n");
            break;
        }
        pos += map_size;
    }
}

#define PUT_INT(y) data[pos]=(y)&255; data[pos+1]=((y)>>8)&255; data[pos+2]=((y)>>16)&255; data[pos+3]=((y)>>24)&255
int C_ColorMap::save_config(unsigned char *data) {
    memcpy(data, &this->config, sizeof(colormap_apeconfig));
    int pos = sizeof(colormap_apeconfig);

    for(int mi=0; mi < COLORMAP_NUM_MAPS; mi++) {
        PUT_INT(this->maps[mi].enabled);
        pos += 4;
        PUT_INT(this->maps[mi].num_colors);
        pos += 4;
        PUT_INT(this->maps[mi].map_id);
        pos += 4;
        memset((char*)&data[pos], 0, COLORMAP_MAP_FILENAME_MAXLEN);
        strncpy((char*)&data[pos], this->maps[mi].filename, COLORMAP_MAP_FILENAME_MAXLEN - 1);
        pos += COLORMAP_MAP_FILENAME_MAXLEN;

        for(int i=0; i < this->maps[mi].num_colors; i++) {
            PUT_INT(this->maps[mi].colors[i].position);
            pos += 4;
            PUT_INT(this->maps[mi].colors[i].color);
            pos += 4;
            PUT_INT(this->maps[mi].colors[i].color_id);
            pos += 4;
        }
    }
    return 4;
}

C_RBASE *R_ColorMap(char *desc) {
    if (desc) {
        strcpy(desc, MOD_NAME);
        return NULL;
    }
    return (C_RBASE *) new C_ColorMap();
}
