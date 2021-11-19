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
#include "c_bump.h"

#include "r_defs.h"

#include "avs_eelif.h"

#include <stdio.h>
#include <stdlib.h>

#ifndef LASER

C_THISCLASS::~C_THISCLASS() {
    freeCode(codeHandle);
    freeCode(codeHandleBeat);
    freeCode(codeHandleInit);
    DeleteCriticalSection(&rcs);
    AVS_EEL_QUITINST();
}

// configuration read/write

C_THISCLASS::C_THISCLASS()  // set up default configuration
{
    AVS_EEL_INITINST();
    InitializeCriticalSection(&rcs);
    buffern = 0;
    oldstyle = 0;
    invert = 0;
    enabled = 1;
    onbeat = 0;
    durFrames = 15;
    depth = 30;
    depth2 = 100;
    nF = 0;
    showlight = 0;
    thisDepth = depth;
    blend = 0;
    blendavg = 0;
    code1.assign("x=0.5+cos(t)*0.3;\r\ny=0.5+sin(t)*0.3;\r\nt=t+0.1;");
    code2.assign("");
    code3.assign("t=0;");
    codeHandle = 0;
    codeHandleBeat = 0;
    codeHandleInit = 0;
    var_bi = 0;
    initted = 0;

    need_recompile = 1;
}

#define GET_INT() \
    (data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (data[pos + 3] << 24))

