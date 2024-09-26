#include "blend.h"

#include "pixel_format.h"

#include "../platform.h"

#include <immintrin.h>
#include <stdio.h>
#include <string.h>

static uint8_t lut_u8_multiply[256][256];
static uint8_t lut_u8_screen[256][256];
static uint8_t lut_u8_linear_burn[256][256];
static uint8_t lut_u8_color_dodge[256][256];
static uint8_t lut_u8_color_burn[256][256];

/**
 * Generate lookup tables (LUTs) for u8.
 * The resulting multiply table looks like this (for a hypothetical 16x16 table):
 *
 *     00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
 *     00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 01
 *     00 00 00 00 00 00 00 00 01 01 01 01 01 01 01 02
 *     00 00 00 00 00 01 01 01 01 01 02 02 02 02 02 03
 *     00 00 00 00 01 01 01 01 02 02 02 02 03 03 03 04
 *     00 00 00 01 01 01 02 02 02 03 03 03 04 04 04 05
 *     00 00 00 01 01 02 02 02 03 03 04 04 04 05 05 06
 *     00 00 00 01 01 02 02 03 03 04 04 05 05 06 06 07
 *     00 00 01 01 02 02 03 03 04 04 05 05 06 06 07 08
 *     00 00 01 01 02 03 03 04 04 05 06 06 07 07 08 09
 *     00 00 01 02 02 03 04 04 05 06 06 07 08 08 09 10
 *     00 00 01 02 02 03 04 05 05 06 07 08 08 09 10 11
 *     00 00 01 02 03 04 04 05 06 07 08 08 09 10 11 12
 *     00 00 01 02 03 04 05 06 06 07 08 09 10 11 12 13
 *     00 00 01 02 03 04 05 06 07 08 09 10 11 12 13 14
 *     00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15
 *
 */
void make_blend_LUTs() {
    for (int k = 0; k < 256; k++) {
        for (int i = 0; i < 256; i++) {
            lut_u8_multiply[i][k] = (uint8_t)((i * k) / 255);
            lut_u8_screen[i][k] = (uint8_t)(255 - ((255 - i) * (255 - k) / 255));
            lut_u8_color_dodge[i][k] = (uint8_t)((float)i / (255.0 - k));
            lut_u8_linear_burn[i][k] = (uint8_t)(i + k - 255);
            lut_u8_color_burn[i][k] = (uint8_t)(255 - ((255.0 - i) / (float)(k)));
        }
    }
}

// REPLACE BLEND

void blend_replace(const uint32_t* src, uint32_t* dest, uint32_t w, uint32_t h) {
    memcpy(dest, src, w * h * pixel_size(pixel_rgb0_8));
}

// ADDITIVE BLEND

static inline void blend_add_rgb0_8_c(const uint32_t* src, uint32_t* dest) {
    uint32_t channel = (*src & 0xff) + (*dest & 0xff);
    uint32_t out = min(channel, 0xff);
    channel = (*src & 0xff00) + (*dest & 0xff00);
    out |= min(channel, 0xff00);
    channel = (*src & 0xff0000) + (*dest & 0xff0000);
    *dest = out | min(channel, 0xff0000);
}

static inline void blend_add_rgb0_8_x86v128(const uint32_t* src, uint32_t* dest) {
    __m128i src_4px = _mm_load_si128((__m128i*)src);
    __m128i dest_4px = _mm_load_si128((__m128i*)dest);
    _mm_store_si128((__m128i*)dest, _mm_adds_epu8(src_4px, dest_4px));
}

void blend_add(const uint32_t* src, uint32_t* dest, uint32_t w, uint32_t h) {
#ifdef SIMD_MODE_X86_SSE
    for (size_t i = 0; i < w * h; i += 4) {
        blend_add_rgb0_8_x86v128(&src[i], &dest[i]);
    }
#else
    for (size_t i = 0; i < w * h; i++) {
        blend_add_rgb0_8_c(&src[i], &dest[i]);
    }
#endif
}

