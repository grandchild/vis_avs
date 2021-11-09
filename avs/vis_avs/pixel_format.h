#pragma once

#include <stdint.h>

enum AVSPixelFormat {
    AVS_PXL_FMT_RGB8 = 0,
    /* possibly other formats in the future: */
    // AVS_PXL_FMT_ARGB8 = 1,
    // AVS_PXL_FMT_RGB10 = 2,
    // AVS_PXL_FMT_RGB16 = 3,
    // AVS_PXL_FMT_ARGB16 = 4,
};

typedef uint32_t pixel_rgb8;
// typedef uint32_t pixel_argb8;
// typedef uint32_t pixel_rgb10;
// typedef uint64_t pixel_rgb16;
// typedef uint64_t pixel_argb16;

#define pixel_size(_) sizeof(pixel_rgb8)

/* use this in the future:
size_t pixel_size(AVSPixelFormat pixel_format) {
    switch (pixel_format) {
        default:
        case AVS_PXL_FMT_RGB8:
            return sizeof(pixel_rgb8);
        case AVS_PXL_FMT_ARGB8:
            return sizeof(pixel_argb8);
        case AVS_PXL_FMT_RGB10:
            return sizeof(pixel_rgb10);
        case AVS_PXL_FMT_RGB16:
            return sizeof(pixel_rgb16);
        case AVS_PXL_FMT_ARGB16:
            return sizeof(pixel_argb16);
    }
}
*/

#define AVS_PXL_COLOR_MASK_RGB8 0x00ffffff
// #define AVS_PXL_COLOR_MASK_ARGB8 0x00ffffff
// #define AVS_PXL_COLOR_MASK_RGB10 0x3fffffff
// #define AVS_PXL_COLOR_MASK_RGB16 0x0000ffffffffffff
// #define AVS_PXL_COLOR_MASK_ARGB16 0x0000ffffffffffff
