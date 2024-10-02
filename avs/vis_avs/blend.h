#include <stdint.h>

void make_blend_LUTs();

// The following SIMD modes may be targeted by a build variable definition. If a SIMD
// implementation is not available, the C fallback (SIMD_MODE_NONE) version will be
// used.
//
//   SIMD_MODE_NONE
//   SIMD_MODE_X86_SSE

#define ARGS_SRC_DEST                const uint32_t *src, uint32_t *dest
#define ARGS_SRC_DEST_WH             ARGS_SRC_DEST, uint32_t w, uint32_t h
#define blend_replace_1px(src, dest) (*(dest) = *(src))
void blend_replace(ARGS_SRC_DEST_WH);
void blend_add_1px(ARGS_SRC_DEST);
void blend_add(ARGS_SRC_DEST_WH);
void blend_5050_1px(ARGS_SRC_DEST);
void blend_5050(ARGS_SRC_DEST_WH);
void blend_multiply_1px(ARGS_SRC_DEST);
void blend_multiply(ARGS_SRC_DEST_WH);
void blend_screen_1px(ARGS_SRC_DEST);
void blend_screen(ARGS_SRC_DEST_WH);
void blend_color_dodge_1px(ARGS_SRC_DEST);
void blend_color_dodge(ARGS_SRC_DEST_WH);
void blend_color_burn_1px(ARGS_SRC_DEST);
void blend_color_burn(ARGS_SRC_DEST_WH);
void blend_linear_burn_1px(ARGS_SRC_DEST);
void blend_linear_burn(ARGS_SRC_DEST_WH);
void blend_maximum_1px(ARGS_SRC_DEST);
void blend_maximum(ARGS_SRC_DEST_WH);
void blend_minimum_1px(ARGS_SRC_DEST);
void blend_minimum(ARGS_SRC_DEST_WH);
void blend_sub_src_from_dest_1px(ARGS_SRC_DEST);
void blend_sub_src_from_dest(ARGS_SRC_DEST_WH);
void blend_sub_dest_from_src_1px(ARGS_SRC_DEST);
void blend_sub_dest_from_src(ARGS_SRC_DEST_WH);
void blend_sub_src_from_dest_abs_1px(ARGS_SRC_DEST);
void blend_sub_src_from_dest_abs(ARGS_SRC_DEST_WH);
void blend_xor_1px(ARGS_SRC_DEST);
void blend_xor(ARGS_SRC_DEST_WH);
void blend_every_other_pixel(ARGS_SRC_DEST_WH);
void blend_every_other_line(ARGS_SRC_DEST_WH);

#define ARGS_SRC_DEST_PARAM    const uint32_t *src, uint32_t *dest, uint32_t param
#define ARGS_SRC_DEST_PARAM_WH ARGS_SRC_DEST_PARAM, uint32_t w, uint32_t h
void blend_adjustable_1px(ARGS_SRC_DEST_PARAM);
void blend_adjustable(ARGS_SRC_DEST_PARAM_WH);

#define ARGS_SRC_DEST_BUF_INVERT \
    const uint32_t *src, uint32_t *dest, const uint32_t *buf, bool invert
#define ARGS_SRC_DEST_BUF_INVERT_WH ARGS_SRC_DEST_BUF_INVERT, uint32_t w, uint32_t h
void blend_buffer_1px(ARGS_SRC_DEST_BUF_INVERT);
void blend_buffer(ARGS_SRC_DEST_BUF_INVERT_WH);

extern int32_t g_line_blend_mode;
inline void blend_default_1px(const uint32_t* src, uint32_t* dest) {
    switch (g_line_blend_mode & 0xff) {
        default: blend_replace_1px(src, dest); break;
        case 1: blend_add_1px(src, dest); break;
        case 2: blend_maximum_1px(src, dest); break;
        case 3: blend_5050_1px(src, dest); break;
        case 4: blend_sub_src_from_dest_1px(src, dest); break;
        case 5: blend_sub_dest_from_src_1px(src, dest); break;
        case 6: blend_multiply_1px(src, dest); break;
        case 7: blend_adjustable_1px(src, dest, g_line_blend_mode >> 8 & 0xff); break;
        case 8: blend_xor_1px(src, dest); break;
        case 9: blend_minimum_1px(src, dest); break;
    }
}
