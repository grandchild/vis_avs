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
#include "c_list.h"
#include "c_unkn.h"

#include "r_defs.h"

#include "avs_eelif.h"
#include "c__base.h"
#include "effect.h"
#include "files.h"
#include "rlib.h"

#include "../util.h"

#define PUT_INT(y)                   \
    data[pos] = (y)&255;             \
    data[pos + 1] = (y >> 8) & 255;  \
    data[pos + 2] = (y >> 16) & 255; \
    data[pos + 3] = (y >> 24) & 255
#define GET_INT() \
    (data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (data[pos + 3] << 24))

void C_RBASE::load_string(std::string& s, unsigned char* data, int& pos, int len) {
    int size = GET_INT();
    pos += 4;
    if (size > 0 && len - pos >= size) {
        s.assign((char*)(data + pos), size);
        pos += size;
    } else {
        s.assign("");
    }
}
void C_RBASE::save_string(unsigned char* data, int& pos, std::string& text) {
    if (!text.empty()) {
        if (text.length() > 32768) {
            printf("string too long to fit in save format\n");
        }
        int l = text.length() + 1;
        PUT_INT(l);
        pos += 4;
        memcpy(data + pos, text.c_str(), l);
        pos += l;
    } else {
        PUT_INT(0);
        pos += 4;
    }
}

void C_RLibrary::add_dofx(void* rf,
                          Effect_Info* (*create_info)(void) = NULL,
                          Effect* (*create)(void) = NULL,
                          void (*set_legacy_desc)(char*) = NULL) {
    if ((NumRetrFuncs & 7) == 0 || !RetrFuncs) {
        void* newdl = (void*)malloc(sizeof(rfStruct) * (NumRetrFuncs + 8));
        if (!newdl) {
            printf("out of memory");
            exit(0);
        }
        memset(newdl, 0, sizeof(rfStruct) * (NumRetrFuncs + 8));
        if (RetrFuncs) {
            memcpy(newdl, RetrFuncs, NumRetrFuncs * sizeof(rfStruct));
            free(RetrFuncs);
        }
        RetrFuncs = (rfStruct*)newdl;
    }
    RetrFuncs[NumRetrFuncs].is_legacy = rf != NULL;
    *((void**)&RetrFuncs[NumRetrFuncs].create_legacy) = rf;
    RetrFuncs[NumRetrFuncs].create_info = create_info;
    RetrFuncs[NumRetrFuncs].create = create;
    RetrFuncs[NumRetrFuncs].set_legacy_desc = set_legacy_desc;
    NumRetrFuncs++;
}

// declarations for built-in effects
#define DECLARE_EFFECT_LEGACY(name)    \
    extern C_RBASE*(name)(char* desc); \
    add_dofx((void*)name);
