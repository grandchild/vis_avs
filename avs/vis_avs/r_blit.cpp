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
// highly optimized on 10/10/00 JF. MMX.

#include "c_blit.h"
#include <windows.h>
#include <commctrl.h>
#include "r_defs.h"
#include "resource.h"

// TODO: doesn't complain if not included. -> research.
#include "timing.h"


#ifndef LASER

static const unsigned int revn[2]={0xff00ff,0xff00ff};//{0x1000100,0x1000100}; <<- this is actually more correct, but we're going for consistency vs. the non-mmx ver-jf
static const int zero=0;

#define PUT_INT(y) data[pos]=(y)&255; data[pos+1]=(y>>8)&255; data[pos+2]=(y>>16)&255; data[pos+3]=(y>>24)&255
#define GET_INT() (data[pos]|(data[pos+1]<<8)|(data[pos+2]<<16)|(data[pos+3]<<24))
void C_THISCLASS::load_config(unsigned char *data, int len)
{
	int pos=0;
	if (len-pos >= 4) { scale=GET_INT(); pos+=4; }
	if (len-pos >= 4) { scale2=GET_INT(); pos+=4; }
	if (len-pos >= 4) { blend=GET_INT(); pos+=4; }
	if (len-pos >= 4) { beatch=GET_INT(); pos+=4; }
	if (len-pos >= 4) { subpixel=GET_INT(); pos+=4; }
  else subpixel=0;

	fpos=scale;
	
}
int  C_THISCLASS::save_config(unsigned char *data)
{
	int pos=0;
	PUT_INT(scale); pos+=4;
	PUT_INT(scale2); pos+=4;
	PUT_INT(blend); pos+=4;
	PUT_INT(beatch); pos+=4;
  PUT_INT(subpixel); pos+=4;
	return pos;
}


C_THISCLASS::C_THISCLASS()
{
	scale=30;
	scale2=30;
	blend=0;
	fpos=scale;
	beatch=0;
  subpixel=1;
}

C_THISCLASS::~C_THISCLASS()
{
}

int C_THISCLASS::blitter_out(int *framebuffer, int *fbout, int w, int h, int f_val)
{
	const int adj=7;
	int ds_x = ((f_val+(1<<adj)-32)<<(16-adj));	
	int x_len=((w<<16)/ds_x)&~3;
	int y_len=(h<<16)/ds_x;

  if (x_len >= w || y_len >= h) return 0;


	int start_x=(w-x_len)/2;
	int start_y=(h-y_len)/2;
	int s_y=32768;

	int *dest=(int *)framebuffer+start_y*w + start_x;
	int *src=(int *)fbout+start_y*w + start_x;
  int y;

	fbout += start_y*w;
	for (y = 0; y < y_len; y ++)
	{
		int s_x=32768;
		unsigned int *src = ((unsigned int *) framebuffer)+(s_y>>16)*w;
		unsigned int *old_dest=((unsigned int *)fbout) + start_x;
		s_y+=ds_x;
    if (!blend) 
		{
			int x=x_len/4;
			while (x--)
			{ 
				old_dest[0]=src[s_x>>16];
				s_x += ds_x;
				old_dest[1]=src[s_x>>16];
				s_x += ds_x;
				old_dest[2]=src[s_x>>16];
				s_x += ds_x;
				old_dest[3]=src[s_x>>16];
				s_x += ds_x;
				old_dest+=4;
			}
		}
		else
		{
			unsigned int *s2=(unsigned int *)framebuffer+((y+start_y)*w)+start_x;
			int x=x_len/4;
			while (x--)
			{ 
				old_dest[0]=BLEND_AVG(s2[0],src[s_x>>16]);
				s_x += ds_x;
				old_dest[1]=BLEND_AVG(s2[1],src[s_x>>16]);
				s_x += ds_x;
				old_dest[2]=BLEND_AVG(s2[2],src[s_x>>16]);
				s_x += ds_x;
				old_dest[3]=BLEND_AVG(s2[3],src[s_x>>16]);
				s2+=4;
				old_dest+=4;
				s_x += ds_x;
			}
 		}
		fbout += w;
	}
	for (y = 0; y < y_len; y ++)
	{
		memcpy(dest,src,x_len*sizeof(int));
		dest+=w;
		src+=w;
	}

  return 0;
}



