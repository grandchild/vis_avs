#define OUT_REPLACE       0
#define OUT_ADDITIVE      1
#define OUT_MAXIMUM       2
#define OUT_5050          3
#define OUT_SUB1          4
#define OUT_SUB2          5
#define OUT_MULTIPLY      6
#define OUT_ADJUSTABLE    7
#define OUT_XOR           8
#define OUT_MINIMUM       9

#define ID_TEXER2_EXAMPLE_COLOROSC 31337
#define ID_TEXER2_EXAMPLE_FLUMMYSPEC 31338
#define ID_TEXER2_EXAMPLE_BEATCIRCLE 31339
#define ID_TEXER2_EXAMPLE_3DRINGS 31340

// this will be the directory and APE name displayed in the AVS Editor
#define MOD_NAME "Render / Texer II"

// this is how WVS will recognize this APE internally
#define UNIQUEIDSTRING "Acko.net: Texer II"

struct texer2_apeconfig {
    int mode;
    char img[MAX_PATH];
    int resize;
    int wrap;
    int mask;
    int d;
};

struct Code {
    char *init;
    char *frame;
    char *beat;
    char *point;
    Code() { init = new char[1]; frame = new char[1]; beat = new char[1]; point = new char[1]; init[0] = frame[0] = beat[0] = point[0] = 0; }
    ~Code() { delete[] init; delete[] frame; delete[] beat; delete[] point; }

    void SetInit(char *str) { delete init; init = str; }
    void SetFrame(char *str) { delete frame; frame = str; }
    void SetBeat(char *str) { delete beat; beat = str; }
    void SetPoint(char *str) { delete point; point = str; }
};

struct Vars {
    double *n;
    double *i;
    double *x;
    double *y;
    double *w;
    double *h;
    double *v;
    double *sizex;
    double *sizey;
    double *red;
    double *green;
    double *blue;
    double *skip;
};

//extended APE stuff

typedef void *VM_CONTEXT;
typedef void *VM_CODEHANDLE;
typedef struct
{
    int ver; // ver=1 to start
    double *global_registers; // 100 of these

    // lineblendmode: 0xbbccdd
    // bb is line width (minimum 1)
    // dd is blend mode:
        // 0=replace
        // 1=add
        // 2=max
        // 3=avg
        // 4=subtractive (1-2)
        // 5=subtractive (2-1)
        // 6=multiplicative
        // 7=adjustable (cc=blend ratio)
        // 8=xor
        // 9=minimum
    int *lineblendmode;

    //evallib interface
    VM_CONTEXT (*allocVM)(); // return a handle
    void (*freeVM)(VM_CONTEXT); // free when done with a VM and ALL of its code have been freed, as well

    // you should only use these when no code handles are around (i.e. it's okay to use these before
    // compiling code, or right before you are going to recompile your code.
    void (*resetVM)(VM_CONTEXT);
    double * (*regVMvariable)(VM_CONTEXT, char *name);

    // compile code to a handle
    VM_CODEHANDLE (*compileVMcode)(VM_CONTEXT, char *code);

    // execute code from a handle
    void (*executeCode)(VM_CODEHANDLE, char visdata[2][2][576]);

    // free a code block
    void (*freeCode)(VM_CODEHANDLE);

} APEinfo;


__forceinline int __stdcall DoubleToInt(double x) {
    int    t;
#ifdef _MSC_VER
    __asm  fld   x
    __asm  fistp t
#else  // GCC
    // __asm__ __volatile__(
    //     "fld   x\n\t"
    //     "fistp t\n\t"
    // );
#endif
    return t;
}

__forceinline int __stdcall FloorToInt(double f) {
    static float Half = 0.5;
    int i;
#ifdef _MSC_VER
    __asm fld [f]
    __asm fsub [Half]
    __asm fistp [i]
#else  // GCC
#endif
    return i;
}

__forceinline double __stdcall Fractional(double f) {
    return f - FloorToInt(f);
}

/* asm shorthands */
#ifdef _MSC_VER

#define T2_PREP_MASK_COLOR  \
    __asm {                 \
        pxor mm5, mm5;      \
        movd mm7, color;    \
        punpcklbw mm7, mm5; \
    }


