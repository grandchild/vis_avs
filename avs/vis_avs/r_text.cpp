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
#include "e_text.h"

#include "r_defs.h"

#include "avs_eelif.h"

#include "../util.h"

#include <math.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#define GET_INT() \
    (data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (data[pos + 3] << 24))

#define PUT_INT(y)                   \
    data[pos] = (y) & 255;           \
    data[pos + 1] = (y >> 8) & 255;  \
    data[pos + 2] = (y >> 16) & 255; \
    data[pos + 3] = (y >> 24) & 255

constexpr Parameter Text_Info::parameters[];

/*
void Text_Info::callback(Effect* component,
                         const Parameter*,
                         const std::vector<int64_t>&) {
    auto text = (E_Text*)component;
    text->callback();
}
*/

// Reinit bitmap buffer since size changed
void E_Text::reinit(int w, int h) {
    // Free anything if needed
    if (lw || lh) {
#ifdef _WIN32
        SelectObject(hBitmapDC, hOldBitmap);
        if (hOldFont) {
            SelectObject(hBitmapDC, hOldFont);
        }
        DeleteDC(hBitmapDC);
        ReleaseDC(NULL, hDesktopDC);
#endif  // _WIN32
        if (myBuffer) {
            free(myBuffer);
        }
    }

    // Alloc buffers, select objects, init structures
    myBuffer = (int*)malloc(w * h * 4);
#ifdef _WIN32
    hDesktopDC = GetDC(NULL);
    hRetBitmap = CreateCompatibleBitmap(hDesktopDC, w, h);
    hBitmapDC = CreateCompatibleDC(hDesktopDC);
    hOldBitmap = (HBITMAP)SelectObject(hBitmapDC, hRetBitmap);
    SetTextColor(hBitmapDC,
                 ((this->config.color & 0xFF0000) >> 16) | (this->config.color & 0xFF00)
                     | (this->config.color & 0xFF) << 16);
    SetBkMode(hBitmapDC, TRANSPARENT);
    SetBkColor(hBitmapDC, 0);
    if (myFont) {
        hOldFont = (HFONT)SelectObject(hBitmapDC, myFont);
    } else {
        hOldFont = NULL;
    }
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = h;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    bi.bmiHeader.biSizeImage = 0;
    bi.bmiHeader.biXPelsPerMeter = 0;
    bi.bmiHeader.biYPelsPerMeter = 0;
    bi.bmiHeader.biClrUsed = 0;
    bi.bmiHeader.biClrImportant = 0;
#endif  // _WIN32
}

E_Text::E_Text(AVS_Instance* avs) : Configurable_Effect(avs) {
    old_valign = 0;
    old_halign = 0;
    old_border_mode = -1;
    old_curword = -1;
    old_clipcolor = -1;
    *oldtxt = 0;
    old_blend1 = 0;
    old_blend2 = 0;
    old_blend3 = 0;
    _xshift = 0;
    _yshift = 0;
    forceredraw = 0;
    myBuffer = NULL;
    curword = 0;
    forceshift = 0;
    forceBeat = 0;
    forcealign = 1;

#ifdef _WIN32
    myFont = NULL;
    memset(&cf, 0, sizeof(CHOOSEFONT));
    cf.lStructSize = sizeof(CHOOSEFONT);
    cf.lpLogFont = &lf;
    cf.Flags = CF_EFFECTS | CF_SCREENFONTS | CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT;
    cf.rgbColors = this->config.color;
    memset(&lf, 0, sizeof(LOGFONT));
#endif  // _WIN32
    lw = lh = 0;
    updating = false;
    nb = 0;
    oddeven = 0;
    nf = 0;
    shiftinit = 1;
}

E_Text::~E_Text() {
    if (lw || lh) {
#ifdef _WIN32
        SelectObject(hBitmapDC, hOldBitmap);
        if (hOldFont) {
            SelectObject(hBitmapDC, hOldFont);
        }
        DeleteDC(hBitmapDC);
        ReleaseDC(NULL, hDesktopDC);
#endif  // _WIN32
        if (myBuffer) {
            free(myBuffer);
        }
    }
#ifdef _WIN32
    if (myFont) {
        DeleteObject(myFont);
    }
#endif  // _WIN32
}

#ifdef CAN_TALK_TO_WINAMP
extern HWND hwnd_WinampParent;
#endif

