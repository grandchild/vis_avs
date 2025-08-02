#include "text.h"

#include <windows.h>

int family_to_win32_family(FontFamily family) {
    switch (family) {
        default:
        case FONT_FAMILY_DONTCARE: return 0;
        case FONT_FAMILY_ROMAN: return 1;
        case FONT_FAMILY_SWISS: return 2;
        case FONT_FAMILY_MODERN: return 3;
        case FONT_FAMILY_SCRIPT: return 4;
        case FONT_FAMILY_DECORATIVE: return 5;
    }
}

FontFamily win32_pitchfamily_to_family(int pitch_and_family) {
    switch (pitch_and_family >> 4) {
        default:
        case 0: return FONT_FAMILY_DONTCARE;
        case 1: return FONT_FAMILY_ROMAN;
        case 2: return FONT_FAMILY_SWISS;
        case 3: return FONT_FAMILY_MODERN;
        case 4: return FONT_FAMILY_SCRIPT;
        case 5: return FONT_FAMILY_DECORATIVE;
    }
}

uint32_t valign_to_dt(Vertical_Positions valign) {
    switch (valign) {
        case VPOS_TOP: return DT_TOP;
        default:
        case VPOS_CENTER: return DT_VCENTER;
        case VPOS_BOTTOM: return DT_BOTTOM;
    }
}

uint32_t halign_to_dt(Horizontal_Positions halign) {
    switch (halign) {
        case HPOS_LEFT: return DT_LEFT;
        default:
        case HPOS_CENTER: return DT_CENTER;
        case HPOS_RIGHT: return DT_RIGHT;
    }
}

AVS_Font avs_font_from_logfont(const LOGFONT* logfont) {
    if (logfont == nullptr) {
        return {};
    }
    return {
        logfont->lfFaceName,
        (uint32_t)logfont->lfWeight,
        (uint32_t)logfont->lfHeight,
        (bool)logfont->lfItalic,
        (bool)logfont->lfUnderline,
        (bool)logfont->lfStrikeOut,
        (uint32_t)logfont->lfCharSet,
        win32_pitchfamily_to_family(logfont->lfPitchAndFamily),
        (void*)logfont,
    };
}

int CALLBACK enum_font_fam_ex_proc(const LOGFONT* logfont,
                                   const TEXTMETRIC*,
                                   DWORD,
                                   LPARAM font_list) {
    ((std::vector<AVS_Font>*)font_list)->push_back(avs_font_from_logfont(logfont));
    return 1;  // return nonzero to continue enumerating more fonts
}

std::vector<AVS_Font> AVS_Font::get_fonts() {
    static std::vector<AVS_Font> font_list = {};

    LOGFONT match_all_fonts = {};
    // TODO [compat]: Find out if using ANSI_CHARSETS excludes fonts. Setting lfCharSet
    // to DEFAULT_CHARSET matches every font several times for all available charsets.
    // This is creates a lot of duplicates since AVS_Font doesn't have the concept of
    // "char sets".
    match_all_fonts.lfCharSet = ANSI_CHARSET;
    match_all_fonts.lfPitchAndFamily = 0;
    match_all_fonts.lfFaceName[0] = '\0';
    EnumFontFamiliesEx(
        GetDC(nullptr), &match_all_fonts, enum_font_fam_ex_proc, (LPARAM)&font_list, 0);

    return font_list;
}

struct TextPlatformContext {
    HDC device = nullptr;
    HBITMAP bitmap = nullptr;
    BITMAPINFO bm_info;
    RECT rect;
    HBITMAP previous_bitmap = nullptr;
    HFONT previous_font = nullptr;

    TextPlatformContext(size_t w, size_t h, HFONT font);
    ~TextPlatformContext();
};

TextPlatformContext::TextPlatformContext(size_t w, size_t h, HFONT font)
    : device(CreateCompatibleDC(nullptr)),
      bitmap(CreateCompatibleBitmap(this->device, w, h)) {
    this->previous_bitmap = (HBITMAP)SelectObject(this->device, this->bitmap);
    this->previous_font = (HFONT)SelectObject(this->device, font);
    this->bm_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    this->bm_info.bmiHeader.biWidth = 1;
    this->bm_info.bmiHeader.biHeight = 1;
    this->bm_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    this->bm_info.bmiHeader.biWidth = w;
    this->bm_info.bmiHeader.biHeight = h;
    this->bm_info.bmiHeader.biPlanes = 1;
    this->bm_info.bmiHeader.biBitCount = 32;  // ARGB
    this->bm_info.bmiHeader.biCompression = BI_RGB;
    this->rect.left = 0;
    this->rect.top = 0;
    this->rect.right = w;
    this->rect.bottom = h;
    SetBkMode(this->device, TRANSPARENT);
    SetBkColor(this->device, 0);
}

