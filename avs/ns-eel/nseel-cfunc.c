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

#include "../gcc-hacks.h"
#include <windows.h>
#include <math.h>
#include "ns-eel-int.h"


// these are used by our assembly code
static float g_cmpaddtab[2]={0.0,1.0};
static float g_signs[2]={1.0,-1.0};
static double g_closefact = 0.00001;
static float g_half=0.5;
static float negativezeropointfive=-0.5f;
static float onepointfive=1.5f;



/// functions called by built code

#define isnonzero(x) (fabs(x) > g_closefact)

//---------------------------------------------------------------------------------------------------------------
static double NSEEL_CGEN_CALL _rand(double *x)
{
  if (*x < 1.0) *x=1.0;
  return (double)(rand()%(int)max(*x,1.0));
}

//---------------------------------------------------------------------------------------------------------------
static double NSEEL_CGEN_CALL _band(double *var, double *var2)
{
  return isnonzero(*var) && isnonzero(*var2) ? 1 : 0;
}

//---------------------------------------------------------------------------------------------------------------
static double NSEEL_CGEN_CALL _bor(double *var, double *var2)
{
  return isnonzero(*var) || isnonzero(*var2) ? 1 : 0;
}

//---------------------------------------------------------------------------------------------------------------
static double NSEEL_CGEN_CALL _sig(double *x, double *constraint)
{
  double t = (1+exp(-*x * (*constraint)));
  return isnonzero(t) ? 1.0/t : 0;
}


// end functions called by inline code

// these make room on the stack for local variables, but do not need to 
// worry about trashing ebp, since none of our code uses ebp and there's
// a pushad+popad surrounding the call


static double (*__asin)(double) = &asin;
//---------------------------------------------------------------------------------------------------------------
NAKED void nseel_asm_asin(void)
{
  FUNC1_ENTER

  *__nextBlock = __asin(*parm_a);

  FUNC_LEAVE
}
NAKED void nseel_asm_asin_end(void) {}

static double (*__acos)(double) = &acos;
//---------------------------------------------------------------------------------------------------------------
NAKED void nseel_asm_acos(void)
{
  FUNC1_ENTER

  *__nextBlock = __acos(*parm_a);

  FUNC_LEAVE
}
NAKED void nseel_asm_acos_end(void) {}

//---------------------------------------------------------------------------------------------------------------
static double (*__atan)(double) = &atan;
NAKED void nseel_asm_atan(void)
{
  FUNC1_ENTER

  *__nextBlock = __atan(*parm_a);

  FUNC_LEAVE
}
NAKED void nseel_asm_atan_end(void) {}

//---------------------------------------------------------------------------------------------------------------
static double (*__atan2)(double,double) = &atan2;
NAKED void nseel_asm_atan2(void)
{
  FUNC2_ENTER

  *__nextBlock = __atan2(*parm_b, *parm_a);

  FUNC_LEAVE
}
NAKED void nseel_asm_atan2_end(void) {}


//---------------------------------------------------------------------------------------------------------------
static double (NSEEL_CGEN_CALL  * __sig)(double *,double *) = &_sig;
NAKED void nseel_asm_sig(void)
{
  FUNC2_ENTER

  *__nextBlock = __sig(parm_b, parm_a);

  FUNC_LEAVE
}
NAKED void nseel_asm_sig_end(void) {}

//---------------------------------------------------------------------------------------------------------------
static double (NSEEL_CGEN_CALL *__rand)(double *) = &_rand;
NAKED void nseel_asm_rand(void)
{
  FUNC1_ENTER

  *__nextBlock = __rand(parm_a);

  FUNC_LEAVE
}
NAKED void nseel_asm_rand_end(void) {}

//---------------------------------------------------------------------------------------------------------------
static double (NSEEL_CGEN_CALL *__band)(double *,double *) = &_band;
NAKED void nseel_asm_band(void)
{
  FUNC2_ENTER

  *__nextBlock = __band(parm_b, parm_a);

  FUNC_LEAVE
}
NAKED void nseel_asm_band_end(void) {}

//---------------------------------------------------------------------------------------------------------------
static double ( NSEEL_CGEN_CALL  *__bor)(double *,double *) = &_bor;
NAKED void nseel_asm_bor(void)
{
  FUNC2_ENTER

  *__nextBlock = __bor(parm_b, parm_a);

  FUNC_LEAVE
}
NAKED void nseel_asm_bor_end(void) {}

