#include "c_eeltrans.h"
#include "r_eeltrans.h"

#include "../ns-eel/ns-eel.h"
#include "r_defs.h"

#include <stdio.h>  // for logging
#include <windows.h>

APEinfo* g_extinfo = 0;

mylist* root = NULL;
int listcount = 0;

int addtolist(int newentry) {
    mylist* newroot = new mylist();
    newroot->value = newentry;
    newroot->next = root;
    root = newroot;
    return ++listcount;
}

int isinlist(int val) {
    mylist* tmp = root;
    int num = listcount;
    while (tmp) {
        if (val == tmp->value) {
            return num;
        }
        tmp = tmp->next;
        num--;
    }
    return 0;
}

void clearelem(mylist* bla) {
    if (bla) {
        clearelem(bla->next);
        delete[] bla;
    }
}

void clearlist() {
    clearelem(root);
    root = NULL;
    listcount = 0;
}

// global configuration dialog pointer
static C_EelTrans* g_ConfigThis;

int enabled_avstrans = 0;
int enabled_log = 0;
char* logpath = NULL;

#define defMode mExec
#define defFilterComments 1
bool defTransFirst = false;
int ReadCommentCodes = 1;

int hacked = 0;
bool notify = true;

void* tmp_eip_inner;
void* tmp_eip_outer;
void* tmp_ctx;
void* patchloc;
char* tmp_expression;
char* newbuf;
DWORD oldaccess;

/*
unsigned short *acp2unicode(char *InputString) {
        int oldlen = strlen(InputString)+1;
        int newlen = MultiByteToWideChar(CP_THREAD_ACP,0,InputString,oldlen,NULL,0);
        unsigned short *buffer = new unsigned short[newlen];
        MultiByteToWideChar(CP_THREAD_ACP,0,InputString,oldlen,buffer,newlen);
        //delete[] InputString;
        return buffer;
}

char *unicode2acp(unsigned short *InputString) {
        int oldlen = wcslen(InputString)+1;
        int newlen =
WideCharToMultiByte(CP_THREAD_ACP,0,InputString,oldlen,NULL,0,NULL,NULL); char *buffer =
new char[newlen];
        WideCharToMultiByte(CP_THREAD_ACP,0,InputString,oldlen,buffer,newlen,NULL,NULL);
        //delete[] InputString;
        return buffer;
}
*/

int compnum(int ctx) {
    int pos = isinlist(ctx);
    if (pos) {
        return pos;
    } else {
        return addtolist(ctx);
    }
}

char* event_compile_start(int ctx, char* _expression) {
    static int lastctx = 0;
    static int num = 0;
    if (_expression) {
        if (enabled_log) {
            if (ctx == lastctx) {
                num = (num % 4) + 1;
            } else {
                num = 1;
                lastctx = ctx;
            }
            char filename[200];
            sprintf(filename, "%s\\comp%02dpart%d.log", logpath, compnum(ctx), num);
            FILE* logfile = fopen(filename, "w");
            if (logfile) {
                fprintf(logfile, "%s", _expression);
                fclose(logfile);
            } else {
                if (!CreateDirectory(logpath, NULL)) {
                    MessageBox(
                        NULL,
                        "Could not create log directory, deactivating Code Logger.",
                        "Code Logger Error",
                        0);
                    enabled_log = 0;
                }
            }
        }
        if (enabled_avstrans && (strncmp(_expression, "//$notrans", 10) != 0)) {
            std::string tmp = translate(_expression, defMode, defTransFirst);
            newbuf = new char[tmp.size() + 1];
            strcpy(newbuf, tmp.c_str());
            return newbuf;
        }
    }
    return _expression;
}

void event_compile_done() {
    if (enabled_avstrans) {
        delete[] newbuf;
    }
}

NAKED void hook_after() {
#ifdef _MSC_VER
    __asm {
        pushad
        call event_compile_done
        popad
        jmp tmp_eip_outer
    }
#else
    __asm__ __volatile__(
        "pushad\n\t"
        "call %[event_compile_done]\n\t"
        "popad\n\t"
        "jmp %[tmp_eip_outer]\n\t"
        :
        :
        [event_compile_done] "m"(event_compile_done), [tmp_eip_outer] "m"(tmp_eip_outer)
        :);
#endif
}