// 50/50 BLEND

static inline void blend_5050_rgb0_8_c(const uint32_t* src, uint32_t* dest) {
    static uint32_t strip_high_bit_mask = 0x007f7f7f;
    *dest = ((*src >> 1) & strip_high_bit_mask) + ((*dest >> 1) & strip_high_bit_mask);
}

static inline void blend_5050_rgb0_8_x86v128(const uint32_t* src, uint32_t* dest) {
    __m128i src_4px = _mm_load_si128((__m128i*)src);
    __m128i dest_4px = _mm_load_si128((__m128i*)dest);
    // This should use _mm_avg_epu8(), but that does a correct average, i.e. it does
    // (a + b + 1) / 2, while avs does (a + b) / 2. We opt for consistency and backward
    // compatibility here.
    __m128i strip_high_bit_mask = _mm_set1_epi32(0x007f7f7f);
    __m128i src_half = _mm_and_si128(_mm_srli_epi16(src_4px, 1), strip_high_bit_mask);
    __m128i dest_half = _mm_and_si128(_mm_srli_epi16(dest_4px, 1), strip_high_bit_mask);
    _mm_store_si128((__m128i*)dest, _mm_adds_epu8(src_half, dest_half));
}

void blend_5050(const uint32_t* src, uint32_t* dest, uint32_t w, uint32_t h) {
#ifdef SIMD_MODE_X86_SSE
    for (size_t i = 0; i < w * h; i += 4) {
        blend_5050_rgb0_8_x86v128(&src[i], &dest[i]);
    }
#else
    for (size_t i = 0; i < w * h; i++) {
        blend_5050_rgb0_8_c(&src[i], &dest[i]);
    }
#endif
}

// MULTIPLY BLEND

static inline void blend_multiply_rgb0_8_c(const uint32_t* src, uint32_t* dest) {
    uint8_t result = lut_u8_multiply[*src & 0xFF][*dest & 0xFF];
    result |= lut_u8_multiply[(*src & 0xFF00) >> 8][(*dest & 0xFF00) >> 8] << 8;
    result |= lut_u8_multiply[(*src & 0xFF0000) >> 16][(*dest & 0xFF0000) >> 16] << 16;
    *dest = result;
}

static inline void blend_multiply_rgb0_8_x86v128(const uint32_t* src, uint32_t* dest) {
    __m128i src_4px = _mm_load_si128((__m128i*)src);
    __m128i dest_4px = _mm_load_si128((__m128i*)dest);
    __m128i src_lo_2px = _mm_unpacklo_epi8(src_4px, _mm_setzero_si128());
    __m128i src_hi_2px = _mm_unpackhi_epi8(src_4px, _mm_setzero_si128());
    __m128i dest_lo_2px = _mm_unpacklo_epi8(dest_4px, _mm_setzero_si128());
    __m128i dest_hi_2px = _mm_unpackhi_epi8(dest_4px, _mm_setzero_si128());
    __m128i mul_lo = _mm_mullo_epi16(src_lo_2px, dest_lo_2px);
    __m128i mul_hi = _mm_mullo_epi16(src_hi_2px, dest_hi_2px);
    __m128i mul_lo_norm = _mm_srli_epi16(mul_lo, 8);
    __m128i mul_hi_norm = _mm_srli_epi16(mul_hi, 8);
    __m128i packed = _mm_packus_epi16(mul_lo_norm, mul_hi_norm);
    _mm_store_si128((__m128i*)dest, packed);
}

void blend_multiply(const uint32_t* src, uint32_t* dest, uint32_t w, uint32_t h) {
#ifdef SIMD_MODE_X86_SSE
    for (size_t i = 0; i < w * h; i += 4) {
        blend_multiply_rgb0_8_x86v128(&src[i], &dest[i]);
    }
#else
    for (size_t i = 0; i < w * h; i++) {
        blend_multiply_rgb0_8_c(&src[i], &dest[i]);
    }
#endif
}

