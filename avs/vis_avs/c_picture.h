#pragma once

#include "r_defs.h"

#include "c__base.h"
#include "pixel_format.h"

#include "../platform.h"

#include <vector>

#define MOD_NAME    "Render / Picture"
#define C_THISCLASS C_PictureClass

class C_THISCLASS : public C_RBASE {
   protected:
   public:
    C_THISCLASS();
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
    void find_image_files();
    void clear_image_files();
    void load_image();

    int enabled;
    int width;
    int height;
    int blend;
    int blendavg;
    int adapt;
    int persist;
    int persistCount;
    int ratio;
    int axis_ratio;
    char image[LEGACY_SAVE_PATH_LEN];
    pixel_rgb8* image_data;
    lock_t* image_lock;

    static std::vector<char*> file_list;
    static unsigned int instance_count;
};
