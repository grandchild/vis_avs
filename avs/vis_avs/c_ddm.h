#pragma once

#include "c__base.h"

#include <windows.h>
#include <string>

#define C_THISCLASS C_PulseClass
#define MOD_NAME    "Trans / Dynamic Distance Modifier"

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
    std::string effect_exp[4];
    int blend;

    int m_wt;
    int m_lastw, m_lasth;
    int* m_wmul;
    int* m_tab;
    int AVS_EEL_CONTEXTNAME;
    double *var_d, *var_b;
    double max_d;
    int inited;
    int codehandle[4];
    int need_recompile;
    int subpixel;
    CRITICAL_SECTION rcs;
};
