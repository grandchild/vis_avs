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

#include "c_multidelay.h"
#include <windows.h>
#include "resource.h"
#include "r_defs.h"


// saved
bool g_usebeats[MULTIDELAY_NUM_BUFFERS];
int g_delay[MULTIDELAY_NUM_BUFFERS];

// unsaved
LPVOID buffer[MULTIDELAY_NUM_BUFFERS];
LPVOID inpos[MULTIDELAY_NUM_BUFFERS];
LPVOID outpos[MULTIDELAY_NUM_BUFFERS];
unsigned long buffersize[MULTIDELAY_NUM_BUFFERS];
unsigned long virtualbuffersize[MULTIDELAY_NUM_BUFFERS];
unsigned long oldvirtualbuffersize[MULTIDELAY_NUM_BUFFERS];
unsigned long g_framedelay[MULTIDELAY_NUM_BUFFERS];
unsigned int numinstances = 0;
unsigned long framessincebeat;
unsigned long g_framesperbeat;
unsigned long framemem;
unsigned long oldframemem;
unsigned int renderid;


// configuration screen
int win32_dlgproc_multidelay(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
	C_DELAY* g_Delay = (C_DELAY*)g_current_render;
	char value[16];
	unsigned int objectcode, objectmessage;
	HWND hwndEdit;
	switch (uMsg)
	{
		case WM_INITDIALOG: //init
      {
        int i;
			  for (i=0;i<3;i++) CheckDlgButton(hwndDlg,1100+i,g_Delay->mode==i);
			  for (i=0;i<MULTIDELAY_NUM_BUFFERS;i++)
			  {
				  CheckDlgButton(hwndDlg,1000+i,g_Delay->activebuffer==i);
				  CheckDlgButton(hwndDlg,1020+i,g_Delay->usebeats(i));
				  CheckDlgButton(hwndDlg,1030+i,!g_Delay->usebeats(i));
				  hwndEdit = GetDlgItem(hwndDlg,1010+i);
				  _itoa(g_Delay->delay(i),value,10);
				  SetWindowText(hwndEdit,value);
			  }
      }
			return 1;
		case WM_COMMAND:
			objectcode = LOWORD(wParam);
			objectmessage = HIWORD(wParam);
			// mode stuff
			if (objectcode >= 1100)
			{
				if(IsDlgButtonChecked(hwndDlg,objectcode)==1)
				{
					g_Delay->mode = objectcode - 1100;
					for (int i=1100;i<1103;i++)	if (objectcode != i) CheckDlgButton(hwndDlg,i,BST_UNCHECKED);
				}
			    return 0;
			}
			// frames stuff
			if (objectcode >= 1030)
			{
				if(IsDlgButtonChecked(hwndDlg,objectcode)==1)
				{
					g_Delay->usebeats(objectcode-1030, false);
					CheckDlgButton(hwndDlg,objectcode-10,BST_UNCHECKED);
					g_Delay->framedelay(objectcode-1030, g_Delay->delay(objectcode-1030)+1);
				}
				else g_Delay->usebeats(objectcode-1030, true);
			    return 0;
			}
			// beats stuff
			if (objectcode >= 1020)
			{
				if(IsDlgButtonChecked(hwndDlg,objectcode)==1)
				{
					g_Delay->usebeats(objectcode-1020, true);
					CheckDlgButton(hwndDlg,objectcode+10,BST_UNCHECKED);
					g_Delay->framedelay(objectcode-1020, g_Delay->framesperbeat()+1);
				}
				else g_Delay->usebeats(objectcode-1020, false);
			    return 0;
			}
			// edit box stuff
			if (objectcode >= 1010)
			{
				hwndEdit = GetDlgItem(hwndDlg,objectcode);
				if (objectmessage == EN_CHANGE)
				{
					GetWindowText(hwndEdit,value,16);
					g_Delay->delay(objectcode-1010, max(atoi(value),0));
					g_Delay->framedelay(objectcode-1010, (g_Delay->usebeats(objectcode-1010)?g_Delay->framesperbeat():g_Delay->delay(objectcode-1010))+1);
				}
				else if (objectmessage == EN_KILLFOCUS)
				{
					_itoa(g_Delay->delay(objectcode-1010),value,10);
					SetWindowText(hwndEdit,value);
				}
				return 0;
			}
			// active buffer stuff
			if (objectcode >= 1000)
			{
				if(IsDlgButtonChecked(hwndDlg,objectcode)==1)
				{
					g_Delay->activebuffer = objectcode - 1000;
					for (int i=1000;i<1006;i++)	if (objectcode != i) CheckDlgButton(hwndDlg,i,BST_UNCHECKED);
				}
			    return 0;
			}
	}
	return 0;
}

