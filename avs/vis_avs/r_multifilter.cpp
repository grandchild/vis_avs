#include "c_multifilter.h"

#include "pixel_format.h"  // AVS_PIXEL_COLOR_MASK_RGB0_8

#include "../util.h"  // ssizeof32()

#include <emmintrin.h>  // SSE2 SIMD intrinsics
#include <string.h>

C_MultiFilter::C_MultiFilter()
    : config({true, MULTIFILTER_CHROME, false}), toggle_state(false) {}

C_MultiFilter::~C_MultiFilter() {}

int C_MultiFilter::render(char[2][2][576],
                          int is_beat,
                          int* framebuffer,
                          int*,
                          int w,
                          int h) {
    if (!this->config.enabled) {
        return 0;
    }
    if (this->config.enabled && this->config.toggle_on_beat) {
        if (is_beat) {
            this->toggle_state = !this->toggle_state;
        }
        if (!this->toggle_state) {
            return 0;
        }
    }
    if (this->config.effect <= MULTIFILTER_TRIPLE_CHROME) {
        this->chrome_sse2(framebuffer, w * h);
    } else if (this->config.effect == MULTIFILTER_INFROOT_BORDERCONVO) {
        this->infroot_borderconvo(framebuffer, w, h);
    }
    return 0;
}

inline void C_MultiFilter::chrome(int* framebuffer, unsigned int fb_length) {
    /*
    The "chrome" effect transforms the color values of each channel so that middle
    values are the brightest, and both dark and bright values become darker. If a row of
    pixels has brightness values roughly like

        ▁▁▂▂▃▃▄▄▅▅▆▆▇▇██

    the pixels will have their brightness value changed to

        ▁▂▃▄▅▆▇██▇▆▅▄▃▂▁

    and for "double chrome", the process is repeated and the result is

        ▁▃▅██▅▃▁▁▃▅██▅▃▁

    and so on for "triple chrome" as well.
    */
    for (unsigned int i = 0; i < fb_length; i++) {
        unsigned char* chan = (unsigned char*)&framebuffer[i];
        switch (this->config.effect) {
            case MULTIFILTER_CHROME:
                chan[0] = chan[0] < 128 ? chan[0] * 2 : 510 - (chan[0] * 2);
                chan[1] = chan[1] < 128 ? chan[1] * 2 : 510 - (chan[1] * 2);
                chan[2] = chan[2] < 128 ? chan[2] * 2 : 510 - (chan[2] * 2);
                break;
            case MULTIFILTER_DOUBLE_CHROME:
                chan[0] = chan[0] < 128 ? chan[0] * 2 : 510 - (chan[0] * 2);
                chan[1] = chan[1] < 128 ? chan[1] * 2 : 510 - (chan[1] * 2);
                chan[2] = chan[2] < 128 ? chan[2] * 2 : 510 - (chan[2] * 2);
                chan[0] = chan[0] < 128 ? chan[0] * 2 : 510 - (chan[0] * 2);
                chan[1] = chan[1] < 128 ? chan[1] * 2 : 510 - (chan[1] * 2);
                chan[2] = chan[2] < 128 ? chan[2] * 2 : 510 - (chan[2] * 2);
                break;
            case MULTIFILTER_TRIPLE_CHROME:
                chan[0] = chan[0] < 128 ? chan[0] * 2 : 510 - (chan[0] * 2);
                chan[1] = chan[1] < 128 ? chan[1] * 2 : 510 - (chan[1] * 2);
                chan[2] = chan[2] < 128 ? chan[2] * 2 : 510 - (chan[2] * 2);
                chan[0] = chan[0] < 128 ? chan[0] * 2 : 510 - (chan[0] * 2);
                chan[1] = chan[1] < 128 ? chan[1] * 2 : 510 - (chan[1] * 2);
                chan[2] = chan[2] < 128 ? chan[2] * 2 : 510 - (chan[2] * 2);
                chan[0] = chan[0] < 128 ? chan[0] * 2 : 510 - (chan[0] * 2);
                chan[1] = chan[1] < 128 ? chan[1] * 2 : 510 - (chan[1] * 2);
                chan[2] = chan[2] < 128 ? chan[2] * 2 : 510 - (chan[2] * 2);
                break;
            default: break;
        }
    }
}

