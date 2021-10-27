#pragma once

#include "ape.h"
#include "c__base.h"

#include <string>
#include <vector>

#define MOD_NAME "Misc / Global Variables"

class GlobalVarsVars : public VarsBase {
   public:
    double* w;
    double* h;
    double* b;
    double* load;
    double* save;

    virtual void register_variables(void*);
    virtual void init_variables(int, int, int, ...);
};

enum GlobalVarsLoadOpts {
    GLOBALVARS_LOAD_NONE = 0,
    GLOBALVARS_LOAD_ONCE,
    GLOBALVARS_LOAD_CODE,
    GLOBALVARS_LOAD_FRAME,
};

class C_GlobalVars {
   public:
    C_GlobalVars();
    virtual ~C_GlobalVars();
    virtual int render(char visdata[2][2][576],
                       int isBeat,
                       int* framebuffer,
                       int* fbout,
                       int w,
                       int h);
    virtual char* get_desc();
    virtual void load_config(unsigned char* data, int len);
    virtual int save_config(unsigned char* data);

    ComponentCode<GlobalVarsVars> code;
    GlobalVarsLoadOpts load_option;
    bool is_first_frame;
    bool load_code_next_frame;
    std::string filepath;
    std::string reg_ranges_str;
    std::vector<int*> reg_ranges;
    static const int max_regs_index;
    std::string buf_ranges_str;
    std::vector<int*> buf_ranges;
    static const int max_gmb_index;
    void exec_code_from_file(char visdata[2][2][576]);
    void save_ranges_to_file();
    bool check_set_range(std::string input, std::vector<int*>& ranges, int max_index);
    double reg_value(int index);
    double buf_value(int index);

    char* help_text =
        "Global Variable Manager\0"
        "The Global Variable Manager (Misc / Global Variables) is a component designed "
        "to ease handling the gmegabuf and reg## global variables in AVS.\r\n"
        "\r\n"
        "w, h \r\n"
        "  The width and the height of the screen respectively\r\n"
        "\r\n"
        "b\r\n"
        "  Set to 1 on beat and is otherwise 0\r\n"
        "\r\n"
        "load, save\r\n"
        "  These start as 0 every frame and when set to nonzero values in code these "
        "signal to either load or save at the beginning of the next frame.\r\n"
        "\r\n"
        "The first part of the component is custom init, frame and beat code, this "
        "allows you to do things like set up a global timer or work with the gmegabuf "
        "before you access it with superscopes or other programmable components. It "
        "doesn't allow you to render anything directly and works much like a "
        "superscope with n=0.\r\n"
        "\r\n"
        "The second feature is the load/save functionality, which allows you to load "
        "or save reg## or gmegabuf to files. The variables are saved by generating a "
        "block of code which sets them, when loaded this block of code is executed. "
        "The code is easily readable and can be hand edited, you may declare variables "
        "and such in here but they won't be maintained after load or passed on into "
        "the main code.\r\n"
        "\r\n"
        "The loading and saving can be code-controlled using the load and save "
        "variables described above, if 'code control' is enabled. Alternatively you "
        "can set it to load once or every frame. The data is always loaded at the "
        "beginning of the next frame so that you can create loading screens.\r\n"
        "\r\n"
        "Enter numerical ranges for the register (reg00..reg99) and gmegabuf values "
        "you want to load/save into the range boxes.\r\n"
        "Enter them like a print range in MS Word, e.g. 1-4,10-38,50,99. If the range "
        "is invalid it won't be used and Err will be displayed next to it. Note that "
        "the range is only used to decide how much data is saved. When loading a file "
        "it is simply loaded and has its code executed which will typically fill the "
        "range specified when that file was saved. The file will be stored with the "
        "specified name in the AVS directory. If you specify a name like "
        "\"path\\filename.gvm\" then you can save to subdirectories within AVS' main "
        "folder too.";
};