// set up default configuration 
C_DELAY::C_DELAY() 
{	
	// enable
	mode = 0;
	activebuffer = 0;
	numinstances++;
	creationid = numinstances;
	if (creationid == 1)
	{
		for (int i=0; i<MULTIDELAY_NUM_BUFFERS; i++)
		{
			renderid = 0;
			framessincebeat = 0;
			g_framesperbeat = 0;
			framemem = 1;
			oldframemem = 1;
			g_usebeats[i] = false;
			g_delay[i] = 0;
			g_framedelay[i] = 0;
			buffersize[i] = 1;
			virtualbuffersize[i] = 1;
			oldvirtualbuffersize[i] = 1;
			buffer[i] = VirtualAlloc(NULL,buffersize[i],MEM_COMMIT,PAGE_READWRITE);
			inpos[i] = buffer[i];
			outpos[i] = buffer[i];
		}
	}
}

// virtual destructor
C_DELAY::~C_DELAY() 
{
	numinstances--;
	if (numinstances == 0) for (int i=0; i<MULTIDELAY_NUM_BUFFERS; i++) VirtualFree(buffer[i],buffersize[i],MEM_DECOMMIT);
}

// RENDER FUNCTION:
// render should return 0 if it only used framebuffer, or 1 if the new output data is in fbout
// w and h are the-width and height of the screen, in pixels.
// isBeat is 1 if a beat has been detected.
// visdata is in the format of [spectrum:0,wave:1][channel][band].

int C_DELAY::render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h)
{
  if (isBeat&0x80000000) return 0;

	if (renderid == numinstances) renderid = 0;
	renderid++;
	if (renderid == 1)
	{
		framemem = w*h*4;
		if (isBeat)
		{
			g_framesperbeat = framessincebeat;
			for (int i=0;i<MULTIDELAY_NUM_BUFFERS;i++) if (g_usebeats[i]) g_framedelay[i] = g_framesperbeat+1;
			framessincebeat = 0;
		}
		framessincebeat++;
		for (int i=0;i<MULTIDELAY_NUM_BUFFERS;i++)
		{
			if (g_framedelay[i]>1)
			{
				virtualbuffersize[i] = g_framedelay[i]*framemem;
				if (framemem == oldframemem)
				{
					if (virtualbuffersize[i] != oldvirtualbuffersize[i])
					{
						if (virtualbuffersize[i] > oldvirtualbuffersize[i])
						{
							if (virtualbuffersize[i] > buffersize[i])
							{
								// allocate new memory
								if (!VirtualFree(buffer[i],buffersize[i],MEM_DECOMMIT)) return 0;
								if (g_usebeats[i])
								{
									buffersize[i] = 2*virtualbuffersize[i];
									buffer[i] = VirtualAlloc(NULL,buffersize[i],MEM_COMMIT,PAGE_READWRITE);
									if (buffer[i] == NULL)
									{
										buffersize[i] = virtualbuffersize[i];
										buffer[i] = VirtualAlloc(NULL,buffersize[i],MEM_COMMIT,PAGE_READWRITE);
									}
								}
								else
								{
									buffersize[i] = virtualbuffersize[i];
									buffer[i] = VirtualAlloc(NULL,buffersize[i],MEM_COMMIT,PAGE_READWRITE);
								}
								outpos[i] = buffer[i];
								inpos[i] = (LPVOID)(((unsigned long)buffer[i])+virtualbuffersize[i]-framemem);
								if (buffer[i] == NULL)
								{
									g_framedelay[i] = 0;
									if (g_usebeats[i])
									{
										g_framesperbeat = 0;
										framessincebeat = 0;
										g_framedelay[i] = 0;
										g_delay[i] = 0;
									}
								}
							}
							else
							{
								unsigned long size = (((unsigned long)buffer[i])+oldvirtualbuffersize[i]) - ((unsigned long)outpos[i]);
								unsigned long l = ((unsigned long)buffer[i])+virtualbuffersize[i];
								unsigned long d =  l - size;
								MoveMemory((LPVOID)d, outpos[i], size);
								for (l = (unsigned long)outpos[i]; l < d; l += framemem) CopyMemory((LPVOID)l,(LPVOID)d,framemem);
							}
						}
						else
						{	// virtualbuffersize < oldvirtualbuffersize
							unsigned long presegsize = ((unsigned long)outpos[i])-((unsigned long)buffer[i]);
							if (presegsize > virtualbuffersize[i])
							{
								MoveMemory(buffer[i],(LPVOID)(((unsigned long)buffer[i])+presegsize-virtualbuffersize[i]),virtualbuffersize[i]);
								inpos[i] = (LPVOID)(((unsigned long)buffer[i])+virtualbuffersize[i]-framemem);
								outpos[i] = buffer[i];
							}
							else if (presegsize < virtualbuffersize[i]) MoveMemory(outpos[i],(LPVOID)(((unsigned long)buffer[i])+oldvirtualbuffersize[i]+presegsize-virtualbuffersize[i]),virtualbuffersize[i]-presegsize);
						}
						oldvirtualbuffersize[i] = virtualbuffersize[i];
					}
				}
				else
				{
					// allocate new memory
					if (!VirtualFree(buffer[i],buffersize[i],MEM_DECOMMIT)) return 0;
					if (g_usebeats[i])
					{
						buffersize[i] = 2*virtualbuffersize[i];
						buffer[i] = VirtualAlloc(NULL,buffersize[i],MEM_COMMIT,PAGE_READWRITE);
						if (buffer[i] == NULL)
						{
							buffersize[i] = virtualbuffersize[i];
							buffer[i] = VirtualAlloc(NULL,buffersize[i],MEM_COMMIT,PAGE_READWRITE);
						}
					}
					else
					{
						buffersize[i] = virtualbuffersize[i];
						buffer[i] = VirtualAlloc(NULL,buffersize[i],MEM_COMMIT,PAGE_READWRITE);
					}
					outpos[i] = buffer[i];
					inpos[i] = (LPVOID)(((unsigned long)buffer[i])+virtualbuffersize[i]-framemem);
					if (buffer[i] == NULL)
					{
						g_framedelay[i] = 0;
						if (g_usebeats[i])
						{
							g_framesperbeat = 0;
							framessincebeat = 0;
							g_framedelay[i] = 0;
							g_delay[i] = 0;
						}
					}
					oldvirtualbuffersize[i] = virtualbuffersize[i];
				}
				oldframemem = framemem;
			}
		}
	}
	if (mode != 0 && g_framedelay[activebuffer]>1)
	{
		if (mode == 2) CopyMemory(framebuffer,outpos[activebuffer],framemem);
		else CopyMemory(inpos[activebuffer],framebuffer,framemem);
	}
	if (renderid == numinstances) for (int i=0;i<MULTIDELAY_NUM_BUFFERS;i++)
	{
		inpos[i] = (LPVOID)(((unsigned long)inpos[i])+framemem);
		outpos[i] = (LPVOID)(((unsigned long)outpos[i])+framemem);
		if ((unsigned long)inpos[i]>=((unsigned long)buffer[i])+virtualbuffersize[i]) inpos[i] = buffer[i];
		if ((unsigned long)outpos[i]>=((unsigned long)buffer[i])+virtualbuffersize[i]) outpos[i] = buffer[i];
	}
	return 0;
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
		// load mode
		mode=GET_INT();
		pos+=4; 
	}
	if (len-pos >= 4) 
	{
		// load active buffer
		activebuffer=GET_INT();
		pos+=4; 
	}
	if (len-pos >= 4) 
	{
		for (int i=0;i<MULTIDELAY_NUM_BUFFERS;i++)
		{
			if (len-pos >= 4) 
			{
				// load usebeats
				g_usebeats[i]=(GET_INT()==1);
				pos+=4;
			}
			if (len-pos >= 4) 
			{
				// load delay
				g_delay[i]=GET_INT();
				g_framedelay[i] = (g_usebeats[i]?g_framesperbeat:g_delay[i])+1;
				pos+=4;
			}
		}
	}
}

