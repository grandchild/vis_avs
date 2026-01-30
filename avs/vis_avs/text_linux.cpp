#include "text.h"

struct TextPlatformContext {};

std::vector<AVS_Font> AVS_Font::get_fonts() { return std::vector<AVS_Font>(); }

void AVS_Font::load() {}

AVS_Font::~AVS_Font() {}

AVS_Text::~AVS_Text() {}

void AVS_Text::reset(size_t, size_t) {}

void AVS_Text::get_text_render_size(std::string,
                                    AVS_Font*,
                                    size_t,
                                    size_t,
                                    size_t*,
                                    size_t*) {}

void AVS_Text::render(std::string,
                      AVS_Font*,
                      pixel_rgb0_8*,
                      size_t,
                      size_t,
                      Horizontal_Positions,
                      Vertical_Positions,
                      int32_t,
                      int32_t,
                      pixel_rgb0_8,
                      TextBorderMode,
                      pixel_rgb0_8,
                      uint32_t) {}
