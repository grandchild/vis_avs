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
#include "c_mirror.h"
#include "r_defs.h"


#ifndef LASER

C_THISCLASS::C_THISCLASS() // set up default configuration
{
  framecount=0;
  enabled=1;
  onbeat=0;
  smooth=0;
  slower=4;
  mode=HORIZONTAL1;
}

#define GET_INT() (data[pos]|(data[pos+1]<<8)|(data[pos+2]<<16)|(data[pos+3]<<24))
void C_THISCLASS::load_config(unsigned char *data, int len) // read configuration of max length "len" from data.
{
	int pos=0;
	if (len-pos >= 4) { enabled=GET_INT(); pos+=4; }
	if (len-pos >= 4) { mode=GET_INT(); pos+=4; }
	if (len-pos >= 4) { onbeat=GET_INT(); pos+=4; }
	if (len-pos >= 4) { smooth=GET_INT(); pos+=4; }
	if (len-pos >= 4) { slower=GET_INT(); pos+=4; }
}

#define PUT_INT(y) data[pos]=(y)&255; data[pos+1]=(y>>8)&255; data[pos+2]=(y>>16)&255; data[pos+3]=(y>>24)&255
int  C_THISCLASS::save_config(unsigned char *data) // write configuration to data, return length. config data should not exceed 64k.
{
	int pos=0;
	PUT_INT(enabled); pos+=4;
	PUT_INT(mode); pos+=4;
	PUT_INT(onbeat); pos+=4;
	PUT_INT(smooth); pos+=4;
	PUT_INT(slower); pos+=4;
	return pos;
}

static unsigned int __inline BLEND_ADAPT(unsigned int a, unsigned int b, /*float*/int divisor)
{
return ((((a >> 4) & 0x0F0F0F) * (16-divisor) + (((b >> 4) & 0x0F0F0F) * divisor)));
}


// render function
// render should return 0 if it only used framebuffer, or 1 if the new output data is in fbout. this is
// used when you want to do something that you'd otherwise need to make a copy of the framebuffer.
// w and h are the width and height of the screen, in pixels.
// isBeat is 1 if a beat has been detected.
// visdata is in the format of [spectrum:0,wave:1][channel][band].

int C_THISCLASS::render(char[2][2][576], int isBeat, int *framebuffer, int*, int w, int h)
{
  int hi,t,j,halfw,halfh,m,d;
  int *thismode=&mode;
  static int lastMode;
  static int divisors=0;
  static int inc=0;
  int divis;
  int *fbp;

  if (isBeat&0x80000000) return 0;
  if (!enabled) return 0;
    
  t=w*h;
  halfw = w / 2 ;
  halfh = h / 2 ;

  if (onbeat)
	{
	  if (isBeat)	rbeat = (rand()%16) & mode;
	  thismode=&rbeat;
	}

  if (*thismode != lastMode)
	{
	  int dif = *thismode ^ lastMode;
    int i;
	  for (i=1,m=0xFF,d=0;i<16;i<<=1,m<<=8,d+=8)
		if (dif & i) 
		{
			inc = (inc & ~m) | ((lastMode & i) ? 0xFF : 1) << d;
			if (!(divisors & m)) divisors = (divisors & ~m) | ((lastMode & i) ? 16 : 1) << d;
		}
	  lastMode = *thismode;
	}

  fbp=framebuffer;
  if (*thismode & VERTICAL1 || (smooth && (divisors & 0x00FF0000)))
  {
    divis = (divisors & M_VERTICAL1) >> D_VERTICAL1;
	  for ( hi=0 ; hi < h ; hi++) 
	  {
		  if (smooth && divis)
      {
        int *tmp=fbp+w-1;
        int n=halfw;
        while (n--) *tmp = BLEND_ADAPT(*tmp, *fbp++, divis);
        --tmp;
      }
		  else
      {
        int *tmp=fbp+w-1;
        int n=halfw;
        while (n--) *tmp-- = *fbp++;
      }
		  fbp+=halfw;
	  }
  }
  fbp=framebuffer;
  if (*thismode & VERTICAL2 || (smooth && (divisors & 0xFF000000)))
  {
	  divis = (divisors & M_VERTICAL2) >> D_VERTICAL2;
	  for ( hi=0 ; hi < h ; hi++)
	  {
		  if (smooth && divis)
      {
        int *tmp=fbp+w-1;
        int n=halfw;
        while (n--) *fbp = BLEND_ADAPT(*fbp,*tmp--,divis);
        ++fbp;
      }
		  else
      {
        int *tmp=fbp+w-1;
        int n=halfw;
        while (n--) *fbp++ = *tmp--;
      }
		  fbp+=halfw;
	  }
  }
  fbp=framebuffer;
  j=t-w;
  if (*thismode & HORIZONTAL1 || (smooth && (divisors & 0x000000FF)))
  {
	  divis = (divisors & M_HORIZONTAL1) >> D_HORIZONTAL1;
	  for ( hi=0 ; hi < halfh ; hi++) 
	  {
		  if (smooth && divis) 
      {
        int n=w;
        while (n--) fbp[j]=BLEND_ADAPT(fbp[j], *fbp, divis);
        ++fbp;
      }
		  else 
      {
        memcpy(fbp+j,fbp,w*sizeof(int));
        fbp+=w;
      }
		  j-=2*w;
	  }
  }
  fbp=framebuffer;
  j=t-w;
  if (*thismode & HORIZONTAL2 || (smooth && (divisors & 0x0000FF00)))
  {
	  divis = (divisors & M_HORIZONTAL2) >> D_HORIZONTAL2;
	  for ( hi=0 ; hi < halfh ; hi++) 
	  {
		  if (smooth && divis) 
      {
        int n=w;
        while (n--)
          *fbp = BLEND_ADAPT(*fbp, fbp[j], divis);
          ++fbp;
      }
		  else 
      {
        memcpy(fbp,fbp+j,w*sizeof(int));
        fbp+=w;
      }
		  j-=2*w;
	  }
  }

  if (smooth && !(++framecount % slower))
	{
    int i;
	  for (i=1,m=0xFF,d=0;i<16;i<<=1,m<<=8,d+=8)
		  {
		  if (divisors & m)
			  divisors = (divisors & ~m) | ((((divisors & m) >> d) + (unsigned char)((inc & m) >> d)) % 16) << d;
		  }
	}
  return 0;
}


C_RBASE *R_Mirror(char *desc) // creates a new effect object if desc is NULL, otherwise fills in desc with description
{
	if (desc) { strcpy(desc,MOD_NAME); return NULL; }
	return (C_RBASE *) new C_THISCLASS();
}

#else
C_RBASE *R_Mirror(char *desc) // creates a new effect object if desc is NULL, otherwise fills in desc with description
{ return NULL; }
#endif