// write configuration to data, return length. config data should not exceed 64k.
#define PUT_INT(y) data[pos]=(y)&255; data[pos+1]=(y>>8)&255; data[pos+2]=(y>>16)&255; data[pos+3]=(y>>24)&255
int  C_DELAY::save_config(unsigned char *data) 
{
	int pos=0;
	PUT_INT(mode);
	pos+=4;
	PUT_INT(activebuffer);
	pos+=4;
	if (creationid == 1)
	{
		for (int i=0;i<MULTIDELAY_NUM_BUFFERS;i++)
		{
			PUT_INT((int)g_usebeats[i]);
			pos+=4;
			PUT_INT(g_delay[i]);
			pos+=4;
		}
	}
	return pos;
}

// export stuff
C_RBASE *R_MultiDelay(char *desc) // creates a new effect object if desc is NULL, otherwise fills in desc with description
{
	if (desc) 
	{ 
		strcpy(desc,MOD_NAME); 
		return NULL; 
	}
	return (C_RBASE *) new C_DELAY();
}

bool          C_DELAY::usebeats(int buf) { return g_usebeats[buf]; }
void          C_DELAY::usebeats(int buf, bool value) { g_usebeats[buf] = value; }
int           C_DELAY::delay(int buf) { return g_delay[buf]; }
void          C_DELAY::delay(int buf, int value) { g_delay[buf] = value; }
void          C_DELAY::framedelay(int buf, unsigned long value) { g_framedelay[buf] = value; }
unsigned long C_DELAY::framesperbeat() { return g_framesperbeat; }
