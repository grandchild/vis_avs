#pragma once

#include "c__base.h"

#include <windows.h>
#include <string>

#define C_THISCLASS C_ShiftClass
#define MOD_NAME    "Trans / Dynamic Shift"

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

    std::string effect_exp[3];
    int blend, subpixel;

    int m_lastw, m_lasth;
    int AVS_EEL_CONTEXTNAME;
    double *var_x, *var_y, *var_w, *var_h, *var_b, *var_alpha;
    double max_d;
    int inited;
    int codehandle[3];
    int need_recompile;
    CRITICAL_SECTION rcs;
};
