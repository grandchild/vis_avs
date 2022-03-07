#pragma once

#include <stdint.h>

enum Audio_Sources { AUDIO_WAVEFORM = 0, AUDIO_SPECTRUM = 1 };
enum Audio_Channels { AUDIO_LEFT = 0, AUDIO_RIGHT = 1, AUDIO_CENTER = 2 };
enum Draw_Modes { DRAW_DOTS = 0, DRAW_LINES = 1, DRAW_SOLID = 2 };
enum Horizontal_Positions { HPOS_LEFT = 0, HPOS_RIGHT = 1, HPOS_CENTER = 2 };
enum Vertical_Positions { VPOS_TOP = 0, VPOS_BOTTOM = 1, VPOS_CENTER = 2 };

const char* const* audio_sources(int64_t* length_out);
const char* const* draw_modes(int64_t* length_out);
const char* const* audio_channels(int64_t* length_out);
const char* const* h_positions(int64_t* length_out);
const char* const* v_positions(int64_t* length_out);