inline void C_MultiFilter::chrome_sse2(int* framebuffer, unsigned int fb_length) {
    /*
    (See chrome() for description of how the "chrome" effect works.)

    The SIMD-optimized version uses operations with saturation instead of if/else to
    decide if the brightness is > 128. If "chan" is a single channel's value then the
    resulting brightness becomes

        out = 2 * ( min( 255, 2 * chan ) - chan )

    An "add-with-saturation" instruction for 8-bit unsigned integers (whose maximum is
    255) will add two 8-bit values and clamp the result at 255. "2 * chan" of course is
    equal to "chan + chan", and if "⨮" here means add-with-saturation then

        tmp = chan ⨮ chan
        tmp = tmp - chan
        out = tmp ⨮ tmp    // (saturation not really in effect here, since tmp ≤ 127)

    And this cadence of add, sub, add can be seen below. For double and triple chrome
    the process is repeated.

    Historical note: The original APE did something similar with MMX instructions, but
    multiplied by 2 instead of adding the same value. Intel CPUs don't provide packed
    8-bit-multiply instructions. So this required repacking as 16-bit, then multiplying,
    then repacking as 8-bit again. Add,sub,add is a tiny bit faster, but most of the
    speedup compared to the original is of course the fact, that this more modern SSE2
    version can calculate four pixels at a time, instead of only one.

    TODO [performance]: Extend to AVX2 and 256-bit (i.e. 8 pixels).
    */
    __m128i four_px_in;
    __m128i four_px_1;
    __m128i four_px_2;
    __m128i four_px_out;
    switch (this->config.effect) {
        case MULTIFILTER_CHROME:
            for (unsigned int i = 0; i < fb_length; i += 4) {
                four_px_in = _mm_loadu_si128((__m128i*)&framebuffer[i]);
                four_px_out = _mm_adds_epu8(four_px_in, four_px_in);
                four_px_out = _mm_subs_epu8(four_px_out, four_px_in);
                four_px_out = _mm_adds_epu8(four_px_out, four_px_out);
                _mm_store_si128((__m128i*)&framebuffer[i], four_px_out);
            }
            break;
        case MULTIFILTER_DOUBLE_CHROME:
            for (unsigned int i = 0; i < fb_length; i += 4) {
                four_px_in = _mm_loadu_si128((__m128i*)&framebuffer[i]);
                four_px_1 = _mm_adds_epu8(four_px_in, four_px_in);
                four_px_1 = _mm_subs_epu8(four_px_1, four_px_in);
                four_px_1 = _mm_adds_epu8(four_px_1, four_px_1);
                four_px_out = _mm_adds_epu8(four_px_1, four_px_1);
                four_px_out = _mm_subs_epu8(four_px_out, four_px_1);
                four_px_out = _mm_adds_epu8(four_px_out, four_px_out);
                _mm_store_si128((__m128i*)&framebuffer[i], four_px_out);
            }
            break;
        case MULTIFILTER_TRIPLE_CHROME:
            for (unsigned int i = 0; i < fb_length; i += 4) {
                four_px_in = _mm_loadu_si128((__m128i*)&framebuffer[i]);
                four_px_1 = _mm_adds_epu8(four_px_in, four_px_in);
                four_px_1 = _mm_subs_epu8(four_px_1, four_px_in);
                four_px_1 = _mm_adds_epu8(four_px_1, four_px_1);
                four_px_2 = _mm_adds_epu8(four_px_1, four_px_1);
                four_px_2 = _mm_subs_epu8(four_px_2, four_px_1);
                four_px_2 = _mm_adds_epu8(four_px_2, four_px_2);
                four_px_out = _mm_adds_epu8(four_px_2, four_px_2);
                four_px_out = _mm_subs_epu8(four_px_out, four_px_2);
                four_px_out = _mm_adds_epu8(four_px_out, four_px_out);
                _mm_store_si128((__m128i*)&framebuffer[i], four_px_out);
            }
            break;
    }
}

inline void C_MultiFilter::infroot_borderconvo(int* framebuffer, int w, int h) {
    /*
    "Infinite root" basically means that the output of a pixel will be white for all
    pixel values except when it's completely black. This is useful for masking.

    A "small border convolution" could be expected to be something like

        o              o   o            o o o
      o x o     or       x       or     o x o
        o              o   o            o o o

    Where "x" is the current pixel, and "o" are surrounding pixels that are affected.
    However, in the original APE (probably by mistake) the result of this convolution is
    just

        o
      o x

    and that's it. This has the advantage of being faster, because both "o" pixels are
    pixels that lie "behind" the current pixel in the framebuffer memory as the for-loop
    moves through it. And since all pixels are either full black or full white, this
    makes dealing with pixel values further "ahead" in the framebuffer, and thus double
    buffering, unnecessary. The disadvantage is that it's not actually a very high-
    quality edge detection filter.
    */
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            unsigned int p = x + y * w;
            if (framebuffer[p] & AVS_PIXEL_COLOR_MASK_RGB0_8) {
                framebuffer[p] = 0xffffff;
                if (y > 0) {
                    framebuffer[p - w] = 0xffffff;
                }
                if (x > 0) {
                    framebuffer[p - 1] = 0xffffff;
                }
            } else {
                framebuffer[p] = 0;
            }
        }
    }
}

void C_MultiFilter::load_config(unsigned char* data, int len) {
    if (len >= ssizeof32(multifilter_config))
        memcpy(&this->config, data, ssizeof32(multifilter_config));
    else {
        this->config.enabled = true;
        this->config.effect = MULTIFILTER_CHROME;
        this->config.toggle_on_beat = false;
    }
}

int C_MultiFilter::save_config(unsigned char* data) {
    memcpy(data, &this->config, sizeof(multifilter_config));
    memset(data + sizeof(multifilter_config), 0, sizeof(uint32_t));
    return sizeof(multifilter_config) + sizeof(uint32_t);
}

char* C_MultiFilter::get_desc() { return MOD_NAME; }

C_RBASE* R_MultiFilter(char* desc) {
    if (desc) {
        strcpy(desc, MOD_NAME);
        return NULL;
    }
    return (C_RBASE*)new C_MultiFilter();
}
