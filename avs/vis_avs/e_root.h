#pragma once

#include "effect.h"
#include "effect_info.h"

#include <stdio.h>
#include <stdlib.h>

struct Root_Author_Config : public Effect_Config {
    std::string role = "author";
    std::string name;
};

struct Root_Config : public Effect_Config {
    bool clear = false;
    std::string name;
    std::string date;
    std::vector<Root_Author_Config> authors;
};

struct Root_Info : public Effect_Info {
    static constexpr char const* group = "";
    static constexpr char const* name = "Root";
    static constexpr char const* help =
        "The main component of the preset\n"
        "Define basic settings for the preset here\n";
    static constexpr int32_t legacy_id = -4;
    static constexpr char const* legacy_ape_id = nullptr;

    static constexpr uint32_t num_author_parameters = 2;
    static constexpr Parameter author_parameters[num_author_parameters] = {
        P_STRING(offsetof(Root_Author_Config, name), "Name", "Name of the author"),
        P_STRING(offsetof(Root_Author_Config, role),
                 "Role",
                 "'author', 'remixer', 'editor', 'curator' - or anything else")};

    static constexpr uint32_t num_parameters = 4;
    static constexpr Parameter parameters[num_parameters] = {
        P_BOOL(offsetof(Root_Config, clear),
               "Clear",
               "Clear the screen for every new frame"),
        P_STRING(offsetof(Root_Config, name), "Name", "Name of the preset"),
        P_STRING(offsetof(Root_Config, date), "Date", "Date of the preset"),
        P_LIST<Root_Author_Config>(offsetof(Root_Config, authors),
                                   "Authors",
                                   author_parameters,
                                   num_author_parameters,
                                   0,
                                   0,
                                   "Authors of the preset"),
    };

    virtual bool can_have_child_components() const { return true; }
    virtual bool is_createable_by_user() const { return false; }
    EFFECT_INFO_GETTERS;
};

class E_Root : public Configurable_Effect<Root_Info, Root_Config> {
   public:
    E_Root(AVS_Instance* avs);
    virtual ~E_Root();
    virtual int render(char visdata[2][2][576],
                       int is_beat,
                       int* framebuffer,
                       int* fbout,
                       int w,
                       int h);
    virtual void render_with_context(RenderContext& ctx);
    virtual void load_legacy(unsigned char* data, int len);
    virtual int save_legacy(unsigned char* data);
    virtual E_Root* clone() { return new E_Root(*this); }
    int64_t get_num_renders() { return this->children.size(); }
    void start_buffer_context();
    void end_buffer_context();

   private:
#define NBUF 8
    // these are our framebuffers (formerly nb_save)
    int buffers_w[NBUF] = {};
    int buffers_h[NBUF] = {};
    void* buffers[NBUF] = {};

    // these are temp space for saving the global ones (formerly nb_save2)
    int buffers_temp_w[NBUF] = {};
    int buffers_temp_h[NBUF] = {};
    void* buffers_temp[NBUF] = {};

    bool buffers_saved;
};
