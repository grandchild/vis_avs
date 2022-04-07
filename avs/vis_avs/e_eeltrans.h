#pragma once

#include "effect.h"
#include "effect_info.h"
#include "effect_programmable.h"

#include <set>

struct EelTrans_Config : public Effect_Config {
    bool log_enabled = false;
    std::string log_path = "C:\\avslog";
    bool translate_firstlevel = false;
    bool read_comment_codes = true;
    std::string code = "";
};

struct EelTrans_Info : public Effect_Info {
    static constexpr char* group = "Misc";
    static constexpr char* name = "AVS Trans Automation";
    static constexpr char* help = "";
    static constexpr int32_t legacy_id = -1;
    static constexpr char* legacy_ape_id = "Misc: AVSTrans Automation";

    static void apply_global(Effect*, const Parameter*, std::vector<int64_t>);
    static constexpr uint32_t num_parameters = 5;
    static constexpr Parameter parameters[num_parameters] = {
        P_BOOL(offsetof(EelTrans_Config, log_enabled),
               "Log Enabled",
               NULL,
               apply_global),
        P_STRING_X(offsetof(EelTrans_Config, log_path), "Log File", NULL, apply_global),
        P_BOOL(offsetof(EelTrans_Config, translate_firstlevel),
               "Translate First Level",
               NULL,
               apply_global),
        P_BOOL(offsetof(EelTrans_Config, read_comment_codes),
               "Read Comment Codes",
               NULL,
               apply_global),
        P_STRING(offsetof(EelTrans_Config, code), "Code", NULL),
    };

    EFFECT_INFO_GETTERS;
};

class E_EelTrans : public Configurable_Effect<EelTrans_Info, EelTrans_Config> {
   public:
    E_EelTrans();
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
    static std::string all_code();

    static bool translate_enabled;
    static bool log_enabled;
    static std::string log_path;
    static bool translate_firstlevel;
    static bool read_comment_codes;
    static bool need_new_first_instance;
    static std::set<E_EelTrans*> instances;

    bool is_first_instance;
};
