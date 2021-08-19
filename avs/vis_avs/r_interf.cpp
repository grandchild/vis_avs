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
#include "c_interf.h"
#include <stdlib.h>
#include <math.h>
#include "r_defs.h"


#ifndef LASER

C_THISCLASS::~C_THISCLASS() 
{
}

// configuration read/write

C_THISCLASS::C_THISCLASS() // set up default configuration
{
enabled=1;
nPoints = 2;
distance=10;
alpha=128;
rotation=0;
rotationinc=0;
rgb=1;
blendavg=0;
blend=0;
onbeat=1;
distance2=32;
rotationinc2=25;
alpha2=192;
status=2;
speed = (float)0.2;
a=(float)rotation/255*(float)M_PI*2;
}

void C_THISCLASS::PUT_FLOAT(float f, unsigned char *data, int pos)
{
unsigned char* y = (unsigned char*)&f;
data[pos]=y[0]; data[pos+1]=y[1]; data[pos+2]=y[2]; data[pos+3]=y[3];
}

float C_THISCLASS::GET_FLOAT(unsigned char *data, int pos)
{
  float f;
  unsigned char* a = (unsigned char*)&f;
  a[0] = data[pos];
  a[1] = data[pos + 1];
  a[2] = data[pos + 2];
  a[3] = data[pos + 3];
  return f;
}

#define GET_INT() (data[pos]|(data[pos+1]<<8)|(data[pos+2]<<16)|(data[pos+3]<<24))
void C_THISCLASS::load_config(unsigned char *data, int len) // read configuration of max length "len" from data.
{
	int pos=0;
	if (len-pos >= 4) { enabled=GET_INT(); pos+=4; }
	if (len-pos >= 4) { nPoints=GET_INT(); pos+=4; }
	if (len-pos >= 4) { rotation=GET_INT(); pos+=4; }
	if (len-pos >= 4) { distance=GET_INT(); pos+=4; }
	if (len-pos >= 4) { alpha=GET_INT(); pos+=4; }
	if (len-pos >= 4) { rotationinc=GET_INT(); pos+=4; }
	if (len-pos >= 4) { blend=GET_INT(); pos+=4; }
	if (len-pos >= 4) { blendavg=GET_INT(); pos+=4; }
	if (len-pos >= 4) { distance2=GET_INT(); pos+=4; }
	if (len-pos >= 4) { alpha2=GET_INT(); pos+=4; }
	if (len-pos >= 4) { rotationinc2=GET_INT(); pos+=4; }
	if (len-pos >= 4) { rgb=GET_INT(); pos+=4; }
	if (len-pos >= 4) { onbeat=GET_INT(); pos+=4; }
	if (len-pos >= 4) { speed=GET_FLOAT(data, pos); pos+=4; }
	a=(float)rotation/255*(float)M_PI*2;
	status=(float)M_PI;
}

#define PUT_INT(y) data[pos]=(y)&255; data[pos+1]=(y>>8)&255; data[pos+2]=(y>>16)&255; data[pos+3]=(y>>24)&255
int  C_THISCLASS::save_config(unsigned char *data) // write configuration to data, return length. config data should not exceed 64k.
{
  int pos=0;
  
	PUT_INT(enabled); pos+=4;
  PUT_INT(nPoints); pos+=4;
  PUT_INT(rotation); pos+=4;
  PUT_INT(distance); pos+=4;
  PUT_INT(alpha); pos+=4;
  PUT_INT(rotationinc); pos+=4;
  PUT_INT(blend); pos+=4;
  PUT_INT(blendavg); pos+=4;
  PUT_INT(distance2); pos+=4;
  PUT_INT(alpha2); pos+=4;
  PUT_INT(rotationinc2); pos+=4;
  PUT_INT(rgb); pos+=4;
  PUT_INT(onbeat); pos+=4;
  PUT_FLOAT(speed, data, pos); pos+=4;

	return pos;
}

// render function
// render should return 0 if it only used framebuffer, or 1 if the new output data is in fbout. this is
// used when you want to do something that you'd otherwise need to make a copy of the framebuffer.
// w and h are the width and height of the screen, in pixels.
// isBeat is 1 if a beat has been detected.
// visdata is in the format of [spectrum:0,wave:1][channel][band].