// SCREEN BLEND

static inline void blend_screen_rgb0_8_c(const uint32_t* src, uint32_t* dest) {
    uint8_t result = lut_u8_screen[*src & 0xFF][*dest & 0xFF];
    result |= lut_u8_screen[(*src & 0xFF00) >> 8][(*dest & 0xFF00) >> 8] << 8;
    result |= lut_u8_screen[(*src & 0xFF0000) >> 16][(*dest & 0xFF0000) >> 16] << 16;
    *dest = result;
}

static inline void blend_screen_rgb0_8_x86v128(const uint32_t* src, uint32_t* dest) {
    __m128i src_4px = _mm_load_si128((__m128i*)src);
    __m128i dest_4px = _mm_load_si128((__m128i*)dest);
    src_4px = _mm_xor_si128(src_4px, _mm_set1_epi32(0x00ffffff));
    dest_4px = _mm_xor_si128(dest_4px, _mm_set1_epi32(0x00ffffff));
    __m128i src_lo_2px = _mm_unpacklo_epi8(src_4px, _mm_setzero_si128());
    __m128i src_hi_2px = _mm_unpackhi_epi8(src_4px, _mm_setzero_si128());
    __m128i dest_lo_2px = _mm_unpacklo_epi8(dest_4px, _mm_setzero_si128());
    __m128i dest_hi_2px = _mm_unpackhi_epi8(dest_4px, _mm_setzero_si128());
    __m128i mul_lo = _mm_mullo_epi16(src_lo_2px, dest_lo_2px);
    __m128i mul_hi = _mm_mullo_epi16(src_hi_2px, dest_hi_2px);
    __m128i mul_lo_norm = _mm_srli_epi16(mul_lo, 8);
    __m128i mul_hi_norm = _mm_srli_epi16(mul_hi, 8);
    __m128i packed = _mm_packus_epi16(mul_lo_norm, mul_hi_norm);
    packed = _mm_xor_si128(packed, _mm_set1_epi32(0x00ffffff));
    _mm_store_si128((__m128i*)dest, packed);
}

void blend_screen(const uint32_t* src, uint32_t* dest, uint32_t w, uint32_t h) {
#ifdef SIMD_MODE_X86_SSE
    for (size_t i = 0; i < w * h; i += 4) {
        blend_screen_rgb0_8_x86v128(&src[i], &dest[i]);
    }
#else
    for (size_t i = 0; i < w * h; i++) {
        blend_screen_rgb0_8_c(&src[i], &dest[i]);
    }
#endif
}

// MAXIMUM BLEND

static inline void blend_maximum_rgb0_8_c(const uint32_t* src, uint32_t* dest) {
    uint32_t channel = *src & 0xff;
    uint32_t out = max(channel, *dest & 0xff);
    channel = *src & 0xff00;
    out |= max(channel, *dest & 0xff00);
    channel = *src & 0xff0000;
    *dest = out | max(channel, *dest & 0xff0000);
}

static inline void blend_maximum_rgb0_8_x86v128(const uint32_t* src, uint32_t* dest) {
    __m128i src_4px = _mm_load_si128((__m128i*)src);
    __m128i dest_4px = _mm_load_si128((__m128i*)dest);
    _mm_store_si128((__m128i*)dest, _mm_max_epu8(src_4px, dest_4px));
}

void blend_maximum(const uint32_t* src, uint32_t* dest, uint32_t w, uint32_t h) {
#ifdef SIMD_MODE_X86_SSE
    for (size_t i = 0; i < w * h; i += 4) {
        blend_maximum_rgb0_8_x86v128(&src[i], &dest[i]);
    }
#else
    for (size_t i = 0; i < w * h; i++) {
        blend_maximum_rgb0_8_c(&src[i], &dest[i]);
    }
#endif
}

// MINIMUM BLEND

