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
#include "../ns-eel/ns-eel.h"
#include "../ns-eel/ns-eel-int.h"
#include "megabuf.h"

void megabuf_ppproc(void *data, int data_size, void **userfunc_data)
{
  if (data_size > 5 && *(int*)((char *)data+1) == (int)0xFFFFFFFF)
  {
    *(int*)((char *)data+1) = (int) (userfunc_data+0);
  }
}

void megabuf_cleanup(NSEEL_VMCTX ctx)
{
  if (ctx)
  {
    compileContext *c=(compileContext*)ctx;
    if (c->userfunc_data[0])
    {
      double **blocks = (double **)c->userfunc_data[0];
      int x;
      for (x = 0; x < MEGABUF_BLOCKS; x ++)
      {
        if (blocks[x]) GlobalFree(blocks[x]);
        blocks[x]=0;
      }
      GlobalFree((HGLOBAL)blocks);
      c->userfunc_data[0]=0;
    }
  }
}

static double * NSEEL_CGEN_CALL megabuf_(double ***blocks, double *which)
{
  static double error;
  int w=(int)(*which + 0.0001);
  int whichblock = w/MEGABUF_ITEMSPERBLOCK;

  if (!*blocks)
  {
    *blocks = (double **)GlobalAlloc(GPTR,sizeof(double *)*MEGABUF_BLOCKS);
  }
  if (!*blocks) return &error;

  if (w >= 0 && whichblock >= 0 && whichblock < MEGABUF_BLOCKS)
  {
    int whichentry = w%MEGABUF_ITEMSPERBLOCK;
    if (!(*blocks)[whichblock])
    {
      (*blocks)[whichblock]=(double *)GlobalAlloc(GPTR,sizeof(double)*MEGABUF_ITEMSPERBLOCK);
    }
    if ((*blocks)[whichblock])
      return &(*blocks)[whichblock][whichentry];
  }

  return &error;
}

static double * (NSEEL_CGEN_CALL *__megabuf)(double ***,double *) = &megabuf_;
NAKED void _asm_megabuf(void)
{
  double ***my_ctx;
  double *parm_a, *__nextBlock;
#ifdef _MSC_VER
  __asm { mov edx, 0xFFFFFFFF }
  __asm { mov ebp, esp }
  __asm { sub esp, __LOCAL_SIZE }
  __asm { mov dword ptr my_ctx, edx }
  __asm { mov dword ptr parm_a, eax }
#else
  __asm__ __volatile__ (
    "  mov %%edx, 0xffffffff\n\t"
    "  mov %%ebp, %%esp\n\t"
    "  sub %%esp, 0\n\t"  // __LOCAL_SIZE = 0
    "  mov dword ptr %0, %%edx\n\t"
    "  mov dword ptr %1, %%eax\n\t"
    :"=m"(my_ctx), "=m"(parm_a)
    :
    :"eax", "edx"
  );
#endif
  __nextBlock = __megabuf(my_ctx,parm_a);

#ifdef _MSC_VER
  __asm { mov eax, __nextBlock } // this is custom, returning pointer
  __asm { mov esp, ebp }
#else
  __asm__ __volatile__ (
    "  mov  %%eax, %0\n\t"
    "  mov  %%esp, %%ebp\n\t"
    :
    :"m"(__nextBlock)
    :"eax", "ebx"
  );
#endif

}
NAKED void _asm_megabuf_end(void) {}
