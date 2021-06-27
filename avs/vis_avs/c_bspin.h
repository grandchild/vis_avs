#pragma once

#include "c__base.h"

#define C_THISCLASS C_BSpinClass
#define MOD_NAME "Render / Bass Spin"


class C_THISCLASS : public C_RBASE {
	protected:
	public:
		C_THISCLASS();
		virtual ~C_THISCLASS();
		virtual int render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h);
		virtual char *get_desc() { return MOD_NAME; }
		virtual void load_config(unsigned char *data, int len);
		virtual int  save_config(unsigned char *data);

		void my_triangle(int *fb, int points[6], int width, int height, int color);
		int enabled;
		int colors[2];
		int mode;


		int last_a;
		int lx[2][2],ly[2][2];
		double r_v[2];
		double v[2];
		double dir[2];
};
