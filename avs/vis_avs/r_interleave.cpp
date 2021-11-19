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
#include "c_interleave.h"

#include "r_defs.h"

#include <stdlib.h>

#ifndef LASER

C_THISCLASS::~C_THISCLASS()  // set up default configuration
{}

// configuration read/write

C_THISCLASS::C_THISCLASS()  // set up default configuration
{
    onbeat = 0;
    beatdur = 4;

    color = 0;
    x2 = x = 1;
    y2 = y = 1;
    cur_x = cur_y = 1.0;
    enabled = 1;
    blend = 0;
    blendavg = 0;
}

#define GET_INT() \
    (data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (data[pos + 3] << 24))
void C_THISCLASS::load_config(unsigned char* data, int len)  // read configuration of
                                                             // max length "len" from
                                                             // data.
{
    int pos = 0;
    if (len - pos >= 4) {
        enabled = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        x = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        y = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        color = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        blend = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        blendavg = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        onbeat = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        x2 = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        y2 = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        beatdur = GET_INT();
        pos += 4;
    }
    cur_x = x;
    cur_y = y;
}

#define PUT_INT(y)                   \
    data[pos] = (y)&255;             \
    data[pos + 1] = (y >> 8) & 255;  \
    data[pos + 2] = (y >> 16) & 255; \
    data[pos + 3] = (y >> 24) & 255
int C_THISCLASS::save_config(unsigned char* data)  // write configuration to data,
                                                   // return length. config data should
                                                   // not exceed 64k.
{
    int pos = 0;
    PUT_INT(enabled);
    pos += 4;
    PUT_INT(x);
    pos += 4;
    PUT_INT(y);
    pos += 4;
    PUT_INT(color);
    pos += 4;
    PUT_INT(blend);
    pos += 4;
    PUT_INT(blendavg);
    pos += 4;
    PUT_INT(onbeat);
    pos += 4;
    PUT_INT(x2);
    pos += 4;
    PUT_INT(y2);
    pos += 4;
    PUT_INT(beatdur);
    pos += 4;
    return pos;
}

// render function
// render should return 0 if it only used framebuffer, or 1 if the new output data is in
// fbout. this is used when you want to do something that you'd otherwise need to make a
// copy of the framebuffer. w and h are the width and height of the screen, in pixels.
// isBeat is 1 if a beat has been detected.
// visdata is in the format of [spectrum:0,wave:1][channel][band].

int C_THISCLASS::render(char[2][2][576],
                        int isBeat,
                        int* framebuffer,
                        int*,
                        int w,
                        int h) {
    if (isBeat & 0x80000000) return 0;
    if (!enabled) return 0;
    int ystat = 0;
    int yp = 0;

    double sc1 = (beatdur + 512.0 - 64.0) / 512.0;
    cur_x = (cur_x * sc1 + x * (1.0 - sc1));
    cur_y = (cur_y * sc1 + y * (1.0 - sc1));

    if (isBeat && onbeat) {
        cur_x = x2;
        cur_y = y2;
    }
    int tx = (int)cur_x;
    int ty = (int)cur_y;
    int xos = 0;

    int* p = framebuffer;
    int j;
    if (!ty) {
        ystat = 1;
    }
    if (tx > 0) {
        xos = (w % tx) / 2;
    }
    if (ty > 0) yp = (h % ty) / 2;
    if (tx >= 0 && ty >= 0)
        for (j = 0; j < h; j++) {
            int xstat = 0;
            if (ty && ++yp >= ty) {
                ystat = !ystat;
                yp = 0;
            }
            int l = w;

            if (!ystat)  // this line is pure color
            {
                if (blend) {
                    while (l--) {
                        *p = BLEND(*p, color);
                        p++;
                    }
                } else if (blendavg) {
                    while (l--) {
                        *p = BLEND_AVG(*p, color);
                        p++;
                    }
                } else {
                    while (l--) {
                        *p++ = color;
                    }
                }
            } else if (tx) {
                if (blend) {
                    int xo = xos;
                    while (l > 0) {
                        int l2 = min(l, tx - xo);
                        xo = 0;
                        l -= l2;
                        if (xstat)
                            p += l2;
                        else
                            while (l2--) {
                                *p = BLEND(*p, color);
                                p++;
                            }
                        xstat = !xstat;
                    }
                } else if (blendavg) {
                    int xo = xos;
                    while (l > 0) {
                        int l2 = min(l, tx - xo);
                        xo = 0;
                        l -= l2;
                        if (xstat)
                            p += l2;
                        else
                            while (l2--) {
                                *p = BLEND_AVG(*p, color);
                                p++;
                            }
                        xstat = !xstat;
                    }
                } else {
                    int xo = xos;
                    while (l > 0) {
                        int l2 = min(l, tx - xo);
                        xo = 0;
                        l -= l2;
                        if (xstat)
                            p += l2;
                        else
                            while (l2--) {
                                *p++ = color;
                            }
                        xstat = !xstat;
                    }
                }
            } else
                p += w;
        }

    return 0;
}

C_RBASE* R_Interleave(char* desc)  // creates a new effect object if desc is NULL,
                                   // otherwise fills in desc with description
{
    if (desc) {
        strcpy(desc, MOD_NAME);
        return NULL;
    }
    return (C_RBASE*)new C_THISCLASS();
}

#else
C_RBASE* R_Interleave(char* desc)  // creates a new effect object if desc is NULL,
                                   // otherwise fills in desc with description
{
    return NULL;
}
#endif