// Parse text buffer for a specified word
void E_Text::getWord(int n, char* buf, int maxlen) {
    int w = 0;
    const char* p = this->config.text.c_str();
    char* d = buf;
    *d = 0;
    if (!p) {
        return;
    }
    while (w < n && *p) {
        if (*p == ';') {
            w++;
        }
        p++;
    }

    maxlen--;  // null terminator
    while (*p && *p != ';' && maxlen > 0) {
        const char* endp;
#ifdef CAN_TALK_TO_WINAMP
        if ((!strnicmp(p, "$(playpos", 9) || !strnicmp(p, "$(playlen", 9))
            && (endp = strstr(p + 9, ")"))) {
            char buf[128];
            int islen = strnicmp(p, "$(playpos", 9);
            int add_dig = 0;
            if (p[9] == '.') {
                add_dig = atoi(p + 10);
            }

            if (add_dig > 3) {
                add_dig = 3;
            }

            int pos = 0;

            if (IsWindow(hwnd_WinampParent)) {
                if (!SendMessageTimeout(hwnd_WinampParent,
                                        WM_USER,
                                        (WPARAM) !!islen,
                                        (LPARAM)105,
                                        SMTO_BLOCK,
                                        50,
                                        (LPDWORD)&pos)) {
                    pos = 0;
                }
            }
            if (islen) {
                pos *= 1000;
            }
            wsprintf(buf, "%d:%02d", pos / 60000, (pos / 1000) % 60);
            if (add_dig > 0) {
                char fmt[16];
                wsprintf(fmt, ".%%%02dd", add_dig);
                int div = 1;
                int x;
                for (x = 0; x < 3 - add_dig; x++) {
                    div *= 10;
                }
                wsprintf(buf + strlen(buf), fmt, (pos % 1000) / div);
            }

            int l = strlen(buf);
            if (l > maxlen) {
                l = maxlen;
            }
            memcpy(d, buf, l);
            maxlen -= l;
            d += l;

            p = endp + 1;
        } else if (!strnicmp(p, "$(title", 7) && (endp = strstr(p + 7, ")"))) {
            static char this_title[256];
            static unsigned int ltg;

            unsigned int now = GetTickCount();

            if (!this_title[0] || now - ltg > 1000 || now < ltg) {
                ltg = now;

                char* tpp;
                if (IsWindow(hwnd_WinampParent)) {
                    DWORD id;
                    if (!SendMessageTimeout(hwnd_WinampParent,
                                            WM_GETTEXT,
                                            (WPARAM)sizeof(this_title),
                                            (LPARAM)this_title,
                                            SMTO_BLOCK,
                                            50,
                                            &id)
                        || !id) {
                        this_title[0] = 0;
                    }
                }
                tpp = this_title + strlen(this_title);
                while (tpp >= this_title) {
                    char buf[9];
                    memcpy(buf, tpp, 8);
                    buf[8] = 0;
                    if (!lstrcmpi(buf, "- Winamp")) {
                        break;
                    }
                    tpp--;
                }
                if (tpp >= this_title) {
                    tpp--;
                }
                while (tpp >= this_title && *tpp == ' ') {
                    tpp--;
                }
                *++tpp = 0;
            }

            int skipnum = 1, max_fmtlen = 0;
            char* titleptr = this_title;

            if (p[7] == ':') {
                const char* ptr = p + 8;
                if (*ptr == 'n') {
                    ptr++;
                    skipnum = 0;
                }

                max_fmtlen = atoi(ptr);
            }

            // use: $(reg00), $(reg00:4.5), $(title), $(title:32), $(title:n32)

            if (skipnum && *titleptr >= '0' && *titleptr <= '9') {
                while (*titleptr >= '0' && *titleptr <= '9') {
                    titleptr++;
                }
                if (*titleptr == '.') {
                    titleptr++;
                    if (*titleptr == ' ') {
                        titleptr++;
                    } else {
                        titleptr = this_title;
                    }
                } else {
                    titleptr = this_title;
                }
            }
            int n = strlen(titleptr);
            if (n > maxlen) {
                n = maxlen;
            }
            if (max_fmtlen > 0 && max_fmtlen < n) {
                n = max_fmtlen;
            }

            memcpy(d, titleptr, n);
            maxlen -= n;
            d += n;
            p = endp + 1;
        } else
#endif  // CAN_TALK_TO_WINAMP
            if (!strnicmp(p, "$(reg", 5) && p[5] >= '0' && p[5] <= '9' && p[6] >= '0'
                && p[6] <= '9' && (endp = strstr(p + 7, ")"))) {
                char buf[128];
                char fmt[32];
                int wr = atoi(p + 5);
                if (wr < 0) {
                    wr = 0;
                }
                if (wr > 99) {
                    wr = 99;
                }
                p += 7;
                char* fmtptr = fmt;
                *fmtptr++ = '%';
                if (*p == ':') {
                    p++;
                    while (fmtptr - fmt < 16
                           && ((*p >= '0' && *p <= '9') || *p == '.')) {
                        *fmtptr++ = *p++;
                    }
                }
                *fmtptr++ = 'f';
                *fmtptr++ = 0;

                int l = sprintf(buf, fmt, NSEEL_getglobalregs()[wr]);
                if (l > maxlen) {
                    l = maxlen;
                }
                memcpy(d, buf, l);
                maxlen -= l;
                d += l;
                p = endp + 1;
            } else {
                *d++ = *p++;
                maxlen--;
            }
    }
    *d = 0;
}

