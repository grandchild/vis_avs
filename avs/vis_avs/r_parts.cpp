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
#include "c_parts.h"

#include "r_defs.h"

#include <math.h>

#ifndef LASER

#define PUT_INT(y)                   \
    data[pos] = (y)&255;             \
    data[pos + 1] = (y >> 8) & 255;  \
    data[pos + 2] = (y >> 16) & 255; \
    data[pos + 3] = (y >> 24) & 255
#define GET_INT() \
    (data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (data[pos + 3] << 24))
void C_THISCLASS::load_config(unsigned char* data, int len) {
    int pos = 0;
    if (len - pos >= 4) {
        enabled = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        colors = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        maxdist = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        size = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        size2 = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        blend = GET_INT();
        pos += 4;
    }
    s_pos = size;
}
int C_THISCLASS::save_config(unsigned char* data) {
    int pos = 0;
    PUT_INT(enabled);
    pos += 4;
    PUT_INT(colors);
    pos += 4;
    PUT_INT(maxdist);
    pos += 4;
    PUT_INT(size);
    pos += 4;
    PUT_INT(size2);
    pos += 4;
    PUT_INT(blend);
    pos += 4;
    return pos;
}

C_THISCLASS::C_THISCLASS() {
    blend = 1;
    size = size2 = s_pos = 8;
    maxdist = 16;
    enabled = 1;
    colors = RGB(255, 255, 255);
    c[0] = c[1] = 0.0f;
    v[0] = -0.01551;
    v[1] = 0.0;

    p[0] = -0.6;
    p[1] = 0.3;
}

C_THISCLASS::~C_THISCLASS() {}

int C_THISCLASS::render(char[2][2][576],
                        int isBeat,
                        int* framebuffer,
                        int*,
                        int w,
                        int h) {
    if (!(enabled & 1)) return 0;
    if (isBeat & 0x80000000) return 0;
    int xp, yp;
    int ss = min(h / 2, (w * 3) / 8);

    if (isBeat) {
        c[0] = ((rand() % 33) - 16) / 48.0f;
        c[1] = ((rand() % 33) - 16) / 48.0f;
    }

    v[0] -= 0.004 * (p[0] - c[0]);
    v[1] -= 0.004 * (p[1] - c[1]);

    p[0] += v[0];
    p[1] += v[1];

    v[0] *= 0.991;
    v[1] *= 0.991;

    xp = (int)(p[0] * (ss) * (maxdist / 32.0)) + w / 2;
    yp = (int)(p[1] * (ss) * (maxdist / 32.0)) + h / 2;
    if (isBeat && enabled & 2) s_pos = size2;
    int sz = s_pos;
    s_pos = (s_pos + size) / 2;
    if (sz <= 1) {
        framebuffer += xp + (yp)*w;
        if (xp >= 0 && yp >= 0 && xp < w && yp < h) {
            if (blend == 0)
                framebuffer[0] = colors;
            else if (blend == 2)
                framebuffer[0] = BLEND_AVG(framebuffer[0], colors);
            else if (blend == 3)
                BLEND_LINE(framebuffer, colors);
            else
                framebuffer[0] = BLEND(framebuffer[0], colors);
        }
        return 0;
    }
    if (sz > 128) sz = 128;
    {
        int y;
        double md = sz * sz * 0.25;
        yp -= sz / 2;
        for (y = 0; y < sz; y++) {
            if (yp + y >= 0 && yp + y < h) {
                double yd = (y - sz * 0.5);
                double l = sqrt(md - yd * yd);
                int xs = (int)(l + 0.99);
                int x;
                if (xs < 1) xs = 1;
                int xe = xp + xs;
                if (xe > w) xe = w;
                int xst = xp - xs;
                if (xst < 0) xst = 0;
                int* f = &framebuffer[xst + (yp + y) * w];
                if (blend == 0)
                    for (x = xst; x < xe; x++) *f++ = colors;
                else if (blend == 2)
                    for (x = xst; x < xe; x++) {
                        *f = BLEND_AVG(*f, colors);
                        f++;
                    }
                else if (blend == 3)
                    for (x = xst; x < xe; x++) {
                        BLEND_LINE(f++, colors);
                    }
                else
                    for (x = xst; x < xe; x++) {
                        *f = BLEND(*f, colors);
                        f++;
                    }
            }
        }
    }
    return 0;
}

C_RBASE* R_Parts(char* desc) {
    if (desc) {
        strcpy(desc, MOD_NAME);
        return NULL;
    }
    return (C_RBASE*)new C_THISCLASS();
}

#else
C_RBASE* R_Parts(char* desc)  // creates a new effect object if desc is NULL, otherwise
                              // fills in desc with description
{
    return NULL;
}
#endif
