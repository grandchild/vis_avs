#pragma once

#include "c__base.h"

#define C_THISCLASS C_BPartsClass
#define MOD_NAME    "Render / Moving Particle"

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

    int enabled;
    int colors;
    int maxdist, size, size2;
    int blend;

    int s_pos;

    double c[2];
    double v[2];
    double p[2];
};
