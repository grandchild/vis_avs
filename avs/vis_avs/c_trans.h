#pragma once

#include "c__base.h"
#include <windows.h>
#include <string>

#define C_THISCLASS C_TransTabClass
#define MOD_NAME "Trans / Movement"
#define REFFECT_MIN 3
#define REFFECT_MAX 23


typedef struct {
//    int index;       // Just here for descriptive purposes, makes it easy to match stuff up.
    char *list_desc; // The string to show in the listbox.
    char *eval_desc; // The optional string to display in the evaluation editor.
    char uses_eval;   // If this is true, the preset engages the eval library and there is NULL in the radial_effects array for its entry
    char uses_rect;   // This value sets the checkbox for rectangular calculation
} Description;

class C_THISCLASS : public C_RBASE2 {
	protected:
	public:
		C_THISCLASS();
		virtual ~C_THISCLASS();
		virtual int render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h);

    virtual int smp_getflags() { return 1; }
		virtual int smp_begin(int max_threads, char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h); 
    virtual void smp_render(int this_thread, int max_threads, char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h); 
    virtual int smp_finish(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h); // return value is that of render() for fbstuff etc

		virtual char *get_desc() { return MOD_NAME; }
		virtual void load_config(unsigned char *data, int len);
		virtual int  save_config(unsigned char *data);

    inline bool effect_uses_eval(int t);

    int *trans_tab, trans_tab_w, trans_tab_h, trans_tab_subpixel;
    int trans_effect;
    std::string effect_exp;
    int effect_exp_ch;
    int effect,blend;
    int sourcemapped;
    int rectangular;
    int subpixel;
    int wrap;
    CRITICAL_SECTION rcs;

    Description descriptions[24] = {
        {/* 0,*/ "none", "", 0, 0},
        {/* 1,*/ "slight fuzzify", "", 0, 0},
        {/* 2,*/ "shift rotate left", "x=x+1/32; // use wrap for this one", 0, 1},
        {/* 3,*/ "big swirl out", "r = r + (0.1 - (0.2 * d));\r\nd = d * 0.96;", 0, 0},
        {/* 4,*/ "medium swirl", "d = d * (0.99 * (1.0 - sin(r-$PI*0.5) / 32.0));\r\nr = r + (0.03 * sin(d * $PI * 4));", 0, 0},
        {/* 5,*/ "sunburster", "d = d * (0.94 + (cos((r-$PI*0.5) * 32.0) * 0.06));", 0, 0},
        {/* 6,*/ "swirl to center", "d = d * (1.01 + (cos((r-$PI*0.5) * 4) * 0.04));\r\nr = r + (0.03 * sin(d * $PI * 4));", 0, 0},
        {/* 7,*/ "blocky partial out", "", 0, 0},
        {/* 8,*/ "swirling around both ways at once", "r = r + (0.1 * sin(d * $PI * 5));", 0, 0},
        {/* 9,*/ "bubbling outward", "t = sin(d * $PI);\r\nd = d - (8*t*t*t*t*t)/sqrt((sw*sw+sh*sh)/4);", 0, 0},
        {/*10,*/ "bubbling outward with swirl", "t = sin(d * $PI);\r\nd = d - (8*t*t*t*t*t)/sqrt((sw*sw+sh*sh)/4);\r\nt=cos(d*$PI/2.0);\r\nr= r + 0.1*t*t*t;", 0, 0},
        {/*11,*/ "5 pointed distro", "d = d * (0.95 + (cos(((r-$PI*0.5) * 5.0) - ($PI / 2.50)) * 0.03));", 0, 0},
        {/*12,*/ "tunneling", "r = r + 0.04;\r\nd = d * (0.96 + cos(d * $PI) * 0.05);", 0, 0},
        {/*13,*/ "bleedin'", "t = cos(d * $PI);\r\nr = r + (0.07 * t);\r\nd = d * (0.98 + t * 0.10);", 0, 0},
        {/*14,*/ "shifted big swirl out", "// this is a very bad approximation in script. fixme.\r\nd=sqrt(x*x+y*y); r=atan2(y,x);\r\nr=r+0.1-0.2*d; d=d*0.96;\r\nx=cos(r)*d + 8/128; y=sin(r)*d;", 0, 1},
        {/*15,*/ "psychotic beaming outward", "d = 0.15", 0, 0},
        {/*16,*/ "cosine radial 3-way", "r = cos(r * 3)", 0, 0},
        {/*17,*/ "spinny tube", "d = d * (1 - ((d - .35) * .5));\r\nr = r + .1;", 0, 0},
        {/*18,*/ "radial swirlies", "d = d * (1 - (sin((r-$PI*0.5) * 7) * .03));\r\nr = r + (cos(d * 12) * .03);", 1, 0},
        {/*19,*/ "swill", "d = d * (1 - (sin((r - $PI*0.5) * 12) * .05));\r\nr = r + (cos(d * 18) * .05);\r\nd = d * (1-((d - .4) * .03));\r\nr = r + ((d - .4) * .13)", 1, 0},
        {/*20,*/ "gridley", "x = x + (cos(y * 18) * .02);\r\ny = y + (sin(x * 14) * .03);", 1, 1},
        {/*21,*/ "grapevine", "x = x + (cos(abs(y-.5) * 8) * .02);\r\ny = y + (sin(abs(x-.5) * 8) * .05);\r\nx = x * .95;\r\ny = y * .95;", 1, 1},
        {/*22,*/ "quadrant", "y = y * ( 1 + (sin(r + $PI/2) * .3) );\r\nx = x * ( 1 + (cos(r + $PI/2) * .3) );\r\nx = x * .995;\r\ny = y * .995;", 1, 1},
        {/*23,*/ "6-way kaleida (use wrap!)", "y = (r*6)/($PI); x = d;", 1, 1},
    };
};
