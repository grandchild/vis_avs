#pragma once

#include "c__base.h"
#include <windows.h>

#define MOD_NAME "Render / Picture"
#define C_THISCLASS C_PictureClass


class C_THISCLASS : public C_RBASE {
	protected:
	public:
		C_THISCLASS();
		virtual ~C_THISCLASS();
		virtual int render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h);
		virtual char *get_desc() { return MOD_NAME; }
		virtual void load_config(unsigned char *data, int len);
		virtual int  save_config(unsigned char *data);
		void loadPicture(char *name);
		void freePicture();

    int enabled;
		int width,height;
    HBITMAP hOldBitmap; 
		HBITMAP hb;
		HBITMAP hb2;
    HDC hBitmapDC; 
    HDC hBitmapDC2; 
		int lastWidth,lastHeight;
		int blend, blendavg, adapt, persist;
		int ratio,axis_ratio;
		char ascName[MAX_PATH];
  	int persistCount;
};