int C_THISCLASS::blitter_normal(int *framebuffer, int *fbout, int w, int h, int f_val)
{
	int ds_x = ((f_val+32)<<16)/64;
	int isx=(((w<<16)-((ds_x*w)))/2);
	int s_y=(((h<<16)-((ds_x*h)))/2);

  if (subpixel)
  {
    int y;
	  for (y = 0; y < h; y ++)
	  {
		  int s_x=isx;
		  unsigned int *src = ((unsigned int *) framebuffer)+(s_y>>16)*w;
      int ypart=(s_y>>8)&0xff;
		  s_y+=ds_x;
#ifdef NO_MMX
      {
        ypart=(ypart*255)>>8;
    		int x=w/4;
		  	while (x--)
			  {
          fbout[0]=BLEND4(src+(s_x>>16),w,(s_x>>8)&0xff,ypart);
					s_x += ds_x;
          fbout[1]=BLEND4(src+(s_x>>16),w,(s_x>>8)&0xff,ypart);
					s_x += ds_x;
          fbout[2]=BLEND4(src+(s_x>>16),w,(s_x>>8)&0xff,ypart);
					s_x += ds_x;
          fbout[3]=BLEND4(src+(s_x>>16),w,(s_x>>8)&0xff,ypart);
					s_x += ds_x;
          fbout+=4;
				}
      }
#else
      {
          __int64 mem5,mem7;
#ifdef _MSC_VER // MSVC asm
          __asm
          {
            mov edi, fbout         
            mov esi, w

            movd mm7, ypart
            punpcklwd mm7,mm7

            movq mm5, revn
            punpckldq mm7,mm7

            psubw mm5, mm7         
            mov ecx, esi

            movq mem5,mm5
            movq mem7,mm7

            shr ecx, 1
            mov edx, s_x

            align 16
      mmx_scale_loop1:

            mov ebx, edx
            mov eax, edx

            shr eax, 14
            add edx, ds_x

            shr ebx, 8
            and eax, ~3
            
            add eax, src
            and ebx, 0xff

            movd mm6, ebx
            //

            movq mm4, revn
            punpcklwd mm6,mm6

            movd mm0, [eax]
            punpckldq mm6,mm6

            movd mm1, [eax+4]
            psubw mm4, mm6

            movd mm2, [eax+esi*4]
            punpcklbw mm0, [zero]

            movd mm3, [eax+esi*4+4]
            pmullw mm0, mm4         // mm0 used in divide for 3 cycles

            punpcklbw mm1, [zero]
            mov eax, edx

            pmullw mm1, mm6         // mm1 used in divide for 3 cycles
            punpcklbw mm2, [zero]

            pmullw mm2, mm4         // mm2 used in divide for 3 cycles
            punpcklbw mm3, [zero]

            pmullw mm3, mm6         // mm3 used in divide for 3 cycles
            shr eax, 14

            mov ebx, edx
            and eax, ~3

            paddw mm0, mm1
            shr ebx, 8

            psrlw mm0, 8
            add eax, src            

            paddw mm2, mm3
            and ebx, 0xff

            pmullw mm0, mem5
            psrlw mm2, 8
                        
            pmullw mm2, mem7
            add edx, ds_x


            movd mm6, ebx
            // 

            movq mm4, revn
            punpcklwd mm6,mm6


            movd mm1, [eax+4]
            //

            movd mm7, [eax+esi*4]
            paddw mm0, mm2

            movd mm5, [eax]
            psrlw mm0, 8

            movd mm3, [eax+esi*4+4]
            packuswb mm0, mm0

            movd [edi], mm0
            punpckldq mm6,mm6

            //poop

            punpcklbw mm5, [zero]
            psubw mm4, mm6

            punpcklbw mm1, [zero]
            pmullw mm5, mm4

            punpcklbw mm7, [zero]
            pmullw mm1, mm6

            punpcklbw mm3, [zero]
            pmullw mm7, mm4

            //cycle
            //cycle
            
            paddw mm5, mm1
            //

            pmullw mm3, mm6
            psrlw mm5, 8

            pmullw mm5, mem5
            add edi, 8

            // cycle
            // cycle

            paddw mm7, mm3
            //
            
            psrlw mm7, 8
            //

            pmullw mm7, mem7
            dec ecx

            // cycle
            // cycle
            // cycle

            paddw mm5, mm7

            psrlw mm5, 8

            packuswb mm5, mm5

            movd [edi-4], mm5

            
            jnz mmx_scale_loop1
            mov fbout, edi
            emms
          }
#else // _MSC_VER, GCC asm
          __asm__ __volatile__ (
            "mov       %%edi, %[fbout]\n\t"
            "mov       %%esi, %[w]\n\t"
            "movd      %%mm7, %[ypart]\n\t"
            "punpcklwd %%mm7, %%mm7\n\t"
            "movq      %%mm5, %[revn]\n\t"
            "punpckldq %%mm7, %%mm7\n\t"
            "psubw     %%mm5, %%mm7\n\t"
            "mov       %%ecx, %%esi\n\t"
            "movq      %[mem5], %%mm5\n\t"
            "movq      %[mem7], %%mm7\n\t"
            "shr       %%ecx, 1\n\t"
            "mov       %%edx, %[s_x]\n\t"
            ".align    16\n"
            "mmx_scale_loop1:\n\t"
            "mov       %%ebx, %%edx\n\t"
            "mov       %%eax, %%edx\n\t"
            "shr       %%eax, 14\n\t"
            "add       %%edx, %[ds_x]\n\t"
            "shr       %%ebx, 8\n\t"
            "and       %%eax, ~3\n\t"
            "add       %%eax, %[src]\n\t"
            "and       %%ebx, 0xff\n\t"
            "movd      %%mm6, %%ebx\n\t"
            "movq      %%mm4, %[revn]\n\t"
            "punpcklwd %%mm6, %%mm6\n\t"
            "movd      %%mm0, [%%eax]\n\t"
            "punpckldq %%mm6, %%mm6\n\t"
            "movd      %%mm1, [%%eax+4]\n\t"
            "psubw     %%mm4, %%mm6\n\t"
            "movd      %%mm2, [%%eax + %%esi * 4]\n\t"
            "punpcklbw %%mm0, [%[zero]]\n\t"
            "movd      %%mm3, [%%eax + %%esi * 4 + 4]\n\t"
            "pmullw    %%mm0, %%mm4\n\t"
            "punpcklbw %%mm1, [%[zero]]\n\t"
            "mov       %%eax, %%edx\n\t"
            "pmullw    %%mm1, %%mm6\n\t"
            "punpcklbw %%mm2, [%[zero]]\n\t"
            "pmullw    %%mm2, %%mm4\n\t"
            "punpcklbw %%mm3, [%[zero]]\n\t"
            "pmullw    %%mm3, %%mm6\n\t"
            "shr       %%eax, 14\n\t"
            "mov       %%ebx, %%edx\n\t"
            "and       %%eax, ~3\n\t"
            "paddw     %%mm0, %%mm1\n\t"
            "shr       %%ebx, 8\n\t"
            "psrlw     %%mm0, 8\n\t"
            "add       %%eax, %[src]\n\t"
            "paddw     %%mm2, %%mm3\n\t"
            "and       %%ebx, 0xff\n\t"
            "pmullw    %%mm0, %[mem5]\n\t"
            "psrlw     %%mm2, 8\n\t"
            "pmullw    %%mm2, %[mem7]\n\t"
            "add       %%edx, %[ds_x]\n\t"
            "movd      %%mm6, %%ebx\n\t"
            "movq      %%mm4, %[revn]\n\t"
            "punpcklwd %%mm6, %%mm6\n\t"
            "movd      %%mm1, [%%eax+4]\n\t"
            "movd      %%mm7, [%%eax + %%esi * 4]\n\t"
            "paddw     %%mm0, %%mm2\n\t"
            "movd      %%mm5, [%%eax]\n\t"
            "psrlw     %%mm0, 8\n\t"
            "movd      %%mm3, [%%eax + %%esi * 4 + 4]\n\t"
            "packuswb  %%mm0, %%mm0\n\t"
            "movd      [%%edi], %%mm0\n\t"
            "punpckldq %%mm6, %%mm6\n\t"
            "punpcklbw %%mm5, [%[zero]]\n\t"
            "psubw     %%mm4, %%mm6\n\t"
            "punpcklbw %%mm1, [%[zero]]\n\t"
            "pmullw    %%mm5, %%mm4\n\t"
            "punpcklbw %%mm7, [%[zero]]\n\t"
            "pmullw    %%mm1, %%mm6\n\t"
            "punpcklbw %%mm3, [%[zero]]\n\t"
            "pmullw    %%mm7, %%mm4\n\t"
            "paddw     %%mm5, %%mm1\n\t"
            "pmullw    %%mm3, %%mm6\n\t"
            "psrlw     %%mm5, 8\n\t"
            "pmullw    %%mm5, %[mem5]\n\t"
            "add       %%edi, 8\n\t"
            "paddw     %%mm7, %%mm3\n\t"
            "psrlw     %%mm7, 8\n\t"
            "pmullw    %%mm7, %[mem7]\n\t"
            "dec       %%ecx\n\t"
            "paddw     %%mm5, %%mm7\n\t"
            "psrlw     %%mm5, 8\n\t"
            "packuswb  %%mm5, %%mm5\n\t"
            "movd      [%%edi - 4], %%mm5\n\t"
            "jnz       mmx_scale_loop1\n\t"
            "mov       %[fbout], %%edi\n\t"
            "emms\n\t"
            : [fbout]"+m"(fbout)
            : [w]"m"(w), [ypart]"m"(ypart), [revn]"m"(revn), [s_x]"m"(s_x),
              [ds_x]"m"(ds_x), [src]"m"(src), [zero]"m"(zero), [mem5]"m"(mem5),
              [mem7]"m"(mem7)
            : "eax", "ebx", "ecx", "edx", "esi", "edi"
          );
#endif // _MSC_VER
      } // end mmx
#endif
      if (blend) // reblend this scanline with the original
      {
        fbout-=w;
        int x=w/4;
        unsigned int *s2=(unsigned int *)framebuffer+y*w;
        while (x--)
        {
          fbout[0]=BLEND_AVG(fbout[0],s2[0]);
          fbout[1]=BLEND_AVG(fbout[1],s2[1]);
          fbout[2]=BLEND_AVG(fbout[2],s2[2]);
          fbout[3]=BLEND_AVG(fbout[3],s2[3]);
          fbout+=4;
          s2+=4;
        }
      }
    } // end subpixel y loop
  }
  else // !subpixel
  {
    int y;
	  for (y = 0; y < h; y ++)
	  {
		  int s_x=isx;
		  unsigned int *src = ((unsigned int *) framebuffer)+(s_y>>16)*w;
      int ypart=(s_y>>8)&0xff;
		  s_y+=ds_x;
		  if (!blend) 
		  {
    	  int x=w/4;
			  while (x--)
			  { 
				  fbout[0]=src[s_x>>16];
				  s_x += ds_x;
				  fbout[1]=src[s_x>>16];
				  s_x += ds_x;
				  fbout[2]=src[s_x>>16];
				  s_x += ds_x;
				  fbout[3]=src[s_x>>16];
				  s_x += ds_x;
				  fbout+=4;
			  }
		  }
		  else
		  {
			  unsigned int *s2=(unsigned int *)framebuffer+(y*w);
    	  int x=w/4;
			  while (x--)
			  { 
				  fbout[0]=BLEND_AVG(s2[0],src[s_x>>16]);
				  s_x += ds_x;
				  fbout[1]=BLEND_AVG(s2[1],src[s_x>>16]);
				  s_x += ds_x;
				  fbout[2]=BLEND_AVG(s2[2],src[s_x>>16]);
				  s_x += ds_x;
				  fbout[3]=BLEND_AVG(s2[3],src[s_x>>16]);
				  s2+=4;
				  fbout+=4;
				  s_x += ds_x;
			  }
 		  }
	  }
  }
  return 1;
}

	
int C_THISCLASS::render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h)
{
  if (isBeat&0x80000000) return 0;
	int f_val;
	unsigned int *dest=(unsigned int *) fbout;

	if (isBeat && beatch) 
	{
		fpos=scale2;
	}
	
	if (scale < scale2) 
	{
		f_val=(fpos > scale? fpos:scale);
		fpos -= 3;
	}
	else 
	{
		f_val=(fpos < scale? fpos :scale);
		fpos+=3;
	}

	if (f_val<0) f_val=0;

  if (f_val < 32) return blitter_normal(framebuffer,fbout,w,h,f_val);
  if (f_val > 32) return blitter_out(framebuffer,fbout,w,h,f_val);
  return 0;
}

