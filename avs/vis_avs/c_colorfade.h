#pragma once

#include "c__base.h"

#define C_THISCLASS C_ColorFadeClass
#define MOD_NAME    "Trans / Colorfade"

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
    static int ft[4][3];

    int enabled;
    int faders[3];
    int beatfaders[3];
    int faderpos[3];
    unsigned char c_tab[512][512];
    unsigned char clip[256 + 40 + 40];
};
