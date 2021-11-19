#pragma once

#include "c__base.h"

#define MOD_NAME    "Trans / Multiplier"
#define C_THISCLASS C_MultiplierClass
#define MD_XI       0
#define MD_X8       1
#define MD_X4       2
#define MD_X2       3
#define MD_X05      4
#define MD_X025     5
#define MD_X0125    6
#define MD_XS       7

typedef struct {
    int ml;
} apeconfig;

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
    virtual char* get_desc();
    virtual void load_config(unsigned char* data, int len);
    virtual int save_config(unsigned char* data);

    apeconfig config;
};
