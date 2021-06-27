#pragma once

#include "c__base.h"
#include <windows.h>
#include <string>

#define C_THISCLASS C_SScopeClass
#define MOD_NAME "Render / SuperScope"


class C_THISCLASS : public C_RBASE {
	public:
		C_THISCLASS();
		virtual ~C_THISCLASS();
		virtual int render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h);
		virtual char *get_desc() { return MOD_NAME; }
		virtual void load_config(unsigned char *data, int len);
		virtual int  save_config(unsigned char *data);
    std::string effect_exp[4];
    int which_ch;
    int num_colors;
		int colors[16];
    int mode;

    int color_pos;

    int AVS_EEL_CONTEXTNAME;
    double *var_b, *var_x, *var_y, *var_i, *var_n, *var_v, *var_w, *var_h, *var_red, *var_green, *var_blue;
    double *var_skip, *var_linesize, *var_drawmode;
		int inited;
    int codehandle[4];
    int need_recompile;
    CRITICAL_SECTION rcs;
};
