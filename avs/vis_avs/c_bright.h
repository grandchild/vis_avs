#pragma once

#include "c__base.h"

#define MOD_NAME    "Trans / Brightness"
#define C_THISCLASS C_BrightnessClass

class C_THISCLASS : public C_RBASE2 {
   protected:
   public:
    C_THISCLASS();
    virtual ~C_THISCLASS();
    virtual int render(char visdata[2][2][576],
                       int isBeat,
                       int* framebuffer,
                       int* fbout,
                       int w,
                       int h);
    virtual char* get_desc() { return MOD_NAME; }
    virtual void load_config(unsigned char* data, int len);
    virtual int save_config(unsigned char* data);

    virtual int smp_getflags() { return 1; }
    virtual int smp_begin(int max_threads,
                          char visdata[2][2][576],
                          int isBeat,
                          int* framebuffer,
                          int* fbout,
                          int w,
                          int h);
    virtual void smp_render(int this_thread,
                            int max_threads,
                            char visdata[2][2][576],
                            int isBeat,
                            int* framebuffer,
                            int* fbout,
                            int w,
                            int h);
    virtual int smp_finish(char visdata[2][2][576],
                           int isBeat,
                           int* framebuffer,
                           int* fbout,
                           int w,
                           int h);  // return value is that of render() for fbstuff etc

    int enabled;
    int redp, greenp, bluep;
    int blend, blendavg;
    int dissoc;
    int color;
    int exclude;
    int distance;
    int tabs_needinit;
    int green_tab[256];
    int red_tab[256];
    int blue_tab[256];
};
