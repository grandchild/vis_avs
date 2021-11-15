#pragma once

#include "c__base.h"

#include <vector>
#include <windows.h>  // CRITICAL_SECTION

#define MOD_NAME "Render / Picture II"

#define OUT_REPLACE 0
#define OUT_ADDITIVE 1
#define OUT_MAXIMUM 2
#define OUT_MINIMUM 3
#define OUT_5050 4
#define OUT_SUB1 5
#define OUT_SUB2 6
#define OUT_MULTIPLY 7
#define OUT_XOR 8
#define OUT_ADJUSTABLE 9
#define OUT_IGNORE 10

struct picture2_config {
    char image[MAX_PATH];
    int output;
    int outputbeat;
    int bilinear;
    int bilinearbeat;
    int adjustable;
    int adjustablebeat;
};

class C_Picture2 : public C_RBASE {
   public:
    C_Picture2();
    virtual ~C_Picture2();
    virtual int render(char visdata[2][2][576],
                       int isBeat,
                       int* framebuffer,
                       int* fbout,
                       int w,
                       int h);
    virtual char* get_desc();
    virtual void load_config(unsigned char* data, int len);
    virtual int save_config(unsigned char* data);

    bool load_image();
    void refresh_image(int w, int h);
    void find_image_files();
    void clear_image_files();

    CRITICAL_SECTION criticalsection;
    picture2_config config;
    int* image;
    int iw, ih;
    int* work_image;
    int* work_image_bilinear;
    int wiw, wih;
    bool need_image_refresh;
    static std::vector<char*> file_list;
    static u_int instance_count;

    const char* blendmodes[11] = {
        "Replace",
        "Additive",
        "Maximum",
        "Minimum",
        "50/50",
        "Subtractive 1",
        "Subtractive 2",
        "Multiply",
        "XOR",
        "Adjustable",
        "Ignore",
    };
};
