#pragma once

#include "c__base.h"
#include <windows.h>
#include "svp_vis.h"

#define C_THISCLASS C_SVPClass
#define MOD_NAME "Render / SVP Loader"


class C_THISCLASS : public C_RBASE {
	protected:
	public:
		C_THISCLASS();
		virtual ~C_THISCLASS();
		virtual int render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h);
		virtual char *get_desc() { return MOD_NAME; }
		virtual void load_config(unsigned char *data, int len);
		virtual int  save_config(unsigned char *data);


    char m_library[MAX_PATH];
    void SetLibrary();
    HMODULE hLibrary;
    CRITICAL_SECTION cs;
    VisInfo *vi;
    VisData vd;
};