//---------------------------------------------------------------------------------------------------------------
static double (* __pow)(double,double) = &pow;
NAKED void nseel_asm_pow(void)
{
  FUNC2_ENTER

  *__nextBlock = __pow(*parm_b, *parm_a);

  FUNC_LEAVE
}
NAKED void nseel_asm_pow_end(void) {}

//---------------------------------------------------------------------------------------------------------------
static double (*__exp)(double) = &exp;
NAKED void nseel_asm_exp(void)
{
  FUNC1_ENTER

  *__nextBlock = __exp(*parm_a);

  FUNC_LEAVE
}
NAKED void nseel_asm_exp_end(void) {}

//---------------------------------------------------------------------------------------------------------------
static double (*__floor)(double) = &floor;
NAKED void nseel_asm_floor(void)
{
  FUNC1_ENTER

  *__nextBlock = __floor(*parm_a);

  FUNC_LEAVE
}
NAKED void nseel_asm_floor_end(void) {}


//---------------------------------------------------------------------------------------------------------------
static double (*__ceil)(double) = &ceil;
NAKED void nseel_asm_ceil(void)
{
  FUNC1_ENTER

  *__nextBlock = __ceil(*parm_a);

  FUNC_LEAVE
}
NAKED void nseel_asm_ceil_end(void) {}

//---------------------------------------------------------------------------------------------------------------




// do nothing, eh
NAKED void nseel_asm_exec2(void)
{
}
NAKED void nseel_asm_exec2_end(void) { }



NAKED void nseel_asm_invsqrt(void)
{
#ifdef _MSC_VER
  __asm
  {
    fld qword ptr [eax]

    mov edx, 0x5f3759df
    fst dword ptr [esi]
    // floating point stack has input, as does [eax]
    fmul dword ptr [negativezeropointfive]
    mov ecx, [esi]
    sar ecx, 1
    sub edx, ecx
    mov [esi], edx
    
    // st(0) = input, [eax] has x
    fmul dword ptr [esi]

    fmul dword ptr [esi]

    fadd dword ptr [onepointfive]

    fmul dword ptr [esi]
    mov eax, esi

    fstp qword ptr [esi]

    add esi, 8
  }
#else
  __asm__ __volatile__ (
    "fld qword ptr [%%eax]\n\t"

    "mov %%edx, 0x5f3759df\n\t"
    "fst dword ptr [%%esi]\n\t"
    // floating point stack has input, as does [eax]
    "fmul dword ptr [%0]\n\t"
    "mov %%ecx, [%%esi]\n\t"
    "sar %%ecx, 1\n\t"
    "sub %%edx, %%ecx\n\t"
    "mov [%%esi], %%edx\n\t"

    // st(0) = input, [eax] has x
    "fmul dword ptr [%%esi]\n\t"
    "fmul dword ptr [%%esi]\n\t"
    "fadd dword ptr [%1]\n\t"
    "fmul dword ptr [%%esi]\n\t"
    "mov %%eax, %%esi\n\t"

    "fstp qword ptr [%%esi]\n\t"

    "add %%esi, 8\n\t"
    :
    :"m"(negativezeropointfive), "m"(onepointfive)
    :"eax", "edx", "esi", "ecx"
  );
#endif
}
NAKED void nseel_asm_invsqrt_end(void) {}


//---------------------------------------------------------------------------------------------------------------
NAKED void nseel_asm_sin(void)
{
#ifdef _MSC_VER
  __asm
  {
    fld qword ptr [eax]
    fsin
    mov eax, esi
    fstp qword ptr [esi]
    add esi, 8
  }
#else
  __asm__ __volatile__ (
    "fld qword ptr [%%eax]\n\t"
    "fsin\n\t"
    "mov %%eax, %%esi\n\t"
    "fstp qword ptr [%%esi]\n\t"
    "add %%esi, 8\n"
    : : :"eax", "esi"
  );
#endif
}
NAKED void nseel_asm_sin_end(void) {}

