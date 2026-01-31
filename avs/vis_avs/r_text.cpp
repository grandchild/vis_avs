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

#include "avs_eelif.h"
#include "blend.h"
#include "constants.h"  // MAX_CODE_LEN

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

#define SIZEOF_CHOOSEFONT 60
#define SIZEOF_LOGFONT    60
#define FONT_NAME_MAX_LEN 32

constexpr Parameter Text_Info::parameters[];

void Text_Info::redraw(Effect* component,
                       const Parameter*,
                       const std::vector<int64_t>&) {
    auto text = (E_Text*)component;
    text->redraw();
}
void Text_Info::on_shift(Effect* component,
                         const Parameter*,
                         const std::vector<int64_t>&) {
    auto text = (E_Text*)component;
    text->on_shift();
}
void Text_Info::on_onbeatduration(Effect* component,
                                  const Parameter*,
                                  const std::vector<int64_t>&) {
    auto text = (E_Text*)component;
    text->on_onbeatduration();
}
void Text_Info::on_font(Effect* component,
                        const Parameter*,
                        const std::vector<int64_t>&) {
    auto text = (E_Text*)component;
    text->on_font();
}

void E_Text::reinit(int w, int h) {
    free(myBuffer);
    myBuffer = (int*)malloc(w * h * 4);
}

E_Text::E_Text(AVS_Instance* avs)
    : Configurable_Effect(avs), font(nullptr), text(new AVS_Text()) {
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

    lw = lh = 0;
    r = {0, 0, 0, 0};
    updating = false;
    nb = 0;
    oddeven = 0;
    nf = 0;
    shiftinit = 1;
}

E_Text::~E_Text() { free(myBuffer); }

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

uint32_t valign_to_dt(int valign) {
    switch (valign) {
        case VPOS_TOP: return 0;
        default:
        case VPOS_CENTER: return 4;
        case VPOS_BOTTOM: return 8;
    }
}
uint32_t halign_to_dt(int halign) {
    switch (halign) {
        case HPOS_LEFT: return 0;
        default:
        case HPOS_CENTER: return 1;
        case HPOS_RIGHT: return 2;
    }
}

int config_family_to_font_family(FontFamily family) {
    switch (family) {
        default:
        case FONT_FAMILY_DONTCARE: return 0;
        case FONT_FAMILY_SERIF: return 1;
        case FONT_FAMILY_SANSSERIF: return 2;
        case FONT_FAMILY_MONOSPACE: return 3;
        case FONT_FAMILY_CURSIVE: return 4;
        case FONT_FAMILY_FANTASY: return 5;
    }
}
FontFamily font_pitchfamily_to_config_family(int pitch_and_family) {
    switch (pitch_and_family >> 4) {
        default:
        case 0: return FONT_FAMILY_DONTCARE;
        case 1: return FONT_FAMILY_SERIF;
        case 2: return FONT_FAMILY_SANSSERIF;
        case 3: return FONT_FAMILY_MONOSPACE;
        case 4: return FONT_FAMILY_CURSIVE;
        case 5: return FONT_FAMILY_FANTASY;
    }
}

void E_Text::redraw() {
    this->forceredraw = 1;
    this->forceshift = 1;
}

void E_Text::on_shift() {
    this->forceredraw = 1;
    this->forceshift = 1;
    this->shiftinit = 1;
}

void E_Text::on_onbeatduration() {
    if (this->nb > this->config.on_beat_duration) {
        this->nb = this->config.on_beat_duration;
    }
}

void E_Text::on_font() {
    if (this->font) {
        delete this->font;
        this->text->unregister_font();
    }
    this->font = new AVS_Font{
        this->config.font_name,
        (uint32_t)this->config.weight * 100,
        (uint32_t)this->config.height,
        this->config.italic,
        this->config.underline,
        this->config.strike_out,
        (uint32_t)this->config.char_set,
        font_pitchfamily_to_config_family(this->config.family),
    };
}

