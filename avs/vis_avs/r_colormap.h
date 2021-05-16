/* See r_colormap.cpp for credits. */
#pragma once

#include <windows.h>
#include <immintrin.h>
#include "r_defs.h"
#include "resource.h"


#define MOD_NAME "Trans / ColorMap"
#define UNIQUEIDSTRING "Color Map"

#define NUM_COLOR_VALUES 256  // 2 ^ BITS_PER_CHANNEL (i.e. 8)
#define COLORMAP_NUM_MAPS 8
#define COLORMAP_MAP_FILENAME_MAXLEN 48
#define COLORMAP_SAVE_MAP_HEADER_SIZE (sizeof(map) - sizeof(map_color*))

#define COLORMAP_COLOR_KEY_RED          0
#define COLORMAP_COLOR_KEY_GREEN        1
#define COLORMAP_COLOR_KEY_BLUE         2
#define COLORMAP_COLOR_KEY_RGB_SUM_HALF 3
#define COLORMAP_COLOR_KEY_MAX          4
#define COLORMAP_COLOR_KEY_RGB_AVERAGE  5

#define COLORMAP_MAP_CYCLE_NONE            0
#define COLORMAP_MAP_CYCLE_BEAT_RANDOM     1
#define COLORMAP_MAP_CYCLE_BEAT_SEQUENTIAL 2

#define COLORMAP_MAP_CYCLE_TIMER_ID 1
#define COLORMAP_MAP_CYCLE_SPEED_MIN 1
#define COLORMAP_MAP_CYCLE_SPEED_MAX 64
#define COLORMAP_MAP_CYCLE_ANIMATION_STEPS NUM_COLOR_VALUES

#define COLORMAP_BLENDMODE_REPLACE    0
#define COLORMAP_BLENDMODE_ADDITIVE   1
#define COLORMAP_BLENDMODE_MAXIMUM    2
#define COLORMAP_BLENDMODE_MINIMUM    3
#define COLORMAP_BLENDMODE_5050       4
#define COLORMAP_BLENDMODE_SUB1       5
#define COLORMAP_BLENDMODE_SUB2       6
#define COLORMAP_BLENDMODE_MULTIPLY   7
#define COLORMAP_BLENDMODE_XOR        8
#define COLORMAP_BLENDMODE_ADJUSTABLE 9

#define COLORMAP_ADJUSTABLE_BLEND_MAX 255

#define COLORMAP_NUM_CYCLEMODES 3
#define COLORMAP_NUM_KEYMODES 6
#define COLORMAP_NUM_BLENDMODES 10


typedef struct {
    unsigned int position;
    unsigned int color;
    unsigned int color_id;
} map_color;

typedef struct {
    int enabled;
    unsigned int length;
    int map_id;
    char filename[COLORMAP_MAP_FILENAME_MAXLEN];
    map_color* colors;
} map;

typedef struct {
    unsigned int color_key;
    unsigned int blendmode;
    unsigned int map_cycle_mode;
    unsigned char adjustable_alpha;
    unsigned char _unused;
    unsigned char dont_skip_fast_beats;
    unsigned char map_cycle_speed; // 1 to 64
} colormap_apeconfig;

typedef struct {
    int colors[NUM_COLOR_VALUES];
} map_cache;

typedef struct {
    RECT window_rect;
    HDC context;
    HBITMAP bitmap;
    LPRECT region;
} ui_map;

class C_ColorMap : public C_RBASE {
    public:
        C_ColorMap();
        virtual ~C_ColorMap();

        /* APE interface */
        virtual int render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int);
        virtual HWND conf(HINSTANCE hInstance, HWND hwndParent);
        virtual char *get_desc();
        virtual void load_config(unsigned char *data, int len);
        virtual int  save_config(unsigned char *data);

        /* Other utilities */
        void load_map_file(int map_index);
        void save_map_file(int map_index);
        void make_default_map(int map_index);
        void make_map_cache(int map_index);
        void add_map_color(int map_index, int position, int color);
        void remove_map_color(int map_index, int index);
        void sort_colors(int map_index);

        colormap_apeconfig config;
        map maps[COLORMAP_NUM_MAPS];
        map_cache map_caches[COLORMAP_NUM_MAPS];
        map_cache tween_map_cache;
        int current_map = 0;
        int next_map = 0;
        HWND dialog = NULL;
        int change_animation_step;
        int disable_map_change;

    protected:
        int next_id = 1337;
        int get_new_id();
        bool any_maps_enabled();
        void animation_step();
        void animation_step_sse2();
        void reset_tween_map_cache();
        map_cache* animate_map_frame(int is_beat);
        int get_key(int color);
        void blend(map_cache* blend_map_cache, int* framebuffer, int w, int h);
        __m128i get_key_ssse3(__m128i color4);
        void blend_ssse3(map_cache* blend_map_cache, int* framebuffer, int w, int h);
        bool load_map_header(unsigned char *data, int len, int map_index, int pos);
        bool load_map_colors(unsigned char *data, int len, int map_index, int pos);
};
