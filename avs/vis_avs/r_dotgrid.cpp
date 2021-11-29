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
#include "c_dotgrid.h"

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
        num_colors = GET_INT();
        pos += 4;
    }
    if (num_colors <= 16)
        while (len - pos >= 4 && x < num_colors) {
            colors[x++] = GET_INT();
            pos += 4;
        }
    else
        num_colors = 0;
    if (len - pos >= 4) {
        spacing = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        x_move = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        y_move = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        blend = GET_INT();
        pos += 4;
    }
}

int C_THISCLASS::save_config(unsigned char* data) {
    int pos = 0, x = 0;
    PUT_INT(num_colors);
    pos += 4;
    while (x < num_colors) {
        PUT_INT(colors[x]);
        x++;
        pos += 4;
    }
    PUT_INT(spacing);
    pos += 4;
    PUT_INT(x_move);
    pos += 4;
    PUT_INT(y_move);
    pos += 4;
    PUT_INT(blend);
    pos += 4;
    return pos;
}

C_THISCLASS::C_THISCLASS() {
    num_colors = 1;
    memset(colors, 0, sizeof(colors));
    colors[0] = RGB(255, 255, 255);
    color_pos = 0;
    xp = 0;
    yp = 0;
    x_move = 128;
    y_move = 128;
    spacing = 8;
    blend = 3;
}

C_THISCLASS::~C_THISCLASS() {}

int C_THISCLASS::render(char[2][2][576],
                        int isBeat,
                        int* framebuffer,
                        int*,
                        int w,
                        int h) {
    int x, y;
    int current_color;

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
    if (spacing < 2) spacing = 2;
    while (yp < 0) yp += spacing * 256;
    while (xp < 0) xp += spacing * 256;

    int sy = (yp >> 8) % spacing;
    int sx = (xp >> 8) % spacing;
    framebuffer += sy * w;
    for (y = sy; y < h; y += spacing) {
        if (blend == 1)
            for (x = sx; x < w; x += spacing)
                framebuffer[x] = BLEND(framebuffer[x], current_color);
        else if (blend == 2)
            for (x = sx; x < w; x += spacing)
                framebuffer[x] = BLEND_AVG(framebuffer[x], current_color);
        else if (blend == 3)
            for (x = sx; x < w; x += spacing)
                BLEND_LINE(framebuffer + x, current_color);
        else
            for (x = sx; x < w; x += spacing) framebuffer[x] = current_color;
        framebuffer += w * spacing;
    }
    xp += x_move;
    yp += y_move;

    return 0;
}

C_RBASE* R_DotGrid(char* desc) {
    if (desc) {
        strcpy(desc, MOD_NAME);
        return NULL;
    }
    return (C_RBASE*)new C_THISCLASS();
}
