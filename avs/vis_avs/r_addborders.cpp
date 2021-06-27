/* Reconstructed from scratch. Original APE by Goebish (https://github.com/goebish) */
#include "c_addborders.h"
#include <windows.h>
#include <commctrl.h>
#include "r_defs.h"
#include "resource.h"


#define RGB_TO_BGR(color) (((color) & 0xff0000) >> 16 | ((color) & 0xff00) | ((color) & 0xff) << 16)
#define BGR_TO_RGB(color) RGB_TO_BGR(color)  // is its own inverse


static COLORREF g_cust_colors[16];

int win32_dlgproc_addborders(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam) {
    C_AddBorders* g_addborder_this = (C_AddBorders*)g_current_render;
    DRAWITEMSTRUCT* drawitem;
    HWND width_slider;

    switch(uMsg) {
        case WM_DRAWITEM:  // 0x02b
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
            SendMessage(width_slider, TBM_SETRANGE, 0, MAKELONG(ADDBORDERS_WIDTH_MIN, ADDBORDERS_WIDTH_MAX));
            SendMessage(width_slider, TBM_SETPOS, 1, g_addborder_this->width);
            if (g_addborder_this->enabled) {
                CheckDlgButton(hwndDlg, IDC_ADDBORDERS_ENABLE, 1);
            }
            break;
        case WM_HSCROLL:  // 0x114
            if (lParam == IDC_ADDBORDERS_WIDTH) {
                g_addborder_this->width = SendMessage((HWND)lParam, WM_USER, 0, 0);
            }
            break;
        case WM_COMMAND:  // 0x111
            if (LOWORD(wParam) == IDC_ADDBORDERS_ENABLE) {
                g_addborder_this->enabled = IsDlgButtonChecked(hwndDlg, IDC_ADDBORDERS_ENABLE) != 0;
            } else if (LOWORD(wParam) == IDC_ADDBORDERS_COLOR) {
                CHOOSECOLOR choosecolor;
                choosecolor.lStructSize = sizeof(CHOOSECOLOR);
                choosecolor.hwndOwner = hwndDlg;
                choosecolor.hInstance = NULL;
                choosecolor.rgbResult = RGB_TO_BGR(g_addborder_this->color);
                choosecolor.lpCustColors = g_cust_colors;
                choosecolor.Flags = CC_RGBINIT | CC_FULLOPEN;
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


int C_AddBorders::render(char visdata[2][2][576], int is_beat, int *framebuffer, int *fbout, int w, int h) {
    (void)visdata;
    (void)is_beat;
    (void)fbout;
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
