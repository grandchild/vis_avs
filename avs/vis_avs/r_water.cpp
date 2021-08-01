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
// alphachannel safe 11/21/99
#include "c_water.h"
#include "r_defs.h"
#include "timing.h"


#ifndef LASER

#define PUT_INT(y) data[pos]=(y)&255; data[pos+1]=(y>>8)&255; data[pos+2]=(y>>16)&255; data[pos+3]=(y>>24)&255
#define GET_INT() (data[pos]|(data[pos+1]<<8)|(data[pos+2]<<16)|(data[pos+3]<<24))
void C_THISCLASS::load_config(unsigned char *data, int len)
{
	int pos=0;
	if (len-pos >= 4) { enabled=GET_INT(); pos+=4; }
}
int  C_THISCLASS::save_config(unsigned char *data)
{
	int pos=0;
	PUT_INT(enabled); pos+=4;
	return pos;
}


C_THISCLASS::C_THISCLASS()
{
  enabled=1;
  lastframe_len=0;
  lastframe=NULL;
}

C_THISCLASS::~C_THISCLASS()
{
  if (lastframe) free(lastframe);
}


#define _R(x) (( x ) & 0xff)
#define _G(x) ((( x )) & 0xff00)
#define _B(x) ((( x )) & 0xff0000)
#define _RGB(r,g,b) (( r ) | (( g ) & 0xff00) | (( b ) & 0xff0000))

static const int zero=0;

int C_THISCLASS::render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h)
{
  smp_begin(1,visdata,isBeat,framebuffer,fbout,w,h);
  if (isBeat & 0x80000000) return 0;

  smp_render(0,1,visdata,isBeat,framebuffer,fbout,w,h);
  return smp_finish(visdata,isBeat,framebuffer,fbout,w,h);
}


int C_THISCLASS::smp_begin(int max_threads, char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h)
{
  if (!enabled) return 0;

  if (!lastframe || w*h != lastframe_len)
  {
    if (lastframe) free(lastframe);
    lastframe_len=w*h;
    lastframe=(unsigned int *)calloc(w*h, sizeof(int));
  }

  return max_threads;
}


int C_THISCLASS::smp_finish(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h) // return value is that of render() for fbstuff etc
{
  return !!enabled;
}


