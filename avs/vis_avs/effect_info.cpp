#include "effect_info.h"

bool operator==(const Effect_Info& a, const Effect_Info& b) {
    return a.get_handle() == b.get_handle();
};
bool operator==(const Effect_Info* a, const Effect_Info& b) {
    return a == nullptr ? false : *a == b;
};
bool operator==(const Effect_Info& a, const Effect_Info* b) {
    return b == nullptr ? false : *b == a;
};
bool operator!=(const Effect_Info& a, const Effect_Info& b) { return !(a == b); };
bool operator!=(const Effect_Info* a, const Effect_Info& b) { return !(a == b); };
bool operator!=(const Effect_Info& a, const Effect_Info* b) { return !(b == a); };