#define T2_SCALE_BLEND_ASM_ENTER(LOOP_LABEL)                                   \
    __asm {                                                                    \
        push esi;                                                              \
        push edi;                                                              \
        push edx;                                                              \
        /* esi = texture width */                                              \
        mov esi, imagewidth;                                                   \
        inc esi;                                                               \
        pxor mm5, mm5;                                                         \
                                                                               \
        /* bilinear */                                                         \
        push ebx;                                                              \
                                                                               \
        pxor mm5, mm5;                                                         \
                                                                               \
        /* calculate dy coefficient for this scanline */                       \
        /* store in mm4 = dy cloned into all bytes */                          \
        movd mm4, cy0;                                                         \
        psrlw mm4, 8;                                                          \
        punpcklwd mm4, mm4;                                                    \
        punpckldq mm4, mm4;                                                    \
                                                                               \
        /* loop counter */                                                     \
        mov ecx, tot;                                                          \
        inc ecx;                                                               \
                                                                               \
        /* set output pointer */                                               \
        mov edi, outp;                                                         \
                                                                               \
        /* beginning x tex coordinate */                                       \
        mov ebx, cx0;                                                          \
                                                                               \
        /* calculate y combined address for first point a for this scanline */ \
        /* store in ebp = texture start address for this scanline */           \
        mov eax, cy0;                                                          \
        shr eax, 16;                                                           \
        imul eax, esi;                                                         \
        shl eax, 2;                                                            \
        mov edx, texdata;                                                      \
        add edx, eax;                                                          \
                                                                               \
        /* begin loop */                                                       \
        LOOP_LABEL:                                                            \
                                                                               \
        /* calculate dx, fractional part of tex coord */                       \
        movd mm3, ebx;                                                         \
        psrlw mm3, 8;                                                          \
        punpcklwd mm3, mm3;                                                    \
        punpckldq mm3, mm3;                                                    \
                                                                               \
        /* convert fixed point into floor and load pixel address */            \
        mov eax, ebx;                                                          \
        shr eax, 16;                                                           \
        lea eax, dword ptr [edx+eax*4];                                        \
                                                                               \
        /* mm0 = b*dx */                                                       \
        movd mm0, dword ptr [eax+4]; /* b */                                   \
        punpcklbw mm0, mm5;                                                    \
        pmullw mm0, mm3;                                                       \
        psrlw mm0, 8;                                                          \
        /* mm1 = a*(1-dx) */                                                   \
        movd mm1, dword ptr [eax]; /* a */                                     \
        pxor mm3, mmxxor;                                                      \
        punpcklbw mm1, mm5;                                                    \
        pmullw mm1, mm3;                                                       \
        psrlw mm1, 8;                                                          \
        paddw mm0, mm1;                                                        \
                                                                               \
        /* mm1 = c*(1-dx) */                                                   \
        movd mm1, dword ptr [eax+esi*4]; /* c */                               \
        punpcklbw mm1, mm5;                                                    \
        pmullw mm1, mm3;                                                       \
        psrlw mm1, 8;                                                          \
        /* mm2 = d*dx */                                                       \
        movd mm2, dword ptr [eax+esi*4+4]; /* d */                             \
        pxor mm3, mmxxor;                                                      \
        punpcklbw mm2, mm5;                                                    \
        pmullw mm2, mm3;                                                       \
        psrlw mm2, 8;                                                          \
        paddw mm1, mm2;                                                        \
                                                                               \
        /* combine */                                                          \
        pmullw mm1, mm4;                                                       \
        psrlw mm1, 8;                                                          \
        pxor mm4, mmxxor;                                                      \
        pmullw mm0, mm4;                                                       \
        psrlw mm0, 8;                                                          \
        paddw mm0, mm1;                                                        \
        pxor mm4, mmxxor;                                                      \
                                                                               \
        /* filter color */                                                     \
        /* (already unpacked) punpcklbw mm0, mm5; */                           \
        pmullw mm0, mm7;

/* blendmode-specific code in between here */

#define T2_SCALE_BLEND_ASM_LEAVE(LOOP_LABEL) \
        /* write pixel */                    \
        movd dword ptr [edi], mm0;           \
        add edi, 4;                          \
                                             \
        /* advance tex coords, cx += sdx; */ \
        add ebx, sdx;                        \
                                             \
        dec ecx;                             \
        jnz LOOP_LABEL;                      \
                                             \
        pop ebx;                             \
        pop edx;                             \
        pop edi;                             \
        pop esi;                             \
    }



