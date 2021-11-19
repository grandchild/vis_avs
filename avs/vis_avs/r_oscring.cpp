/*
  LICENSE
  -------
Copyright 2005 Nullsoft, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

  * Neither the name of Nullsoft nor the names of its contributors may be used to
    endorse or promote products derived from this software without specific prior
    written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
// alphachannel safe 11/21/99
#include "c_oscring.h"

#include "r_defs.h"

#include <math.h>

#define PUT_INT(y)                   \
    data[pos] = (y)&255;             \
    data[pos + 1] = (y >> 8) & 255;  \
    data[pos + 2] = (y >> 16) & 255; \
    data[pos + 3] = (y >> 24) & 255
#define GET_INT() \
    (data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (data[pos + 3] << 24))

void C_THISCLASS::load_config(unsigned char* data, int len) {
    int pos = 0;
    int x = 0;
    if (len - pos >= 4) {
        effect = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        num_colors = GET_INT();
        pos += 4;
    }
    if (num_colors <= 16)
        while (len - pos >= 4 && x < num_colors) {
            colors[x++] = GET_INT();
            pos += 4;
        }
    else
        num_colors = 1;
    if (len - pos >= 4) {
        size = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        source = GET_INT();
        pos += 4;
    }
}

int C_THISCLASS::save_config(unsigned char* data) {
    int pos = 0, x = 0;
    PUT_INT(effect);
    pos += 4;
    PUT_INT(num_colors);
    pos += 4;
    while (x < num_colors) {
        PUT_INT(colors[x]);
        x++;
        pos += 4;
    }
    PUT_INT(size);
    pos += 4;
    PUT_INT(source);
    pos += 4;
    return pos;
}

C_THISCLASS::C_THISCLASS() {
    effect = 0 | (2 << 2) | (2 << 4);
    num_colors = 1;
    memset(colors, 0, sizeof(colors));
    colors[0] = RGB(255, 255, 255);
    color_pos = 0;
    size = 8;  // of 16
    source = 0;
}

C_THISCLASS::~C_THISCLASS() {}

int C_THISCLASS::render(char visdata[2][2][576],
                        int isBeat,
                        int* framebuffer,
                        int*,
                        int w,
                        int h) {
    int x;
    int current_color;
    unsigned char* fa_data;
    char center_channel[576];
    int which_ch = (effect >> 2) & 3;
    int y_pos = (effect >> 4);

    if (isBeat & 0x80000000) return 0;
    if (!num_colors) return 0;
    color_pos++;
    if (color_pos >= num_colors * 64) color_pos = 0;

    {
        int p = color_pos / 64;
        int r = color_pos & 63;
        int c1, c2;
        int r1, r2, r3;
        c1 = colors[p];
        if (p + 1 < num_colors)
            c2 = colors[p + 1];
        else
            c2 = colors[0];

        r1 = (((c1 & 255) * (63 - r)) + ((c2 & 255) * r)) / 64;
        r2 = ((((c1 >> 8) & 255) * (63 - r)) + (((c2 >> 8) & 255) * r)) / 64;
        r3 = ((((c1 >> 16) & 255) * (63 - r)) + (((c2 >> 16) & 255) * r)) / 64;

        current_color = r1 | (r2 << 8) | (r3 << 16);
    }

    if (which_ch >= 2) {
        for (x = 0; x < 576; x++)
            center_channel[x] =
                visdata[source ? 0 : 1][0][x] / 2 + visdata[source ? 0 : 1][1][x] / 2;
    }
    if (which_ch < 2)
        fa_data = (unsigned char*)&visdata[source ? 0 : 1][which_ch][0];
    else
        fa_data = (unsigned char*)center_channel;

    {
        double s = size / 32.0;
#ifdef LASER
        double lx, ly;
        double is = s;
        double c_x;
        double c_y = 0;
        if (y_pos == 2)
            c_x = 0;
        else if (y_pos == 0)
            c_x = -0.5;
        else
            c_x = 0.5;
#else
        int lx, ly;
        double is = min((h * s), (w * s));
        int c_x;
        int c_y = h / 2;
        if (y_pos == 2)
            c_x = w / 2;
        else if (y_pos == 0)
            c_x = (w / 4);
        else
            c_x = w / 2 + w / 4;
#endif
        {
            int q = 0;
            double a = 0.0;
            double sca;
            if (!source)
                sca = 0.1 + ((fa_data[q] ^ 128) / 255.0) * 0.9;
            else
                sca =
                    0.1 + ((fa_data[q * 2] / 2 + fa_data[q * 2 + 1] / 2) / 255.0) * 0.9;
#ifdef LASER
            int n_seg = 4;
#else
            int n_seg = 1;
#endif
            lx = c_x + (cos(a) * is * sca);
            ly = c_y + (sin(a) * is * sca);

            for (q = 1; q <= 80; q += n_seg) {
#ifdef LASER
                double tx, ty;
#else
                int tx, ty;
#endif
                a -= 3.14159 * 2.0 / 80.0 * n_seg;
                if (!source)
                    sca = 0.1 + ((fa_data[q > 40 ? 80 - q : q] ^ 128) / 255.0) * 0.90;
                else
                    sca = 0.1
                          + ((fa_data[q > 40 ? (80 - q) * 2 : q * 2] / 2
                              + fa_data[q > 40 ? (80 - q) * 2 + 1 : q * 2 + 1] / 2)
                             / 255.0)
                                * 0.9;
                tx = c_x + (cos(a) * is * sca);
                ty = c_y + (sin(a) * is * sca);
#ifdef LASER
                if ((tx > -1.0 && tx < 1.0 && ty > -1.0 && ty < 1.0)
                    || (lx > -1.0 && lx < 1.0 && ly > -1.0 && ly < 1.0)) {
                    LineType l;
                    l.color = current_color;
                    l.mode = 0;
                    l.x1 = (float)tx;
                    l.x2 = (float)lx;
                    l.y1 = (float)ty;
                    l.y2 = (float)ly;
                    g_laser_linelist->AddLine(&l);
                }
#else

                if ((tx >= 0 && tx < w && ty >= 0 && ty < h)
                    || (lx >= 0 && lx < w && ly >= 0 && ly < h)) {
                    line(framebuffer,
                         tx,
                         ty,
                         lx,
                         ly,
                         w,
                         h,
                         current_color,
                         (g_line_blend_mode & 0xff0000) >> 16);
                }
#endif
                //        line(framebuffer,tx,ty,c_x,c_y,w,h,current_color);
                //        if (tx >= 0 && tx < w && ty >= 0 && ty < h)
                //        framebuffer[tx+ty*w]=current_color;
                lx = tx;
                ly = ty;
            }
        }
    }
    return 0;
}

C_RBASE* R_OscRings(char* desc) {
    if (desc) {
        strcpy(desc, MOD_NAME);
        return NULL;
    }
    return (C_RBASE*)new C_THISCLASS();
}
