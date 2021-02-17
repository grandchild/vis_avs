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
  FUNC1_ENTER

  *__nextBlock = __acos(*parm_a);

  FUNC_LEAVE
}
NAKED void _asm_acos_end(void) {}



If you want to do straight asm, then , well, you can use your imagination
(eax, ebx, ecx are input, eax is output, all points to "double")
if you need 8 bytes of temp space for your output, use esi and increment esi by 8
be sure to preserve edi, too.

*/



#ifdef _MSC_VER

#define FUNC1_ENTER \
  double *parm_a, *__nextBlock; \
  __asm { mov ebp, esp } \
  __asm { sub esp, __LOCAL_SIZE } \
  __asm { mov dword ptr parm_a, eax } \
  __asm { mov __nextBlock, esi }

#define FUNC2_ENTER \
  double *parm_a,*parm_b,*__nextBlock; \
  __asm { mov ebp, esp } \
  __asm { sub esp, __LOCAL_SIZE } \
  __asm { mov dword ptr parm_a, eax } \
  __asm { mov dword ptr parm_b, ebx } \
  __asm { mov __nextBlock, esi }

#define FUNC3_ENTER \
  double *parm_a,*parm_b,*parm_c,*__nextBlock; \
  __asm { mov ebp, esp } \
  __asm { sub esp, __LOCAL_SIZE } \
  __asm { mov dword ptr parm_a, eax } \
  __asm { mov dword ptr parm_b, ebx } \
  __asm { mov dword ptr parm_c, ecx } \
  __asm { mov __nextBlock, esi }

#define FUNC_LEAVE \
  __asm { mov eax, esi } \
  __asm { add esi, 8 }  \
  __asm { mov esp, ebp }

#else

// __LOCAL_SIZE = 0 ?
#define FUNC1_ENTER \
  double *parm_a, *__nextBlock; \
    __asm__ __volatile__ ( \
    "  mov  %%ebp, %%esp\n" \
    "  sub  %%esp, 0\n" \
    "  mov  dword ptr %0, %%eax\n" \
    "  mov  %1, %%esi\n" \
    :"=m"(parm_a), "=m"(__nextBlock) \
    : \
    :"esi", "eax" \
  );

#define FUNC2_ENTER \
  double *parm_a,*parm_b,*__nextBlock; \
  __asm__ __volatile__ ( \
    "  mov  %%ebp, %%esp\n" \
    "  sub  %%esp, 0\n" \
    "  mov  dword ptr %0, %%eax\n" \
    "  mov  dword ptr %1, %%ebx\n" \
    "  mov  dword ptr %2, %%esi\n" \
    :"=m"(parm_a), "=m"(parm_b), "=m"(__nextBlock) \
    : \
    :"esi", "eax", "ebx" \
  );

#define FUNC3_ENTER \
  double *parm_a,*parm_b,*parm_c,*__nextBlock; \
  __asm__ __volatile__ ( \
    "  mov  %%ebp, %%esp\n" \
    "  sub  %%esp, 0\n" \
    "  mov  dword ptr %0, %%eax\n" \
    "  mov  dword ptr %1, %%ebx\n" \
    "  mov  dword ptr %2, %%ecx\n" \
    "  mov  dword ptr %3, %%esi\n" \
    :"=m"(parm_a), "=m"(parm_b), "=m"(parm_c), "=m"(__nextBlock) \
    : \
    :"esi", "eax", "ebx", "ecx" \
  );

#define FUNC_LEAVE \
  __asm__ __volatile__ ( \
    "  mov  %%eax, %%esi\n" \
    "  add  %%esi, 8\n" \
    "  mov  %%esp, %%ebp\n" \
    : : \
    :"esi", "eax" \
  );

#endif

#ifdef _MSC_VER
#define NSEEL_CGEN_CALL __fastcall
#else
#define NSEEL_CGEN_CALL __attribute__((fastcall))
#define GCC_INLINE inline __attribute__((always_inline))
#endif


#endif//__NS_EEL_ADDFUNCS_H__
 