static inline void blend_minimum_rgb0_8_c(const uint32_t* src, uint32_t* dest) {
    *dest = min(*src & 0xff, *dest & 0xff) | min(*src & 0xff00, *dest & 0xff00)
            | min(*src & 0xff0000, *dest & 0xff0000);
}

static inline void blend_minimum_rgb0_8_x86v128(const uint32_t* src, uint32_t* dest) {
    __m128i src_4px = _mm_load_si128((__m128i*)src);
    __m128i dest_4px = _mm_load_si128((__m128i*)dest);
    _mm_store_si128((__m128i*)dest, _mm_min_epu8(src_4px, dest_4px));
}

void blend_minimum(const uint32_t* src, uint32_t* dest, uint32_t w, uint32_t h) {
#ifdef SIMD_MODE_X86_SSE
    for (size_t i = 0; i < w * h; i += 4) {
        blend_minimum_rgb0_8_x86v128(&src[i], &dest[i]);
    }
#else
    for (size_t i = 0; i < w * h; i++) {
        blend_minimum_rgb0_8_c(&src[i], &dest[i]);
    }
#endif
}

// COLOR DODGE BLEND

static inline void blend_colordodge_rgb0_8_c(const uint32_t* src, uint32_t* dest) {
    uint8_t result = lut_u8_color_dodge[*src & 0xFF][*dest & 0xFF];
    result |= lut_u8_color_dodge[(*src & 0xFF00) >> 8][(*dest & 0xFF00) >> 8] << 8;
    result |= lut_u8_color_dodge[(*src & 0xFF0000) >> 16][(*dest & 0xFF0000) >> 16]
              << 16;
    *dest = result;
}

void blend_colordodge(const uint32_t* src, uint32_t* dest, uint32_t w, uint32_t h) {
    for (size_t i = 0; i < w * h; i++) {
        blend_colordodge_rgb0_8_c(&src[i], &dest[i]);
    }
}

// COLOR BURN BLEND

static inline void blend_colorburn_rgb0_8_c(const uint32_t* src, uint32_t* dest) {
    uint8_t result = lut_u8_color_burn[*src & 0xFF][*dest & 0xFF];
    result |= lut_u8_color_burn[(*src & 0xFF00) >> 8][(*dest & 0xFF00) >> 8] << 8;
    result |= lut_u8_color_burn[(*src & 0xFF0000) >> 16][(*dest & 0xFF0000) >> 16]
              << 16;
    *dest = result;
}

void blend_colorburn(const uint32_t* src, uint32_t* dest, uint32_t w, uint32_t h) {
    for (size_t i = 0; i < w * h; i++) {
        blend_colorburn_rgb0_8_c(&src[i], &dest[i]);
    }
}

// LINEAR BURN BLEND

static inline void blend_linearburn_rgb0_8_c(const uint32_t* src, uint32_t* dest) {
    uint8_t result = lut_u8_linear_burn[*src & 0xFF][*dest & 0xFF];
    result |= lut_u8_linear_burn[(*src & 0xFF00) >> 8][(*dest & 0xFF00) >> 8] << 8;
    result |= lut_u8_linear_burn[(*src & 0xFF0000) >> 16][(*dest & 0xFF0000) >> 16]
              << 16;
    *dest = result;
}

void blend_linearburn(const uint32_t* src, uint32_t* dest, uint32_t w, uint32_t h) {
    for (size_t i = 0; i < w * h; i++) {
        blend_linearburn_rgb0_8_c(&src[i], &dest[i]);
    }
}

// SUBTRACTIVE 1 BLEND (source from destination)

static inline void blend_sub_src_from_dest_rgb0_8_c(const uint32_t* src,
                                                    uint32_t* dest) {
    int32_t channel = (*dest & 0xff) - (*src & 0xff);
    uint32_t out = max(0, channel);
    channel = (*dest & 0xff00) - (*src & 0xff00);
    out |= max(0, channel);
    channel = (*dest & 0xff0000) - (*src & 0xff0000);
    *dest = out | max(0, channel);
}