// effect-api-capable effects
#define DECLARE_EFFECT_NEW(name)                \
    extern Effect_Info* create_##name##_Info(); \
    extern Effect* create_##name();             \
    extern void set_##name##_desc(char*);       \
    add_dofx(NULL, create_##name##_Info, create_##name, set_##name##_desc);

void C_RLibrary::initfx(void) {
    DECLARE_EFFECT_NEW(Simple);
    DECLARE_EFFECT_NEW(DotPlane);
    DECLARE_EFFECT_NEW(OscilloscopeStar);
    DECLARE_EFFECT_NEW(Fadeout);
    DECLARE_EFFECT_NEW(BlitterFeedback);
    DECLARE_EFFECT_LEGACY(R_NFClear);
    DECLARE_EFFECT_NEW(Blur);
    DECLARE_EFFECT_NEW(BassSpin);
    DECLARE_EFFECT_NEW(MovingParticle);
    DECLARE_EFFECT_NEW(RotoBlitter);
    DECLARE_EFFECT_NEW(SVP);
    DECLARE_EFFECT_NEW(Colorfade);
    DECLARE_EFFECT_NEW(ColorClip);
    DECLARE_EFFECT_NEW(RotStar);
    DECLARE_EFFECT_NEW(Ring);
    DECLARE_EFFECT_NEW(Movement);
    DECLARE_EFFECT_NEW(Scatter);
    DECLARE_EFFECT_NEW(DotGrid);
    DECLARE_EFFECT_NEW(BufferSave);
    DECLARE_EFFECT_NEW(DotFountain);
    DECLARE_EFFECT_NEW(Water);
    DECLARE_EFFECT_NEW(Comment);
    DECLARE_EFFECT_NEW(Brightness);
    DECLARE_EFFECT_NEW(Interleave);
    DECLARE_EFFECT_NEW(Grain);
    DECLARE_EFFECT_NEW(ClearScreen);
    DECLARE_EFFECT_NEW(Mirror);
    DECLARE_EFFECT_NEW(Starfield);
    DECLARE_EFFECT_LEGACY(R_Text);
    DECLARE_EFFECT_NEW(Bump);
    DECLARE_EFFECT_NEW(Mosaic);
    DECLARE_EFFECT_NEW(WaterBump);
    DECLARE_EFFECT_NEW(AVI);
    DECLARE_EFFECT_NEW(CustomBPM);
    DECLARE_EFFECT_NEW(Picture);
    DECLARE_EFFECT_NEW(DynamicDistanceModifier);
    DECLARE_EFFECT_NEW(SuperScope);
    DECLARE_EFFECT_NEW(Invert);
    DECLARE_EFFECT_NEW(UniqueTone);
    DECLARE_EFFECT_NEW(Timescope);
    DECLARE_EFFECT_NEW(SetRenderMode);
    DECLARE_EFFECT_NEW(Interferences);
    DECLARE_EFFECT_NEW(DynamicShift);
    DECLARE_EFFECT_NEW(DynamicMovement);
    DECLARE_EFFECT_NEW(FastBright);
    DECLARE_EFFECT_NEW(ColorModifier);
}

static const struct {
    char id[33];
    int newidx;
} NamedApeToBuiltinTrans[] = {{"Winamp Brightness APE v1", 22},
                              {"Winamp Interleave APE v1", 23},
                              {"Winamp Grain APE v1", 24},
                              {"Winamp ClearScreen APE v1", 25},
                              {"Nullsoft MIRROR v1", 26},
                              {"Winamp Starfield v1", 27},
                              {"Winamp Text v1", 28},
                              {"Winamp Bump v1", 29},
                              {"Winamp Mosaic v1", 30},
                              {"Winamp AVIAPE v1", 32},
                              {"Nullsoft Picture Rendering v1", 34},
                              {"Winamp Interf APE v1", 41}};

typedef C_RBASE* (*create_legacy_func)(char*);

void C_RLibrary::add_dll(create_legacy_func create_legacy,
                         char* ape_id,
                         Effect_Info* (*create_info)(void) = NULL,
                         Effect* (*create)(void) = NULL,
                         void (*set_legacy_desc)(char*) = NULL) {
    if ((NumDLLFuncs & 7) == 0 || !DLLFuncs) {
        DLLInfo* newdl = (DLLInfo*)malloc(sizeof(DLLInfo) * (NumDLLFuncs + 8));
        if (!newdl) {
            printf("out of memory\n");
            exit(0);
        }
        memset(newdl, 0, sizeof(DLLInfo) * (NumDLLFuncs + 8));
        if (DLLFuncs) {
            memcpy(newdl, DLLFuncs, NumDLLFuncs * sizeof(DLLInfo));
            free(DLLFuncs);
        }
        DLLFuncs = newdl;
    }
    DLLFuncs[NumDLLFuncs].is_legacy = create_legacy != NULL;
    DLLFuncs[NumDLLFuncs].idstring = ape_id;
    DLLFuncs[NumDLLFuncs].create_legacy = create_legacy;
    DLLFuncs[NumDLLFuncs].create_info = create_info;
    DLLFuncs[NumDLLFuncs].create = create;
    DLLFuncs[NumDLLFuncs].set_legacy_desc = set_legacy_desc;
    NumDLLFuncs++;
}

void C_RLibrary::initbuiltinape(void) {
#define ADD(name, ape_id)             \
    extern C_RBASE* name(char* desc); \
    this->add_dll(name, ape_id)
#define ADD_NEW(name, ape_id)                   \
    extern Effect_Info* create_##name##_Info(); \
    extern Effect* create_##name();             \
    extern void set_##name##_desc(char*);       \
    this->add_dll(NULL, ape_id, create_##name##_Info, create_##name, set_##name##_desc)

    ADD_NEW(ChannelShift, "Channel Shift");
    ADD_NEW(ColorReduction, "Color Reduction");
    ADD_NEW(Multiplier, "Multiplier");
    ADD_NEW(VideoDelay, "Holden04: Video Delay");
    ADD_NEW(MultiDelay, "Holden05: Multi Delay");
    ADD_NEW(Convolution, "Holden03: Convolution Filter");
    ADD_NEW(Texer2, "Acko.net: Texer II");
    ADD_NEW(Normalise, "Trans: Normalise");
    ADD_NEW(ColorMap, "Color Map");
    ADD_NEW(AddBorders, "Virtual Effect: Addborders");
    ADD_NEW(Triangle, "Render: Triangle");
    ADD_NEW(EelTrans, "Misc: AVSTrans Automation");
    ADD_NEW(GlobalVariables, "Jheriko: Global");
    ADD(R_MultiFilter, "Jheriko : MULTIFILTER");
    ADD_NEW(Picture2, "Picture II");
    ADD_NEW(FramerateLimiter, "VFX FRAMERATE LIMITER");
    ADD_NEW(Texer, "Texer");
#undef ADD
}

int C_RLibrary::GetRendererDesc(int which, char* str) {
    *str = 0;
    if (which >= 0 && which < NumRetrFuncs) {
        if (RetrFuncs[which].create_legacy != NULL) {
            RetrFuncs[which].create_legacy(str);
        } else {
            RetrFuncs[which].set_legacy_desc(str);
        }
        return 1;
    }
    if (which >= DLLRENDERBASE) {
        which -= DLLRENDERBASE;
        if (which < NumDLLFuncs) {
            if (DLLFuncs[which].create_legacy != NULL) {
                DLLFuncs[which].create_legacy(str);
            } else {
                DLLFuncs[which].set_legacy_desc(str);
            }
            return (int)DLLFuncs[which].idstring;
        }
    }
    return 0;
}

Legacy_Effect_Proxy C_RLibrary::CreateRenderer(int* which) {
    if (*which >= 0 && *which < NumRetrFuncs) {
        if (RetrFuncs[*which].is_legacy) {
            return Legacy_Effect_Proxy(
                RetrFuncs[*which].create_legacy(NULL), NULL, *which);
        } else {
            return Legacy_Effect_Proxy(NULL, RetrFuncs[*which].create(), *which);
        }
    }

    if (*which == LIST_ID) {
        return Legacy_Effect_Proxy((C_RBASE*)new C_RenderListClass(), NULL, *which);
    }

    if (*which >= DLLRENDERBASE) {
        int x;
        char* p = (char*)*which;
        for (x = 0; x < NumDLLFuncs; x++) {
            if (!DLLFuncs[x].create_legacy && !DLLFuncs[x].create) {
                break;
            }
            if (DLLFuncs[x].idstring) {
                if (!strncmp(p, DLLFuncs[x].idstring, 32)) {
                    *which = (int)DLLFuncs[x].idstring;
                    if (DLLFuncs[x].is_legacy) {
                        return Legacy_Effect_Proxy(
                            DLLFuncs[x].create_legacy(NULL), NULL, *which);
                    } else {
                        return Legacy_Effect_Proxy(NULL, DLLFuncs[x].create(), *which);
                    }
                }
            }
        }
        for (x = 0; x < ssizeof32(NamedApeToBuiltinTrans)
                            / ssizeof32(NamedApeToBuiltinTrans[0]);
             x++) {
            if (!strncmp(p, NamedApeToBuiltinTrans[x].id, 32)) {
                *which = NamedApeToBuiltinTrans[x].newidx;
                if (RetrFuncs[*which].is_legacy) {
                    return Legacy_Effect_Proxy(
                        RetrFuncs[*which].create_legacy(NULL), NULL, *which);
                } else {
                    return Legacy_Effect_Proxy(
                        NULL, RetrFuncs[*which].create(), *which);
                }
            }
        }
    }
    int r = *which;
    *which = UNKN_ID;
    C_UnknClass* p = new C_UnknClass();
    p->SetID(r, (r >= DLLRENDERBASE) ? (char*)r : (char*)"");
    return Legacy_Effect_Proxy(p, NULL, *which);
}

C_RLibrary::C_RLibrary() {
    DLLFuncs = NULL;
    NumDLLFuncs = 0;
    RetrFuncs = 0;
    NumRetrFuncs = 0;

    initfx();
    initbuiltinape();
}

C_RLibrary::~C_RLibrary() {
    if (RetrFuncs) {
        free(RetrFuncs);
    }
    RetrFuncs = 0;
    NumRetrFuncs = 0;

    if (DLLFuncs) {
        free(DLLFuncs);
    }
    DLLFuncs = NULL;
    NumDLLFuncs = 0;
}

void* g_n_buffers[NBUF];
int g_n_buffers_w[NBUF], g_n_buffers_h[NBUF];

void* getGlobalBuffer(int w, int h, int n, int do_alloc) {
    if (n < 0 || n >= NBUF) {
        return 0;
    }

    if (!g_n_buffers[n] || g_n_buffers_w[n] != w || g_n_buffers_h[n] != h) {
        if (g_n_buffers[n]) {
            free(g_n_buffers[n]);
        }
        if (do_alloc) {
            g_n_buffers_w[n] = w;
            g_n_buffers_h[n] = h;
            return g_n_buffers[n] = calloc(w * h, sizeof(int));
        }

        g_n_buffers[n] = NULL;
        g_n_buffers_w[n] = 0;
        g_n_buffers_h[n] = 0;

        return 0;
    }
    return g_n_buffers[n];
}
