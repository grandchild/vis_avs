#pragma once

#include "c__base.h"

#include <windows.h>

#define MOD_NAME "Trans / Multi Filter"
#define MULTIFILTER_NUM_EFFECTS 4

enum MultiFilterEffect {
    MULTIFILTER_CHROME = 0,
    MULTIFILTER_DOUBLE_CHROME,
    MULTIFILTER_TRIPLE_CHROME,
    MULTIFILTER_INFROOT_BORDERCONVO,
};

typedef struct {
    int32_t enabled;
    int32_t effect;
    int32_t toggle_on_beat;
} multifilter_config;

class C_MultiFilter : public C_RBASE {
   public:
    C_MultiFilter();
    virtual ~C_MultiFilter();
    virtual int render(char visdata[2][2][576],
                       int isBeat,
                       int* framebuffer,
                       int* fbout,
                       int w,
                       int h);
    virtual char* get_desc();
    virtual void load_config(unsigned char* data, int len);
    virtual int save_config(unsigned char* data);

    multifilter_config config;

   private:
    void chrome(int* framebuffer, unsigned int fb_length);
    void chrome_sse2(int* framebuffer, unsigned int fb_length);
    void infroot_borderconvo(int* framebuffer, int w, int h);

    bool toggle_state;

   public:
    const char* effects[MULTIFILTER_NUM_EFFECTS] = {
        "Chrome",
        "Double Chrome",
        "Triple Chrome",
        "Infinite Root Multiplier + Small Border Convolution",
    };
};