//---------------------------------------------------------------------------------------------------------------
NAKED void nseel_asm_cos(void)
{
#ifdef _MSC_VER
  __asm
  {
    fld qword ptr [eax]
    fcos
    mov eax, esi
    fstp qword ptr [esi]
    add esi, 8
  }
#else
  __asm__ __volatile__ (
    "fld qword ptr [%%eax]\n\t"
    "fcos\n\t"
    "mov %%eax, %%esi\n\t"
    "fstp qword ptr [%%esi]\n\t"
    "add %%esi, 8\n"
    : : :"eax", "esi"
  );
#endif
}
NAKED void nseel_asm_cos_end(void) {}

//---------------------------------------------------------------------------------------------------------------
NAKED void nseel_asm_tan(void)
{
#ifdef _MSC_VER
  __asm
  {
    fld qword ptr [eax]
    fsincos
    fdiv
    mov eax, esi
    fstp qword ptr [esi]
    add esi, 8
  }
#else
  __asm__ __volatile__ (
    "fld qword ptr [%%eax]\n\t"
    "fsincos\n\t"
    "fdivp\n\t"  // no-operand version of fdiv is implicit fdivp
    "mov %%eax, %%esi\n\t"
    "fstp qword ptr [%%esi]\n\t"
    "add %%esi, 8\n"
    : : : "eax", "esi"
  );
#endif
}
NAKED void nseel_asm_tan_end(void) {}

//---------------------------------------------------------------------------------------------------------------
NAKED void nseel_asm_sqr(void)
{
#ifdef _MSC_VER
  __asm
  {
    fld qword ptr [eax]
    fmul st(0), st(0)
    mov eax, esi
    fstp qword ptr [esi]
    add esi, 8
  }
#else
  __asm__ __volatile__ (
    "fld qword ptr [%%eax]\n\t"
    "fmul st(0), st(0)\n\t"
    "mov %%eax, %%esi\n\t"
    "fstp qword ptr [%%esi]\n\t"
    "add %%esi, 8\n"
    : : : "eax", "esi"
  );
#endif
}
NAKED void nseel_asm_sqr_end(void) {}

//---------------------------------------------------------------------------------------------------------------
NAKED void nseel_asm_sqrt(void)
{
#ifdef _MSC_VER
  __asm
  {
    fld qword ptr [eax]
    fabs
    fsqrt
    mov eax, esi
    fstp qword ptr [esi]
    add esi, 8
  }
#else
  __asm__ __volatile__ (
    "fld qword ptr [%%eax]\n\t"
    "fabs\n\t"
    "fsqrt\n\t"
    "mov %%eax, %%esi\n\t"
    "fstp qword ptr [%%esi]\n\t"
    "add %%esi, 8\n"
    : : : "eax", "esi"
  );
#endif
}
NAKED void nseel_asm_sqrt_end(void) {}


//---------------------------------------------------------------------------------------------------------------
NAKED void nseel_asm_log(void)
{
#ifdef _MSC_VER
  __asm
  {
    fld1
    fldl2e
    fdiv
    fld qword ptr [eax]
    mov eax, esi
    fyl2x
    fstp qword ptr [esi]
    add esi, 8
  }
#else
  __asm__ __volatile__ (
    "fld1\n\t"
    "fldl2e\n\t"
    "fdivp\n\t"
    "fld qword ptr [%%eax]\n\t"
    "mov %%eax, %%esi\n\t"
    "fyl2x\n\t"
    "fstp qword ptr [%%esi]\n\t"
    "add %%esi, 8\n"
    : : : "eax", "esi"
  );
#endif
}
NAKED void nseel_asm_log_end(void) {}

//---------------------------------------------------------------------------------------------------------------
NAKED void nseel_asm_log10(void)
{
#ifdef _MSC_VER
  __asm
  {
    fld1
    fldl2t
    fdiv
    fld qword ptr [eax]
    mov eax, esi
    fyl2x
    fstp qword ptr [esi]
    add esi, 8
  }
#else
  __asm__ __volatile__ (
    "fld1\n\t"
    "fldl2t\n\t"
    "fdivp\n\t"
    "fld qword ptr [%%eax]\n\t"
    "mov %%eax, %%esi\n\t"
    "fyl2x\n\t"
    "fstp qword ptr [%%esi]\n\t"
    "add %%esi, 8\n"
    : : : "eax", "esi"
  );
#endif
}
NAKED void nseel_asm_log10_end(void) {}

