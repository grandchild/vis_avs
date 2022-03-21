#include "platform.h"

int min(int a, int b) { return a > b ? b : a; }
int max(int a, int b) { return a < b ? b : a; }
unsigned int umin(unsigned int a, unsigned int b) { return a > b ? b : a; }
unsigned int umax(unsigned int a, unsigned int b) { return a < b ? b : a; }