void C_THISCLASS::load_config(unsigned char* data, int len) {
    int pos = 0;
    if (len - pos >= 4) {
        enabled = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        onbeat = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        durFrames = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        depth = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        depth2 = GET_INT();
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
    load_string(code1, data, pos, len);
    load_string(code2, data, pos, len);
    load_string(code3, data, pos, len);
    if (len - pos >= 4) {
        showlight = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        invert = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        oldstyle = GET_INT();
        pos += 4;
    } else
        oldstyle = 1;
    if (len - pos >= 4) {
        buffern = GET_INT();
        pos += 4;
    }
    thisDepth = depth;
    nF = 0;

    need_recompile = 1;
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
    PUT_INT(onbeat);
    pos += 4;
    PUT_INT(durFrames);
    pos += 4;
    PUT_INT(depth);
    pos += 4;
    PUT_INT(depth2);
    pos += 4;
    PUT_INT(blend);
    pos += 4;
    PUT_INT(blendavg);
    pos += 4;
    save_string(data, pos, code1);
    save_string(data, pos, code2);
    save_string(data, pos, code3);
    PUT_INT(showlight);
    pos += 4;
    PUT_INT(invert);
    pos += 4;
    PUT_INT(oldstyle);
    pos += 4;
    PUT_INT(buffern);
    pos += 4;
    return pos;
}

int __inline C_THISCLASS::depthof(int c, int i) {
    int r = max(max((c & 0xFF), ((c & 0xFF00) >> 8)), (c & 0xFF0000) >> 16);
    return i ? 255 - r : r;
}

static int __inline setdepth(int l, int c) {
    int r;
    r = min((c & 0xFF) + l, 254);
    r |= min(((c & 0xFF00)) + (l << 8), 254 << 8);
    r |= min(((c & 0xFF0000)) + (l << 16), 254 << 16);
    return r;
}

static int __inline setdepth0(int c) {
    int r;
    r = min((c & 0xFF), 254);
    r |= min(((c & 0xFF00)), 254 << 8);
    r |= min(((c & 0xFF0000)), 254 << 16);
    return r;
}

// render function
// render should return 0 if it only used framebuffer, or 1 if the new output data is in
// fbout. this is used when you want to do something that you'd otherwise need to make a
// copy of the framebuffer. w and h are the width and height of the screen, in pixels.
// isBeat is 1 if a beat has been detected.
// visdata is in the format of [spectrum:0,wave:1][channel][band].
#define abs(x) ((x) >= 0 ? (x) : -(x))

int C_THISCLASS::render(char visdata[2][2][576],
                        int isBeat,
                        int* framebuffer,
                        int* fbout,
                        int w,
                        int h) {
    int cx, cy;
    int curbuf;

    if (!enabled) return 0;

    if (need_recompile) {
        EnterCriticalSection(&rcs);
        if (!var_bi || g_reset_vars_on_recompile) {
            clearVars();
            var_x = registerVar("x");
            var_y = registerVar("y");
            var_isBeat = registerVar("isbeat");
            var_isLongBeat = registerVar("islbeat");
            var_bi = registerVar("bi");
            *var_bi = 1.0;
            initted = 0;
        }

        need_recompile = 0;

        freeCode(codeHandle);
        freeCode(codeHandleBeat);
        freeCode(codeHandleInit);
        codeHandle = compileCode((char*)code1.c_str());
        codeHandleBeat = compileCode((char*)code2.c_str());
        codeHandleInit = compileCode((char*)code3.c_str());

        LeaveCriticalSection(&rcs);
    }
    if (isBeat & 0x80000000) return 0;

    int* depthbuffer =
        !buffern ? framebuffer : (int*)getGlobalBuffer(w, h, buffern - 1, 0);
    if (!depthbuffer) return 0;

    curbuf = (depthbuffer == framebuffer);

    if (!initted) {
        executeCode(codeHandleInit, visdata);
        initted = 1;
    }

    executeCode(codeHandle, visdata);
    if (isBeat) executeCode(codeHandleBeat, visdata);

    if (isBeat)
        *var_isBeat = -1;
    else
        *var_isBeat = 1;
    if (nF)
        *var_isLongBeat = -1;
    else
        *var_isLongBeat = 1;

    if (onbeat && isBeat) {
        thisDepth = depth2;
        nF = durFrames;
    } else if (!nF)
        thisDepth = depth;

    memset(fbout, 0, w * h * 4);  // previous effects may have left fbout in a mess

    if (oldstyle) {
        cx = (int)(*var_x / 100.0 * w);
        cy = (int)(*var_y / 100.0 * h);
    } else {
        cx = (int)(*var_x * w);
        cy = (int)(*var_y * h);
    }
    cx = max(0, min(w, cx));
    cy = max(0, min(h, cy));
    if (showlight) fbout[cx + cy * w] = 0xFFFFFF;

    if (var_bi) {
        *var_bi = min(max(*var_bi, 0), 1);
        thisDepth = (int)(thisDepth * *var_bi);
    }

    int thisDepth_scaled = (thisDepth << 8) / 100;
    depthbuffer += w + 1;
    framebuffer += w + 1;
    fbout += w + 1;

    int ly = 1 - cy;
    int i = h - 2;
    while (i--) {
        int j = w - 2;
        int lx = 1 - cx;
        if (blend) {
            while (j--) {
                int m1, p1, mw, pw;
                m1 = depthbuffer[-1];
                p1 = depthbuffer[1];
                mw = depthbuffer[-w];
                pw = depthbuffer[w];
                if (!curbuf || (curbuf && (m1 || p1 || mw || pw))) {
                    int coul1, coul2;
                    coul1 = depthof(p1, invert) - depthof(m1, invert) - lx;
                    coul2 = depthof(pw, invert) - depthof(mw, invert) - ly;
                    coul1 = 127 - abs(coul1);
                    coul2 = 127 - abs(coul2);
                    if (coul1 <= 0 || coul2 <= 0)
                        coul1 = setdepth0(framebuffer[0]);
                    else
                        coul1 = setdepth((coul1 * coul2 * thisDepth_scaled) >> (8 + 6),
                                         framebuffer[0]);
                    fbout[0] = BLEND(framebuffer[0], coul1);
                }
                depthbuffer++;
                framebuffer++;
                fbout++;
                lx++;
            }
        } else if (blendavg) {
            while (j--) {
                int m1, p1, mw, pw;
                m1 = depthbuffer[-1];
                p1 = depthbuffer[1];
                mw = depthbuffer[-w];
                pw = depthbuffer[w];
                if (!curbuf || (curbuf && (m1 || p1 || mw || pw))) {
                    int coul1, coul2;
                    coul1 = depthof(p1, invert) - depthof(m1, invert) - lx;
                    coul2 = depthof(pw, invert) - depthof(mw, invert) - ly;
                    coul1 = 127 - abs(coul1);
                    coul2 = 127 - abs(coul2);
                    if (coul1 <= 0 || coul2 <= 0)
                        coul1 = setdepth0(framebuffer[0]);
                    else
                        coul1 = setdepth((coul1 * coul2 * thisDepth_scaled) >> (8 + 6),
                                         framebuffer[0]);
                    fbout[0] = BLEND_AVG(framebuffer[0], coul1);
                }
                depthbuffer++;
                framebuffer++;
                fbout++;
                lx++;
            }
        } else {
            while (j--) {
                int m1, p1, mw, pw;
                m1 = depthbuffer[-1];
                p1 = depthbuffer[1];
                mw = depthbuffer[-w];
                pw = depthbuffer[w];
                if (!curbuf || (curbuf && (m1 || p1 || mw || pw))) {
                    int coul1, coul2;
                    coul1 = depthof(p1, invert) - depthof(m1, invert) - lx;
                    coul2 = depthof(pw, invert) - depthof(mw, invert) - ly;
                    coul1 = 127 - abs(coul1);
                    coul2 = 127 - abs(coul2);
                    if (coul1 <= 0 || coul2 <= 0)
                        coul1 = setdepth0(framebuffer[0]);
                    else
                        coul1 = setdepth((coul1 * coul2 * thisDepth_scaled) >> (8 + 6),
                                         framebuffer[0]);
                    fbout[0] = coul1;
                }
                depthbuffer++;
                framebuffer++;
                fbout++;
                lx++;
            }
        }
        depthbuffer += 2;
        framebuffer += 2;
        fbout += 2;
        ly++;
    }

    if (nF) {
        nF--;
        if (nF) {
            int a = abs(depth - depth2) / durFrames;
            thisDepth += a * (depth2 > depth ? -1 : 1);
        }
    }

    return 1;
}

C_RBASE* R_Bump(char* desc)  // creates a new effect object if desc is NULL, otherwise
                             // fills in desc with description
{
    if (desc) {
        strcpy(desc, MOD_NAME);
        return NULL;
    }
    return (C_RBASE*)new C_THISCLASS();
}

#else
C_RBASE* R_Bump(char* desc) { return NULL; }
#endif