static inline void blend_sub_src_from_dest_rgb0_8_x86v128(const uint32_t* src,
                                                          uint32_t* dest) {
    __m128i src_4px = _mm_load_si128((__m128i*)src);
    __m128i dest_4px = _mm_load_si128((__m128i*)dest);
    _mm_store_si128((__m128i*)dest, _mm_subs_epu8(dest_4px, src_4px));
}

void blend_sub_src_from_dest(const uint32_t* src,
                             uint32_t* dest,
                             uint32_t w,
                             uint32_t h) {
#ifdef SIMD_MODE_X86_SSE
    for (size_t i = 0; i < w * h; i += 4) {
        blend_sub_src_from_dest_rgb0_8_x86v128(&src[i], &dest[i]);
    }
#else
    for (size_t i = 0; i < w * h; i++) {
        blend_sub_src_from_dest_rgb0_8_c(&src[i], &dest[i]);
    }
#endif
}

// SUBTRACTIVE 2 BLEND (destination from source)

static inline void blend_sub_dest_from_src_rgb0_8_c(const uint32_t* src,
                                                    uint32_t* dest) {
    int32_t channel = (*src & 0xff) - (*dest & 0xff);
    uint32_t out = max(0, channel);
    channel = (*src & 0xff00) - (*dest & 0xff00);
    out |= max(0, channel);
    channel = (*src & 0xff0000) - (*dest & 0xff0000);
    *dest = out | max(0, channel);
}

static inline void blend_sub_dest_from_src_rgb0_8_x86v128(const uint32_t* src,
                                                          uint32_t* dest) {
    __m128i src_4px = _mm_load_si128((__m128i*)src);
    __m128i dest_4px = _mm_load_si128((__m128i*)dest);
    _mm_store_si128((__m128i*)dest, _mm_subs_epu8(src_4px, dest_4px));
}

void blend_sub_dest_from_src(const uint32_t* src,
                             uint32_t* dest,
                             uint32_t w,
                             uint32_t h) {
#ifdef SIMD_MODE_X86_SSE
    for (size_t i = 0; i < w * h; i += 4) {
        blend_sub_dest_from_src_rgb0_8_x86v128(&src[i], &dest[i]);
    }
#else
    for (size_t i = 0; i < w * h; i++) {
        blend_sub_dest_from_src_rgb0_8_c(&src[i], &dest[i]);
    }
#endif
}

// SUBTRACTIVE 1 ABS BLEND (absolute source from destination)

static inline int32_t abs_i32(int32_t x) { return x < 0 ? -x : x; }

static inline void blend_sub_src_from_dest_abs_rgb0_8_c(const uint32_t* src,
                                                        uint32_t* dest) {
    *dest = abs_i32((int32_t)(*dest & 0xff) - (*src & 0xff))
            | abs_i32((int32_t)(*dest & 0xff00) - (*src & 0xff00))
            | abs_i32((int32_t)(*dest & 0xff0000) - (*src & 0xff0000));
}

static inline void blend_sub_src_from_dest_abs_rgb0_8_x86v128(const uint32_t* src,
                                                              uint32_t* dest) {
    __m128i src_4px = _mm_load_si128((__m128i*)src);
    __m128i dest_4px = _mm_load_si128((__m128i*)dest);
    // abs(x - y)  <=>  (x ⊢ y)  | (y ⊢ x) (where ⊢ is minus-with-saturation)
    _mm_store_si128((__m128i*)dest,
                    _mm_or_si128(_mm_subs_epu8(dest_4px, src_4px),
                                 _mm_subs_epu8(src_4px, dest_4px)));
    // alternatively: TODO[performance]: test if this is faster
    // abs(x - y)  <=>  max(x, y) - min(x, y)
    // _mm_store_si128((__m128i*)dest,
    //                 _mm_subs_epu8(_mm_max_epu8(dest_4px, src_4px),
    //                               _mm_min_epu8(dest_4px, src_4px)));
}

