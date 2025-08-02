#pragma once

#include "effect.h"
#include "effect_common.h"
#include "effect_info.h"

#ifdef _WIN32
#include "windows.h"
#endif

enum TEXT_BORDER_MODE {
    TEXT_BORDER_NONE = 0,
    TEXT_BORDER_OUTLINE = 1,
    TEXT_BORDER_SHADOW = 2,
};

enum TEXT_FONT_WEIGHT {
    TEXT_FW_DONTCARE = 0,
    TEXT_FW_THIN = 100,
    TEXT_FW_EXTRALIGHT,
    TEXT_FW_LIGHT,
    TEXT_FW_REGULAR,
    TEXT_FW_MEDIUM,
    TEXT_FW_SEMIBOLD,
    TEXT_FW_BOLD,
    TEXT_FW_EXTRABOLD,
    TEXT_FW_BLACK,
};

enum TEXT_FONT_FAMILY {
    TEXT_FAMILY_DONTCARE = 0,
    TEXT_FAMILY_ROMAN = 1,
    TEXT_FAMILY_SWISS = 2,
    TEXT_FAMILY_MODERN = 3,
    TEXT_FAMILY_SCRIPT = 4,
    TEXT_FAMILY_DECORATIVE = 5,
};

struct Text_Config : public Effect_Config {
    uint64_t color = 0xffffff;
    int64_t blend_mode = BLEND_SIMPLE_REPLACE;
    bool on_beat = false;
    bool insert_blanks = false;
    bool random_position = false;
    int64_t vertical_align = VPOS_CENTER;
    int64_t horizontal_align = HPOS_CENTER;
    int64_t on_beat_speed = 15;
    int64_t speed = 15;
    int64_t weight = 0;
    int64_t height = 0;
    int64_t width = 0;
    bool italic = false;
    bool underline = false;
    bool strike_out = false;
    int64_t char_set = 0;
    int64_t family = TEXT_FAMILY_DONTCARE;
    std::string font_name;
    std::string text;
    int64_t border = TEXT_BORDER_NONE;
    uint64_t border_color = 0xffffff;
    int64_t border_size = 1;
    int64_t shift_x = 0;
    int64_t shift_y = 0;
    bool random_word = false;
};

struct Text_Info : public Effect_Info {
    static constexpr char const* group = "Render";
    static constexpr char const* name = "Text";
    static constexpr char const* help = "";
    static constexpr int32_t legacy_id = 28;
    static constexpr char const* legacy_ape_id = nullptr;

    static const char* const* weights(int64_t* length_out) {
        *length_out = 10;
        static const char* const options[10] = {
            "Dont Care",
            "Thin",
            "Extralight",
            "Light",
            "Regular",
            "Medium",
            "Semibold",
            "Bold",
            "Extrabold",
            "Black",
        };
        return options;
    }

    static const char* const* families(int64_t* length_out) {
        *length_out = 6;
        static const char* const options[6] = {
            "Dont Care",
            "Roman",
            "Swiss",
            "Modern",
            "Script",
            "Decorative",
        };
        return options;
    }

    static const char* const* border_modes(int64_t* length_out) {
        *length_out = 3;
        static const char* const options[3] = {
            "None",
            "Outline",
            "Shadow",
        };
        return options;
    }

    static void callback(Effect*, const Parameter*, const std::vector<int64_t>&);

    static constexpr uint32_t num_parameters = 25;
    static constexpr Parameter parameters[num_parameters] = {
        P_COLOR(offsetof(Text_Config, color), "Color"),
        P_SELECT(offsetof(Text_Config, blend_mode), "Blend Mode", blend_modes_simple),
        P_BOOL(offsetof(Text_Config, on_beat), "On Beat"),
        P_BOOL(offsetof(Text_Config, insert_blanks), "Insert Blanks"),
        P_BOOL(offsetof(Text_Config, random_position), "Random Position"),
        P_SELECT(offsetof(Text_Config, vertical_align), "Vertical Align", v_positions),
        P_SELECT(offsetof(Text_Config, horizontal_align),
                 "Horizontal Align",
                 h_positions),
        P_IRANGE(offsetof(Text_Config, on_beat_speed), "On Beat Speed", 1, 400),
        P_IRANGE(offsetof(Text_Config, speed), "Speed", 1, 400),
        P_SELECT(offsetof(Text_Config, weight), "Weight", weights),
        P_IRANGE(offsetof(Text_Config, height), "Height"),
        P_IRANGE(offsetof(Text_Config, width), "Width"),
        P_BOOL(offsetof(Text_Config, italic), "Italic"),
        P_BOOL(offsetof(Text_Config, underline), "Underline"),
        P_BOOL(offsetof(Text_Config, strike_out), "Strike Out"),
        P_IRANGE(offsetof(Text_Config, char_set), "Character Set", 0, INT64_MAX),
        P_SELECT(offsetof(Text_Config, family), "Font Family", families),
        P_STRING(offsetof(Text_Config, font_name), "Font Name"),
        P_STRING(offsetof(Text_Config, text), "Text"),
        P_SELECT(offsetof(Text_Config, border), "Border", border_modes),
        P_COLOR(offsetof(Text_Config, border_color), "Border Color"),
        P_IRANGE(offsetof(Text_Config, border_size), "Border Size", 1, 16),
        P_IRANGE(offsetof(Text_Config, shift_x), "Shift X", 0, 200),
        P_IRANGE(offsetof(Text_Config, shift_y), "Shift Y", 0, 200),
        P_BOOL(offsetof(Text_Config, random_word), "Random Word"),
    };

    EFFECT_INFO_GETTERS;
};

class E_Text : public Configurable_Effect<Text_Info, Text_Config> {
   public:
    E_Text(AVS_Instance* avs);
    virtual ~E_Text();
    virtual int render(char visdata[2][2][576],
                       int is_beat,
                       int* framebuffer,
                       int* fbout,
                       int w,
                       int h);
    virtual void load_legacy(unsigned char* data, int len);
    virtual int save_legacy(unsigned char* data);
    virtual E_Text* clone() { return new E_Text(*this); }

    void reinit(int w, int h);
    void getWord(int n, char* buf, int max);

#ifdef _WIN32
    CHOOSEFONT cf;
    LOGFONT lf;
    HBITMAP hRetBitmap;
    HBITMAP hOldBitmap;
    HFONT hOldFont;
    HFONT myFont;
    HDC hDesktopDC;
    HDC hBitmapDC;
    BITMAPINFO bi;
#endif

    int lw, lh;
    bool updating;
    int shadow;
    int outlinecolor;
    int outlinesize;
    int curword;
    int nb;
    int forceBeat;
    int _xshift;
    int _yshift;
    int forceshift;
    int forcealign;
    int _valign, _halign;
    int oddeven;
    int nf;
    int* myBuffer;
    int forceredraw;
    int old_valign, old_halign;
    int old_border_mode;
    int old_curword, old_clipcolor;
    char oldtxt[256];
    int old_blend1, old_blend2, old_blend3;
    int oldxshift, oldyshift;
    int randomword;
    int shiftinit;
};
