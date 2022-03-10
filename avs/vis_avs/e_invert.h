#pragma once

#include "effect.h"
#include "effect_info.h"

struct Invert_Config : public Effect_Config {};

struct Invert_Info : public Effect_Info {
    static constexpr char* group = "Trans";
    static constexpr char* name = "Invert";
    static constexpr char* help = "";
    static constexpr int32_t legacy_id = 37;
    static constexpr char* legacy_ape_id = NULL;

    static constexpr uint32_t num_parameters = 0;
    static constexpr Parameter parameters[num_parameters] = {

    };

    EFFECT_INFO_GETTERS;
};

class E_Invert : public Configurable_Effect<Invert_Info, Invert_Config> {
   public:
    E_Invert();
    virtual ~E_Invert();
    virtual int render(char visdata[2][2][576],
                       int isBeat,
                       int* framebuffer,
                       int* fbout,
                       int w,
                       int h);
    virtual void load_legacy(unsigned char* data, int len);
    virtual int save_legacy(unsigned char* data);
};