void blend_sub_src_from_dest_abs(const uint32_t* src,
                                 uint32_t* dest,
                                 uint32_t w,
                                 uint32_t h) {
#ifdef SIMD_MODE_X86_SSE
    for (size_t i = 0; i < w * h; i += 4) {
        blend_sub_src_from_dest_abs_rgb0_8_x86v128(&src[i], &dest[i]);
    }
#else
    for (size_t i = 0; i < w * h; i++) {
        blend_sub_src_from_dest_abs_rgb0_8_c(&src[i], &dest[i]);
    }
#endif
}

// XOR BLEND

static inline void blend_xor_rgb0_8_c(const uint32_t* src, uint32_t* dest) {
    *dest = *src ^ *dest;
}

static inline void blend_xor_rgb0_8_x86v128(const uint32_t* src, uint32_t* dest) {
    __m128i src_4px = _mm_load_si128((__m128i*)src);
    __m128i dest_4px = _mm_load_si128((__m128i*)dest);
    _mm_store_si128((__m128i*)dest, _mm_xor_si128(src_4px, dest_4px));
}

void blend_xor(const uint32_t* src, uint32_t* dest, uint32_t w, uint32_t h) {
#ifdef SIMD_MODE_X86_SSE
    for (size_t i = 0; i < w * h; i += 4) {
        blend_xor_rgb0_8_x86v128(&src[i], &dest[i]);
    }
#else
    for (size_t i = 0; i < w * h; i++) {
        blend_xor_rgb0_8_c(&src[i], &dest[i]);
    }
#endif
}

// EVERY OTHER PIXEL BLEND

void blend_every_other_pixel(const uint32_t* src,
                             uint32_t* dest,
                             uint32_t w,
                             uint32_t h) {
    // The top-left pixel is from src.
    for (size_t k = 0, odd = 0; k < h; k++, odd = !odd) {
        dest += odd;
        src += odd;
        for (size_t l = odd; l < w; l += 2) {
            *(dest += 2) = *(src += 2);
        }
    }
}

// EVERY OTHER LINE BLEND

void blend_every_other_line(const uint32_t* src,
                            uint32_t* dest,
                            uint32_t w,
                            uint32_t h) {
    // The top line is from src.
    for (size_t i = 0; i < w * h; i += w) {
        memcpy(&dest[i], &src[i], w * pixel_size(pixel_rgb0_8));
    }
}

// ADJUSTABLE BLEND

static inline void blend_adjustable_rgb0_8_c(const uint32_t* src,
                                             uint32_t* dest,
                                             uint32_t param) {
    uint8_t v = param & 0xff;
    uint8_t iv = 0xff - v;
    int32_t output =
        lut_u8_multiply[*src & 0xFF][v] + lut_u8_multiply[*dest & 0xFF][iv];
    output |= (lut_u8_multiply[(*src & 0xFF00) >> 8][v]
               + lut_u8_multiply[(*dest & 0xFF00) >> 8][iv])
              << 8;
    output |= (lut_u8_multiply[(*src & 0xFF0000) >> 16][v]
               + lut_u8_multiply[(*dest & 0xFF0000) >> 16][iv])
              << 16;
    *dest = output;
}