#define MAX_POINTS 8

int C_THISCLASS::render(char[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h)
{
  int pnts=nPoints;
	int x,y;
	int i;
	float s;

  if (isBeat&0x80000000) return 0;
  if (!enabled) return 0;
	if (pnts == 0) return 0;

	float angle=(float)(2*M_PI)/pnts;
	
	if (onbeat && isBeat)
		if (status >= M_PI)
			status=0;

	s = (float)sin(status);
	_rotationinc = rotationinc + (int)((float)(rotationinc2-rotationinc) * s);
	_alpha = alpha + (int)((float)(alpha2-alpha) * s);
	_distance = distance + (int)((float)(distance2-distance) * s);

	a=(float)rotation/255*(float)M_PI*2;

  int xpoints[MAX_POINTS],ypoints[MAX_POINTS];

  int minx=0, maxx=0;
  int miny=0, maxy=0;

	for (i=0;i<pnts;i++)
		{
		xpoints[i] = (int)(cos(a)*_distance);
		ypoints[i] = (int)(sin(a)*_distance);
    if (ypoints[i] > miny) miny=ypoints[i];
    if (-ypoints[i] > maxy) maxy=-ypoints[i];
    if (xpoints[i] > minx) minx=xpoints[i];
    if (-xpoints[i] > maxx) maxx=-xpoints[i];
		a += angle;
		}

  unsigned char *bt=g_blendtable[_alpha];

  int *outp=fbout;
  for (y = 0; y < h; y ++)
  {
    int yoffs[MAX_POINTS];
    for (i = 0; i < pnts; i ++)
    {
      if (y >= ypoints[i] && y-ypoints[i] < h)
        yoffs[i]=(y-ypoints[i])*w;
      else yoffs[i]=-1;
    }
    if (rgb && (pnts==3 || pnts==6))
    {
      if (pnts == 3) for (x = 0; x < w; x ++)
      {
        int r=0,g=0,b=0;
        int xp;
        xp=x-xpoints[0];
        if (xp >= 0 && xp < w && yoffs[0]!=-1)
        {
          int pix=framebuffer[xp+yoffs[0]];
          r=bt[pix&0xff];
        }
        xp=x-xpoints[1];
        if (xp >= 0 && xp < w && yoffs[1]!=-1)
        {
          int pix=framebuffer[xp+yoffs[1]];
          g=bt[(pix>>8)&0xff];
        }
        xp=x-xpoints[2];
        if (xp >= 0 && xp < w && yoffs[2]!=-1)
        {
          int pix=framebuffer[xp+yoffs[2]];
          b=bt[(pix>>16)&0xff];
        }
        *outp++ = r|(g<<8)|(b<<16);
      }    
      else for (x = 0; x < w; x ++)
      {
        int r=0,g=0,b=0;
        int xp;
        xp=x-xpoints[0];
        if (xp >= 0 && xp < w && yoffs[0]!=-1)
        {
          int pix=framebuffer[xp+yoffs[0]];
          r=bt[pix&0xff];
        }
        xp=x-xpoints[1];
        if (xp >= 0 && xp < w && yoffs[1]!=-1)
        {
          int pix=framebuffer[xp+yoffs[1]];
          g=bt[(pix>>8)&0xff];
        }
        xp=x-xpoints[2];
        if (xp >= 0 && xp < w && yoffs[2]!=-1)
        {
          int pix=framebuffer[xp+yoffs[2]];
          b=bt[(pix>>16)&0xff];
        }
        xp=x-xpoints[3];
        if (xp >= 0 && xp < w && yoffs[3]!=-1)
        {
          int pix=framebuffer[xp+yoffs[3]];
          r+=bt[pix&0xff];
        }
        xp=x-xpoints[4];
        if (xp >= 0 && xp < w && yoffs[4]!=-1)
        {
          int pix=framebuffer[xp+yoffs[4]];
          g+=bt[(pix>>8)&0xff];
        }
        xp=x-xpoints[5];
        if (xp >= 0 && xp < w && yoffs[5]!=-1)
        {
          int pix=framebuffer[xp+yoffs[5]];
          b+=bt[(pix>>16)&0xff];
        }
        if (r > 255) r=255;
        if (g > 255) g=255;
        if (b > 255) b=255;
        *outp++ = r|(g<<8)|(b<<16);
      }    
    }
    else if (y > miny && y < h-maxy && minx+maxx < w) // no y clipping required
    {
      for (x = 0; x < minx; x ++)
      {
        int r=0,g=0,b=0;
        for (i = 0; i < pnts; i++)
        {
          int xp=x-xpoints[i];
          if (xp >= 0 && xp < w)
          {
            int pix=framebuffer[xp+yoffs[i]];
            r+=bt[pix&0xff];
            g+=bt[(pix>>8)&0xff];
            b+=bt[(pix>>16)&0xff];
          }
        }
        if (r > 255) r=255;
        if (g > 255) g=255;
        if (b > 255) b=255;
        *outp++ = r|(g<<8)|(b<<16);
      }
      int *lfb=framebuffer+x;
      for (; x < w-maxx; x ++)
      {
        int r=0,g=0,b=0;
        for (i = 0; i < pnts; i++)
        {
          int pix=lfb[yoffs[i]-xpoints[i]];
          r+=bt[pix&0xff];
          g+=bt[(pix>>8)&0xff];
          b+=bt[(pix>>16)&0xff];
        }
        if (r > 255) r=255;
        if (g > 255) g=255;
        if (b > 255) b=255;
        lfb++;
        *outp++ = r|(g<<8)|(b<<16);
      }
      for (; x < w; x ++)
      {
        int r=0,g=0,b=0;
        for (i = 0; i < pnts; i++)
        {
          int xp=x-xpoints[i];
          if (xp >= 0 && xp < w)
          {
            int pix=framebuffer[xp+yoffs[i]];
            r+=bt[pix&0xff];
            g+=bt[(pix>>8)&0xff];
            b+=bt[(pix>>16)&0xff];
          }
        }
        if (r > 255) r=255;
        if (g > 255) g=255;
        if (b > 255) b=255;
        *outp++ = r|(g<<8)|(b<<16);
      }
    }
    else for (x = 0; x < w; x ++)
    {
      int r=0,g=0,b=0;
      for (i = 0; i < pnts; i++)
      {
        int xp=x-xpoints[i];
        if (xp >= 0 && xp < w && yoffs[i]!=-1)
        {
          int pix=framebuffer[xp+yoffs[i]];
          r+=bt[pix&0xff];
          g+=bt[(pix>>8)&0xff];
          b+=bt[(pix>>16)&0xff];
        }
      }
      if (r > 255) r=255;
      if (g > 255) g=255;
      if (b > 255) b=255;
      *outp++ = r|(g<<8)|(b<<16);
    }
  }


  


	rotation+=_rotationinc;
	rotation=rotation>255 ? rotation-255 : rotation;
	rotation=rotation<-255 ? rotation+255 : rotation;

	status += speed;
	status=min(status, (float)M_PI);
	if (status<-M_PI) status = (float) M_PI;

	int *p = framebuffer;
	int *d = fbout;

  if (!blend && !blendavg) return 1;
  if (blendavg)
  {
  	int i=w*h/4;
	  while (i--)
	  {
		  p[0] = BLEND_AVG(p[0], d[0]);
		  p[1] = BLEND_AVG(p[1], d[1]);
		  p[2] = BLEND_AVG(p[2], d[2]);
		  p[3] = BLEND_AVG(p[3], d[3]);
		  p+=4;
		  d+=4;
	  }
  }
  else
    mmx_addblend_block(p,d,w*h);

  return 0;
}


C_RBASE *R_Interferences(char *desc) // creates a new effect object if desc is NULL, otherwise fills in desc with description
{
	if (desc) { strcpy(desc,MOD_NAME); return NULL; }
	return (C_RBASE *) new C_THISCLASS();
}

#else
C_RBASE *R_Interferences(char *desc) // creates a new effect object if desc is NULL, otherwise fills in desc with description
{
return NULL;
}
#endif
