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
#include "c_mosaic.h"
#include <stdlib.h>
#include "r_defs.h"


#ifndef LASER

C_THISCLASS::~C_THISCLASS() 
{
}


C_THISCLASS::C_THISCLASS() // set up default configuration
{
  enabled=1;
  quality = 50;
  blend = 0;
  blendavg = 0;
  onbeat = 0;
  durFrames = 15;
  nF=0;
}

#define GET_INT() (data[pos]|(data[pos+1]<<8)|(data[pos+2]<<16)|(data[pos+3]<<24))

void C_THISCLASS::load_config(unsigned char *data, int len) // read configuration of max length "len" from data.
{
	int pos=0;
	if (len-pos >= 4) { enabled=GET_INT(); pos+=4; }
	if (len-pos >= 4) { quality=GET_INT(); pos+=4; }
	if (len-pos >= 4) { quality2=GET_INT(); pos+=4; }
	if (len-pos >= 4) { blend=GET_INT(); pos+=4; }
	if (len-pos >= 4) { blendavg=GET_INT(); pos+=4; }
	if (len-pos >= 4) { onbeat=GET_INT(); pos+=4; }
	if (len-pos >= 4) { durFrames=GET_INT(); pos+=4; }
}

#define PUT_INT(y) data[pos]=(y)&255; data[pos+1]=(y>>8)&255; data[pos+2]=(y>>16)&255; data[pos+3]=(y>>24)&255

int  C_THISCLASS::save_config(unsigned char *data) // write configuration to data, return length. config data should not exceed 64k.
{
  int pos=0;
  PUT_INT(enabled); pos+=4;
  PUT_INT(quality); pos+=4;
  PUT_INT(quality2); pos+=4;
  PUT_INT(blend); pos+=4;
  PUT_INT(blendavg); pos+=4;
  PUT_INT(onbeat); pos+=4;
  PUT_INT(durFrames); pos+=4;
  return pos;
}

// render function
// render should return 0 if it only used framebuffer, or 1 if the new output data is in fbout. this is
// used when you want to do something that you'd otherwise need to make a copy of the framebuffer.
// w and h are the width and height of the screen, in pixels.
// isBeat is 1 if a beat has been detected.
// visdata is in the format of [spectrum:0,wave:1][channel][band].

int C_THISCLASS::render(char[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h)
{
  int rval=0;

  if (isBeat&0x80000000) return 0;

  if (!enabled) return 0;

  if (onbeat && isBeat)
	{
	  thisQuality=quality2;
	  nF = durFrames;
	}
  else if (!nF) thisQuality = quality;

  if (thisQuality<100)
  {
    int y;
  	int *p = fbout;
		int *p2 = framebuffer;
    int sXInc = (w*65536) / thisQuality;
    int sYInc = (h*65536) / thisQuality;
    int ypos=(sYInc>>17);
    int dypos=0;

    for (y = 0; y < h; y ++)
    {
      int x=w;
      int *fbread=framebuffer+ypos*w;
      int dpos=0;
      int xpos=(sXInc>>17);
      int src=fbread[xpos];

      if (blend)
      {
        while (x--)
        {
			    *p++ = BLEND(*p2++, src);
          dpos+=1<<16;
          if (dpos>=sXInc)
          {
            xpos+=dpos>>16;
            if (xpos >= w) break;
            src=fbread[xpos];
            dpos-=sXInc;
          }
        }
      }
      else if (blendavg)
      {
        while (x--)
        {
			    *p++ = BLEND_AVG(*p2++, src);
          dpos+=1<<16;
          if (dpos>=sXInc)
          {
            xpos+=dpos>>16;
            if (xpos >= w) break;
            src=fbread[xpos];
            dpos-=sXInc;
          }
        }
      }
      else
      {
        while (x--)
        {
			    *p++ = src;
          dpos+=1<<16;
          if (dpos>=sXInc)
          {
            xpos+=dpos>>16;
            if (xpos >= w) break;
            src=fbread[xpos];
            dpos-=sXInc;
          }
        }
      }
      dypos+=1<<16;
      if (dypos>=sYInc)
      {
        ypos+=(dypos>>16);
        dypos-=sYInc;
        if (ypos >= h) break;
      }
    }
    rval=1;
  }

  if (nF)
	{
	nF--;
	if (nF)
		{
		int a = abs(quality - quality2) / durFrames;
		thisQuality += a * (quality2 > quality ? -1 : 1);
		}
	}

  return rval;
}


C_RBASE *R_Mosaic(char *desc) // creates a new effect object if desc is NULL, otherwise fills in desc with description
{
	if (desc) { strcpy(desc,MOD_NAME); return NULL; }
	return (C_RBASE *) new C_THISCLASS();
}

#else
C_RBASE *R_Mosaic(char *desc) // creates a new effect object if desc is NULL, otherwise fills in desc with description
{return NULL; }
#endif