static inline void blend_adjustable_rgb0_8_x86v128(const uint32_t* src,
                                                   uint32_t* dest,
                                                   uint32_t param) {
    __m128i src_4px = _mm_load_si128((__m128i*)src);
    __m128i dest_4px = _mm_load_si128((__m128i*)dest);
    __m128i zero = _mm_setzero_si128();
    __m128i src_lo_2px = _mm_unpacklo_epi8(src_4px, zero);
    __m128i src_hi_2px = _mm_unpackhi_epi8(src_4px, zero);
    __m128i dest_lo_2px = _mm_unpacklo_epi8(dest_4px, zero);
    __m128i dest_hi_2px = _mm_unpackhi_epi8(dest_4px, zero);
    __m128i v = _mm_set1_epi16(param);
    __m128i iv = _mm_xor_si128(v, _mm_set1_epi16(0x00ff));
    __m128i mul_src_lo = _mm_mullo_epi16(src_lo_2px, v);
    __m128i mul_src_hi = _mm_mullo_epi16(src_hi_2px, v);
    __m128i mul_dest_lo = _mm_mullo_epi16(dest_lo_2px, iv);
    __m128i mul_dest_hi = _mm_mullo_epi16(dest_hi_2px, iv);
    __m128i lo = _mm_adds_epu16(mul_src_lo, mul_dest_lo);
    __m128i hi = _mm_adds_epu16(mul_src_hi, mul_dest_hi);
    __m128i lo_norm = _mm_srli_epi16(lo, 8);
    __m128i hi_norm = _mm_srli_epi16(hi, 8);
    __m128i packed = _mm_packus_epi16(lo_norm, hi_norm);
    _mm_store_si128((__m128i*)dest, packed);
}

void blend_adjustable(const uint32_t* src,
                      uint32_t* dest,
                      uint32_t w,
                      uint32_t h,
                      uint32_t param) {
#ifdef SIMD_MODE_X86_SSE
    for (size_t i = 0; i < w * h; i += 4) {
        blend_adjustable_rgb0_8_x86v128(&src[i], &dest[i], param);
    }
#else
    for (size_t i = 0; i < w * h; i++) {
        blend_adjustable_rgb0_8_c(&src[i], &dest[i], param);
    }
#endif
}

// BUFFER BLEND

static inline void blend_buffer_rgb0_8_c(const uint32_t* src,
                                         uint32_t* dest,
                                         const uint32_t* buf,
                                         bool invert) {
    // find the maximum channel value and spread it to all channels
    uint8_t v = *buf >> 16 & 0xff;
    uint8_t buf_g = *buf >> 8 & 0xff;
    v = v >= buf_g ? v : buf_g;
    uint8_t buf_b = *buf & 0xff;
    v = v >= buf_b ? v : buf_b;
    uint8_t iv = 255 - v;
    if (invert) {
        iv = v;
        v = 255 - v;
    }

    // do an adjustable blend
    uint8_t src_r = *src >> 16 & 0xff;
    uint8_t src_g = *src >> 8 & 0xff;
    uint8_t src_b = *src & 0xff;
    uint8_t dest_r = *dest >> 16 & 0xff;
    uint8_t dest_g = *dest >> 8 & 0xff;
    uint8_t dest_b = *dest & 0xff;
    *dest = (lut_u8_multiply[src_r][v] + lut_u8_multiply[dest_r][iv]) << 16
            | (lut_u8_multiply[src_g][v] + lut_u8_multiply[dest_g][iv]) << 8
            | (lut_u8_multiply[src_b][v] + lut_u8_multiply[dest_b][iv]);
}

