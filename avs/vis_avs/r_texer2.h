// AVS APE (Plug-in Effect) header

// base class to derive from
class C_RBASE {
    public:
        C_RBASE() { }
        virtual ~C_RBASE() { };
        virtual int render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h)=0; // returns 1 if fbout has dest, 0 if framebuffer has dest
        virtual HWND conf(HINSTANCE hInstance, HWND hwndParent){return 0;};
        virtual char *get_desc()=0;
        virtual void load_config(unsigned char *data, int len) { }
        virtual int  save_config(unsigned char *data) { return 0; }
};

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

// this will be the directory and APE name displayed in the AVS Editor
#define MOD_NAME "Render / Texer II"

// this is how WVS will recognize this APE internally
#define UNIQUEIDSTRING "Acko.net: Texer II"

struct apeconfig {
    int mode;
    char img[MAX_PATH];
    int resize;
    int bilinear;
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
    __asm  fld   x 
    __asm  fistp t
    return t;
}

__forceinline int __stdcall FloorToInt(double f) {
    static float Half = 0.5;
    int i;
    __asm fld [f]
    __asm fsub [Half]
    __asm fistp [i]
    return i;
}

__forceinline int __stdcall CeilToInt(double f) {
    static float Half = 0.5;
    int i;
    __asm fld [f]
    __asm fadd [Half]
    __asm fistp [i]
    return i;
}

__forceinline double __stdcall Fractional(double f) {
    return f - FloorToInt(f);
}