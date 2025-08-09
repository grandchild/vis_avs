#pragma once

#include "effect_common.h"
#include "pixel_format.h"

#include <string>
#include <vector>

enum FontWeight {
    FONT_WEIGHT_DONTCARE = 0,
    FONT_WEIGHT_THIN = 100,
    FONT_WEIGHT_EXTRALIGHT = 200,
    FONT_WEIGHT_LIGHT = 300,
    FONT_WEIGHT_REGULAR = 400,
    FONT_WEIGHT_MEDIUM = 500,
    FONT_WEIGHT_SEMIBOLD = 600,
    FONT_WEIGHT_BOLD = 700,
    FONT_WEIGHT_EXTRABOLD = 800,
    FONT_WEIGHT_BLACK = 900,
};

enum FontFamily {
    FONT_FAMILY_DONTCARE = 0,
    FONT_FAMILY_ROMAN = 1,
    FONT_FAMILY_SWISS = 2,
    FONT_FAMILY_MODERN = 3,
    FONT_FAMILY_SCRIPT = 4,
    FONT_FAMILY_DECORATIVE = 5,
};

struct AVS_Font {
    std::string name;
    uint32_t weight = 0;
    uint32_t height = 0;
    bool italic = false;
    bool underline = false;
    bool strike_out = false;
    uint32_t char_set = 0;
    FontFamily family = FONT_FAMILY_DONTCARE;

    static std::vector<AVS_Font> get_fonts();
    void load();
    ~AVS_Font();

    void* platform_font = nullptr;
};

enum TextBorderMode {
    TEXT_BORDER_NONE = 0,
    TEXT_BORDER_OUTLINE = 1,
    TEXT_BORDER_SHADOW = 2,
};

struct TextPlatformContext;  // defined in the platform-specific implementation

class AVS_Text {
   public:
    AVS_Text() {}
    ~AVS_Text();
    void get_text_render_size(std::string string,
                              AVS_Font* font,
                              size_t w,
                              size_t h,
                              size_t* out_w,
                              size_t* out_h);
    void render(std::string string,
                AVS_Font* font,
                pixel_rgb0_8* buffer,
                size_t w,
                size_t h,
                Horizontal_Positions h_align,
                Vertical_Positions v_align,
                int32_t shift_x,
                int32_t shift_y,
                pixel_rgb0_8 color,
                TextBorderMode border,
                pixel_rgb0_8 border_color,
                uint32_t border_size);
    void unregister_font() { this->last_font = nullptr; }

   private:
    void reset(size_t w, size_t h);

    AVS_Font* last_font = nullptr;
    TextPlatformContext* context = nullptr;
    uint32_t last_w = 0;
    uint32_t last_h = 0;
};
