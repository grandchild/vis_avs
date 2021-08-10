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
#include "c_bright.h"
#include <stdlib.h>
#include <math.h>
#include "r_defs.h"


#ifndef LASER

C_THISCLASS::~C_THISCLASS() 
{
}

C_THISCLASS::C_THISCLASS() // set up default configuration
{
  tabs_needinit=1;
  redp=0;
  bluep=0;
  greenp=0;
  blend=0;
  blendavg=1;
  enabled=1;
  color=0;
  exclude=0;
  distance = 16;
  dissoc=0;
}

#define GET_INT() (data[pos]|(data[pos+1]<<8)|(data[pos+2]<<16)|(data[pos+3]<<24))
void C_THISCLASS::load_config(unsigned char *data, int len) // read configuration of max length "len" from data.
{
	int pos=0;
	if (len-pos >= 4) { enabled=GET_INT(); pos+=4; }
	if (len-pos >= 4) { blend=GET_INT(); pos+=4; }
	if (len-pos >= 4) { blendavg=GET_INT(); pos+=4; }
	if (len-pos >= 4) { redp=GET_INT(); pos+=4; }
	if (len-pos >= 4) { greenp=GET_INT(); pos+=4; }
	if (len-pos >= 4) { bluep=GET_INT(); pos+=4; }
	if (len-pos >= 4) { dissoc=GET_INT(); pos+=4; }
	if (len-pos >= 4) { color=GET_INT(); pos+=4; }
	if (len-pos >= 4) { exclude=GET_INT(); pos+=4; }
	if (len-pos >= 4) { distance=GET_INT(); pos+=4; }
  tabs_needinit=1;
}

#define PUT_INT(y) data[pos]=(y)&255; data[pos+1]=(y>>8)&255; data[pos+2]=(y>>16)&255; data[pos+3]=(y>>24)&255
int  C_THISCLASS::save_config(unsigned char *data) // write configuration to data, return length. config data should not exceed 64k.
{
  int pos=0;
  PUT_INT(enabled); pos+=4;
  PUT_INT(blend); pos+=4;
  PUT_INT(blendavg); pos+=4;
  PUT_INT(redp); pos+=4;
  PUT_INT(greenp); pos+=4;
  PUT_INT(bluep); pos+=4;
  PUT_INT(dissoc); pos+=4;
  PUT_INT(color); pos+=4;
  PUT_INT(exclude); pos+=4;
  PUT_INT(distance); pos+=4;
  return pos;
}

static __inline int iabs(int v)
{
return (v<0) ? -v : v;
}

static __inline int inRange(int color, int ref, int distance)
{
if (iabs((color & 0xFF) - (ref & 0xFF)) > distance) return 0;
if (iabs(((color) & 0xFF00) - ((ref) & 0xFF00)) > (distance<<8)) return 0;
if (iabs(((color) & 0xFF0000) - ((ref) & 0xFF0000)) > (distance<<16)) return 0;
return 1;
}

int C_THISCLASS::render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h)
{
  smp_begin(1,visdata,isBeat,framebuffer,fbout,w,h);
  if (isBeat & 0x80000000) return 0;

  smp_render(0,1,visdata,isBeat,framebuffer,fbout,w,h);
  return smp_finish(visdata,isBeat,framebuffer,fbout,w,h);
}

