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
// video delay
// copyright tom holden, 2002
// mail: cfp@myrealbox.com

#include "c_videodelay.h"
#include "r_defs.h"

#ifndef LASER


// set up default configuration 
C_DELAY::C_DELAY() 
{	
	// enable
	enabled = true;
	usebeats = false;
	delay = 10;
	framedelay = 10;
	framessincebeat = 0;
	buffersize = 1;
	virtualbuffersize = 1;
	oldvirtualbuffersize = 1;
	buffer = calloc(buffersize, 1);
	inoutpos = buffer;
}

// virtual destructor
C_DELAY::~C_DELAY() 
{
	free(buffer);
}

// RENDER FUNCTION:
// render should return 0 if it only used framebuffer, or 1 if the new output data is in fbout
// w and h are the-width and height of the screen, in pixels.
// isBeat is 1 if a beat has been detected.
// visdata is in the format of [spectrum:0,wave:1][channel][band].

int C_DELAY::render(char[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h)
{
  if (isBeat&0x80000000) return 0;

	framemem = w*h*4;
	if (usebeats)
	{
		if (isBeat)
		{
			framedelay = framessincebeat*delay;	//changed
			if (framedelay > 400) framedelay = 400;	//new
			framessincebeat = 0;
		}
		framessincebeat++;
	}
	if (enabled && framedelay!=0)
	{
		virtualbuffersize = framedelay*framemem;
		if (framemem == oldframemem)
		{
			if (virtualbuffersize != oldvirtualbuffersize)
			{
				if (virtualbuffersize > oldvirtualbuffersize)
				{
					if (virtualbuffersize > buffersize)
					{
						// allocate new memory
						free(buffer);
						if (usebeats)
						{
							buffersize = 2*virtualbuffersize;
							if (buffersize > framemem*400) buffersize = framemem*400;	//new
							buffer = calloc(buffersize, 1);
							if (buffer == NULL)
							{
								buffersize = virtualbuffersize;
								buffer = calloc(buffersize, 1);
							}
						}
						else
						{
							buffersize = virtualbuffersize;
							buffer = calloc(buffersize, 1);
						}
						inoutpos = buffer;
						if (buffer == NULL)
						{
							framedelay = 0;
							framessincebeat = 0;
							return 0;
						}
					}
					else
					{
						unsigned long size = (((unsigned long)buffer)+oldvirtualbuffersize) - ((unsigned long)inoutpos);
						unsigned long l = ((unsigned long)buffer)+virtualbuffersize;
						unsigned long d =  l - size;
						memmove((void*)d, inoutpos, size);
						for (l = (unsigned long)inoutpos; l < d; l += framemem) memcpy((void*)l,(void*)d,framemem);
					}
				}
				else
				{	// virtualbuffersize < oldvirtualbuffersize
					unsigned long presegsize = ((unsigned long)inoutpos)-((unsigned long)buffer)+framemem;
					if (presegsize > virtualbuffersize)
					{
						memmove(buffer,(void*)(((unsigned long)buffer)+presegsize-virtualbuffersize),virtualbuffersize);
						inoutpos = (void*)(((unsigned long)buffer)+virtualbuffersize-framemem);
					}
					else if (presegsize < virtualbuffersize) memmove((void*)(((unsigned long)inoutpos)+framemem),(void*)(((unsigned long)buffer)+oldvirtualbuffersize+presegsize-virtualbuffersize),virtualbuffersize-presegsize);
				}
				oldvirtualbuffersize = virtualbuffersize;
			}
		}
		else
		{
			// allocate new memory
			free(buffer);
			if (usebeats)
			{
				buffersize = 2*virtualbuffersize;
				buffer = calloc(buffersize, 1);
				if (buffer == NULL)
				{
					buffersize = virtualbuffersize;
					buffer = calloc(buffersize, 1);
				}
			}
			else
			{
				buffersize = virtualbuffersize;
				buffer = calloc(buffersize, 1);
			}
			inoutpos = buffer;
			if (buffer == NULL)
			{
				framedelay = 0;
				framessincebeat = 0;
				return 0;
			}
			oldvirtualbuffersize = virtualbuffersize;
		}
		oldframemem = framemem;
		memcpy(fbout,inoutpos,framemem);
		memcpy(inoutpos,framebuffer,framemem);
		inoutpos = (void*)(((unsigned long)inoutpos)+framemem);
		if ((unsigned long)inoutpos>=((unsigned long)buffer)+virtualbuffersize) inoutpos = buffer;
		return 1;
	}
	else return 0;
}

char *C_DELAY::get_desc(void)
{ 
	return MOD_NAME; 
}

// load_/save_config are called when saving and loading presets (.avs files)
#define GET_INT() (data[pos]|(data[pos+1]<<8)|(data[pos+2]<<16)|(data[pos+3]<<24))
void C_DELAY::load_config(unsigned char *data, int len) // read configuration of max length "len" from data.
{
	int pos=0;
	// always ensure there is data to be loaded
	if (len-pos >= 4) 
	{
		// load activation toggle
		enabled=(GET_INT()==1);
		pos+=4; 
	}
	if (len-pos >= 4) 
	{
		// load beats toggle
		usebeats=(GET_INT()==1);
		pos+=4; 
	}
	if (len-pos >= 4) 
	{
		// load delay
		delay=GET_INT();

    if (usebeats) {if (delay > 16) delay = 16;}	//new
		else {if (delay > 200) delay = 200;}	//new

		pos+=4; 
	}
}

// write configuration to data, return length. config data should not exceed 64k.
#define PUT_INT(y) data[pos]=(y)&255; data[pos+1]=(y>>8)&255; data[pos+2]=(y>>16)&255; data[pos+3]=(y>>24)&255
int  C_DELAY::save_config(unsigned char *data) 
{
	int pos=0;
	PUT_INT((int)enabled);
	pos+=4;
	PUT_INT((int)usebeats);
	pos+=4;
	PUT_INT((unsigned int)delay);
	pos+=4;
	return pos;
}

// export stuff
C_RBASE *R_VideoDelay(char *desc) // creates a new effect object if desc is NULL, otherwise fills in desc with description
{
	if (desc) 
	{ 
		strcpy(desc,MOD_NAME); 
		return NULL; 
	}
	return (C_RBASE *) new C_DELAY();
}

#endif
