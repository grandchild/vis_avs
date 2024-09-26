#include <stdint.h>

// The following SIMD modes may be targeted by a build variable definition. If a SIMD
// implementation is not available, the C fallback (SIMD_MODE_NONE) version will be
// used.
//
//   SIMD_MODE_NONE
//   SIMD_MODE_X86_SSE

void make_blend_LUTs();

#define ARGS_SRC_DEST const uint32_t *src, uint32_t *dest, uint32_t w, uint32_t h
void blend_replace(ARGS_SRC_DEST);
void blend_add(ARGS_SRC_DEST);
void blend_5050(ARGS_SRC_DEST);
void blend_multiply(ARGS_SRC_DEST);
void blend_screen(ARGS_SRC_DEST);
void blend_color_dodge(ARGS_SRC_DEST);
void blend_color_burn(ARGS_SRC_DEST);
void blend_linear_burn(ARGS_SRC_DEST);
void blend_maximum(ARGS_SRC_DEST);
void blend_minimum(ARGS_SRC_DEST);
void blend_sub_src_from_dest(ARGS_SRC_DEST);
void blend_sub_dest_from_src(ARGS_SRC_DEST);
void blend_sub_src_from_dest_abs(ARGS_SRC_DEST);
void blend_xor(ARGS_SRC_DEST);
void blend_every_other_pixel(ARGS_SRC_DEST);
void blend_every_other_line(ARGS_SRC_DEST);

#define ARGS_SRC_DEST_PARAM \
    const uint32_t *src, uint32_t *dest, uint32_t w, uint32_t h, uint32_t param
void blend_adjustable(ARGS_SRC_DEST_PARAM);

#define ARGS_SRC_DEST_BUF_INVERT                                                       \
    const uint32_t *src, uint32_t *dest, const uint32_t *buf, bool invert, uint32_t w, \
        uint32_t h
void blend_buffer(ARGS_SRC_DEST_BUF_INVERT);