void C_THISCLASS::smp_render(int this_thread, int max_threads, char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h)
{
  if (!enabled) return;

	unsigned int *f = (unsigned int *) framebuffer;
  unsigned int *of = (unsigned int *) fbout;
  unsigned int *lfo = (unsigned int *) lastframe;


  int start_l = ( this_thread * h ) / max_threads;
  int end_l;

  if (this_thread >= max_threads - 1) end_l = h;
  else end_l = ( (this_thread+1) * h ) / max_threads;  

  int outh=end_l-start_l;
  if (outh<1) return;

  int skip_pix=start_l*w;

  f += skip_pix;
  of+= skip_pix;
  lfo += skip_pix;

  int at_top=0, at_bottom=0;

  if (!this_thread) at_top=1;
  if (this_thread >= max_threads - 1) at_bottom=1;


  timingEnter(0);

  {

    if (at_top)
    // top line
    {
      int x;
    
      // left edge
	    {
        int r=_R(f[1]); int g=_G(f[1]); int b=_B(f[1]);
        r += _R(f[w]);  g += _G(f[w]);  b += _B(f[w]);
        f++;

        r-=_R(lfo[0]); g-=_G(lfo[0]); b-=_B(lfo[0]);
        lfo++;

        if (r < 0) r=0;
        else if (r > 255) r=255;
        if (g < 0) g=0;
        else if (g > 255*256) g=255*256;
        if (b < 0) b=0;
        else if (b > 255*65536) b=255*65536;
		    *of++=_RGB(r,g,b);          
      }

      // middle of line
      x=(w-2);
	    while (x--)
	    {
        int r=_R(f[1]); int g=_G(f[1]); int b=_B(f[1]);
        r += _R(f[-1]); g += _G(f[-1]); b += _B(f[-1]);
        r += _R(f[w]);  g += _G(f[w]);  b += _B(f[w]);
        f++;

        r/=2; g/=2; b/=2;

        r-=_R(lfo[0]); g-=_G(lfo[0]); b-=_B(lfo[0]);
        lfo++;

        if (r < 0) r=0;
        else if (r > 255) r=255;
        if (g < 0) g=0;
        else if (g > 255*256) g=255*256;
        if (b < 0) b=0;
        else if (b > 255*65536) b=255*65536;
		    *of++=_RGB(r,g,b);          
      }

      // right block
	    {
        int r=_R(f[-1]); int g=_G(f[-1]); int b=_B(f[-1]);
        r += _R(f[w]);  g += _G(f[w]);  b += _B(f[w]);
        f++;

        r-=_R(lfo[0]); g-=_G(lfo[0]); b-=_B(lfo[0]);
        lfo++;

        if (r < 0) r=0;
        else if (r > 255) r=255;
        if (g < 0) g=0;
        else if (g > 255*256) g=255*256;
        if (b < 0) b=0;
        else if (b > 255*65536) b=255*65536;
		    *of++=_RGB(r,g,b);          
      }
	  }


	  // middle block
    {
      int y=outh-at_top-at_bottom;
      while (y--)
      {
        int x;
      
        // left edge
	      {
          int r=_R(f[1]); int g=_G(f[1]); int b=_B(f[1]);
          r += _R(f[w]);  g += _G(f[w]);  b += _B(f[w]);
          r += _R(f[-w]); g += _G(f[-w]); b += _B(f[-w]);
          f++;

          r/=2; g/=2; b/=2;

          r-=_R(lfo[0]); g-=_G(lfo[0]); b-=_B(lfo[0]);
          lfo++;

          if (r < 0) r=0;
          else if (r > 255) r=255;
          if (g < 0) g=0;
          else if (g > 255*256) g=255*256;
          if (b < 0) b=0;
          else if (b > 255*65536) b=255*65536;
		      *of++=_RGB(r,g,b);          
        }

        // middle of line
        x=(w-2);
#ifdef NO_MMX
	      while (x--)
	      {
          int r=_R(f[1]); int g=_G(f[1]); int b=_B(f[1]);
          r += _R(f[-1]); g += _G(f[-1]); b += _B(f[-1]);
          r += _R(f[w]);  g += _G(f[w]);  b += _B(f[w]);
          r += _R(f[-w]); g += _G(f[-w]); b += _B(f[-w]);
          f++;

          r/=2; g/=2; b/=2;

          r-=_R(lfo[0]); g-=_G(lfo[0]); b-=_B(lfo[0]);
          lfo++;

          if (r < 0) r=0;
          else if (r > 255) r=255;
          if (g < 0) g=0;
          else if (g > 255*256) g=255*256;
          if (b < 0) b=0;
          else if (b > 255*65536) b=255*65536;
		      *of++=_RGB(r,g,b);          
        }
#else
#ifdef _MSC_VER // MSVC asm
        __asm
        {
          mov esi, f
          mov edi, of
          mov edx, lfo
          mov ecx, x
          mov ebx, w
          shl ebx, 2
          shr ecx, 1
          sub esi, ebx
          align 16
mmx_water_loop1:
          movd mm0, [esi+ebx+4]

          movd mm1, [esi+ebx-4]
          punpcklbw mm0, [zero]

          movd mm2, [esi+ebx*2]
          punpcklbw mm1, [zero]

          movd mm3, [esi]
          punpcklbw mm2, [zero]

          movd mm4, [edx]
          paddw mm0, mm1

          punpcklbw mm3, [zero]
          movd mm7, [esi+ebx+8]

          punpcklbw mm4, [zero]
          paddw mm2, mm3

          movd mm6, [esi+ebx]
          paddw mm0, mm2

          psrlw mm0, 1
          punpcklbw mm7, [zero]

          movd mm2, [esi+ebx*2+4]
          psubw mm0, mm4

          movd mm3, [esi+4]
          packuswb mm0, mm0

          movd [edi], mm0
          punpcklbw mm6, [zero]

          movd mm4, [edx+4]
          punpcklbw mm2, [zero]

          paddw mm7, mm6
          punpcklbw mm3, [zero]

          punpcklbw mm4, [zero]
          paddw mm2, mm3
          
          paddw mm7, mm2
          add edx, 8

          psrlw mm7, 1
          add esi, 8

          psubw mm7, mm4

          packuswb mm7, mm7

          movd [edi+4], mm7

          add edi, 8

          dec ecx
          jnz mmx_water_loop1

          add esi, ebx
          mov f, esi
          mov of, edi
          mov lfo, edx
        };
#else // _MSC_VER, GCC asm
        __asm__ __volatile__ (
          "mov       %%esi, %[f]\n\t"
          "mov       %%edi, %[of]\n\t"
          "mov       %%edx, %[lfo]\n\t"
          "mov       %%ecx, %[x]\n\t"
          "mov       %%ebx, %[w]\n\t"
          "shl       %%ebx, 2\n\t"
          "shr       %%ecx, 1\n\t"
          "sub       %%esi, %%ebx\n\t"
          ".align    16\n\t"
          "mmx_water_loop1:\n\t"
          "movd      %%mm0, [%%esi + %%ebx + 4]\n\t"
          "movd      %%mm1, [%%esi + %%ebx - 4]\n\t"
          "punpcklbw %%mm0, [%[zero]]\n\t"
          "movd      %%mm2, [%%esi + %%ebx * 2]\n\t"
          "punpcklbw %%mm1, [%[zero]]\n\t"
          "movd      %%mm3, [%%esi]\n\t"
          "punpcklbw %%mm2, [%[zero]]\n\t"
          "movd      %%mm4, [%%edx]\n\t"
          "paddw     %%mm0, %%mm1\n\t"
          "punpcklbw %%mm3, [%[zero]]\n\t"
          "movd      %%mm7, [%%esi + %%ebx + 8]\n\t"
          "punpcklbw %%mm4, [%[zero]]\n\t"
          "paddw     %%mm2, %%mm3\n\t"
          "movd      %%mm6, [%%esi + %%ebx] \n\t"
          "paddw     %%mm0, %%mm2\n\t"
          "psrlw     %%mm0, 1\n\t"
          "punpcklbw %%mm7, [%[zero]]\n\t"
          "movd      %%mm2, [%%esi + %%ebx * 2 + 4]\n\t"
          "psubw     %%mm0, %%mm4\n\t"
          "movd      %%mm3, [%%esi + 4]\n\t"
          "packuswb  %%mm0, %%mm0\n\t"
          "movd      [%%edi], %%mm0\n\t"
          "punpcklbw %%mm6, [%[zero]]\n\t"
          "movd      %%mm4, [%%edx + 4]\n\t"
          "punpcklbw %%mm2, [%[zero]]\n\t"
          "paddw     %%mm7, %%mm6\n\t"
          "punpcklbw %%mm3, [%[zero]]\n\t"
          "punpcklbw %%mm4, [%[zero]]\n\t"
          "paddw     %%mm2, %%mm3\n\t"
          "paddw     %%mm7, %%mm2\n\t"
          "add       %%edx, 8\n\t"
          "psrlw     %%mm7, 1\n\t"
          "add       %%esi, 8\n\t"
          "psubw     %%mm7, %%mm4\n\t"
          "packuswb  %%mm7, %%mm7\n\t"
          "movd      [%%edi + 4], %%mm7\n\t"
          "add       %%edi, 8\n\t"
          "dec       %%ecx\n\t"
          "jnz       mmx_water_loop1\n\t"
          "add       %%esi, %%ebx\n\t"
          "mov       %[f], %%esi\n\t"
          "mov       %[of], %%edi\n\t"
          "mov       %[lfo], %%edx\n\t"
          : [f]"+m"(f), [of]"+m"(of), [lfo]"+m"(lfo)
          : [x]"m"(x), [w]"m"(w), [zero]"m"(zero)
          : "ebx", "ecx", "edx", "edi", "esi"
        );
#endif // _MSC_VER
#endif
        // right block
	      {
          int r=_R(f[-1]); int g=_G(f[-1]); int b=_B(f[-1]);
          r += _R(f[w]);  g += _G(f[w]);  b += _B(f[w]);
          r += _R(f[-w]); g += _G(f[-w]); b += _B(f[-w]);
          f++;

          r/=2; g/=2; b/=2;

          r-=_R(lfo[0]); g-=_G(lfo[0]); b-=_B(lfo[0]);
          lfo++;

          if (r < 0) r=0;
          else if (r > 255) r=255;
          if (g < 0) g=0;
          else if (g > 255*256) g=255*256;
          if (b < 0) b=0;
          else if (b > 255*65536) b=255*65536;
		      *of++=_RGB(r,g,b);          
        }
	    }
    }
    // bottom line
    if (at_bottom)
    {
      int x;
    
      // left edge
	    {
        int r=_R(f[1]); int g=_G(f[1]); int b=_B(f[1]);
        r += _R(f[-w]);  g += _G(f[-w]);  b += _B(f[-w]);
        f++;

        r-=_R(lfo[0]); g-=_G(lfo[0]); b-=_B(lfo[0]);
        lfo++;

        if (r < 0) r=0;
        else if (r > 255) r=255;
        if (g < 0) g=0;
        else if (g > 255*256) g=255*256;
        if (b < 0) b=0;
        else if (b > 255*65536) b=255*65536;
		    *of++=_RGB(r,g,b);          
      }

      // middle of line
      x=(w-2);
	    while (x--)
	    {
        int r=_R(f[1]); int g=_G(f[1]); int b=_B(f[1]);
        r += _R(f[-1]); g += _G(f[-1]); b += _B(f[-1]);
        r += _R(f[-w]);  g += _G(f[-w]);  b += _B(f[-w]);
        f++;

        r/=2; g/=2; b/=2;

        r-=_R(lfo[0]); g-=_G(lfo[0]); b-=_B(lfo[0]);
        lfo++;

        if (r < 0) r=0;
        else if (r > 255) r=255;
        if (g < 0) g=0;
        else if (g > 255*256) g=255*256;
        if (b < 0) b=0;
        else if (b > 255*65536) b=255*65536;
		    *of++=_RGB(r,g,b);          
      }

      // right block
	    {
        int r=_R(f[-1]); int g=_G(f[-1]); int b=_B(f[-1]);
        r += _R(f[-w]);  g += _G(f[-w]);  b += _B(f[-w]);
        f++;

        r-=_R(lfo[0]); g-=_G(lfo[0]); b-=_B(lfo[0]);
        lfo++;

        if (r < 0) r=0;
        else if (r > 255) r=255;
        if (g < 0) g=0;
        else if (g > 255*256) g=255*256;
        if (b < 0) b=0;
        else if (b > 255*65536) b=255*65536;
		    *of++=_RGB(r,g,b);          
      }
	  }
  }

  memcpy(lastframe+skip_pix,framebuffer+skip_pix,w*outh*sizeof(int));
  
#ifndef NO_MMX
#ifdef _MSC_VER // MSVC asm
    __asm emms;
#else // _MSC_VER, GCC asm
    __asm__ __volatile__ ("emms");
#endif // _MSC_VER
#endif
	timingLeave(0);
}

C_RBASE *R_Water(char *desc)
{
	if (desc) { strcpy(desc,MOD_NAME); return NULL; }
	return (C_RBASE *) new C_THISCLASS();
}

#else
C_RBASE *R_Water(char *desc) { return NULL; }
#endif