TextPlatformContext::~TextPlatformContext() {
    if (this->bitmap != nullptr) {
        DeleteObject(this->bitmap);
    }
    if (this->previous_bitmap) {
        SelectObject(this->device, this->previous_bitmap);
    }
    if (this->previous_font) {
        SelectObject(this->device, this->previous_font);
    }
    if (this->device != nullptr) {
        DeleteDC(this->device);
    }
}

AVS_Text::~AVS_Text() { delete this->context; }

void AVS_Text::get_text_render_size(std::string string,
                                    uint32_t* out_w,
                                    uint32_t* out_h) {
    if (out_w == nullptr || out_h == nullptr) {
        return;
    }
    SIZE size = {0, 0};
    GetTextExtentPoint32(this->context->device, string.c_str(), string.length(), &size);
    *out_w = size.cx;
    *out_h = size.cy;
}

#define RGB_TO_BGR(color) \
    (((color) & 0xff0000) >> 16 | ((color) & 0xff00) | ((color) & 0xff) << 16)

void AVS_Text::render(std::string string,
                      pixel_rgb0_8* buffer,
                      size_t w,
                      size_t h,
                      Horizontal_Positions h_align,
                      Vertical_Positions v_align,
                      pixel_rgb0_8 color,
                      TextBorderMode border,
                      pixel_rgb0_8 border_color,
                      size_t border_size) {
    (void)w;
    if (string.empty() || buffer == nullptr) {
        return;
    }
    if (this->last_h != h || this->last_w != w) {
        this->reset(w, h);
        this->last_w = w;
        this->last_h = h;
    }

    auto device = this->context->device;
    auto bitmap = this->context->bitmap;
    auto bm_info = this->context->bm_info;
    auto rect = this->context->rect;

    int alignment =
        valign_to_dt(v_align) | halign_to_dt(h_align) | DT_NOCLIP | DT_SINGLELINE;
    SetDIBits(device, bitmap, 0, h, (void*)buffer, &bm_info, DIB_RGB_COLORS);
    switch (border) {
        case TEXT_BORDER_OUTLINE:
            SetTextColor(device, RGB_TO_BGR(border_color));
            rect.left -= border_size;
            rect.right -= border_size;
            rect.top -= border_size;
            rect.bottom -= border_size;
            DrawText(device, string.c_str(), string.length(), &rect, alignment);
            rect.left += border_size;
            rect.right += border_size;
            DrawText(device, string.c_str(), string.length(), &rect, alignment);
            rect.left += border_size;
            rect.right += border_size;
            DrawText(device, string.c_str(), string.length(), &rect, alignment);
            rect.top += border_size;
            rect.bottom += border_size;
            DrawText(device, string.c_str(), string.length(), &rect, alignment);
            rect.top += border_size;
            rect.bottom += border_size;
            DrawText(device, string.c_str(), string.length(), &rect, alignment);
            rect.left -= border_size;
            rect.right -= border_size;
            DrawText(device, string.c_str(), string.length(), &rect, alignment);
            rect.left -= border_size;
            rect.right -= border_size;
            DrawText(device, string.c_str(), string.length(), &rect, alignment);
            rect.top -= border_size;
            rect.bottom -= border_size;
            DrawText(device, string.c_str(), string.length(), &rect, alignment);
            rect.left += border_size;
            rect.right += border_size;
            break;
        case TEXT_BORDER_SHADOW:
            SetTextColor(device, RGB_TO_BGR(border_color));
            rect.left += border_size;
            rect.right += border_size;
            rect.top += border_size;
            rect.bottom += border_size;
            DrawText(device, string.c_str(), string.length(), &rect, alignment);
            rect.left -= border_size;
            rect.right -= border_size;
            rect.top -= border_size;
            rect.bottom -= border_size;
            break;
        default: break;
    }
    SetTextColor(device, RGB_TO_BGR(color));
    DrawText(device, string.c_str(), string.length(), &rect, alignment);
    GetDIBits(device, bitmap, 0, h, (void*)buffer, &bm_info, DIB_RGB_COLORS);
}

void AVS_Text::reset(size_t w, size_t h) {
    auto old_context = this->context;
    this->context = new TextPlatformContext(w, h, (HFONT)this->font.platform_font);
    if (old_context != nullptr) {
        delete old_context;
    }
}
