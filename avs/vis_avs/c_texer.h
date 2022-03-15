#pragma once

#include "c__base.h"
#include "pixel_format.h"

#include <vector>
#define MOD_NAME "Trans / Texer"
#define MAX_PATH 260

enum Texer_Input_Mode {
    TEXER_INPUT_IGNORE,
    TEXER_INPUT_REPLACE,
};

enum Texer_Output_Mode {
    TEXER_OUTPUT_NORMAL,
    TEXER_OUTPUT_MASKED,
};

class C_Texer : public C_RBASE {
   public:
    C_Texer();
    virtual ~C_Texer();
    virtual int render(char visdata[2][2][576],
                       int isBeat,
                       int* framebuffer,
                       int* fbout,
                       int w,
                       int h);
    virtual char* get_desc();
    virtual void load_config(unsigned char* data, int len);
    virtual int save_config(unsigned char* data);

    void find_image_files();
    void clear_image_files();
    bool load_image();

    char image[MAX_PATH];
    Texer_Input_Mode input_mode;
    Texer_Output_Mode output_mode;
    int num_particles;
    static std::vector<char*> file_list;

   private:
    void render_particle(int* framebuffer, int* fbout, int x, int y, int w, int h);
    void render_particle_sse2(int* framebuffer, int* fbout, int x, int y, int w, int h);

    pixel_rgb0_8* image_data;
    int32_t image_width;
    int32_t image_width_half;
    int32_t image_width_other_half;
    int32_t image_data_width;
    int32_t image_height;
    int32_t image_height_half;
    int32_t image_height_other_half;
    bool need_image_refresh;
    static int instance_count;
};