//---------------------------------------------------------------------------------------------------------------
NAKED void nseel_asm_abs(void)
{
#ifdef _MSC_VER
  __asm
  {
    fld qword ptr [eax]
    fabs
    mov eax, esi
    fstp qword ptr [esi]
    add esi, 8
  }
#else
  __asm__ __volatile__ (
    "fld qword ptr [%%eax]\n\t"
    "fabs\n\t"
    "mov %%eax, %%esi\n\t"
    "fstp qword ptr [%%esi]\n\t"
    "add %%esi, 8\n"
    : : : "eax", "esi"
  );
#endif
}
NAKED void nseel_asm_abs_end(void) {}


//---------------------------------------------------------------------------------------------------------------
NAKED void nseel_asm_assign(void)
{
#ifdef _MSC_VER
  __asm
  {
    fld qword ptr [eax]
    fstp qword ptr [ebx]
  }
#else
  __asm__ __volatile__ (
    "fld qword ptr [%%eax]\n\t"
    "fstp qword ptr [%%ebx]\n"
    : : : "eax", "ebx"
  );
#endif
}
NAKED void nseel_asm_assign_end(void) {}

//---------------------------------------------------------------------------------------------------------------
NAKED void nseel_asm_add(void)
{
#ifdef _MSC_VER
  __asm
  {
    fld qword ptr [eax]
    fadd qword ptr [ebx]
    mov eax, esi
    fstp qword ptr [esi]
    add esi, 8
  }
#else
  __asm__ __volatile__ (
    "fld qword ptr [%%eax]\n\t"
    "fadd qword ptr [%%ebx]\n\t"
    "mov %%eax, %%esi\n\t"
    "fstp qword ptr [%%esi]\n\t"
    "add %%esi, 8\n"
    : : : "eax", "esi"
  );
#endif
}
NAKED void nseel_asm_add_end(void) {}

//---------------------------------------------------------------------------------------------------------------
NAKED void nseel_asm_sub(void)
{
#ifdef _MSC_VER
  __asm
  {
    fld qword ptr [ebx]
    fsub qword ptr [eax]
    mov eax, esi
    fstp qword ptr [esi]
    add esi, 8
  }
#else
  __asm__ __volatile__ (
    "fld qword ptr [%%ebx]\n\t"
    "fsub qword ptr [%%eax]\n\t"
    "mov %%eax, %%esi\n\t"
    "fstp qword ptr [%%esi]\n\t"
    "add %%esi, 8\n"
    : : : "eax", "esi"
  );
#endif
}
NAKED void nseel_asm_sub_end(void) {}

//---------------------------------------------------------------------------------------------------------------
NAKED void nseel_asm_mul(void)
{
#ifdef _MSC_VER
  __asm
  {
    fld qword ptr [ebx]
    fmul qword ptr [eax]
    mov eax, esi
    fstp qword ptr [esi]
    add esi, 8
  }
#else
  __asm__ __volatile__ (
    "fld qword ptr [%%ebx]\n\t"
    "fmul qword ptr [%%eax]\n\t"
    "mov %%eax, %%esi\n\t"
    "fstp qword ptr [%%esi]\n\t"
    "add %%esi, 8\n"
    : : : "eax", "esi"
  );
#endif
}
NAKED void nseel_asm_mul_end(void) {}

//---------------------------------------------------------------------------------------------------------------
NAKED void nseel_asm_div(void)
{
#ifdef _MSC_VER
  __asm
  {
    fld qword ptr [ebx]
    fdiv qword ptr [eax]
    mov eax, esi
    fstp qword ptr [esi]
    add esi, 8
  }
#else
  __asm__ __volatile__ (
    "fld qword ptr [%%ebx]\n\t"
    "fdiv qword ptr [%%eax]\n\t"
    "mov %%eax, %%esi\n\t"
    "fstp qword ptr [%%esi]\n\t"
    "add %%esi, 8\n"
    : : : "eax", "esi"
  );
#endif
}
NAKED void nseel_asm_div_end(void) {}