int E_Text::render(char[2][2][576], int is_beat, int* framebuffer, int*, int w, int h) {
    int i, j;
    int clipcolor;
    char thisText[256];

    if (updating) {
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
    if ((!this->config.on_beat && nf >= this->config.duration)
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
        // forceshift = 1;
    }

    if (forceBeat) {
        is_beat = 1;
        forceBeat = 0;
    }

    // If beat sensitive and frame is a beat and last beat expired, start frame timer
    // for this beat
    if (this->config.on_beat && is_beat && !nb) {
        nb = this->config.on_beat_duration;
    }

    // Get the word(s) to show
    getWord(curword, thisText, 256);
    if (this->config.insert_blanks && !oddeven) {
        *thisText = 0;
    }

    // Same test as above but takes care of nb init
    if ((!this->config.on_beat && nf >= this->config.duration)
        || (this->config.on_beat && is_beat && nb == this->config.on_beat_duration)) {
        nf = 0;
        if (this->config.random_position && w && h)  // Handle random position
        {
            // Don't write outside the screen
            size_t text_w = 0;
            size_t text_h = 0;
            this->text->get_text_render_size(
                thisText, this->font, w, h, &text_w, &text_h);
            _halign = HPOS_LEFT;
            if (text_w < w) {
                _xshift = rand() % (int)(((float)(w - text_w) / (float)w) * 100.0F);
            }
            _valign = VPOS_TOP;
            if (text_h < h) {
                _yshift = rand() % (int)(((float)(h - text_h) / (float)h) * 100.0F);
            }
            forceshift = 1;
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
        int* p = myBuffer;
        while (p < myBuffer + h * w) {
            *p = clipcolor;
            p++;
        }
        if (*thisText) {
            this->text->render(thisText,
                               this->font,
                               (pixel_rgb0_8*)myBuffer,
                               w,
                               h,
                               (Horizontal_Positions)_halign,
                               (Vertical_Positions)_valign,
                               _xshift,
                               _yshift,
                               this->config.color,
                               (TextBorderMode)this->config.border,
                               this->config.border_color,
                               this->config.border_size);
        }
    }

    // Now render the bitmap text buffer over framebuffer, handle blending options.
    // Separate blocks here so we don4t have to make w*h tests
    uint32_t* p = (uint32_t*)myBuffer;
    uint32_t* d = (uint32_t*)framebuffer + w * (h - 1);

    if (this->config.blend_mode == BLEND_SIMPLE_ADDITIVE
        && !(this->config.on_beat && !nb)) {
        for (i = 0; i < h; i++) {
            for (j = 0; j < w; j++) {
                if (*p != clipcolor) {
                    blend_add_1px(p, d, d);
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
                    blend_5050_1px(p, d, d);
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
                    blend_replace_1px(p, d);
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

void E_Text::on_load() { this->on_font(); }

void E_Text::load_legacy(unsigned char* data, int len) {
    char* str_data = (char*)data;
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
        this->config.on_beat_duration = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        this->config.duration = GET_INT();
        pos += 4;
    }
    if (len - pos >= SIZEOF_CHOOSEFONT) {
        pos += SIZEOF_CHOOSEFONT;
    }
    if (len - pos >= SIZEOF_LOGFONT) {
        this->config.weight = *(int32_t*)&data[pos + 16] / 100;
        this->config.height = abs(*(int32_t*)&data[pos]);
        this->config.width = *(int32_t*)&data[pos + 4];
        this->config.italic = data[pos + 20];
        this->config.underline = data[pos + 21];
        this->config.strike_out = data[pos + 22];
        this->config.family = font_pitchfamily_to_config_family(data[pos + 27]);
        this->config.font_name = &str_data[pos + 28];
        pos += SIZEOF_LOGFONT;
        this->on_font();
    }
    if (len - pos >= 4) {
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
    PUT_INT(this->config.on_beat_duration);
    pos += 4;
    PUT_INT(this->config.duration);
    pos += 4;
    memset(data + pos, 0, SIZEOF_CHOOSEFONT);
    pos += SIZEOF_CHOOSEFONT;

    // memcpy(data + pos, &this->lf, SIZEOF_LOGFONT);
    *(int32_t*)&data[pos] = -this->config.height;  // height is negative
    pos += 4;
    *(int32_t*)&data[pos] = this->config.width;
    pos += 4;
    pos += 4 + 4;  // lfEscapement + lfOrientation
    *(int32_t*)&data[pos] = this->config.weight * 100;
    pos += 4;
    data[pos++] = this->config.italic;
    data[pos++] = this->config.underline;
    data[pos++] = this->config.strike_out;
    data[pos++] = this->config.char_set;
    pos += 1 + 1 + 1;  // lfOutPrecision + lfClipPrecision + lfQuality
    data[pos++] = config_family_to_font_family((FontFamily)this->config.family) << 4;
    strncpy((char*)&data[pos], this->config.font_name.c_str(), FONT_NAME_MAX_LEN);
    data[pos + FONT_NAME_MAX_LEN - 1] = '\0';
    pos += FONT_NAME_MAX_LEN;

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
