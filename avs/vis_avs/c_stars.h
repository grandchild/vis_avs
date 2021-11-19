#pragma once

#include "c__base.h"

#define MOD_NAME    "Render / Starfield"
#define C_THISCLASS C_StarField

typedef struct {
    int X, Y;
    float Z;
    float Speed;
    int OX, OY;
} StarFormat;

class C_THISCLASS : public C_RBASE {
   protected:
   public:
    C_THISCLASS();
    float GET_FLOAT(unsigned char* data, int pos);
    void PUT_FLOAT(float f, unsigned char* data, int pos);
    void InitializeStars(void);
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
    int color;
    int MaxStars, MaxStars_set;
    int Xoff;
    int Yoff;
    int Zoff;
    float WarpSpeed;
    int blend;
    int blendavg;
    StarFormat Stars[4096];
    int Width, Height;
    int onbeat;
    float spdBeat;
    float incBeat;
    int durFrames;
    float CurrentSpeed;
    int nc;
};