// Returns number of words in buffer
static int getNWords(std::string buf) {
    const char* p = buf.c_str();
    int n = 0;
    while (p && *p) {
        if (*p == ';') {
            n++;
        }
        p++;
    }
    return n;
}

// TODO [clean]: Move to utils together with the ones from Texer2
struct RectI {
    int left;
    int top;
    int right;
    int bottom;
};

struct TextRenderInfo {
#ifdef _WIN32
    HDC context;
    HBITMAP bitmap;
    BITMAPINFO info;
    unsigned int valign;
    unsigned int halign;
    RECT r;
    unsigned int lines;
#endif
};

/* Win32 winuser.h constants used for legacy-preset position values:
DT_TOP     0  -> VPOS_TOP
DT_LEFT    0  -> HPOS_LEFT
DT_CENTER  1  -> HPOS_CENTER
DT_RIGHT   2  -> HPOS_RIGHT
DT_VCENTER 4  -> VPOS_CENTER
DT_BOTTOM  8  -> VPOS_BOTTOM
*/

int valign_to_dt(int valign) {
    switch (valign) {
        case VPOS_TOP: return 0;
        default:
        case VPOS_CENTER: return 4;
        case VPOS_BOTTOM: return 8;
    }
}
int halign_to_dt(int halign) {
    switch (halign) {
        case HPOS_LEFT: return 0;
        default:
        case HPOS_CENTER: return 1;
        case HPOS_RIGHT: return 2;
    }
}

void draw_text_buffer(const char* text,
                      uint64_t color,
                      uint64_t border,
                      uint64_t border_color,
                      size_t border_size,
                      void* buffer,
                      TextRenderInfo* render_info) {
#ifdef _WIN32
#define RGB_TO_BGR(color) \
    (((color) & 0xff0000) >> 16 | ((color) & 0xff00) | ((color) & 0xff) << 16)
#define BGR_TO_RGB(color) RGB_TO_BGR(color)  // is its own inverse

    HDC context = render_info->context;
    HBITMAP bitmap = render_info->bitmap;
    BITMAPINFO info = render_info->info;
    RECT r = render_info->r;
    int h = render_info->lines;

    int alignment =
        render_info->valign | render_info->halign | DT_NOCLIP | DT_SINGLELINE;
    size_t text_len = strlen(text);
    SetDIBits(context, bitmap, 0, h, (void*)buffer, &info, DIB_RGB_COLORS);
    if (border == TEXT_BORDER_OUTLINE) {
        SetTextColor(context, RGB_TO_BGR(border_color));
        r.left -= border_size;
        r.right -= border_size;
        r.top -= border_size;
        r.bottom -= border_size;
        DrawText(context, text, text_len, &r, alignment);
        r.left += border_size;
        r.right += border_size;
        DrawText(context, text, text_len, &r, alignment);
        r.left += border_size;
        r.right += border_size;
        DrawText(context, text, text_len, &r, alignment);
        r.top += border_size;
        r.bottom += border_size;
        DrawText(context, text, text_len, &r, alignment);
        r.top += border_size;
        r.bottom += border_size;
        DrawText(context, text, text_len, &r, alignment);
        r.left -= border_size;
        r.right -= border_size;
        DrawText(context, text, text_len, &r, alignment);
        r.left -= border_size;
        r.right -= border_size;
        DrawText(context, text, text_len, &r, alignment);
        r.top -= border_size;
        r.bottom -= border_size;
        DrawText(context, text, text_len, &r, alignment);
        r.left += border_size;
        r.right += border_size;
    } else if (border == TEXT_BORDER_SHADOW) {
        SetTextColor(context, RGB_TO_BGR(border_color));
        r.left += border_size;
        r.right += border_size;
        r.top += border_size;
        r.bottom += border_size;
        DrawText(context, text, text_len, &r, alignment);
        r.left -= border_size;
        r.right -= border_size;
        r.top -= border_size;
        r.bottom -= border_size;
    }
    SetTextColor(context, RGB_TO_BGR(color));
    DrawText(context, text, text_len, &r, alignment);
    GetDIBits(context, bitmap, 0, h, (void*)buffer, &info, DIB_RGB_COLORS);
#endif
}