//---------------------------------------------------------------------------------------------------------------
NAKED void nseel_asm_mod(void)
{
#ifdef _MSC_VER
  __asm
  {
    fld qword ptr [ebx]

    fld qword ptr [eax]
    fsub dword ptr [g_cmpaddtab+4]
    fabs
    fadd qword ptr [eax]
    fadd dword ptr [g_cmpaddtab+4]

    fmul dword ptr [g_half]

    fistp dword ptr [esi]
    fistp dword ptr [esi+4]
    mov eax, [esi+4]
    xor edx, edx
    div dword ptr [esi]
    mov [esi], edx
    fild dword ptr [esi]
    mov eax, esi
    fstp qword ptr [esi]
    add esi, 8
  }
#else
  __asm__ __volatile__ (
    "fld qword ptr [%%ebx]\n\t"

    "fld qword ptr [%%eax]\n\t"
    "fsub qword ptr [%0 + 4]\n\t"
    "fabs\n\t"
    "fadd qword ptr [%%eax]\n\t"
    "fadd dword ptr [%0 + 4]\n\t"

    "fmul dword ptr [%1]\n\t"

    "fistp dword ptr [%%esi]\n\t"
    "fistp dword ptr [%%esi + 4]\n\t"
    "mov %%eax, [%%esi + 4]\n\t"
    "xor %%edx, %%edx\n\t"
    "div dword ptr [%%esi]\n\t"
    "mov [%%esi], %%edx\n\t"
    "fild dword ptr [%%esi]\n\t"
    "mov %%eax, %%esi\n\t"
    "fstp qword ptr [%%esi]\n\t"
    "add %%esi, 8\n"
    :
    : "m"(g_cmpaddtab), "m"(g_half)
    : "eax", "esi"
  );
#endif
}
NAKED void nseel_asm_mod_end(void) {}

//---------------------------------------------------------------------------------------------------------------
NAKED void nseel_asm_or(void)
{
#ifdef _MSC_VER
  __asm
  {
    fld qword ptr [ebx]
    fld qword ptr [eax]
    fistp qword ptr [esi]
    fistp qword ptr [esi+8]
    mov ebx, [esi+8]
    or [esi], ebx
    mov ebx, [esi+12]
    or [esi+4], ebx
    fild qword ptr [esi]
    fstp qword ptr [esi]
    mov eax, esi
    add esi, 8
  }
#else
  __asm__ __volatile__ (
    "fld qword ptr [%%ebx]\n\t"
    "fld qword ptr [%%eax]\n\t"
    "fistp qword ptr [%%esi]\n\t"
    "fistp qword ptr [%%esi + 8]\n\t"
    "mov %%ebx, [%%esi + 8]\n\t"
    "or [%%esi], %%ebx\n\t"
    "mov %%ebx, [%%esi + 12]\n\t"
    "or [%%esi + 4], %%ebx\n\t"
    "fild qword ptr [%%esi]\n\t"
    "fstp qword ptr [%%esi]\n\t"
    "mov %%eax, %%esi\n\t"
    "add %%esi, 8\n"
    : : : "eax", "ebx", "esi"
  );
#endif
}
NAKED void nseel_asm_or_end(void) {}

//---------------------------------------------------------------------------------------------------------------
NAKED void nseel_asm_and(void)
{
#ifdef _MSC_VER
  __asm
  {
    fld qword ptr [ebx]
    fld qword ptr [eax]

    fistp qword ptr [esi]
    fistp qword ptr [esi+8]
    mov ebx, [esi+8]
    and [esi], ebx
    mov ebx, [esi+12]
    and [esi+4], ebx
    fild qword ptr [esi]
    fstp qword ptr [esi]

    mov eax, esi
    add esi, 8
  }
#else
  __asm__ __volatile__ (
    "fld qword ptr [%%ebx]\n\t"
    "fld qword ptr [%%eax]\n\t"
    "fistp qword ptr [%%esi]\n\t"
    "fistp qword ptr [%%esi + 8]\n\t"
    "mov %%ebx, [%%esi + 8]\n\t"
    "and [%%esi], %%ebx\n\t"
    "mov %%ebx, [%%esi + 12]\n\t"
    "and [%%esi + 4], %%ebx\n\t"
    "fild qword ptr [%%esi]\n\t"
    "fstp qword ptr [%%esi]\n\t"
    "mov %%eax, %%esi\n\t"
    "add %%esi, 8\n"
    : : : "eax", "ebx", "esi"
  );
#endif
}
NAKED void nseel_asm_and_end(void) {}

//---------------------------------------------------------------------------------------------------------------
NAKED void nseel_asm_uplus(void) // this is the same as doing nothing, it seems
{
#if 0
  __asm 
  {
    mov ebx, nextBlock
    mov ecx, [eax]
    mov [ebx], ecx
    mov ecx, [eax+4]
    mov [ebx+4], ecx
    mov eax, ebx
    add ebx, 8
    mov nextBlock, ebx
  }
#endif
}
NAKED void nseel_asm_uplus_end(void) {}

