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
#include "c_grain.h"
#include <stdlib.h>
#include "r_defs.h"


#ifndef LASER

C_THISCLASS::~C_THISCLASS() // set up default configuration
{
if (depthBuffer)
	free(depthBuffer);
}

// configuration read/write

C_THISCLASS::C_THISCLASS() // set up default configuration
{
  blend=0;
  blendavg=0;
  smax=100;
  enabled=1;
  oldx=0;
  oldy=0;
  staticgrain=0;
  depthBuffer=NULL;
  int x;
  for (x = 0 ;x < sizeof(randtab); x ++)
    randtab[x]=rand()&255;
  randtab_pos=rand()%sizeof(randtab);
}

unsigned char __inline C_THISCLASS::fastrandbyte(void)
{
  unsigned char r=randtab[randtab_pos];
  randtab_pos++;
  if (!(randtab_pos&15))
  {
    randtab_pos+=rand()%73;
  }
  if (randtab_pos >= 491) randtab_pos-=491;
  return r;
}

#define GET_INT() (data[pos]|(data[pos+1]<<8)|(data[pos+2]<<16)|(data[pos+3]<<24))
void C_THISCLASS::load_config(unsigned char *data, int len) // read configuration of max length "len" from data.
{
	int pos=0;
	if (len-pos >= 4) { enabled=GET_INT(); pos+=4; }
	if (len-pos >= 4) { blend=GET_INT(); pos+=4; }
	if (len-pos >= 4) { blendavg=GET_INT(); pos+=4; }
	if (len-pos >= 4) { smax=GET_INT(); pos+=4; }
	if (len-pos >= 4) { staticgrain=GET_INT(); pos+=4; }
}

#define PUT_INT(y) data[pos]=(y)&255; data[pos+1]=(y>>8)&255; data[pos+2]=(y>>16)&255; data[pos+3]=(y>>24)&255
int  C_THISCLASS::save_config(unsigned char *data) // write configuration to data, return length. config data should not exceed 64k.
{
  int pos=0;
  PUT_INT(enabled); pos+=4;
  PUT_INT(blend); pos+=4;
  PUT_INT(blendavg); pos+=4;
  PUT_INT(smax); pos+=4;
  PUT_INT(staticgrain); pos+=4;
  return pos;
}

void C_THISCLASS::reinit(int w, int h)
{
int x,y;
unsigned char *p;
if (depthBuffer)
	free(depthBuffer);
depthBuffer = (unsigned char *)malloc(w*h*2);
p = depthBuffer;
if (p)
	for (y=0;y<h;y++)
		for (x=0;x<w;x++)
			{
			*p++ = (rand()%255);
			*p++ = (rand()%100);
			}

}

// render function
// render should return 0 if it only used framebuffer, or 1 if the new output data is in fbout. this is
// used when you want to do something that you'd otherwise need to make a copy of the framebuffer.
// w and h are the width and height of the screen, in pixels.
// isBeat is 1 if a beat has been detected.
// visdata is in the format of [spectrum:0,wave:1][channel][band].

