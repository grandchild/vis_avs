#include <map>
#include <string>

enum EnumMode { mLinear = 0, mAssign, mExec, mPlus };

std::string translate(std::string InputString, EnumMode defMode, bool defTransFirst);

extern std::map<void*, std::string> autoprefix;
extern std::string apepath;

struct mylist {
    int value;
    mylist* next;
};
