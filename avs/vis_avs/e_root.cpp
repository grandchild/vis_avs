#include "e_root.h"

constexpr Parameter Root_Info::parameters[];

Effect_Info* create_Root_Info() { return new Root_Info(); }
Effect* create_Root() { return new E_Root(); }
void set_Root_desc(char* desc) { E_Root::set_desc(desc); }
