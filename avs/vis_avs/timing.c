#include "timing.h"

#ifdef TIMING
#ifndef __alpha

#include <stdio.h>

static struct {
    unsigned int st_time[2];
    unsigned int cycles;
    unsigned int calls;
} timingInfo[64];

static int timingEnters;

static void rdtsc(unsigned int t[2]) {
#ifdef _MSC_VER
    __asm
    {
		mov esi, t
         // TODO [cleanup]: This is RDTSC, maybe it wasn't available in MASM at the
         // time?
		_emit 0xf
		_emit 0x31
		mov [esi], eax
		mov [esi+4], edx
    }
#else  // _MSC_VER
    __asm__ __volatile__(
        "mov %%esi, %0\n\t"
        "rdtsc\n\t"
        "mov [%%esi], %%eax\n\t"
        "mov [%%esi + 4], %%edx\n\t"
        : /* no outputs */
        : "m"(t)
        : "eax", "esi");
#endif
}

void _timingInit(void) { memset(timingInfo, 0, sizeof(timingInfo)); }

void _timingEnter(int which) {
    //  if (!timingEnters++) __asm cli
    rdtsc(timingInfo[which].st_time);
}

void _timingLeave(int which) {
    unsigned int t[2];
    rdtsc(t);
    //   if (!--timingEnters) __asm sti
    if (t[1] == timingInfo[which].st_time[1]) {
        timingInfo[which].cycles += t[0] - timingInfo[which].st_time[0];
    } else {
        timingInfo[which].cycles += t[0] + (0xffffffff - timingInfo[which].st_time[0]);
    }
    timingInfo[which].calls++;
}

void _timingPrint(void) {
    int x;
    FILE* fp = fopen("C:\\timings.txt", "wt");
    for (x = 0; x < sizeof(timingInfo) / sizeof(timingInfo[0]); x++) {
        if (timingInfo[x].calls) {
            fprintf(fp,
                    "%d: %d calls, %d clocks/call\n",
                    x,
                    timingInfo[x].calls,
                    timingInfo[x].cycles / timingInfo[x].calls);
        }
    }
    timingInit();
    fclose(fp);
}

#endif
#endif