//---------------------------------------------------------------------------------------------------------------
NAKED void nseel_asm_uminus(void)
{
#ifdef _MSC_VER
  __asm
  {
    mov ecx, [eax]
    mov ebx, [eax+4]
    xor ebx, 0x80000000
    mov [esi], ecx
    mov [esi+4], ebx
    mov eax, esi
    add esi, 8
  }
#else
  __asm__ __volatile__ (
    "mov %%ecx, [%%eax]\n\t"
    "mov %%ebx, [%%eax+4]\n\t"
    "xor %%ebx, 0x80000000\n\t"
    "mov [%%esi], %%ecx\n\t"
    "mov [%%esi+4], %%ebx\n\t"
    "mov %%eax, %%esi\n\t"
    "add %%esi, 8\n"
    : : : "eax", "ebx", "ecx", "esi"
  );
#endif
}
NAKED void nseel_asm_uminus_end(void) {}



//---------------------------------------------------------------------------------------------------------------
NAKED void nseel_asm_sign(void)
{
#ifdef _MSC_VER
  __asm
  {
    mov ecx, [eax+4]
    mov edx, [eax]
    test edx, 0FFFFFFFFh
    jnz nonzero

    // high dword (minus sign bit) is zero 
    test ecx, 07FFFFFFFh
    jz zero // zero zero, return the value passed directly

nonzero:
    shr ecx, 31

    fld dword ptr [g_signs+ecx*4]

    fstp qword ptr [esi]

    mov eax, esi
    add esi, 8
zero:
  }
#else
  __asm__ __volatile__ (
    "mov %%ecx, [%%eax+4]\n\t"
    "mov %%edx, [%%eax]\n\t"
    "test %%edx, 0xFFFFFFFF\n\t"
    "jnz nonzero\n\t"
    // high dword (minus sign bit) is zero 
    "test %%ecx, 0x7FFFFFFF\n\t"
    "jz zero\n"
    // zero zero, return the value passed directly
  "nonzero:\n\t"
    "shr %%ecx, 31\n\t"
    "fld dword ptr [%0 + %%ecx * 4]\n\t"
    "fstp qword ptr [%%esi]\n\t"
    "mov %%eax, %%esi\n\t"
    "add %%esi, 8\n"
  "zero:\n"
    :
    : "m"(g_signs)
    : "eax", "ecx", "edx", "esi"
  );
#endif
}
NAKED void nseel_asm_sign_end(void) {}



//---------------------------------------------------------------------------------------------------------------
NAKED void nseel_asm_bnot(void)
{
#ifdef _MSC_VER
  __asm
  {
    fld qword ptr [eax]
    fabs
    fcomp qword ptr [g_closefact]
    fstsw ax
    shr eax, 6
    and eax, (1<<2)
    fld dword ptr [g_cmpaddtab+eax]
    fstp qword ptr [esi]
    mov eax, esi
    add esi, 8
  }
#else
  __asm__ __volatile__ (
    "fld qword ptr [%%eax]\n\t"
    "fabs\n\t"
    "fcomp qword ptr [%0]\n\t"
    "fstsw %%ax\n\t"
    "shr %%eax, 6\n\t"
    "and %%eax, (1<<2)\n\t"
    "fld dword ptr [%1 + %%eax]\n\t"
    "fstp qword ptr [%%esi]\n\t"
    "mov %%eax, %%esi\n\t"
    "add %%esi, 8\n"
    :
    : "m"(g_closefact), "m"(g_cmpaddtab)
    : "eax", "esi"
  );
#endif
}
NAKED void nseel_asm_bnot_end(void) {}

