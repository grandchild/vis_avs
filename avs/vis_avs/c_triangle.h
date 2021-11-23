#pragma once

#include "ape.h"
#include "c__base.h"

#define MOD_NAME "Render / Triangle"

#define OUT_REPLACE    0
#define OUT_ADDITIVE   1
#define OUT_MAXIMUM    2
#define OUT_5050       3
#define OUT_SUB1       4
#define OUT_SUB2       5
#define OUT_MULTIPLY   6
#define OUT_ADJUSTABLE 7
#define OUT_XOR        8
#define OUT_MINIMUM    9

class TriangleVars : public VarsBase {
   public:
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

    virtual void register_variables(void*);
    virtual void init_variables(int, int, int, va_list);
};

class TriangleDepthBuffer {
   public:
    unsigned int w;
    unsigned int h;
    unsigned int* buffer;

    TriangleDepthBuffer(unsigned int w, unsigned int h);
    ~TriangleDepthBuffer();
    void reset_if_needed(unsigned int w, unsigned int h, bool clear);
};

typedef struct {
    int x;
    int y;
    uint64_t z;
} Vertex;

class C_Triangle : public C_RBASE {
   public:
    C_Triangle();
    virtual ~C_Triangle();
    virtual int render(char visdata[2][2][576],
                       int isBeat,
                       int* framebuffer,
                       int* fbout,
                       int w,
                       int h);
    virtual char* get_desc();
    virtual void load_config(unsigned char* data, int len);
    virtual int save_config(unsigned char* data);
    ComponentCode<TriangleVars> code;

   private:
    static unsigned int instance_count;
    static TriangleDepthBuffer* depth_buffer;
    bool need_depth_buffer = false;
    void init_depthbuffer_if_needed(int w, int h);
    inline void draw_triangle(int* framebuffer,
                              int w,
                              int h,
                              Vertex vertices[3],
                              bool use_depthbuffer,
                              unsigned int blendmode,
                              unsigned int adjustable_blend,
                              unsigned int color);
    inline void draw_line(int* framebuffer,
                          int fb_index,
                          int startx,
                          int endx,
                          uint64_t z,
                          int w,
                          bool use_depthbuffer,
                          unsigned int blendmode,
                          unsigned int adjustable_blend,
                          unsigned int color);
    inline void draw_pixel(int* pixel,
                           unsigned int blendmode,
                           unsigned int adjustable_blend,
                           unsigned int color);
    inline void sort_vertices(Vertex vertices[3]);
};
