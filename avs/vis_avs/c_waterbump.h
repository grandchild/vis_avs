#pragma once

#include "c__base.h"

#define MOD_NAME "Trans / Water Bump"
#define C_THISCLASS C_WaterBumpClass


class C_THISCLASS : public C_RBASE {
	protected:
	public:
		C_THISCLASS();
		virtual ~C_THISCLASS();
		virtual int render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h);
		virtual char *get_desc() { return MOD_NAME; }
		virtual void load_config(unsigned char *data, int len);
		virtual int  save_config(unsigned char *data);
		void SineBlob(int x, int y, int radius, int height, int page);
		void CalcWater(int npage, int density);
		void CalcWaterSludge(int npage, int density);
		void HeightBlob(int x, int y, int radius, int height, int page);

    int enabled;
	int *buffers[2];
	int buffer_w,buffer_h;
	int page;
	int density;
	int depth;
	int random_drop;
	int drop_position_x;
	int drop_position_y;
	int drop_radius;
	int method;
};
