#include "ape.h"
#include "c__base.h"

#include <set>
#include <string>

#define MOD_NAME "Misc / AVSTrans Automation"
#define AVSTransVer "1.25.00"

class C_EelTrans : public C_RBASE {
   public:
    C_EelTrans();
    virtual ~C_EelTrans();

    virtual int render(char visdata[2][2][576],
                       int isBeat,
                       int* framebuffer,
                       int* fbout,
                       int w,
                       int h);

    virtual char* get_desc();

    virtual void load_config(unsigned char* data, int len);
    virtual int save_config(unsigned char* data);

    static char* pre_compile_hook(void* ctx, char* expression);
    static void post_compile_hook();

    static char* logpath;
    static bool log_enabled;
    static bool translate_enabled;
    static bool translate_firstlevel;
    static bool read_comment_codes;
    static bool need_new_first_instance;
    static int num_instances;
    static std::set<C_EelTrans*> instances;

    bool is_first_instance;
    std::string code;
};

char* translate(char const* input, EnumMode mode, bool translate_firstlevel);