#define T2_NONSCALE_PUSH_ESI_EDI __asm { push esi; push edi; }

#define T2_NONSCALE_MINMAX_MASKS \
    __asm {                      \
        movd mm4, maxmask;       \
        movd mm6, signmask;      \
    }

#define T2_BLEND_AND_STORE_ALPHA                       \
    __asm {                                            \
        /* duplicate blend factor into all channels */ \
        mov eax, t;                                    \
        mov edx, eax;                                  \
        shl eax, 16;                                   \
        or  eax, edx;                                  \
        mov [alpha], eax;                              \
        mov [alpha+4], eax;                            \
                                                       \
        /* store alpha and (256 - alpha) */            \
        movq mm2, alpha;                               \
        movq mm3, salpha;                              \
        psubusw mm3, alpha;                            \
    }

#define T2_NONSCALE_BLEND_ASM_ENTER(LOOP_LABEL) \
    __asm {                                     \
        mov ecx, tot;                           \
        inc ecx;                                \
        mov edi, outp;                          \
        mov esi, inp;                           \
        LOOP_LABEL:

/* blendmode-specific code in between here */

#define T2_NONSCALE_BLEND_ASM_LEAVE(LOOP_LABEL) \
        add esi, 4;                             \
        add edi, 4;                             \
        dec ecx;                                \
        jnz LOOP_LABEL;                         \
    }

#define T2_NONSCALE_POP_EDI_ESI __asm { pop edi; pop esi; }

#define T2_ZERO_MM5 __asm pxor mm5, mm5;

#define EMMS __asm emms;


#else  // GCC


// to-literal-string macro util
#define _STR(x) #x      // x -> "x"
#define STR(x) _STR(x)  // indirection needed to actually evaluate x argument
// GCC extended-asm argument spec (memory location constraint)
#define ASM_M_ARG(x) [x]"m"(x)

#define T2_PREP_MASK_COLOR                 \
    __asm__ __volatile__ (                 \
        "pxor       %%mm5, %%mm5\n\t"      \
        "movd       %%mm7, %[color]\n\t"   \
        "punpcklbw  %%mm7, %%mm5\n\t"      \
        : : [color]"m"(color)              \
        : "mm5", "mm7");

#define T2_SCALE_MINMAX_SIGNMASK            \
    __asm__ __volatile__(                   \
        "movd       %%mm6, %[signmask]\n\t" \
        : : [signmask]"r"(signmask) : "mm6" \
    );

#define T2_SCALE_BLEND_AND_STORE_ALPHA                        \
    __asm__ __volatile__(                                     \
        /* duplicate blend factor into all channels */        \
        "mov      %%eax, %[t]\n\t"                            \
        "mov      %%edx, %%eax\n\t"                           \
        "shl      %%eax, 16\n\t"                              \
        "or       %%eax, %%edx\n\t"                           \
        "mov      [%[alpha]], %%eax\n\t"                      \
        "mov      [%[alpha] + 4], %%eax\n\t"                  \
                                                              \
        /* store alpha and (256 - alpha) */                   \
        "movq     %%mm6, qword ptr %[alpha]\n\t"              \
        "movq     %%mm3, %[salpha]\n\t"                       \
        "psubusw  %%mm3, %%mm6\n\t"                           \
        : : [alpha]"m"(alpha), [salpha]"m"(salpha), [t]"r"(t) \
        : "eax", "edx", "mm3", "mm6"                          \
    );

