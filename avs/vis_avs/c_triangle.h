#pragma once

#include <windows.h>

#include "ape.h"
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

struct TriangleCode {
    TriangleVars vars;
    VM_CONTEXT vm_context;

    char* init_str;
    char* frame_str;
    char* beat_str;
    char* point_str;
    VM_CODEHANDLE init;
    VM_CODEHANDLE frame;
    VM_CODEHANDLE beat;
    VM_CODEHANDLE point;

    TriangleCode() {
        init_str = new char[1];
        frame_str = new char[1];
        beat_str = new char[1];
        point_str = new char[1];
        init_str[0] = frame_str[0] = beat_str[0] = point_str[0] = 0;
    }
    ~TriangleCode() {
        delete[] init_str;
        delete[] frame_str;
        delete[] beat_str;
        delete[] point_str;
    }
    void recompile();
};

class TriangleDepthBuffer {
   public:
    u_int w;
    u_int h;
    double* buffer;

    TriangleDepthBuffer(u_int w, u_int h) {
        this->w = w;
        this->h = h;
        this->buffer = new double[w * h];
    }
    ~TriangleDepthBuffer() { delete[] this->buffer; }
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
    TriangleCode code;

   protected:
    static u_int instance_count;
    static TriangleDepthBuffer* depth_buffer;
    bool need_depth_buffer = false;
};