C_RBASE *R_BlitterFB(char *desc)
{
	if (desc) { strcpy(desc,MOD_NAME); return NULL; }
	return (C_RBASE *) new C_THISCLASS();
}

int win32_dlgproc_blitterfeedback(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
  C_THISCLASS* g_this = (C_THISCLASS*)g_current_render;

	int *a=NULL;
	switch (uMsg)
	{
		case WM_INITDIALOG:
			SendDlgItemMessage(hwndDlg,IDC_SLIDER1,TBM_SETRANGEMIN,0,0);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER1,TBM_SETRANGEMAX,0,256);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER1,TBM_SETPOS,1,g_this->scale);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER2,TBM_SETRANGEMIN,0,0);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER2,TBM_SETRANGEMAX,0,256);
			SendDlgItemMessage(hwndDlg,IDC_SLIDER2,TBM_SETPOS,1,g_this->scale2);
			if (g_this->blend==1) CheckDlgButton(hwndDlg,IDC_BLEND,BST_CHECKED);			
      if (g_this->subpixel) CheckDlgButton(hwndDlg,IDC_CHECK2,BST_CHECKED);
			if (g_this->beatch==1) CheckDlgButton(hwndDlg,IDC_CHECK1,BST_CHECKED);
			return 1;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_BLEND:
					if (IsDlgButtonChecked(hwndDlg,IDC_BLEND))
          {
						g_this->blend=1;
          }
					else g_this->blend=0;
				break;
				case IDC_CHECK1:
					if (IsDlgButtonChecked(hwndDlg,IDC_CHECK1))
						g_this->beatch=1;
					else g_this->beatch=0;
				break;
        case IDC_CHECK2:
					if (IsDlgButtonChecked(hwndDlg,IDC_CHECK2))
          {
						g_this->subpixel=1;
          }
					else g_this->subpixel=0;
        break;
			}
		return 0;

		case WM_HSCROLL:
			{
				HWND swnd = (HWND) lParam;
				int t = (int) SendMessage(swnd,TBM_GETPOS,0,0);
				if (swnd == GetDlgItem(hwndDlg,IDC_SLIDER1))
				{
					g_this->scale=t;
					g_this->fpos=t;
				}
				if (swnd == GetDlgItem(hwndDlg,IDC_SLIDER2))
				{
					g_this->scale2=t;
					g_this->fpos=t;
				}
			}
	}
	return 0;
}




#else
C_RBASE *R_BlitterFB(char *desc) { return NULL; }
#endif