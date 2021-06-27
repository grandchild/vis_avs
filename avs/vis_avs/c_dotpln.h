#pragma once

#include "c__base.h"

#define C_THISCLASS C_DotPlaneClass
#define MOD_NAME "Render / Dot Plane"
#define NUM_WIDTH 64


class C_THISCLASS : public C_RBASE {
	protected:
		float r;
		float atable[NUM_WIDTH*NUM_WIDTH];
		float vtable[NUM_WIDTH*NUM_WIDTH];
		int   ctable[NUM_WIDTH*NUM_WIDTH];
		int color_tab[64];
	public:
		C_THISCLASS();
		virtual ~C_THISCLASS();
		virtual int render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int, int);
		virtual char *get_desc() { return MOD_NAME; }
		virtual void load_config(unsigned char *data, int len);
		virtual int  save_config(unsigned char *data);

		int rotvel,angle;
		int colors[5];

		void initcolortab();
};
