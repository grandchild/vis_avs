#include "ape.h"
#include "c__base.h"

#include <map>

#define MOD_NAME "Misc / AVSTrans Automation"
#define AVSTransVer "1.25.00"

class C_EelTrans : public C_RBASE {
   protected:
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

    bool itsme;  // determines whether the current component is the first created
                 // component
};

extern std::map<void*, std::string> autoprefix;