#define T2_SCALE_BLEND_ASM_ENTER(LOOP_LABEL)                                   \
    __asm__ __volatile__(                                                      \
        "push       %%esi\n\t"                                                 \
        "push       %%edi\n\t"                                                 \
        "push       %%edx\n\t"                                                 \
        /* esi = texture width */                                              \
        "mov        %%esi, %[imagewidth]\n\t"                                  \
        "inc        %%esi\n\t"                                                 \
        "pxor       %%mm5, %%mm5\n\t"                                          \
                                                                               \
        /* bilinear */                                                         \
        "push       %%ebx\n\t"                                                 \
                                                                               \
        "pxor       %%mm5, %%mm5\n\t"                                          \
                                                                               \
        /* calculate dy coefficient for this scanline */                       \
        /* store in mm4 = dy cloned into all bytes */                          \
        "movd       %%mm4, %[cy0]\n\t"                                         \
        "psrlw      %%mm4, 8\n\t"                                              \
        "punpcklwd  %%mm4, %%mm4\n\t"                                          \
        "punpckldq  %%mm4, %%mm4\n\t"                                          \
                                                                               \
        /* loop counter */                                                     \
        "mov        %%ecx, %[tot]\n\t"                                         \
        "inc        %%ecx\n\t"                                                 \
                                                                               \
        /* set output pointer */                                               \
        "mov        %%edi, %[outp]\n\t"                                        \
                                                                               \
        /* beginning x tex coordinate */                                       \
        "mov        %%ebx, %[cx0]\n\t"                                         \
                                                                               \
        /* calculate y combined address for first point a for this scanline */ \
        /* store in ebp = texture start address for this scanline */           \
        "mov        %%eax, %[cy0]\n\t"                                         \
        "shr        %%eax, 16\n\t"                                             \
        "imul       %%eax, %%esi\n\t"                                          \
        "shl        %%eax, 2\n\t"                                              \
        "mov        %%edx, %[texdata]\n\t"                                     \
        "add        %%edx, %%eax\n\t"                                          \
                                                                               \
        /* begin loop */                                                       \
        STR(LOOP_LABEL) ":\n\t"                                                \
                                                                               \
        /* calculate dx, fractional part of tex coord */                       \
        "movd       %%mm3, %%ebx\n\t"                                          \
        "psrlw      %%mm3, 8\n\t"                                              \
        "punpcklwd  %%mm3, %%mm3\n\t"                                          \
        "punpckldq  %%mm3, %%mm3\n\t"                                          \
                                                                               \
        /* convert fixed point into floor and load pixel address */            \
        "mov        %%eax, %%ebx\n\t"                                          \
        "shr        %%eax, 16\n\t"                                             \
        "lea        %%eax, dword ptr [%%edx + %%eax * 4]\n\t"                  \
                                                                               \
        /* mm0 = b*dx */                                                       \
        "movd       %%mm0, dword ptr [%%eax + 4]\n\t" /* b */                  \
        "punpcklbw  %%mm0, %%mm5\n\t"                                          \
        "pmullw     %%mm0, %%mm3\n\t"                                          \
        "psrlw      %%mm0, 8\n\t"                                              \
        /* mm1 = a*(1-dx) */                                                   \
        "movd       %%mm1, dword ptr [%%eax]\n\t" /* a */                      \
        "pxor       %%mm3, %[mmxxor]\n\t"                                      \
        "punpcklbw  %%mm1, %%mm5\n\t"                                          \
        "pmullw     %%mm1, %%mm3\n\t"                                          \
        "psrlw      %%mm1, 8\n\t"                                              \
        "paddw      %%mm0, %%mm1\n\t"                                          \
                                                                               \
        /* mm1 = c*(1-dx) */                                                   \
        "movd       %%mm1, dword ptr [%%eax + %%esi * 4]\n\t" /* c */          \
        "punpcklbw  %%mm1, %%mm5\n\t"                                          \
        "pmullw     %%mm1, %%mm3\n\t"                                          \
        "psrlw      %%mm1, 8\n\t"                                              \
        /* mm2 = d*dx */                                                       \
        "movd       %%mm2, dword ptr [%%eax + %%esi * 4 + 4]\n\t" /* d */      \
        "pxor       %%mm3, %[mmxxor]\n\t"                                      \
        "punpcklbw  %%mm2, %%mm5\n\t"                                          \
        "pmullw     %%mm2, %%mm3\n\t"                                          \
        "psrlw      %%mm2, 8\n\t"                                              \
        "paddw      %%mm1, %%mm2\n\t"                                          \
                                                                               \
        /* combine */                                                          \
        "pmullw     %%mm1, %%mm4\n\t"                                          \
        "psrlw      %%mm1, 8\n\t"                                              \
        "pxor       %%mm4, %[mmxxor]\n\t"                                      \
        "pmullw     %%mm0, %%mm4\n\t"                                          \
        "psrlw      %%mm0, 8\n\t"                                              \
        "paddw      %%mm0, %%mm1\n\t"                                          \
        "pxor       %%mm4, %[mmxxor]\n\t"                                      \
                                                                               \
        /* filter color */                                                     \
        /* (already unpacked) punpcklbw mm0, mm5 */                            \
        "pmullw     %%mm0, %%mm7\n\t"                                          \
        "\n\t"

/* blendmode-specific code in between here */

#define T2_SCALE_BLEND_ASM_LEAVE(LOOP_LABEL, EXTRA_ARGS...)                 \
        "\n\t"                                                              \
        /* write pixel */                                                   \
        "movd       dword ptr [%%edi], %%mm0\n\t"                           \
        "add        %%edi, 4\n\t"                                           \
                                                                            \
        /* advance tex coords, cx += sdx */                                 \
        "add        %%ebx, %[sdx]\n\t"                                      \
                                                                            \
        "dec        %%ecx\n\t"                                              \
        "jnz        " STR(LOOP_LABEL) "\n\t"                                \
                                                                            \
        "pop        %%ebx\n\t"                                              \
                                                                            \
        "pop        %%edx\n\t"                                              \
        "pop        %%edi\n\t"                                              \
        "pop        %%esi\n\t"                                              \
        : /* no outputs */                                                  \
        : [imagewidth]"m"(imagewidth), [sdx]"m"(sdx),                       \
          [cx0]"m"(cx0), [cy0]"m"(cy0), [mmxxor]"m"(mmxxor),                \
          [outp]"m"(outp), [texdata]"m"(texdata), [tot]"m"(tot),            \
          ##EXTRA_ARGS                                                      \
        : "eax", "ecx", "mm0", "mm1", "mm2", "mm3", "mm4", "mm5"            \
    );


