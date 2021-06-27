#pragma once

#include "c__base.h"
#include <windows.h>
#include <string>

#define C_THISCLASS C_DColorModClass
#define MOD_NAME "Trans / Color Modifier"


class C_THISCLASS : public C_RBASE {
	protected:
	public:
		C_THISCLASS();
		virtual ~C_THISCLASS();
		virtual int render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h);
		virtual char *get_desc() { return MOD_NAME; }
		virtual void load_config(unsigned char *data, int len);
		virtual int  save_config(unsigned char *data);
    std::string effect_exp[4];

    int m_recompute;
    
    int m_tab_valid;
    unsigned char m_tab[768];
    int AVS_EEL_CONTEXTNAME;
    double *var_r, *var_g, *var_b, *var_beat;
		int inited;
    int codehandle[4];
    int need_recompile;
    CRITICAL_SECTION rcs;
};
