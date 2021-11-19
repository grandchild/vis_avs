#pragma once

#include "c__base.h"

#define MOD_NAME    "Trans / Interleave"
#define C_THISCLASS C_InterleaveClass

class C_THISCLASS : public C_RBASE {
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
    int x, y;
    int enabled;
    int color;
    int blend;
    int blendavg;
    int onbeat, x2, y2, beatdur;
    double cur_x, cur_y;
};
