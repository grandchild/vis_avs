#pragma once

#include <windows.h>

#define MOD_NAME    "Render / Text"
#define C_THISCLASS C_TextClass

class C_THISCLASS {
   protected:
   public:
    C_THISCLASS();
    void getWord(int n, char* buf, int max);
    void reinit(int w, int h);
    float GET_FLOAT(unsigned char* data, int pos);
    void PUT_FLOAT(float f, unsigned char* data, int pos);
    void InitializeStars(int Start);
    void CreateStar(int A);
    virtual ~C_THISCLASS();
    virtual int render(char visdata[2][2][576],
                       int is_beat,
                       int* framebuffer,
                       int* fbout,
                       int w,
                       int h);
    virtual char* get_desc() { return MOD_NAME; }
    virtual void load_config(unsigned char* data, int len);
    virtual int save_config(unsigned char* data);
    CHOOSEFONT cf;
    LOGFONT lf;
    HBITMAP hRetBitmap;
    HBITMAP hOldBitmap;
    HFONT hOldFont;
    HFONT myFont;
    HDC hDesktopDC;
    HDC hBitmapDC;
    BITMAPINFO bi;
    int enabled;
    int color;
    int blend;
    int blendavg;
    int onbeat;
    int insertBlank;
    int randomPos;
    int valign;
    int halign;
    int onbeatSpeed;
    int normSpeed;
    char* text;
    int lw, lh;
    bool updating;
    int outline;
    int shadow;
    int outlinecolor;
    int outlinesize;
    int curword;
    int nb;
    int forceBeat;
    int xshift, yshift;
    int _xshift, _yshift;
    int forceshift;
    int forcealign;
    int _valign, _halign;
    int oddeven;
    int nf;
    RECT r;
    int* myBuffer;
    int forceredraw;
    int old_valign, old_halign, old_outline, oldshadow;
    int old_curword, old_clipcolor;
    char oldtxt[256];
    int old_blend1, old_blend2, old_blend3;
    int oldxshift, oldyshift;
    int randomword;
    int shiftinit;
};
