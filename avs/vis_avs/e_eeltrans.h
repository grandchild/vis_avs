#pragma once

#include "effect.h"
#include "effect_info.h"
#include "effect_programmable.h"

struct EelTrans_Config : public Effect_Config {
    std::string code = "";
};

struct EelTrans_Global_Config : public Effect_Config {
    bool translate_enabled = false;
    bool log_enabled = false;
    std::string log_path = "C:\\avslog";
    bool translate_firstlevel = false;
    bool read_comment_codes = true;
    bool need_new_first_instance = true;
};

struct EelTrans_Info : public Effect_Info {
    static constexpr char const* group = "Misc";
    static constexpr char const* name = "AVS Trans Automation";
    static constexpr char const* help = "";
    static constexpr int32_t legacy_id = -1;
    static constexpr char* legacy_ape_id = "Misc: AVSTrans Automation";

    static constexpr uint32_t num_parameters = 5;
    static constexpr Parameter parameters[num_parameters] = {
        P_BOOL_G(offsetof(EelTrans_Global_Config, log_enabled), "Log Enabled"),
        P_STRING_GX(offsetof(EelTrans_Global_Config, log_path), "Log File"),
        P_BOOL_G(offsetof(EelTrans_Global_Config, translate_firstlevel),
                 "Translate First Level"),
        P_BOOL_G(offsetof(EelTrans_Global_Config, read_comment_codes),
                 "Read Comment Codes"),
        P_STRING(offsetof(EelTrans_Config, code), "Code"),
    };

    EFFECT_INFO_GETTERS;
};

class E_EelTrans : public Configurable_Effect<EelTrans_Info,
                                              EelTrans_Config,
                                              EelTrans_Global_Config> {
   public:
    E_EelTrans(AVS_Instance* avs);
    virtual ~E_EelTrans();
    virtual int render(char visdata[2][2][576],
                       int is_beat,
                       int* framebuffer,
                       int* fbout,
                       int w,
                       int h);
    virtual void load_legacy(unsigned char* data, int len);
    virtual int save_legacy(unsigned char* data);
    virtual void on_enable(bool enabled);

    static char* pre_compile_hook(void* ctx, char* expression);
    static void post_compile_hook();

    bool is_first_instance;
};
