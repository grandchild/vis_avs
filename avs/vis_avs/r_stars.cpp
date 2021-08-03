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
#include "c_stars.h"
#include <stdlib.h>
#include <math.h>
#include "r_defs.h"


C_THISCLASS::~C_THISCLASS() 
{
}

C_THISCLASS::C_THISCLASS() // set up default configuration
{
  nc=0;
  color = 0xFFFFFF;
  enabled=1;
  MaxStars = 350;
  MaxStars_set=350;
  Xoff = 0; 
  Yoff = 0;
  Zoff = 255;
  WarpSpeed = 6;
  CurrentSpeed = 6;
  blend = 0;
  blendavg = 0;
  Width = 0;
  Height = 0;
  onbeat = 0;
  spdBeat = 4;
  durFrames = 15;
  incBeat = 0;
}

#define GET_INT() (data[pos]|(data[pos+1]<<8)|(data[pos+2]<<16)|(data[pos+3]<<24))

float C_THISCLASS::GET_FLOAT(unsigned char *data, int pos)
{
int a = data[pos]|(data[pos+1]<<8)|(data[pos+2]<<16)|(data[pos+3]<<24);
float f = *(float *)&a;
return f;
}

void C_THISCLASS::load_config(unsigned char *data, int len) // read configuration of max length "len" from data.
{
	int pos=0;
	if (len-pos >= 4) { enabled=GET_INT(); pos+=4; }
	if (len-pos >= 4) { color=GET_INT(); pos+=4; }
	if (len-pos >= 4) { blend=GET_INT(); pos+=4; }
	if (len-pos >= 4) { blendavg=GET_INT(); pos+=4; }
	if (len-pos >= 4) { WarpSpeed=GET_FLOAT(data, pos); pos+=4; }
	CurrentSpeed = WarpSpeed;
	if (len-pos >= 4) { MaxStars_set=GET_INT(); pos+=4; }
	if (len-pos >= 4) { onbeat=GET_INT(); pos+=4; }
	if (len-pos >= 4) { spdBeat=GET_FLOAT(data, pos); pos+=4; }
	if (len-pos >= 4) { durFrames=GET_INT(); pos+=4; }
}

#define PUT_INT(y) data[pos]=(y)&255; data[pos+1]=(y>>8)&255; data[pos+2]=(y>>16)&255; data[pos+3]=(y>>24)&255
void C_THISCLASS::PUT_FLOAT(float f, unsigned char *data, int pos)
{
int y = *(int *)&f;
data[pos]=(y)&255; data[pos+1]=(y>>8)&255; data[pos+2]=(y>>16)&255; data[pos+3]=(y>>24)&255;
}

int  C_THISCLASS::save_config(unsigned char *data) // write configuration to data, return length. config data should not exceed 64k.
{
  int pos=0;
  PUT_INT(enabled); pos+=4;
  PUT_INT(color); pos+=4;
  PUT_INT(blend); pos+=4;
  PUT_INT(blendavg); pos+=4;
  PUT_FLOAT(WarpSpeed, data, pos); pos+=4;
  PUT_INT(MaxStars_set); pos+=4;
  PUT_INT(onbeat); pos+=4;
  PUT_FLOAT(spdBeat, data, pos); pos+=4;
  PUT_INT(durFrames); pos+=4;
  return pos;
}

void C_THISCLASS::InitializeStars(void)
{
  int i;
#ifdef LASER
  MaxStars = MaxStars_set/12;
  if (MaxStars < 10) MaxStars=10;
#else
  MaxStars = MulDiv(MaxStars_set,Width*Height,512*384);
#endif
  if (MaxStars > 4095) MaxStars=4095;
  for (i=0;i<MaxStars;i++)
	{
    Stars[i].X=(rand()%Width)-Xoff;
    Stars[i].Y=(rand()%Height)-Yoff;
    Stars[i].Z=(float)(rand()%255);
    Stars[i].Speed = (float)(rand()%9+1)/10;
	}
}

void C_THISCLASS::CreateStar(int A)
{
  Stars[A].X = (rand()%Width)-Xoff;
  Stars[A].Y = (rand()%Height)-Yoff;
  Stars[A].Z = (float)Zoff;
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
#ifdef LASER
  w=h=1024;
#endif
  int i=w*h;
  int NX,NY;
  int c;

  if (!enabled) return 0;

  if (onbeat && isBeat)
	{
	CurrentSpeed = spdBeat;
	incBeat = (WarpSpeed - CurrentSpeed) / (float)durFrames;
	nc = durFrames;
	}

  if (Width != w || Height != h)
	{
	  Width = w;
	  Height = h;
	  Xoff = Width/2;
	  Yoff = Height/2;
	  InitializeStars();
	}
  if (isBeat&0x80000000) return 0;

  for (i=0;i<MaxStars;i++)
	{
    if ((int)Stars[i].Z > 0)
		{
		NX = ((Stars[i].X << 7) / (int)Stars[i].Z) + Xoff;
        NY = ((Stars[i].Y << 7) / (int)Stars[i].Z) + Yoff;
		if ((NX > 0) && (NX < w) && (NY > 0) && (NY < h))
			{
			c = (int)((255-(int)Stars[i].Z)*Stars[i].Speed);
			if (color != 0xFFFFFF) c = BLEND_ADAPT((c|(c<<8)|(c<<16)), color, c>>4); else c = (c|(c<<8)|(c<<16));
#ifdef LASER
      LineType l;
      l.color=c;
      l.mode=1;
      l.x1=(float)NX/512.0f - 1.0f;
      l.y1=(float)NY/512.0f - 1.0f;
      g_laser_linelist->AddLine(&l);      
#else
			framebuffer[NY*w+NX] = blend ? BLEND(framebuffer[NY*w+NX], c) : blendavg ? BLEND_AVG(framebuffer[NY*w+NX], c) : c;
#endif
      Stars[i].OX = NX;
			Stars[i].OY = NY;
			Stars[i].Z-=Stars[i].Speed*CurrentSpeed;
			}
		else
			CreateStar(i);
		}
	else
		CreateStar(i);
	}

  if (!nc)
		CurrentSpeed = WarpSpeed;
  else
		{
		CurrentSpeed = max(0, CurrentSpeed + incBeat);
		nc--;
		}

  return 0;
}

C_RBASE *R_StarField(char *desc) // creates a new effect object if desc is NULL, otherwise fills in desc with description
{
	if (desc) { strcpy(desc,MOD_NAME); return NULL; }
	return (C_RBASE *) new C_THISCLASS();
}
