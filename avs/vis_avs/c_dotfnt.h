#pragma once

#include "c__base.h"

#define C_THISCLASS C_DotFountainClass
#define MOD_NAME "Render / Dot Fountain"
#define NUM_ROT_DIV 30
#define NUM_ROT_HEIGHT 256


typedef struct {
  float r, dr;
  float h, dh;
  float ax,ay;
  int c;
} FountainPoint;


class C_THISCLASS : public C_RBASE {
	protected:
		float r;
    FountainPoint points[NUM_ROT_HEIGHT][NUM_ROT_DIV];
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