//---------------------------------------------------------------------------------------------------------------
NAKED void nseel_asm_if(void)
{
#ifdef _MSC_VER
  __asm
  {
    fld qword ptr [eax]
    fabs
    fcomp qword ptr [g_closefact]
    fstsw ax

    shr eax, 6

    mov dword ptr [esi], 0FFFFFFFFh
    mov dword ptr [esi+4], 0FFFFFFFFh

    and eax, (1<<2)

    mov eax, [esi+eax]

    call eax // call the proper function

    // at this point, the return value will be in eax, as desired
  }
#else
  __asm__ __volatile__ (
    "fld qword ptr [%%eax]\n\t"
    "fabs\n\t"
    "fcomp qword ptr [%0]\n\t"
    "fstsw %%ax\n\t"
    "shr %%eax, 6\n\t"
    "mov dword ptr [%%esi], 0xFFFFFFFF\n\t"
    "mov dword ptr [%%esi+4], 0xFFFFFFFF\n\t"
    "and %%eax, (1<<2)\n\t"
    "mov %%eax, [%%esi + %%eax]\n\t"
    "mov %%eax, %%esi\n\t"
    "add %%esi, 8\n"
    :
    : "m"(g_closefact), "m"(g_cmpaddtab)
    : "eax", "esi"
  );
#endif
}
NAKED void nseel_asm_if_end(void) {}

#ifdef NSEEL_LOOPFUNC_SUPPORT
#ifndef NSEEL_LOOPFUNC_SUPPORT_MAXLEN
#define NSEEL_LOOPFUNC_SUPPORT_MAXLEN (4096)
#endif
//---------------------------------------------------------------------------------------------------------------
NAKED void nseel_asm_repeat(void)
{
#ifdef _MSC_VER
  __asm
  {
    fld qword ptr [eax]
    fistp dword ptr [esi]
    mov ecx, [esi]
    cmp ecx, 1
    jl skip
    cmp ecx, NSEEL_LOOPFUNC_SUPPORT_MAXLEN
    jl again
    mov ecx, NSEEL_LOOPFUNC_SUPPORT_MAXLEN
again:
      push ecx
      push esi // revert back to last temp workspace
        mov ecx, 0FFFFFFFFh
        call ecx
      pop esi
      pop ecx
    dec ecx
    jnz again
skip:
  }
#else
  __asm__ __volatile__ (
    "fld qword ptr [%%eax]\n\t"
    "fistp dword ptr [%%esi]\n\t"
    "mov %%ecx, [%%esi]\n\t"
    "cmp %%ecx, 1\n\t"
    "jl skip\n\t"
    "cmp %%ecx, %0\n\t"
    "jl again\n\t"
    "mov %%ecx, %0\n"
  "again:\n\t\t"
      "push %%ecx\n\t\t"
      "push %%esi\n\t\t\t" // revert back to last temp workspace
        "mov %%ecx, 0xFFFFFFFF\n\t\t\t"
        "call ecx\n\t\t"
      "pop %%esi\n\t\t"
      "pop %%ecx\n\t"
    "dec %%ecx\n\t"
    "jnz again\n"
  "skip:\n"
    :
    :"i"(NSEEL_LOOPFUNC_SUPPORT_MAXLEN)
    :"eax", "ecx", "esi"
  );
#endif
}
NAKED void nseel_asm_repeat_end(void) {}
#endif

//---------------------------------------------------------------------------------------------------------------
NAKED void nseel_asm_equal(void)
{
#ifdef _MSC_VER
  __asm
  {
    fld qword ptr [eax]
    fsub qword ptr [ebx]
    fabs
    fcomp qword ptr [g_closefact]
    fstsw ax
    shr eax, 6
    and eax, (1<<2)
    fld dword ptr [g_cmpaddtab+eax]
    fstp qword ptr [esi]
    mov eax, esi
    add esi, 8
  }
#else
  __asm__ __volatile__ (
    "fld qword ptr [%%eax]\n\t"
    "fsub qword ptr [%%ebx]\n\t"
    "fabs\n\t"
    "fcomp qword ptr [%0]\n\t"
    "fstsw %%ax\n\t"
    "shr %%eax, 6\n\t"
    "and %%eax, (1<<2)\n\t"
    "fld dword ptr [%1 + %%eax]\n\t"
    "fstp qword ptr [%%esi]\n\t"
    "mov %%eax, %%esi\n\t"
    "add %%esi, 8\n\t"
    :
    :"m"(g_closefact), "m"(g_cmpaddtab)
    :"eax", "ebx", "esi"
  );
#endif
}
NAKED void nseel_asm_equal_end(void) {}