NAKED void hook_before() {
#ifdef _MSC_VER
    // clang-format off
    __asm {
        pop tmp_eip_inner
        pop tmp_eip_outer
        pop tmp_ctx
        pop tmp_expression

        push tmp_expression
        push tmp_ctx
        call event_compile_start
        add esp, 8
        push eax

        push tmp_ctx
        mov eax,offset hook_after
        push eax

        // stuff from the original proc
        /*01CEF330 83 EC 18   */ sub         esp, 0x18
        /*01CEF333 53         */ push        ebx
        /*01CEF334 8B 5C 24 20*/ mov         ebx,dword ptr [esp + 0x20]

        jmp tmp_eip_inner
    }  // clang-format on
#else
    __asm__ __volatile__(
        "pop   %[tmp_eip_inner]\n\t"
        "pop   %[tmp_eip_outer]\n\t"
        "pop   %[tmp_ctx]\n\t"
        "pop   %[tmp_expression]\n\t"

        "push  %[tmp_expression]\n\t"
        "push  %[tmp_ctx]\n\t"
        "call  %[event_compile_start]\n\t"
        "add   %%esp, 8\n\t"
        "push  %%eax\n\t"

        "push  %[tmp_ctx]\n\t"
        "mov   %%eax, offset %[hook_after]\n\t"
        "push  %%eax\n\t"

        // stuff from the original proc
        "sub   %%esp, 0x18\n\t"
        "push  %%ebx\n\t"
        "mov   %%ebx, dword ptr [%%esp + 0x20]\n\t"

        "jmp   %[tmp_eip_inner]"
        :
        : [tmp_eip_inner] "m"(tmp_eip_inner),
          [tmp_eip_outer] "m"(tmp_eip_outer),
          [tmp_ctx] "m"(tmp_ctx),
          [tmp_expression] "m"(tmp_expression),
          [event_compile_start] "m"(event_compile_start),
          [hook_after] "m"(hook_after)
        : "eax", "ebx");
#endif
}

/*
 1. Get the location of compileVMcode() function (a.k.a. NSEEL_code_compile()) in the
    APEinfo struct (at offset 28 / 0x1c).
 2. Overwrite the first 8 bytes (which contain 3 instructions) with a call to
    hook_before(). Our patch is 7 bytes long, but at that point the next byte is the
    fourth byte of a mov instruction we have already started overwriting. Hence we need
    to write that byte too with a 1 byte NOP instruction to avoid unwanted behavior.
 3. Now, whenever NSEEL_code_compile() is called, hook_before() is called at the start
    and can call event_compile_start() to take the code string and modify it before it's
    passed to the compiler.
*/
NAKED void h00kit() {
#ifdef _MSC_VER
    __asm {  // get address
        mov eax,dword ptr [g_extinfo]
        mov eax,dword ptr [eax + 0x1C]

        mov patchloc,eax
        pushad
    }
#else
    __asm__ __volatile__(
        "mov %%eax, dword ptr %[g_extinfo]\n\t"
        "mov %%eax, dword ptr [%%eax + 0x1C]\n\t"
        "mov %[patchloc], %%eax\n\t"
        "pushad\n\t"
        : [patchloc] "=m"(patchloc)
        : [g_extinfo] "m"(g_extinfo)
        : "eax");
#endif

    VirtualProtect(patchloc, 8, PAGE_EXECUTE_READWRITE, &oldaccess);

#ifdef _MSC_VER
    __asm {
        popad

        mov byte ptr[eax],0xB8  // MOV eax,i32
        lea ebx,hook_before
        mov dword ptr[eax+1],ebx  // i32=hook_before
        mov byte ptr[eax+5],0xFF  // CALL r32
        mov byte ptr[eax+6],0xD0  // r32=eax
        mov byte ptr[eax+7],0x90  // NOP
        pushad
    }
#else
    __asm__ __volatile__(
        "popad\n\t"
        "mov byte ptr [%%eax], 0xB8\nt\t"
        "lea %%ebx, %[hook_before]\n\t"
        "mov dword ptr [%%eax + 1], %%ebx\nt\t"
        "mov byte ptr [%%eax + 5], 0xFF\nt\t"
        "mov byte ptr [%%eax + 6], 0xD0\nt\t"
        "mov byte ptr [%%eax + 7], 0x90\nt\t"
        "pushad\n\t"
        :
        : [hook_before] "m"(hook_before)
        : "eax", "ebx");

#endif

    VirtualProtect(patchloc, 8, oldaccess, &oldaccess);

#ifdef _MSC_VER
    __asm {
        popad
        ret
    }
#else
    __asm__ __volatile__(
        "popad\n\t"
        "ret\n\t");
#endif
}

