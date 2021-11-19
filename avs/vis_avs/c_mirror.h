#pragma once

#include "c__base.h"

#define MOD_NAME      "Trans / Mirror"
#define C_THISCLASS   C_MirrorClass
#define HORIZONTAL1   1
#define HORIZONTAL2   2
#define VERTICAL1     4
#define VERTICAL2     8
#define D_HORIZONTAL1 0
#define D_HORIZONTAL2 8
#define D_VERTICAL1   16
#define D_VERTICAL2   24
#define M_HORIZONTAL1 0xFF
#define M_HORIZONTAL2 0xFF00
#define M_VERTICAL1   0xFF0000
#define M_VERTICAL2   0xFF000000

class C_THISCLASS : public C_RBASE {
   protected:
   public:
    C_THISCLASS();
    virtual ~C_THISCLASS() {}
    virtual int render(char visdata[2][2][576],
                       int isBeat,
                       int* framebuffer,
                       int* fbout,
                       int w,
                       int h);
    virtual char* get_desc() { return MOD_NAME; }
    virtual void load_config(unsigned char* data, int len);
    virtual int save_config(unsigned char* data);

    int framecount;
    int enabled;
    int mode;
    int onbeat;
    int rbeat;
    int smooth;
    int slower;
};