int C_THISCLASS::render(char[2][2][576], int isBeat, int *framebuffer, int*, int w, int h)
{
  if (isBeat&0x80000000) return 0;
  if (!enabled) return 0;

  int smax_sc = (smax*255)/100;
  int *p;
  unsigned char *q;
  int l=w*h;

  if (w != oldx || h != oldy)
	{
	  reinit(w, h);
	  oldx = w;
	  oldy = h;
	}
  randtab_pos+=rand()%300;
  if (randtab_pos >= 491) randtab_pos-=491;

  p = framebuffer;
  q = depthBuffer;
  if (staticgrain)
  {
    if (blend)
    {
      while (l--)
	    {
		    if (*p)
		    {
			    int c=0;
			    if (q[1] < smax_sc) 
			    {
    		    int s = q[0];
            int r=(((p[0]&0xff0000)*s)>>8);
            if (r > 0xff0000) r=0xff0000;
            c|=r&0xff0000;
            r=(((p[0]&0xff00)*s)>>8);
            if (r > 0xff00) r=0xff00;
            c|=r&0xff00;
            r=(((p[0]&0xff)*s)>>8);
            if (r > 0xff) r=0xff;
            c|=r;
			    }
			    *p = BLEND(*p, c);
		    }
		    p++;
		    q+=2;
	    }
    }
    else if (blendavg)
    {
      while (l--)
	    {
		    if (*p)
		    {
			    int c=0;
			    if (q[1] < smax_sc) 
			    {
    		    int s = q[0];
            int r=(((p[0]&0xff0000)*s)>>8);
            if (r > 0xff0000) r=0xff0000;
            c|=r&0xff0000;
            r=(((p[0]&0xff00)*s)>>8);
            if (r > 0xff00) r=0xff00;
            c|=r&0xff00;
            r=(((p[0]&0xff)*s)>>8);
            if (r > 0xff) r=0xff;
            c|=r;
			    }
			    *p = BLEND_AVG(*p, c);
		    }
		    p++;
		    q+=2;
	    }
    }
    else
    {
      while (l--)
	    {
		    if (*p)
		    {
			    int c=0;
			    if (q[1] < smax_sc) 
			    {
    		    int s = q[0];
            int r=(((p[0]&0xff0000)*s)>>8);
            if (r > 0xff0000) r=0xff0000;
            c|=r&0xff0000;
            r=(((p[0]&0xff00)*s)>>8);
            if (r > 0xff00) r=0xff00;
            c|=r&0xff00;
            r=(((p[0]&0xff)*s)>>8);
            if (r > 0xff) r=0xff;
            c|=r;
			    }
			    *p = c;
		    }
		    p++;
		    q+=2;
	    }
    }
  }
  else
  {
    if (blend)
    {
      while (l--)
	    {
		    if (*p)
		    {
			    int c=0;
			    if (fastrandbyte() < smax_sc) 
			    {
            int s = fastrandbyte();
            int r=(((p[0]&0xff0000)*s)>>8);
            if (r > 0xff0000) r=0xff0000;
            c|=r&0xff0000;
            r=(((p[0]&0xff00)*s)>>8);
            if (r > 0xff00) r=0xff00;
            c|=r&0xff00;
            r=(((p[0]&0xff)*s)>>8);
            if (r > 0xff) r=0xff;
            c|=r;
			    }
			    *p = BLEND(*p, c);
		    }
		    p++;
		    q+=2;
	    }
    }
    else if (blendavg)
    {
      while (l--)
	    {
		    if (*p)
		    {
			    int c=0;
			    if (fastrandbyte() < smax_sc) 
			    {
            int s = fastrandbyte();
            int r=(((p[0]&0xff0000)*s)>>8);
            if (r > 0xff0000) r=0xff0000;
            c|=r&0xff0000;
            r=(((p[0]&0xff00)*s)>>8);
            if (r > 0xff00) r=0xff00;
            c|=r&0xff00;
            r=(((p[0]&0xff)*s)>>8);
            if (r > 0xff) r=0xff;
            c|=r;
			    }
			    *p = BLEND_AVG(*p, c);
		    }
		    p++;
		    q+=2;
	    }
    }
    else
    {
      while (l--)
	    {
		    if (*p)
		    {
			    int c=0;
			    if (fastrandbyte() < smax_sc) 
			    {
            int s = fastrandbyte();
            int r=(((p[0]&0xff0000)*s)>>8);
            if (r > 0xff0000) r=0xff0000;
            c|=r&0xff0000;
            r=(((p[0]&0xff00)*s)>>8);
            if (r > 0xff00) r=0xff00;
            c|=r&0xff00;
            r=(((p[0]&0xff)*s)>>8);
            if (r > 0xff) r=0xff;
            c|=r;
			    }
			    *p = c;
		    }
		    p++;
		    q+=2;
	    }
    }
  }
  return 0;
}

C_RBASE *R_Grain(char *desc) // creates a new effect object if desc is NULL, otherwise fills in desc with description
{
	if (desc) { strcpy(desc,MOD_NAME); return NULL; }
	return (C_RBASE *) new C_THISCLASS();
}

#else
C_RBASE *R_Grain(char *desc) // creates a new effect object if desc is NULL, otherwise fills in desc with description
{
  return NULL;
}
#endif
