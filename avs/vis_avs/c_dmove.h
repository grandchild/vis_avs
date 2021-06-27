#pragma once

#include "c__base.h"
#include <windows.h>
#include <string>

#define C_THISCLASS C_DMoveClass
#define MOD_NAME "Trans / Dynamic Movement"


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
    std::string effect_exp[4];

    int m_lastw,m_lasth;
    int m_lastxres, m_lastyres, m_xres, m_yres;
    int *m_wmul;
    int *m_tab;
    int AVS_EEL_CONTEXTNAME;
    double *var_d, *var_b, *var_r, *var_x, *var_y, *var_w, *var_h, *var_alpha;
		int inited;
    int codehandle[4];
    int need_recompile;
    int buffern;
    int subpixel,rectcoords,blend,wrap, nomove;
    CRITICAL_SECTION rcs;


    // smp stuff
    int __subpixel,__rectcoords,__blend,__wrap, __nomove;
    int w_adj;
    int h_adj;
    int XRES;
    int YRES;

};
