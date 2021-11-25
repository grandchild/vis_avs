#pragma once

#include "c__base.h"

#include "../platform.h"

#include <string>

#define C_THISCLASS C_DMoveClass
#define MOD_NAME    "Trans / Dynamic Movement"

typedef struct {
    char* name;
    int rect;
    int wrap;
    int grid1;
    int grid2;
    char* init;
    char* point;
    char* frame;
    char* beat;
} dmovePresetType;

class C_THISCLASS : public C_RBASE2 {
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

    virtual int smp_getflags() { return 1; }
    virtual int smp_begin(int max_threads,
                          char visdata[2][2][576],
                          int isBeat,
                          int* framebuffer,
                          int* fbout,
                          int w,
                          int h);
    virtual void smp_render(int this_thread,
                            int max_threads,
                            char visdata[2][2][576],
                            int isBeat,
                            int* framebuffer,
                            int* fbout,
                            int w,
                            int h);
    virtual int smp_finish(char visdata[2][2][576],
                           int isBeat,
                           int* framebuffer,
                           int* fbout,
                           int w,
                           int h);  // return value is that of render() for fbstuff etc

    virtual char* get_desc() { return MOD_NAME; }
    virtual void load_config(unsigned char* data, int len);
    virtual int save_config(unsigned char* data);
    std::string effect_exp[4];

    int m_lastw, m_lasth;
    int m_lastxres, m_lastyres, m_xres, m_yres;
    int* m_wmul;
    int* m_tab;
    int AVS_EEL_CONTEXTNAME;
    double *var_d, *var_b, *var_r, *var_x, *var_y, *var_w, *var_h, *var_alpha;
    int inited;
    int codehandle[4];
    int need_recompile;
    int buffern;
    int subpixel, rectcoords, blend, wrap, nomove;
    lock_t* code_lock;

    // smp stuff
    int __subpixel, __rectcoords, __blend, __wrap, __nomove;
    int w_adj;
    int h_adj;
    int XRES;
    int YRES;

    dmovePresetType presets[8] = {
        {"Random Rotate",
         0,
         1,
         2,
         2,
         "",
         "r = r + dr;",
         "",
         "dr = (rand(100) / 100) * $PI;\r\nd = d * .95;"},
        {"Random Direction",
         1,
         1,
         2,
         2,
         "speed=.05;dr = (rand(200) / 100) * $PI;",
         "x = x + dx;\r\ny = y + dy;",
         "dx = cos(dr) * speed;\r\ndy = sin(dr) * speed;",
         "dr = (rand(200) / 100) * $PI;"},
        {"In and Out",
         0,
         1,
         2,
         2,
         "speed=.2;c=0;",
         "d = d * dd;",
         "",
         "c = c + ($PI/2);\r\ndd = 1 - (sin(c) * speed);"},
        {"Unspun Kaleida",
         0,
         1,
         33,
         33,
         "c=200;f=0;dt=0;dl=0;beatdiv=8",
         "r=cos(r*dr);",
         "f = f + 1;\r\nt = ((f * $pi * 2)/c)/beatdiv;\r\ndt = dl + t;\r\ndr = "
         "4+(cos(dt)*2);",
         "c=f;f=0;dl=dt"},
        {"Roiling Gridley",
         1,
         1,
         32,
         32,
         "c=200;f=0;dt=0;dl=0;beatdiv=8",
         "x=x+(sin(y*dx) * .03);\r\ny=y-(cos(x*dy) * .03);",
         "f = f + 1;\r\nt = ((f * $pi * 2)/c)/beatdiv;\r\ndt = dl + t;\r\ndx = "
         "14+(cos(dt)*8);\r\ndy = 10+(sin(dt*2)*4);",
         "c=f;f=0;dl=dt"},
        {"6-Way Outswirl",
         0,
         0,
         32,
         32,
         "c=200;f=0;dt=0;dl=0;beatdiv=8",
         "d=d*(1+(cos(r*6) * .05));\r\nr=r-(sin(d*dr) * .05);\r\nd = d * .98;",
         "f = f + 1;\r\nt = ((f * $pi * 2)/c)/beatdiv;\r\ndt = dl + t;\r\ndr = "
         "18+(cos(dt)*12);",
         "c=f;f=0;dl=dt"},
        {"Wavy",
         1,
         1,
         6,
         6,
         "c=200;f=0;dx=0;dl=0;beatdiv=16;speed=.05",
         "y = y + ((sin((x+dx) * $PI))*speed);\r\nx = x + .025",
         "f = f + 1;\r\nt = ( (f * 2 * 3.1415) / c ) / beatdiv;\r\ndx = dl + t;",
         "c = f;\r\nf = 0;\r\ndl = dx;"},
        {"Smooth Rotoblitter",
         0,
         1,
         2,
         2,
         "c=200;f=0;dt=0;dl=0;beatdiv=4;speed=.15",
         "r = r + dr;\r\nd = d * dd;",
         "f = f + 1;\r\nt = ((f * $pi * 2)/c)/beatdiv;\r\ndt = dl + t;\r\ndr = "
         "cos(dt)*speed*2;\r\ndd = 1 - (sin(dt)*speed);",
         "c=f;f=0;dl=dt"},
    };
};