NAKED void unh00kit() {
#ifdef _MSC_VER
    __asm { pushad }
#else
    __asm__ __volatile__("pushad\n\t");
#endif
    VirtualProtect(patchloc, 8, PAGE_EXECUTE_READWRITE, &oldaccess);

#ifdef _MSC_VER
    __asm {
        popad
        mov eax,patchloc
        mov byte ptr[eax],0x83  // \.
        mov byte ptr[eax+1],0xEC  //  >-- sub esp,18h
        mov byte ptr[eax+2],0x18  // /
        
        mov byte ptr[eax+3],0x53  // push ebx
        
        mov byte ptr[eax+4],0x8B  // \.
        mov byte ptr[eax+5],0x5C  //  \__ mov ebx,dword ptr [esp+20h]
        mov byte ptr[eax+6],0x24  //  /
        mov byte ptr[eax+7],0x20  // /
        pushad
    }
#else
    __asm__ __volatile__(
        "popad\n\t"
        "mov %%eax, %[patchloc]\n\t"
        "mov byte ptr [%%eax], 0x83\n\t"
        "mov byte ptr [%%eax + 1], 0xEC\n\t"
        "mov byte ptr [%%eax + 2], 0x18\n\t"

        "mov byte ptr [%%eax + 3], 0x53\n\t"

        "mov byte ptr [%%eax + 4], 0x8B\n\t"
        "mov byte ptr [%%eax + 5], 0x5C\n\t"
        "mov byte ptr [%%eax + 6], 0x24\n\t"
        "mov byte ptr [%%eax + 7], 0x20\n\t"
        "pushad\n\t"
        :
        : [patchloc] "m"(patchloc)
        : "eax");
#endif

    VirtualProtect(patchloc, 8, oldaccess, &oldaccess);

#ifdef _MSC_VER
    __asm {
        popad
        ret
    }
#else
    __asm__ __volatile__(
        "popad\n\t"
        "ret\n\t");
#endif
}

// set up default configuration
C_EelTrans::C_EelTrans() {
    // get module file name into string apepath
    char* c_apepath = new char[MAX_PATH];
    GetModuleFileName(g_hDllInstance, c_apepath, MAX_PATH);
    apepath = c_apepath;
    delete[] c_apepath;

    // remove the file name from the path
    apepath.erase(apepath.rfind('\\'));

    if (notify) {
        notify = false;
        enabled_avstrans = 0;
        enabled_log = 0;

        defTransFirst = 0;
        ReadCommentCodes = 1;
        autoprefix[this] = "";

        itsme = true;

        string filename = apepath + "\\eeltrans.ini";
        FILE* ini = fopen(filename.c_str(), "r");
        if (logpath) delete[] logpath;
        if (ini) {
            char* tmp = new char[2048];
            fscanf(ini, "%s", tmp);
            logpath = new char[strlen(tmp) + 1];
            strcpy(logpath, tmp);
            fclose(ini);
        } else {
            logpath = new char[10];
            strcpy(logpath, "C:\\avslog");
        }

        h00kit();
    } else {
        itsme = false;
    }
    hacked++;
}