int C_THISCLASS::smp_begin(int max_threads, char[2][2][576], int isBeat, int*, int*, int, int)
{
	int rm=(int)((1+(redp < 0 ? 1 : 16)*((float)redp/4096))*65536.0);
	int gm=(int)((1+(greenp < 0 ? 1 : 16)*((float)greenp/4096))*65536.0);
	int bm=(int)((1+(bluep < 0 ? 1 : 16)*((float)bluep/4096))*65536.0);

  if (!enabled) return 0;
  if (tabs_needinit)
  {
    int n;
    for (n = 0; n < 256; n ++)
    {
        red_tab[n] = (n*rm)&0xffff0000;
        if (red_tab[n]>0xff0000) red_tab[n]=0xff0000;
        if (red_tab[n]<0) red_tab[n]=0;
        green_tab[n] = ((n*gm)>>8)&0xffff00;
        if (green_tab[n]>0xff00) green_tab[n]=0xff00;
        if (green_tab[n]<0) green_tab[n]=0;
        blue_tab[n] = ((n*bm)>>16)&0xffff;
        if (blue_tab[n]>0xff) blue_tab[n]=0xff;
        if (blue_tab[n]<0) blue_tab[n]=0;
    }
    tabs_needinit=0;
  }
  if (isBeat&0x80000000) return 0;

  return max_threads;
}

int C_THISCLASS::smp_finish(char[2][2][576], int, int*, int*, int, int)
{
  return 0;
}


// render function
// render should return 0 if it only used framebuffer, or 1 if the new output data is in fbout. this is
// used when you want to do something that you'd otherwise need to make a copy of the framebuffer.
// w and h are the width and height of the screen, in pixels.
// isBeat is 1 if a beat has been detected.
// visdata is in the format of [spectrum:0,wave:1][channel][band].
void C_THISCLASS::smp_render(int this_thread, int max_threads, char[2][2][576], int isBeat, int *framebuffer, int*, int w, int h)
{
  if (!enabled || (isBeat&0x80000000)) return;

  if (max_threads < 1) max_threads=1;

  int start_l = ( this_thread * h ) / max_threads;
  int end_l;

  if (this_thread >= max_threads - 1) end_l = h;
  else end_l = ( (this_thread+1) * h ) / max_threads;  

  int outh=end_l-start_l;
  if (outh<1) return;

  int l=w*outh;

  int *p = framebuffer + start_l * w;
  if (blend)
  {
    if (!exclude)
    {
      while (l--)
	    {
        int pix=*p;
			  *p++ = BLEND(pix, red_tab[(pix>>16)&0xff] | green_tab[(pix>>8)&0xff] | blue_tab[pix&0xff]);
	    }
    }
    else
    {
      while (l--)
	    {
        int pix=*p;
        if (!inRange(pix,color,distance))
		    {
			    *p = BLEND(pix, red_tab[(pix>>16)&0xff] | green_tab[(pix>>8)&0xff] | blue_tab[pix&0xff]);
		    }
		    p++;
	    }
    }
  }
  else if (blendavg)
  {
    if (!exclude)
    {
      while (l--)
	    {
        int pix=*p;
			  *p++ = BLEND_AVG(pix, red_tab[(pix>>16)&0xff] | green_tab[(pix>>8)&0xff] | blue_tab[pix&0xff]);
	    }
    }
    else
    {
      while (l--)
	    {
        int pix=*p;
        if (!inRange(pix,color,distance))
		    {
			    *p = BLEND_AVG(pix, red_tab[(pix>>16)&0xff] | green_tab[(pix>>8)&0xff] | blue_tab[pix&0xff]);
		    }
		    p++;
	    }
    }
  }
  else
  {
    if (!exclude)
    {
      while (l--)
	    {
        int pix=*p;
        *p++ = red_tab[(pix>>16)&0xff] | green_tab[(pix>>8)&0xff] | blue_tab[pix&0xff];
	    }
    }
    else
    {
      while (l--)
	    {
        int pix=*p;
        if (!inRange(pix,color,distance))
		    {
          *p = red_tab[(pix>>16)&0xff] | green_tab[(pix>>8)&0xff] | blue_tab[pix&0xff];
		    }
		    p++;
	    }
    }
  }
}

C_RBASE *R_Brightness(char *desc) // creates a new effect object if desc is NULL, otherwise fills in desc with description
{
	if (desc) { strcpy(desc,MOD_NAME); return NULL; }
	return (C_RBASE *) new C_THISCLASS();
}

#else
C_RBASE *R_Brightness(char *desc) { return NULL; }
#endif
