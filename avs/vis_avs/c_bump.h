#pragma once

#include "c__base.h"

#include "../platform.h"

#include <string>

#define MOD_NAME    "Trans / Bump"
#define C_THISCLASS C_BumpClass

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
    int __inline depthof(int c, int i);
    int enabled;
    int depth;
    int depth2;
    int onbeat;
    int durFrames;
    int thisDepth;
    int blend;
    int blendavg;
    int nF;
    int codeHandle;
    int codeHandleBeat;
    int codeHandleInit;
    double* var_x;
    double* var_y;
    double* var_isBeat;
    double* var_isLongBeat;
    double* var_bi;
    std::string code1, code2, code3;
    int need_recompile;
    int showlight;
    int initted;
    int invert;
    int AVS_EEL_CONTEXTNAME;
    int oldstyle;
    int buffern;
    lock_t* code_lock;
};
