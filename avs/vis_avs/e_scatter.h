#pragma once

#include "effect.h"
#include "effect_info.h"

struct Scatter_Config : public Effect_Config {};

struct Scatter_Info : public Effect_Info {
    static constexpr char* group = "Trans";
    static constexpr char* name = "Scatter";
    static constexpr char* help = "A constantly changing fuzzy effect";
    static constexpr int32_t legacy_id = 16;
    static constexpr char* legacy_ape_id = NULL;

    EFFECT_INFO_GETTERS_NO_PARAMETERS;
};

class E_Scatter : public Configurable_Effect<Scatter_Info, Scatter_Config> {
   public:
    E_Scatter();
    virtual ~E_Scatter();
    virtual int render(char visdata[2][2][576],
                       int is_beat,
                       int* framebuffer,
                       int* fbout,
                       int w,
                       int h);
    virtual void load_legacy(unsigned char* data, int len);
    virtual int save_legacy(unsigned char* data);

    int fudgetable[512];
    int ftw;
};
