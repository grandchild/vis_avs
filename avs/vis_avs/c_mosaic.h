#pragma once

#include "c__base.h"

#define MOD_NAME    "Trans / Mosaic"
#define C_THISCLASS C_MosaicClass

class C_THISCLASS : public C_RBASE {
   protected:
   public:
    C_THISCLASS();
    float GET_FLOAT(unsigned char* data, int pos);
    void PUT_FLOAT(float f, unsigned char* data, int pos);
    void InitializeStars(int Start);
    void CreateStar(int A);
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
    int quality;
    int quality2;
    int blend;
    int blendavg;
    int onbeat;
    int durFrames;
    int nF;
    int thisQuality;
};