static inline void blend_buffer_rgb0_8_x86v128(const uint32_t* src,
                                               uint32_t* dest,
                                               const uint32_t* buf,
                                               bool invert) {
    __m128i zero = _mm_setzero_si128();
    __m128i buf_4px = _mm_load_si128((__m128i*)buf);
    // argb.argb.argb.argb -> [0a0r0g0b.0a0r0g0b, 0a0r0g0b.0a0r0g0b]
    __m128i buf_hi_2px = _mm_unpackhi_epi8(buf_4px, zero);
    __m128i buf_lo_2px = _mm_unpacklo_epi8(buf_4px, zero);
    // 0a0r0g0b.0a0r0g0b -> 00____0m.0____0m (where "m" is the maximum of the
    // channels)
    buf_hi_2px = _mm_max_epu16(buf_hi_2px, _mm_srli_si128(buf_hi_2px, 2));
    buf_lo_2px = _mm_max_epu16(buf_lo_2px, _mm_srli_si128(buf_lo_2px, 2));
    buf_hi_2px = _mm_max_epu16(buf_hi_2px, _mm_srli_si128(buf_hi_2px, 4));
    buf_lo_2px = _mm_max_epu16(buf_lo_2px, _mm_srli_si128(buf_lo_2px, 4));
    // alpha 0 & 3x 16bit values from hi:0 (0x80) + lo:index0 (0x00) = mask: 0x8000
    // alpha 0 & 3x 16bit values from hi:0 (0x80) + lo:index8 (0x08) = mask: 0x8008
    static uint64_t buf_shuffle_mask_values[2] = {
        0x8080800080008000,
        0x8080800880088008,
    };
    __m128i buf_shuffle_mask = _mm_load_si128((__m128i*)buf_shuffle_mask_values);
    // spread the maximum back to the channels:
    // ______0m.______0m -> 000m0m0m.000m0m0m
    buf_hi_2px = _mm_shuffle_epi8(buf_hi_2px, buf_shuffle_mask);
    buf_lo_2px = _mm_shuffle_epi8(buf_lo_2px, buf_shuffle_mask);
    // invert the buffer v values for lerp, either switched ("invert") or normal
    // xor is quick unsigned invert (here in 8bit): x ^ 0xff == 255 - x
    __m128i buf_hi_inv_2px;
    __m128i buf_lo_inv_2px;
    static __m128i invert255_mask = _mm_set1_epi16(0x00ff);
    if (invert) {
        buf_hi_inv_2px = buf_hi_2px;
        buf_lo_inv_2px = buf_lo_2px;
        buf_hi_2px = _mm_xor_si128(buf_hi_2px, invert255_mask);
        buf_lo_2px = _mm_xor_si128(buf_lo_2px, invert255_mask);
    } else {
        buf_hi_inv_2px = _mm_xor_si128(buf_hi_2px, invert255_mask);
        buf_lo_inv_2px = _mm_xor_si128(buf_lo_2px, invert255_mask);
    }

    __m128i src_4px = _mm_load_si128((__m128i*)src);
    __m128i dest_4px = _mm_load_si128((__m128i*)dest);
    __m128i src_hi_2px = _mm_unpackhi_epi8(src_4px, zero);
    __m128i src_lo_2px = _mm_unpacklo_epi8(src_4px, zero);
    __m128i dest_hi_2px = _mm_unpackhi_epi8(dest_4px, zero);
    __m128i dest_lo_2px = _mm_unpacklo_epi8(dest_4px, zero);
    // src * v
    __m128i src_hi_v = _mm_mullo_epi16(src_hi_2px, buf_hi_2px);
    __m128i src_lo_v = _mm_mullo_epi16(src_lo_2px, buf_lo_2px);
    // dest * (255 - v)
    __m128i dest_hi_v = _mm_mullo_epi16(dest_hi_2px, buf_hi_inv_2px);
    __m128i dest_lo_v = _mm_mullo_epi16(dest_lo_2px, buf_lo_inv_2px);
    // src * v + dest * (255 - v)
    __m128i hi = _mm_add_epi16(src_hi_v, dest_hi_v);
    __m128i lo = _mm_add_epi16(src_lo_v, dest_lo_v);
    __m128i his8 = _mm_srli_epi16(hi, 8);
    __m128i los8 = _mm_srli_epi16(lo, 8);
    __m128i packed = _mm_packus_epi16(los8, his8);
    _mm_store_si128((__m128i*)dest, packed);
}

void blend_buffer(const uint32_t* src,
                  uint32_t* dest,
                  const uint32_t* buf,
                  bool invert,
                  uint32_t w,
                  uint32_t h) {
#ifdef SIMD_MODE_X86_SSE
    for (size_t i = 0; i < w * h; i += 4) {
        blend_buffer_rgb0_8_x86v128(&src[i], &dest[i], &buf[i], invert);
    }
#else
    for (size_t i = 0; i < w * h; i++) {
        blend_buffer_rgb0_8_c(&src[i], &dest[i], &buf[i], invert);
    }
#endif
}
