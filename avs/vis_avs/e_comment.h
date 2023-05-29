#pragma once

#include "effect.h"
#include "effect_info.h"

struct Comment_Config : public Effect_Config {
    std::string comment = "";
};

struct Comment_Info : public Effect_Info {
    static constexpr char* group = "Misc";
    static constexpr char* name = "Comment";
    static constexpr char* help = "";
    static constexpr int32_t legacy_id = 21;
    static constexpr char* legacy_ape_id = NULL;

    static constexpr uint32_t num_parameters = 1;
    static constexpr Parameter parameters[num_parameters] = {
        P_STRING(offsetof(Comment_Config, comment), "Comment"),
    };

    EFFECT_INFO_GETTERS;
};

class E_Comment : public Configurable_Effect<Comment_Info, Comment_Config> {
   public:
    E_Comment();
    virtual ~E_Comment();
    virtual int render(char visdata[2][2][576],
                       int is_beat,
                       int* framebuffer,
                       int* fbout,
                       int w,
                       int h);
    virtual void load_legacy(unsigned char* data, int len);
    virtual int save_legacy(unsigned char* data);

    std::string comment;
};
