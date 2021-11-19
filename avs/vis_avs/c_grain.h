#pragma once

#include "c__base.h"

#define MOD_NAME    "Trans / Grain"
#define C_THISCLASS C_GrainClass

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
    unsigned char __inline fastrandbyte(void);
    void reinit(int w, int h);
    int enabled;
    int blend, blendavg, smax;
    unsigned char* depthBuffer;
    int oldx, oldy;
    int staticgrain;
    unsigned char randtab[491];
    int randtab_pos;
};
