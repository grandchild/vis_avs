#pragma once

#include "c__base.h"

#define C_THISCLASS C_SimpleClass
#define MOD_NAME    "Render / Simple"

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

    int effect;
    int num_colors;
    int colors[16];

    int color_pos;
};
