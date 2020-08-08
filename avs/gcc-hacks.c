#include "gcc-hacks.h"

// TODO: fast enough for all usecases?
int min(int a, int b) { return a > b ? b : a; }
int max(int a, int b) { return a < b ? b : a; }