int E_Text::render(char[2][2][576], int is_beat, int* framebuffer, int*, int w, int h) {
    int i, j;
    int *p, *d;
    int clipcolor;
    char thisText[256];

    if (updating) {
        return 0;
    }
    if (!enabled) {
        return 0;
    }
    if (is_beat & 0x80000000) {
        return 0;
    }

    if (forcealign) {
        forcealign = false;
        _halign = this->config.horizontal_align;
        _valign = this->config.vertical_align;
    }
    if (shiftinit) {
        shiftinit = 0;
        _xshift = this->config.shift_x;
        _yshift = this->config.shift_y;
    }

    // If not beat sensitive and time is up for this word
    // OR if beat sensitive and this frame is a beat and time is up for last beat
    if ((!this->config.on_beat && nf >= this->config.speed)
        || (this->config.on_beat && is_beat && !nb)) {
        // Then choose which word to show
        if (!(this->config.insert_blanks && !(oddeven % 2))) {
            if (this->config.random_word) {
                curword = rand() % (getNWords(this->config.text) + 1);
            } else {
                curword++;
                curword %= (getNWords(this->config.text) + 1);
            }
        }
        oddeven++;
        oddeven %= 2;
    }

    if (forceBeat) {
        is_beat = 1;
        forceBeat = 0;
    }

    // If beat sensitive and frame is a beat and last beat expired, start frame timer
    // for this beat
    if (this->config.on_beat && is_beat && !nb) {
        nb = this->config.on_beat_speed;
    }

    // Get the word(s) to show
    getWord(curword, thisText, 256);
    if (this->config.insert_blanks && !oddeven) {
        *thisText = 0;
    }

    // Same test as above but takes care of nb init
    if ((!this->config.on_beat && nf >= this->config.speed)
        || (this->config.on_beat && is_beat && nb == this->config.on_beat_speed)) {
        nf = 0;
        if (this->config.random_position && w && h)  // Handle random position
        {
            // Don't write outside the screen
#ifdef _WIN32
            SIZE size = {0, 0};
            GetTextExtentPoint32(hBitmapDC, thisText, strlen(thisText), &size);
            _halign = HPOS_LEFT;
            if (size.cx < w) {
                _xshift = rand() % (int)(((float)(w - size.cx) / (float)w) * 100.0F);
            }
            _valign = VPOS_TOP;
            if (size.cy < h) {
                _yshift = rand() % (int)(((float)(h - size.cy) / (float)h) * 100.0F);
            }
            forceshift = 1;
#endif            // _WIN32
        } else {  // Reset position to what is specified
            _halign = this->config.horizontal_align;
            _valign = this->config.vertical_align;
            _xshift = this->config.shift_x;
            _yshift = this->config.shift_y;
        }
    }

    // Choose cliping color
    if (this->config.color != 0 && this->config.border_color != 0) {
        clipcolor = 0;
    } else if (this->config.color != 0x000008
               && this->config.border_color != 0x000008) {
        clipcolor = 8;
    } else if (this->config.color != 0x0000010
               && this->config.border_color != 0x0000010) {
        clipcolor = 10;
    }

    RectI r;
    // If size changed or if we're forced to shift the buffer
    if ((lw != w || lh != h) || forceshift) {
        if (lw != w || lh != h) {
            // only if size changed reinit buffer, not if its only a forced shifting
            reinit(w, h);
        }
        forceshift = 0;
        forceredraw = 1;  // do redraw!
        // remember last state
        lw = w;
        lh = h;
        // Compute buffer position
        r.left = 0;
        r.right = w;
        r.top = 0;
        r.bottom = h;
        r.left += (int)((float)_xshift * (float)w / 100.0F);
        r.right += (int)((float)_xshift * (float)w / 100.0F);
        r.top += (int)((float)_yshift * (float)h / 100.0F);
        r.bottom += (int)((float)_yshift * (float)h / 100.0F);
        forceredraw = 1;
    }

    // Check if we need to redraw the buffer
    if (forceredraw || old_halign != _halign || old_valign != _valign
        || curword != old_curword || (*thisText && strcmp(thisText, oldtxt))
        || old_clipcolor != clipcolor
        || (old_blend1
            != ((this->config.blend_mode == BLEND_SIMPLE_ADDITIVE)
                && !(this->config.on_beat && !nb)))
        || (old_blend2
            != ((this->config.blend_mode == BLEND_SIMPLE_5050)
                && !(this->config.on_beat && !nb)))
        || (old_blend3
            != ((this->config.blend_mode == BLEND_SIMPLE_REPLACE)
                && !(this->config.on_beat && !nb)))
        || old_border_mode != this->config.border || _xshift != oldxshift
        || _yshift != oldyshift) {
        forceredraw = 0;
        old_halign = _halign;
        old_valign = _valign;
        old_curword = curword;
        strcpy(oldtxt, thisText);
        old_clipcolor = clipcolor;
        old_blend1 = ((this->config.blend_mode == BLEND_SIMPLE_ADDITIVE)
                      && !(this->config.on_beat && !nb));
        old_blend2 = ((this->config.blend_mode == BLEND_SIMPLE_5050)
                      && !(this->config.on_beat && !nb));
        old_blend3 = ((this->config.blend_mode == BLEND_SIMPLE_REPLACE)
                      && !(this->config.on_beat && !nb));
        old_border_mode = this->config.border;
        oldxshift = _xshift;
        oldyshift = _yshift;

        // Draw everything
        p = myBuffer;
        while (p < myBuffer + h * w) {
            *p = clipcolor;
            p++;
        }
        if (*thisText) {
            TextRenderInfo render_info = {
#ifdef _WIN32
                hBitmapDC,
                hRetBitmap,
                bi,
                valign_to_dt(_valign),
                halign_to_dt(_halign),
                RECT{r.left, r.top, r.right, r.bottom},
                h,
#endif  // _WIN32
            };
            draw_text_buffer(thisText,
                             this->config.color,
                             this->config.border,
                             this->config.border_color,
                             this->config.border_size,
                             myBuffer,
                             &render_info);
        }
    }

    // Now render the bitmap text buffer over framebuffer, handle blending options.
    // Separate blocks here so we don4t have to make w*h tests
    p = myBuffer;
    d = framebuffer + w * (h - 1);

    if (this->config.blend_mode == BLEND_SIMPLE_ADDITIVE
        && !(this->config.on_beat && !nb)) {
        for (i = 0; i < h; i++) {
            for (j = 0; j < w; j++) {
                if (*p != clipcolor) {
                    *d = BLEND(*p, *d);
                }
                d++;
                p++;
            }
            d -= w * 2;
        }
    } else if (this->config.blend_mode == BLEND_SIMPLE_5050
               && !(this->config.on_beat && !nb)) {
        for (i = 0; i < h; i++) {
            for (j = 0; j < w; j++) {
                if (*p != clipcolor) {
                    *d = BLEND_AVG(*p, *d);
                }
                d++;
                p++;
            }
            d -= w * 2;
        }
    } else if (!(this->config.on_beat && !nb)) {
        for (i = 0; i < h; i++) {
            for (j = 0; j < w; j++) {
                if (*p != clipcolor) {
                    *d = *p;
                }
                d++;
                p++;
            }
            d -= w * 2;
        }
    }

    // Advance frame counter
    if (!this->config.on_beat) {
        nf++;
    }
    // Decrease frametimer
    if (this->config.on_beat && nb) {
        nb--;
    }
    return 0;
}

