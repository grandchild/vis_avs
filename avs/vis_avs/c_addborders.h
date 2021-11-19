#pragma once

#include "c__base.h"

#define MOD_NAME             "Render / Add Borders"
#define ADDBORDERS_WIDTH_MIN 1
#define ADDBORDERS_WIDTH_MAX 50

class C_AddBorders : public C_RBASE {
   public:
    C_AddBorders();
    virtual ~C_AddBorders();
    virtual int render(char visdata[2][2][576],
                       int is_beat,
                       int* framebuffer,
                       int* fbout,
                       int w,
                       int);
    virtual char* get_desc();
    virtual void load_config(unsigned char* data, int len);
    virtual int save_config(unsigned char* data);
    bool enabled;
    int color;
    int width;
};
