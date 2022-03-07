#include "effect_common.h"

const char* const* audio_sources(int64_t* length_out) {
    *length_out = 2;
    static const char* const options[2] = {
        "Waveform",
        "Spectrum",
    };
    return options;
};

const char* const* draw_modes(int64_t* length_out) {
    *length_out = 3;
    static const char* const options[3] = {
        "Dots",
        "Lines",
        "Solid",
    };
    return options;
};

const char* const* audio_channels(int64_t* length_out) {
    *length_out = 3;
    static const char* const options[3] = {
        "Left",
        "Right",
        "Center",
    };
    return options;
};

const char* const* h_positions(int64_t* length_out) {
    *length_out = 3;
    static const char* const options[3] = {
        "Left",
        "Right",
        "Center",
    };
    return options;
};

const char* const* v_positions(int64_t* length_out) {
    *length_out = 3;
    static const char* const options[3] = {
        "Top",
        "Bottom",
        "Center",
    };
    return options;
};
