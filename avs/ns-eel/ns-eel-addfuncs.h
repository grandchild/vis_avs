/*
  LICENSE
  -------
Copyright 2005 Nullsoft, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer. 

  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution. 

  * Neither the name of Nullsoft nor the names of its contributors may be used to 
    endorse or promote products derived from this software without specific prior written permission. 
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR 
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT 
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef __NS_EEL_ADDFUNCS_H__
#define __NS_EEL_ADDFUNCS_H__



// these are used for making your own naked functions in C.
/*
For example:
static double (*__acos)(double) = &acos;
NAKED void _asm_acos(void)
{
  CALL1(acos);
}
NAKED void _asm_acos_end(void) {}



If you want to do straight asm, then , well, you can use your imagination
(eax, ebx, ecx are input, eax is output, all points to "double")
if you need 8 bytes of temp space for your output, use esi and increment esi by 8
be sure to preserve edi, too.

*/



#ifdef _MSC_VER

#define _ARGS1 \
  double *parm_a, *__nextBlock; \
  __asm { mov ebp, esp } \
  __asm { sub esp, __LOCAL_SIZE } \
  __asm { mov dword ptr parm_a, eax } \
  __asm { mov __nextBlock, esi }

#define _ARGS2 \
  double *parm_a,*parm_b,*__nextBlock; \
  __asm { mov ebp, esp } \
  __asm { sub esp, __LOCAL_SIZE } \
  __asm { mov dword ptr parm_a, eax } \
  __asm { mov dword ptr parm_b, ebx } \
  __asm { mov __nextBlock, esi }

#define _ARGS3 \
  double *parm_a,*parm_b,*parm_c,*__nextBlock; \
  __asm { mov ebp, esp } \
  __asm { sub esp, __LOCAL_SIZE } \
  __asm { mov dword ptr parm_a, eax } \
  __asm { mov dword ptr parm_b, ebx } \
  __asm { mov dword ptr parm_c, ecx } \
  __asm { mov __nextBlock, esi }

// Calls with double-typed parameters
#define CALL1(FUNC) _ARGS1; *__nextBlock = __##FUNC(*parm_a); _LEAVE
#define CALL2(FUNC) _ARGS2; *__nextBlock = __##FUNC(*parm_a, *parm_b); _LEAVE
#define CALL3(FUNC) _ARGS3; *__nextBlock = __##FUNC(*parm_a, *parm_b, *parm_c); _LEAVE

// Calls with pointer-to-double-typed parameters.
// Coincidentally these functions use the fastcall calling convention.
#define CALL1_PFASTCALL(FUNC) _ARGS1; *__nextBlock = __##FUNC(parm_a); _LEAVE
#define CALL2_PFASTCALL(FUNC) _ARGS2; *__nextBlock = __##FUNC(parm_a, parm_b); _LEAVE
#define CALL3_PFASTCALL(FUNC) _ARGS3; *__nextBlock = __##FUNC(parm_a, parm_b, parm_c); _LEAVE

#define _LEAVE \
  __asm { mov eax, esi } \
  __asm { add esi, 8 }  \
  __asm { mov esp, ebp }


#else  // GCC

#define CALL1(FUNC) _CALL(1, STDCALL, FUNC)
#define CALL2(FUNC) _CALL(2, STDCALL, FUNC)
#define CALL3(FUNC) _CALL(3, STDCALL, FUNC)
#define CALL1_PFASTCALL(FUNC) _CALL(1, FASTCALL, FUNC)
#define CALL2_PFASTCALL(FUNC) _CALL(2, FASTCALL, FUNC)
#define CALL3_PFASTCALL(FUNC) _CALL(3, FASTCALL, FUNC)

#define _CALL(NARGS, CALLTYPE, FUNC) \
  __asm__ __volatile__(              \
                                     \
    _ARGS##_##NARGS##_##CALLTYPE     \
    "mov  %%eax, %[func_]\n\t"       \
    "call %%eax\n\t"                 \
                                     \
    _LEAVE                           \
    :                                \
    : [func_]"i"(&FUNC)              \
    : "esi", "eax")

#define _ARGS_1_STDCALL                                    \
  /* new stack frame */                                    \
  "mov  %%ebp, %%esp\n\t"                                  \
  /* make space for a 4-byte pointer param on the stack */ \
  "sub  %%esp, 8\n\t"                                      \
  /* put double-typed param 1 on the stack */              \
  "fld  qword ptr [%%eax]\n\t"                             \
  "fstp qword ptr [%%esp]\n\t"
  /* The parameter is first pushed into an FPU register, and then popped back onto the
     stack. There is no "mov [esp], [eax]" and x86 registers are only 32bit wide, hence
     the FPU detour. */

#define _ARGS_2_STDCALL                                     \
  /* new stack frame */                                     \
  "mov  %%ebp, %%esp\n\t"                                   \
  /* make space for 2 4-byte pointer params on the stack */ \
  "sub  %%esp, 16\n\t"                                      \
  /* put double-typed params 1 & 2 on the stack */          \
  "fld  qword ptr [%%eax]\n\t"                              \
  "fstp qword ptr [%%esp + 8]\n\t"                          \
  "fld  qword ptr [%%ebx]\n\t"                              \
  "fstp qword ptr [%%esp]\n\t"

#define _ARGS_3_STDCALL                                     \
  /* new stack frame */                                     \
  "mov  %%ebp, %%esp\n\t"                                   \
  /* make space for 3 4-byte pointer params on the stack */ \
  "sub  %%esp, 24\n\t"                                      \
  /* put double-typed params 1, 2 & 3 on the stack */       \
  "fld  qword ptr [%%eax]\n\t"                              \
  "fstp qword ptr [%%esp + 16]\n\t"                         \
  "fld  qword ptr [%%ebx]\n\t"                              \
  "fstp qword ptr [%%esp + 8]\n\t"                          \
  "fld  qword ptr [%%ecx]\n\t"                              \
  "fstp qword ptr [%%esp]\n\t"

#define _ARGS_1_FASTCALL  \
  "mov  %%ebp, %%esp\n\t" \
  "sub  %%esp, 12\n\t"    \
  "mov  %%ecx, %%eax\n\t"

#define _ARGS_2_FASTCALL  \
  "mov  %%ebp, %%esp\n\t" \
  "sub  %%esp, 12\n\t"    \
  "mov  %%edx, %%ebx\n\t" \
  "mov  %%ecx, %%eax\n\t"

#define _ARGS_3_FASTCALL  \
  "mov  %%ebp, %%esp\n\t" \
  "sub  %%esp, 16\n\t"    \
  "push %%eax\n\t"        \
  "mov  %%edx, %%ebx\n\t"
  /* The third NS-EEL parameter, passed in ecx, is already in place for fastcall
     convention. */

#define _LEAVE                 \
  "fstp qword ptr [%%esi]\n\t" \
  "mov  %%eax, %%esi\n\t"      \
  "add  %%esi, 8\n\t"          \
  "mov  %%esp, %%ebp\n\t"

#endif


#endif//__NS_EEL_ADDFUNCS_H__
