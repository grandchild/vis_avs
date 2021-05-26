#include <windows.h>
#include "r_defs.h"
#include "resource.h"
#include <xmmintrin.h>
#include <emmintrin.h>


#define MOD_NAME "Render / Add Borders"
#define UNIQUEIDSTRING "Virtual Effect: Addborders"

#define ADDBORDERS_WIDTH_MIN 1
#define ADDBORDERS_WIDTH_MAX 50

#define RGB_TO_BGR(color) (((color) & 0xff0000) >> 16 | ((color) & 0xff00) | ((color) & 0xff) << 16)
#define BGR_TO_RGB(color) RGB_TO_BGR(color)  // is its own inverse


class C_AddBorders : public C_RBASE {
    public:
        C_AddBorders();
        virtual ~C_AddBorders();
        virtual int render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int);
        virtual HWND conf(HINSTANCE hInstance, HWND hwndParent);
        virtual char *get_desc();
        virtual void load_config(unsigned char *data, int len);
        virtual int  save_config(unsigned char *data);
        bool enabled;
        int color;
        int width;
};

static C_AddBorders* g_addborder_this;
static HINSTANCE g_hDllInstance;
static int g_color;
static COLORREF g_cust_colors[16];

static BOOL CALLBACK g_DlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam) {
    DRAWITEMSTRUCT* drawitem;
    CHOOSECOLOR choosecolor;
    unsigned int color;
    HMENU menu;
    HWND width_slider;
    int width;

    switch(uMsg) {
        case WM_DRAWITEM:  // 0x2b
            drawitem = (DRAWITEMSTRUCT*)lParam;
            if (drawitem->CtlID == IDC_ADDBORDERS_COLOR) {
                LOGBRUSH logbrush;
                logbrush.lbStyle = BS_SOLID;
                logbrush.lbColor = RGB_TO_BGR(g_addborder_this->color);
                logbrush.lbHatch = 0;
                HGDIOBJ brush = CreateBrushIndirect(&logbrush);
                HGDIOBJ obj = SelectObject(drawitem->hDC, brush);
                Rectangle(
                    drawitem->hDC,
                    drawitem->rcItem.left,
                    drawitem->rcItem.top,
                    drawitem->rcItem.right,
                    drawitem->rcItem.bottom
                );
                SelectObject(drawitem->hDC, obj);
                DeleteObject(brush);
            }
            break;
        case WM_INITDIALOG:  // 0x110
            width_slider = GetDlgItem(hwndDlg, IDC_ADDBORDERS_WIDTH);
            SendMessage(width_slider, 0x406, 0, 0x320001);
            SendMessage(width_slider, 0x405, 1, g_addborder_this->width);
            if (g_addborder_this->enabled) {
                CheckDlgButton(hwndDlg, IDC_ADDBORDERS_ENABLE, 1);
            }
            break;
        case WM_HSCROLL:  // 0x114
            menu = GetMenu((HWND)lParam);
            width = SendMessage((HWND)lParam, WM_USER, 0, 0);
            if (menu == IDC_ADDBORDERS_WIDTH) {
                g_addborder_this->width = width;
            }
            break;
        case WM_COMMAND:  // 0x111
            if (LOWORD(wParam) == IDC_ADDBORDERS_ENABLE) {
                g_addborder_this->enabled = IsDlgButtonChecked(hwndDlg, IDC_ADDBORDERS_ENABLE) != 0;
            } else if (LOWORD(wParam) == IDC_ADDBORDERS_COLOR) {
                choosecolor.lStructSize = sizeof(CHOOSECOLOR);
                choosecolor.hwndOwner = hwndDlg;
                choosecolor.hInstance = NULL;
                choosecolor.rgbResult = RGB_TO_BGR(g_addborder_this->color);
                choosecolor.lpCustColors = g_cust_colors;
                choosecolor.Flags = 3;
                if (ChooseColor(&choosecolor) != 0) {
                    g_addborder_this->color = BGR_TO_RGB(choosecolor.rgbResult);
                }
                InvalidateRect(GetDlgItem(hwndDlg, IDC_ADDBORDERS_COLOR), NULL, 1);
            }
            break;
    }
    return 0;

}

C_AddBorders::C_AddBorders() {
    this->enabled = false;
    this->color = 0;
    this->width = ADDBORDERS_WIDTH_MIN;
}

C_AddBorders::~C_AddBorders() {}


int C_AddBorders::render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h) {
    int framesize;
    int border_height_px;
    int border_width_px;
    int i;
    int k;

    if (this->enabled) {
        framesize = w * (h + -1);
        border_height_px = (h * this->width) / 100;
        border_width_px = (w * this->width) / 100;
        if (border_width_px < 1) {
            border_width_px = 1;
        }
        if (border_height_px < 1) {
            border_height_px = 1;
        }
        for (i = 0; i < border_height_px; i = i + 1) {
            for (k = 0; k < w; k = k + 1) {
                framebuffer[k + i * w] = this->color;
            }
            for (k = framesize; k < w + framesize; k = k + 1) {
                framebuffer[k - i * w] = this->color;
            }
        }
        for (i = 0; i < border_width_px; i = i + 1) {
            for (k = 0; k < h; k = k + 1) {
                framebuffer[w * k + i] = this->color;
                framebuffer[w * k + ((w + -1) - i)] = this->color;
            }
        }
    }
    return 0;
}

HWND C_AddBorders::conf(HINSTANCE hInstance, HWND hwndParent) {
    g_addborder_this = this;
    return CreateDialog(hInstance, MAKEINTRESOURCE(IDD_CFG_ADDBORDERS), hwndParent, g_DlgProc);
}

char *C_AddBorders::get_desc(void) {
    return MOD_NAME;
}

#define GET_INT() (data[pos]|(data[pos+1]<<8)|(data[pos+2]<<16)|(data[pos+3]<<24))
void C_AddBorders::load_config(unsigned char *data, int len) {
    int pos = 0;
    if (len - pos >= 4) {
        this->enabled = GET_INT();
    } else {
        this->enabled = false;
    }
    pos += 4;
    if (len - pos >= 4) {
        this->color = GET_INT();
    } else {
        this->color = 0;
    }
    pos += 4;
    if (len - pos >= 4) {
        this->width = GET_INT();
        if (this->width < ADDBORDERS_WIDTH_MIN) {
            this->width = ADDBORDERS_WIDTH_MIN;
        } else if (this->width > ADDBORDERS_WIDTH_MAX) {
            this->width = ADDBORDERS_WIDTH_MAX;
        }
    } else {
        this->width = ADDBORDERS_WIDTH_MIN;
    }
}

#define PUT_INT(y) data[pos]=(y)&255; data[pos+1]=((y)>>8)&255; data[pos+2]=((y)>>16)&255; data[pos+3]=((y)>>24)&255
int  C_AddBorders::save_config(unsigned char *data) {
    int pos = 0;
    PUT_INT((int)this->enabled);
    pos += 4;
    PUT_INT((int)this->color);
    pos += 4;
    PUT_INT((int)this->width);
    return 12;
}

C_RBASE *R_AddBorders(char *desc) {
    if (desc) {
        strcpy(desc,MOD_NAME); 
        return NULL; 
    }
    return (C_RBASE *) new C_AddBorders();
}