void E_Text::load_legacy(unsigned char* data, int len) {
    int pos = 0;
    int size = 0;
    updating = true;
    forceredraw = 1;
    if (len - pos >= 4) {
        this->enabled = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        this->config.color = GET_INT();
        pos += 4;
    }
    this->config.blend_mode = BLEND_SIMPLE_REPLACE;
    if (len - pos >= 4) {
        if (GET_INT()) {
            this->config.blend_mode = BLEND_SIMPLE_ADDITIVE;
        }
        pos += 4;
    }
    if (len - pos >= 4) {
        if (GET_INT()) {
            this->config.blend_mode = BLEND_SIMPLE_5050;
        }
        pos += 4;
    }
    if (len - pos >= 4) {
        this->config.on_beat = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        this->config.insert_blanks = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        this->config.random_position = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        int dt_valign = GET_INT();
        switch (dt_valign) {
            case 0: this->config.vertical_align = VPOS_TOP; break;
            default:
            case 4: this->config.vertical_align = VPOS_CENTER; break;
            case 8: this->config.vertical_align = VPOS_BOTTOM; break;
        }
        pos += 4;
    }
    if (len - pos >= 4) {
        int dt_halign = GET_INT();
        switch (dt_halign) {
            case 0: this->config.horizontal_align = HPOS_LEFT; break;
            default:
            case 1: this->config.horizontal_align = HPOS_CENTER; break;
            case 2: this->config.horizontal_align = HPOS_RIGHT; break;
        }
        pos += 4;
    }
    if (len - pos >= 4) {
        this->config.on_beat_speed = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        this->config.speed = GET_INT();
        pos += 4;
    }
#ifdef _WIN32
    if (len - pos >= ssizeof32(cf)) {
        memcpy(&cf, data + pos, sizeof(cf));
        pos += sizeof(cf);
    }
    cf.lpLogFont = &lf;
    if (len - pos >= ssizeof32(lf)) {
        memcpy(&lf, data + pos, sizeof(lf));
        pos += sizeof(lf);
    }
    myFont = CreateFontIndirect(&lf);
#else
    pos += 60;  // sizeof(CHOOSEFONT);
    pos += 60;  // sizeof(LOGFONT);
#endif  // _WIN32
    if (len - pos >= 4) {
        char* str_data = (char*)data;
        pos += this->string_load_legacy(&str_data[pos], this->config.text, len - pos);
    }
    if (len - pos >= 4) {
        this->config.border = GET_INT() ? TEXT_BORDER_OUTLINE : TEXT_BORDER_NONE;
        pos += 4;
    }
    if (len - pos >= 4) {
        this->config.border_color = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        this->config.shift_x = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        this->config.shift_y = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        this->config.border_size = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        this->config.random_word = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        if (GET_INT() && !(this->config.border == TEXT_BORDER_OUTLINE)) {
            this->config.border = TEXT_BORDER_SHADOW;
        }
        pos += 4;
    }
    forcealign = 1;
    forceredraw = 1;
    forceshift = 1;
    shiftinit = 1;
    updating = false;
}

