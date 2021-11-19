/* Reconstructed from scratch. Original APE by Goebish (https://github.com/goebish) */
#include "c_addborders.h"

#include "r_defs.h"

C_AddBorders::C_AddBorders() {
    this->enabled = false;
    this->color = 0;
    this->width = ADDBORDERS_WIDTH_MIN;
}

C_AddBorders::~C_AddBorders() {}

int C_AddBorders::render(char visdata[2][2][576],
                         int is_beat,
                         int* framebuffer,
                         int* fbout,
                         int w,
                         int h) {
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

char* C_AddBorders::get_desc(void) { return MOD_NAME; }

#define GET_INT() \
    (data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (data[pos + 3] << 24))
void C_AddBorders::load_config(unsigned char* data, int len) {
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

#define PUT_INT(y)                     \
    data[pos] = (y)&255;               \
    data[pos + 1] = ((y) >> 8) & 255;  \
    data[pos + 2] = ((y) >> 16) & 255; \
    data[pos + 3] = ((y) >> 24) & 255
int C_AddBorders::save_config(unsigned char* data) {
    int pos = 0;
    PUT_INT((int)this->enabled);
    pos += 4;
    PUT_INT((int)this->color);
    pos += 4;
    PUT_INT((int)this->width);
    return 12;
}

C_RBASE* R_AddBorders(char* desc) {
    if (desc) {
        strcpy(desc, MOD_NAME);
        return NULL;
    }
    return (C_RBASE*)new C_AddBorders();
}
