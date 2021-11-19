#pragma once

#include "c__base.h"

#include <windows.h>
#include <vfw.h>

#define MOD_NAME    "Render / AVI"
#define C_THISCLASS C_AVIClass

class C_THISCLASS : public C_RBASE {
   protected:
   public:
    C_THISCLASS();
    void reinit(int, int);
    void loadAvi(char* name);
    void closeAvi(void);
    virtual ~C_THISCLASS();
    virtual int render(char visdata[2][2][576],
                       int isBeat,
                       int* framebuffer,
                       int* fbout,
                       int w,
                       int h);
    virtual char* get_desc() { return MOD_NAME; }
    virtual void load_config(unsigned char* data, int len);
    virtual int save_config(unsigned char* data);
    int enabled;
    char ascName[MAX_PATH];
    int lastWidth, lastHeight;
    HDRAWDIB hDrawDib;
    PAVISTREAM PAVIVideo;
    PGETFRAME PgetFrame;
    HBITMAP hRetBitmap;
    HBITMAP hOldBitmap;
    HDC hDesktopDC;
    HDC hBitmapDC;
    LPBITMAPINFOHEADER lpFrame;
    BITMAPINFO bi;
    int blend, blendavg, adapt, persist;
    int loaded, rendering;
    int lFrameIndex;
    int length;
    unsigned int speed;
    unsigned int lastspeed;
    int *old_image, old_image_w, old_image_h;
};
