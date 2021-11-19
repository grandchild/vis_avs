#pragma once

#include "c__base.h"

#define MOD_NAME    "Trans / Interferences"
#define C_THISCLASS C_InterferencesClass

class C_THISCLASS : public C_RBASE {
   protected:
   public:
    C_THISCLASS();
    virtual ~C_THISCLASS();
    float GET_FLOAT(unsigned char* data, int pos);
    void PUT_FLOAT(float f, unsigned char* data, int pos);
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
    int nPoints;
    int distance;
    int alpha;
    int rotation;
    int rotationinc;
    int distance2;
    int alpha2;
    int rotationinc2;
    int rgb;
    int blend;
    int blendavg;
    float speed;
    int onbeat;
    float a;
    int _distance;
    int _alpha;
    int _rotationinc;
    int _rgb;
    float status;
};