int E_Text::save_legacy(unsigned char* data) {
    int pos = 0;
    PUT_INT(this->enabled);
    pos += 4;
    PUT_INT(this->config.color);
    pos += 4;
    bool blend_additive = this->config.blend_mode == BLEND_SIMPLE_ADDITIVE;
    PUT_INT(blend_additive);
    pos += 4;
    bool blend_5050 = this->config.blend_mode == BLEND_SIMPLE_5050;
    PUT_INT(blend_5050);
    pos += 4;
    PUT_INT(this->config.on_beat);
    pos += 4;
    PUT_INT(this->config.insert_blanks);
    pos += 4;
    PUT_INT(this->config.random_position);
    pos += 4;
    PUT_INT(valign_to_dt(this->config.vertical_align));
    pos += 4;
    PUT_INT(halign_to_dt(this->config.horizontal_align));
    pos += 4;
    PUT_INT(this->config.on_beat_speed);
    pos += 4;
    PUT_INT(this->config.speed);
    pos += 4;
#ifdef _WIN32
    memcpy(data + pos, &cf, sizeof(cf));
    pos += sizeof(cf);
    memcpy(data + pos, &lf, sizeof(lf));
    pos += sizeof(lf);
#else
    memset(data + pos, 0, 60);  // sizeof(CHOOSEFONT);
    pos += 60;
    memset(data + pos, 0, 60);  // sizeof(LOGFONT);
    pos += 60;
#endif  // _WIN32
    char* str_data = (char*)data;
    pos += this->string_save_legacy(
        this->config.text, &str_data[pos], MAX_CODE_LEN - 1 - pos, /*with_nt*/ true);
    PUT_INT(this->config.border == TEXT_BORDER_OUTLINE);
    pos += 4;
    PUT_INT(this->config.border_color);
    pos += 4;
    PUT_INT(this->config.shift_x);
    pos += 4;
    PUT_INT(this->config.shift_y);
    pos += 4;
    PUT_INT(this->config.border_size);
    pos += 4;
    PUT_INT(this->config.random_word);
    pos += 4;
    PUT_INT(this->config.border == TEXT_BORDER_SHADOW);
    pos += 4;

    return pos;
}

Effect_Info* create_Text_Info() { return new Text_Info(); }
Effect* create_Text(AVS_Instance* avs) { return new E_Text(avs); }
void set_Text_desc(char* desc) { E_Text::set_desc(desc); }