#define T2_NONSCALE_PUSH_ESI_EDI   \
    __asm__ __volatile__( \
        "push  %%esi\n\t" \
        "push  %%edi\n\t" \
        : : :             \
    );

#define T2_NONSCALE_MINMAX_MASKS                           \
    __asm__ __volatile__(                                  \
        "movd  %%mm4, %[maxmask]\n\t"                      \
        "movd  %%mm6, %[signmask]\n\t"                     \
        : : [maxmask]"r"(maxmask), [signmask]"r"(signmask) \
        : "mm4", "mm6"                                     \
    );

#define T2_NONSCALE_BLEND_AND_STORE_ALPHA                     \
    __asm__ __volatile__(                                     \
        /* duplicate blend factor into all channels */        \
        "mov      %%eax, %[t]\n\t"                            \
        "mov      %%edx, %%eax\n\t"                           \
        "shl      %%eax, 16\n\t"                              \
        "or       %%eax, %%edx\n\t"                           \
        "mov      [%[alpha]], %%eax\n\t"                      \
        "mov      [%[alpha] + 4], %%eax\n\t"                  \
                                                              \
        /* store alpha and (256 - alpha) */                   \
        "movq     %%mm6, qword ptr %[alpha]\n\t"              \
        "movq     %%mm3, %[salpha]\n\t"                       \
        "psubusw  %%mm3, %%mm6\n\t"                           \
        : : [alpha]"m"(alpha), [salpha]"m"(salpha), [t]"r"(t) \
        : "eax", "edx", "mm3", "mm6"                          \
    );


#define T2_NONSCALE_BLEND_ASM_ENTER(LOOP_LABEL)           \
    __asm__ __volatile__(                                 \
        "mov   %%ecx, %[tot]\n\t"                         \
        "inc   %%ecx\n\t"                                 \
        "mov   %%edi, %[outp]\n\t"                        \
        "mov   %%esi, %[inp]\n\t"                         \
        STR(LOOP_LABEL) ":\n\t"

/* blendmode-specific code in between here */

#define T2_NONSCALE_BLEND_ASM_LEAVE(LOOP_LABEL)           \
        "add   %%esi, 4\n\t"                              \
        "add   %%edi, 4\n\t"                              \
        "dec   %%ecx\n\t"                                 \
        "jnz   " STR(LOOP_LABEL) "\n\t"                   \
        : : [tot]"r"(tot), [outp]"m"(outp), [inp]"m"(inp) \
        : "ecx", "esi", "edi", "mm0", "mm1", "mm2", "mm3" \
    );

#define T2_NONSCALE_POP_EDI_ESI \
    __asm__ __volatile__(       \
        "pop   %%edi\n\t"       \
        "pop   %%esi\n\t"       \
        : : : "edi", "esi"      \
    );

#define T2_ZERO_MM5 __asm__ __volatile__("pxor mm5, mm5");

#define EMMS __asm__ __volatile__("emms");

#endif