//---------------------------------------------------------------------------------------------------------------
NAKED void nseel_asm_below(void)
{
#ifdef _MSC_VER
  __asm
  {
    fld qword ptr [ebx]
    fcomp qword ptr [eax]
    fstsw ax
    shr eax, 6
    and eax, (1<<2)
    fld dword ptr [g_cmpaddtab+eax]
    fstp qword ptr [esi]
    mov eax, esi
    add esi, 8
  }
#else
  __asm__ __volatile__ (
    "fld qword ptr [%%ebx]\n\t"
    "fcomp qword ptr [%%eax]\n\t"
    "fstsw %%ax\n\t"
    "shr %%eax, 6\n\t"
    "and %%eax, (1<<2)\n\t"
    "fld dword ptr [%0 + %%eax]\n\t"
    "fstp qword ptr [%%esi]\n\t"
    "mov %%eax, %%esi\n\t"
    "add %%esi, 8\n\t"
    :
    :"m"(g_cmpaddtab)
    :"eax", "ebx", "esi"
  );
#endif
}
NAKED void nseel_asm_below_end(void) {}

//---------------------------------------------------------------------------------------------------------------
NAKED void nseel_asm_above(void)
{
#ifdef _MSC_VER
  __asm
  {
    fld qword ptr [eax]
    fcomp qword ptr [ebx]
    fstsw ax
    shr eax, 6
    and eax, (1<<2)
    fld dword ptr [g_cmpaddtab+eax]

    fstp qword ptr [esi]
    mov eax, esi
    add esi, 8
  }
#else
  __asm__ __volatile__ (
    "fld qword ptr [%%eax]\n\t"
    "fcomp qword ptr [%%ebx]\n\t"
    "fstsw %%ax\n\t"
    "shr %%eax, 6\n\t"
    "and %%eax, (1<<2)\n\t"
    "fld dword ptr [%0 + %%eax]\n\t"
    "fstp qword ptr [%%esi]\n\t"
    "mov %%eax, %%esi\n\t"
    "add %%esi, 8\n\t"
    :
    :"m"(g_cmpaddtab)
    :"eax", "ebx", "esi"
  );
#endif
}
NAKED void nseel_asm_above_end(void) {}


NAKED void nseel_asm_min(void)
{
#ifdef _MSC_VER
  __asm
  {
    fld qword ptr [eax]
    fld qword ptr [ebx]
    fld st(1)
    fsub st(0), st(1)
    fabs  // stack contains fabs(1-2),1,2
    fchs
    fadd
    fadd
    fmul dword ptr [g_half]
    fstp qword ptr [esi]
    mov eax, esi
    add esi, 8
  }
#else
  __asm__ __volatile__ (
    "fld qword ptr [%%eax]\n\t"
    "fld qword ptr [%%ebx]\n\t"
    "fld st(1)\n\t"
    "fsub st(0), st(1)\n\t"
    "fabs\n\t"  // stack contains fabs(1-2),1,2
    "fchs\n\t"
    "faddp\n\t" // no-operand version of fadd is implicit faddp
    "faddp\n\t"
    "fmul dword ptr [%0]\n\t"
    "fstp qword ptr [%%esi]\n\t"
    "mov %%eax, %%esi\n\t"
    "add %%esi, 8\n\t"
    :
    :"m"(g_half)
    :"eax", "ebx", "esi"
  );
#endif
}
NAKED void nseel_asm_min_end(void) {}

NAKED void nseel_asm_max(void)
{
#ifdef _MSC_VER
  __asm
  {
    fld qword ptr [eax]
    fld qword ptr [ebx]
    fld st(1)
    fsub st(0), st(1)
    fabs  // stack contains fabs(1-2),1,2
    fadd
    fadd
    fmul dword ptr [g_half]
    fstp qword ptr [esi]
    mov eax, esi
    add esi, 8
  }
#else
  __asm__ __volatile__ (
    "fld qword ptr [%%eax]\n\t"
    "fld qword ptr [%%ebx]\n\t"
    "fld st(1)\n\t"
    "fsub st(0), st(1)\n\t"
    "fabs\n\t"  // stack contains fabs(1-2),1,2
    "faddp\n\t"
    "faddp\n\t"
    "fmul dword ptr [%0]\n\t"
    "fstp qword ptr [%%esi]\n\t"
    "mov %%eax, %%esi\n\t"
    "add %%esi, 8\n\t"
    :
    :"m"(g_half)
    :"eax", "ebx", "esi"
  );
#endif
}
NAKED void nseel_asm_max_end(void) {}

