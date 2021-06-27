#pragma once

#include "c__base.h"
#include <windows.h>

#define MOD_NAME "Render / Texer II"

#define OUT_REPLACE       0
#define OUT_ADDITIVE      1
#define OUT_MAXIMUM       2
#define OUT_5050          3
#define OUT_SUB1          4
#define OUT_SUB2          5
#define OUT_MULTIPLY      6
#define OUT_ADJUSTABLE    7
#define OUT_XOR           8
#define OUT_MINIMUM       9

#define ID_TEXER2_EXAMPLE_COLOROSC 31337
#define ID_TEXER2_EXAMPLE_FLUMMYSPEC 31338
#define ID_TEXER2_EXAMPLE_BEATCIRCLE 31339
#define ID_TEXER2_EXAMPLE_3DRINGS 31340

#define TEXER_II_VERSION_V2_81D 0
#define TEXER_II_VERSION_V2_81D_UPGRADE_HELP \
    "Saved version wraps x/y only once. Upgrade to wrap around forever."
#define TEXER_II_VERSION_CURRENT 1

struct texer2_apeconfig {
    int version;  /* formerly "mode", which was unused. */
    char img[MAX_PATH];
    int resize;
    int wrap;
    int mask;
    int d;
};

struct Code {
    char *init;
    char *frame;
    char *beat;
    char *point;
    Code() { init = new char[1]; frame = new char[1]; beat = new char[1]; point = new char[1]; init[0] = frame[0] = beat[0] = point[0] = 0; }
    ~Code() { delete[] init; delete[] frame; delete[] beat; delete[] point; }

    void SetInit(char *str) { delete init; init = str; }
    void SetFrame(char *str) { delete frame; frame = str; }
    void SetBeat(char *str) { delete beat; beat = str; }
    void SetPoint(char *str) { delete point; point = str; }
};

struct Vars {
    double *n;
    double *i;
    double *x;
    double *y;
    double *w;
    double *h;
    double *iw;
    double *ih;
    double *v;
    double *sizex;
    double *sizey;
    double *red;
    double *green;
    double *blue;
    double *skip;
};

typedef void *VM_CONTEXT;
typedef void *VM_CODEHANDLE;

class C_Texer2 : public C_RBASE
{
    protected:
    public:
        C_Texer2(int default_version);
        virtual ~C_Texer2();
        virtual int render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h);
        virtual char *get_desc();
        virtual void load_config(unsigned char *data, int len);
        virtual int  save_config(unsigned char *data);

        virtual void InitTexture();
        virtual void DeleteTexture();

        virtual void Recompile();
        virtual void DrawParticle(int *framebuffer, int *texture, int w, int h, double x, double y, double sizex, double sizey, unsigned int color);
        virtual void LoadExamples(HWND button, HWND ctl, bool is_init);

        texer2_apeconfig config;

        VM_CONTEXT context;
        VM_CODEHANDLE codeinit;
        VM_CODEHANDLE codeframe;
        VM_CODEHANDLE codebeat;
        VM_CODEHANDLE codepoint;
        Vars vars;

        Code code;

        HWND hwndDlg;
        HBITMAP bmp;
        HDC bmpdc;
        HBITMAP bmpold;
        int iw;
        int ih;
        int *texbits_normal;
        int *texbits_flipped;
        int *texbits_mirrored;
        int *texbits_rot180;
        bool init;

        CRITICAL_SECTION imageload;
        CRITICAL_SECTION codestuff;

        char* help_text = "Texer II\0"
            "Texer II is a rendering component that draws what is commonly known as particles.\r\n"
            "At specified positions on screen, a copy of the source image is placed and blended in various ways.\r\n"
            "\r\n"
            "\r\n"
            "The usage is similar to that of a dot-superscope.\r\n"
            "The following variables are available:\r\n"
            "\r\n"
            "* n: Contains the amount of particles to draw. Usually set in the init code or the frame code.\r\n"
            "\r\n"
            "* w,h: Contain the width and height of the window in pixels.\r\n"
            "\r\n"
            "* i: Contains the percentage of progress of particles drawn. Varies from 0 (first particle) to 1 (last particle).\r\n"
            "\r\n"
            "* x,y: Specify the position of the center of the particle. Range -1 to 1.\r\n"
            "\r\n"
            "* v: Contains the i'th sample of the oscilloscope waveform. Use getspec(...) or getosc(...) for other data (check the function reference for their usage).\r\n"
            "\r\n"
            "* b: Contains 1 if a beat is occuring, 0 otherwise.\r\n"
            "\r\n"
            "* iw, ih: Contain the width and height of the Texer bitmap in pixels\r\n"
            "\r\n"
            "* sizex, sizey: Contain the relative horizontal and vertical size of the current particle. Use 1.0 for original size. Negative values cause the image to be mirrored or flipped. Changing size only works if Resizing is on; mirroring/flipping works in all modes.\r\n"
            "\r\n"
            "* red, green, blue: Set the color of the current particle in terms of its red, green and blue component. Only works if color filtering is on.\r\n"
            "\r\n"
            "* skip: Default is 0. If set to 1, indicates that the current particle should not be drawn.\r\n"
            "\r\n"
            "\r\n"
            "\r\n"
            "The options available are:\r\n"
            "\r\n"
            "* Color filtering: blends the image multiplicatively with the color specified by the red, green and blue variables.\r\n"
            "\r\n"
            "* Resizing: resizes the image according to the variables sizex and sizey.\r\n"
            "\r\n"
            "* Wrap around: wraps any image data that falls off the border of the screen around to the other side. Useful for making tiled patterns.\r\n"
            "\r\n"
            "\r\n"
            "\r\n"
            "Texer II supports the standard AVS blend modes. Just place a Misc / Set Render Mode before the Texer II and choose the appropriate setting. You will most likely use Additive or Maximum blend.\r\n"
            "\r\n"
            "Texer II was written by Steven Wittens. Thanks to everyone at the Winamp.com forums for feedback, especially Deamon and gaekwad2 for providing the examples and Tuggummi for providing the default image.\r\n";
};
