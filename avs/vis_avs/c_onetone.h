#pragma once

#include "c__base.h"

#define MOD_NAME    "Trans / Unique tone"
#define C_THISCLASS C_OnetoneClass

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
    void RebuildTable(void);
    int __inline depthof(int c);
    int enabled;
    int invert;
    int color;
    unsigned char tabler[256];
    unsigned char tableg[256];
    unsigned char tableb[256];
    int blend, blendavg;
};
