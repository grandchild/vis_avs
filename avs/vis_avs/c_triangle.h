#pragma once

#include <windows.h>

#include "c__base.h"

#define MOD_NAME "Render / Triangle"

#define OUT_REPLACE 0
#define OUT_ADDITIVE 1
#define OUT_MAXIMUM 2
#define OUT_5050 3
#define OUT_SUB1 4
#define OUT_SUB2 5
#define OUT_MULTIPLY 6
#define OUT_ADJUSTABLE 7
#define OUT_XOR 8
#define OUT_MINIMUM 9

struct Code {
    char* init;
    char* frame;
    char* beat;
    char* point;
    Code() {
        init = new char[1];
        frame = new char[1];
        beat = new char[1];
        point = new char[1];
        init[0] = frame[0] = beat[0] = point[0] = 0;
    }
    ~Code() {
        delete[] init;
        delete[] frame;
        delete[] beat;
        delete[] point;
    }
};

class C_Triangle : public C_RBASE {
   public:
    C_Triangle();
    virtual ~C_Triangle();
    virtual int render(char visdata[2][2][576],
                       int isBeat,
                       int* framebuffer,
                       int* fbout,
                       int w,
                       int);
    virtual char* get_desc();
    virtual void load_config(unsigned char* data, int len);
    virtual int save_config(unsigned char* data);
    Code code;

   protected:
};

struct TriangleVars {
    double* w;
    double* h;
    double* n;
    double* i;
    double* skip;
    double* x1;
    double* y1;
    double* red1;
    double* green1;
    double* blue1;
    double* x2;
    double* y2;
    double* red2;
    double* green2;
    double* blue2;
    double* x3;
    double* y3;
    double* red3;
    double* green3;
    double* blue3;
    double* z1;
    double* zbuf;
    double* zbclear;
};
