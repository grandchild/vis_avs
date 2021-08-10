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
#include "c_onetone.h"
#include <stdlib.h>
#include "r_defs.h"


#ifndef LASER

C_THISCLASS::~C_THISCLASS() 
{
}


C_THISCLASS::C_THISCLASS() // set up default configuration
{
  color = 0xFFFFFF;
	invert=0;
  blend = 0;
  blendavg = 0;
  enabled=1;
	RebuildTable();
}

#define GET_INT() (data[pos]|(data[pos+1]<<8)|(data[pos+2]<<16)|(data[pos+3]<<24))
void C_THISCLASS::load_config(unsigned char *data, int len) // read configuration of max length "len" from data.
{
	int pos=0;
	if (len-pos >= 4) { enabled=GET_INT(); pos+=4; }
	if (len-pos >= 4) { color=GET_INT(); pos+=4; }
	if (len-pos >= 4) { blend=GET_INT(); pos+=4; }
	if (len-pos >= 4) { blendavg=GET_INT(); pos+=4; }
	if (len-pos >= 4) { invert=GET_INT(); pos+=4; }
	RebuildTable();
}

#define PUT_INT(y) data[pos]=(y)&255; data[pos+1]=(y>>8)&255; data[pos+2]=(y>>16)&255; data[pos+3]=(y>>24)&255
int  C_THISCLASS::save_config(unsigned char *data) // write configuration to data, return length. config data should not exceed 64k.
{
  int pos=0;
  PUT_INT(enabled); pos+=4;
  PUT_INT(color); pos+=4;
  PUT_INT(blend); pos+=4;
  PUT_INT(blendavg); pos+=4;
  PUT_INT(invert); pos+=4;
  return pos;
}

int __inline C_THISCLASS::depthof(int c)
{
int r= max(max((c & 0xFF), ((c & 0xFF00)>>8)), (c & 0xFF0000)>>16);
return invert ? 255 - r : r;
}

void C_THISCLASS::RebuildTable(void)
{
int i;
for (i=0;i<256;i++)
	tableb[i] = (unsigned char)((i / 255.0) * (float)(color & 0xFF));
for (i=0;i<256;i++)
	tableg[i] = (unsigned char)((i / 255.0) * (float)((color & 0xFF00) >> 8));
for (i=0;i<256;i++)
	tabler[i] = (unsigned char)((i / 255.0) * (float)((color & 0xFF0000) >> 16));
}

// render function
// render should return 0 if it only used framebuffer, or 1 if the new output data is in fbout. this is
// used when you want to do something that you'd otherwise need to make a copy of the framebuffer.
// w and h are the width and height of the screen, in pixels.
// isBeat is 1 if a beat has been detected.
// visdata is in the format of [spectrum:0,wave:1][channel][band].

int C_THISCLASS::render(char[2][2][576], int isBeat, int *framebuffer, int*, int w, int h)
{
  int i=w*h;
  int *p=framebuffer;
	int c,d;

  if (!enabled) return 0;
  if (isBeat&0x80000000) return 0;

  if (blend)
  {
    while (i--)
		  {
		  d = depthof(*p);
		  c = tableb[d] | (tableg[d]<<8) | (tabler[d]<<16);
		  *p = BLEND(*p, c);
		  ++p;
		  }
  }
  else if (blendavg)
  {
    while (i--)
		  {
		  d = depthof(*p);
		  c = tableb[d] | (tableg[d]<<8) | (tabler[d]<<16);
		  *p = BLEND_AVG(*p, c);
		  ++p;
		  }
  }
  else
  {
    while (i--)
		  {
		  d = depthof(*p);
		  *p++ = tableb[d] | (tableg[d]<<8) | (tabler[d]<<16);
		  }
  }
  return 0;
}


C_RBASE *R_Onetone(char *desc) // creates a new effect object if desc is NULL, otherwise fills in desc with description
{
	if (desc) { strcpy(desc,MOD_NAME); return NULL; }
	return (C_RBASE *) new C_THISCLASS();
}

#else
C_RBASE *R_Onetone(char *desc) // creates a new effect object if desc is NULL, otherwise fills in desc with description
{
  return NULL;
}
#endif