// virtual destructor
C_EelTrans::~C_EelTrans() {
    hacked--;
    if (this->itsme) {
        notify = true;
        std::string filename = apepath + "\\eeltrans.ini";
        FILE* ini = fopen(filename.c_str(), "w");
        if (ini) {
            fprintf(ini, "%s", logpath);
            fclose(ini);
        }
    }
    if (hacked == 0) {
        autoprefix[this] = "";
        unh00kit();
        clearlist();
    }
}

int C_EelTrans::render(char visdata[2][2][576],
                       int isBeat,
                       int* framebuffer,
                       int* fbout,
                       int w,
                       int h) {
#ifdef _DEBUGgy
    static bool firstrun = false;
    if (!firstrun) {
        firstrun = true;
        VM_CONTEXT vmc = g_extinfo->allocVM();
        compileContext* ctx = (compileContext*)vmc;
        VM_CODEHANDLE cod = g_extinfo->compileVMcode(vmc, "gmegabuf(0)");
        codeHandleType* cht = (codeHandleType*)cod;
        char* output = new char[1024];
        memset(output, 0, 1024);

        void* foo = *((void**)(((unsigned char*)cht->code) + 25));
        static double*(NSEEL_CGEN_CALL * __megabuf)(double***, double*);
        __asm {
            mov eax,foo
            mov __megabuf,eax
        }
        void (*blub)(void) = (void (*)(void))(cht->code);
        blub();
        sprintf(output, "%X", __megabuf);
        MessageBox(NULL, output, "debug output", 0);
        delete[] output;
        g_extinfo->freeCode(cod);
        g_extinfo->freeVM(vmc);
    }
#endif
    if (notify) {
        if (!itsme) {
            notify = false;
            itsme = true;
        }
    }
    return 0;
}

// HWND C_EelTrans::conf(HINSTANCE hInstance,
//                        HWND hwndParent)  // return NULL if no config dialog possible
// {
//     g_ConfigThis = this;
//     HWND cnf;
//     if (itsme) {
//         cnf =
//             CreateDialog(hInstance, MAKEINTRESOURCE(IDD_CONFIG), hwndParent,
//             g_DlgProc);
//     } else {
//         cnf = CreateDialog(
//             hInstance, MAKEINTRESOURCE(IDD_CONF_ERR), hwndParent, g_DlgProc);
//     }
//     SetDlgItemText(cnf, IDC_VERSION, AVSTransVer);
//     return cnf;
// }

char* C_EelTrans::get_desc(void) { return MOD_NAME; }

#define PUT_INT(y)                   \
    data[pos] = (y)&255;             \
    data[pos + 1] = (y >> 8) & 255;  \
    data[pos + 2] = (y >> 16) & 255; \
    data[pos + 3] = (y >> 24) & 255
#define GET_INT() \
    (data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (data[pos + 3] << 24))

void C_EelTrans::load_config(unsigned char* data, int len) {
    int pos = 0;

    // always ensure there is data to be loaded
    if (len - pos >= 4) {
        enabled_avstrans = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        enabled_log = GET_INT();
        pos += 4;
    }
    if (len - pos >= 4) {
        defTransFirst = (GET_INT() != 0);
        pos += 4;
    }
    if (len - pos >= 4) {
        ReadCommentCodes = GET_INT();
        pos += 4;
    }
    if (len > pos) {
        autoprefix[g_ConfigThis] = (char*)(data + pos);
    } else {
        autoprefix[g_ConfigThis] = "";
    }
}

int C_EelTrans::save_config(unsigned char* data) {
    int pos = 0;

    PUT_INT(enabled_avstrans);
    pos += 4;

    PUT_INT(enabled_log);
    pos += 4;

    PUT_INT(((int)defTransFirst));
    pos += 4;

    PUT_INT(ReadCommentCodes);
    pos += 4;

    strcpy((char*)(data + pos), autoprefix[this].c_str());
    pos += autoprefix.size() + 1;

    return pos;
}

/* APE interface */
C_RBASE* R_EelTrans(char* desc) {
    if (desc) {
        strcpy(desc, MOD_NAME);
        return NULL;
    }
    return (C_RBASE*)new C_EelTrans();
}

void R_EelTrans_SetExtInfo(HINSTANCE hDllInstance, APEinfo* ptr) { g_extinfo = ptr; }
