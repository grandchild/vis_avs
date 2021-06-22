// convolution filter
// copyright tom holden, 2002
// mail: cfp@myrealbox.com

#include <windows.h>
#include <stdlib.h>
#include <mmintrin.h>
#include "r_defs.h"
#include "resource.h"

#define MOD_NAME "Trans / Convolution Filter"

#define MAX_DRAW_SIZE 16384
#define MAX_FILENAME_SIZE 1024

typedef int (* FunctionType)(void *, void *, void *);	// framebuffer, fbout, m64farray

char lastszFile[MAX_FILENAME_SIZE];						// stores the name of the last opened file in any instance of the ape
unsigned int instances = 0;								// stores the number of instances currently open

class C_CONVOLUTION : public C_RBASE 
{
	public:
		// standard ape members
		C_CONVOLUTION();
		virtual ~C_CONVOLUTION();
		virtual int render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h);
		virtual char *get_desc();
		virtual void load_config(unsigned char *data, int len);
		virtual int  save_config(unsigned char *data);

		// file access members
		OPENFILENAME FileBox;			// the file dialogue
		char szFile[MAX_FILENAME_SIZE];				// buffer for file name
		HANDLE hf;						// file handle

		// functions
		void createdraw(void);			// create the draw function
		_inline void deletedraw(void);	// delete it
		FunctionType draw;				// the draw function

		// main filter data (saved)
		bool enabled;					// toggles plug-in on and off
		bool wraparound;				// toggles wrap around subtraction
		bool absolute;					// toggles absolute values in subtraction results
		bool twopass;					// toggles two pass mode
		int farray[51];					// box data

		// non-saved values
		__m64 m64farray[50];			// abs(farray) packed into the four unsigned shorts that make up an m64 
		int width,height;				// does what it says on the tin
		DWORD oldprotect;				// needed for virtual protect stuff
		int drawstate;					// 0 not allocated 1 creating 2 created
		bool updatedraw;				// true if draw needs updating
		unsigned int codelength;		// length of draw
};


// configuration screen
int win32_dlgproc_convolution(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
	C_CONVOLUTION* g_Filter = (C_CONVOLUTION*)g_current_render;
	char value[16];
	int val;
	unsigned int objectcode, objectmessage;
	HWND hwndEdit;
	switch (uMsg)
	{
		case WM_INITDIALOG: //init e.g. fill in boxes
			CheckDlgButton(hwndDlg,IDC_CONVOLUTION_CHECK1,g_Filter->enabled);
			CheckDlgButton(hwndDlg,IDC_CONVOLUTION_CHECK2,g_Filter->wraparound);
			CheckDlgButton(hwndDlg,IDC_CONVOLUTION_CHECK3,g_Filter->absolute);
			CheckDlgButton(hwndDlg,IDC_CONVOLUTION_CHECK4,g_Filter->twopass);
			if (g_Filter->farray[50] == 0) g_Filter->farray[50] = 1;
			for(int i=0;i<51;i++)
			{
				hwndEdit = GetDlgItem(hwndDlg,IDC_CONVOLUTION_EDIT1+i);
				_itoa(g_Filter->farray[i],value,10);
				SetWindowText(hwndEdit,value);
			}
			// file dialogue initialisation.
			g_Filter->FileBox.lStructSize = sizeof(OPENFILENAME);
			g_Filter->FileBox.hwndOwner = hwndDlg;
			g_Filter->FileBox.hInstance = NULL;
			g_Filter->FileBox.lpstrFilter = "Convolution Filter File (*.cff)\0*.cff\0All Files (*.*)\0*.*\0";
			g_Filter->FileBox.lpstrCustomFilter = NULL;
			g_Filter->FileBox.nMaxCustFilter = 0;
			g_Filter->FileBox.nFilterIndex = 1;
			g_Filter->FileBox.nMaxFile = sizeof(g_Filter->szFile);
			g_Filter->FileBox.lpstrFileTitle = NULL;
			g_Filter->FileBox.nMaxFileTitle = 0;
			if (lastszFile[0] != NULL)
			{
				for (int i=0; i<MAX_FILENAME_SIZE;i++) g_Filter->szFile[i] = lastszFile[i];
				g_Filter->FileBox.lpstrInitialDir = NULL;
			}
			else g_Filter->FileBox.lpstrInitialDir = "C:\\Program Files\\Winamp3\\Wacs\\data\\avs";
			g_Filter->FileBox.lpstrFile = g_Filter->szFile;
			g_Filter->FileBox.lpstrTitle = NULL;
			g_Filter->FileBox.lpfnHook = NULL;
			g_Filter->FileBox.lpstrDefExt = "cff";
			return 1;
		case WM_COMMAND:
			objectcode = LOWORD(wParam);
			objectmessage = HIWORD(wParam);
			//see if the auto button's been clicked and if so do the calculations to work out scale's value
			if ((objectcode == IDC_CONVOLUTION_BUTTON1)&&(objectmessage == BN_CLICKED))
			{
				hwndEdit = GetDlgItem(hwndDlg,IDC_CONVOLUTION_EDIT51);
				val = 0;
				for (int i=0;i<50;i++) val+=g_Filter->farray[i];
				val = (g_Filter->twopass)?(2*val):val;
				if (val==0) val = 1;
				g_Filter->farray[50] = val;
				_itoa(val,value,10);
				SetWindowText(hwndEdit,value);
				g_Filter->updatedraw = true;
				return 0;
			}
			// see if the clear button's been clicked
			if ((objectcode == IDC_CONVOLUTION_BUTTON4)&&(objectmessage == BN_CLICKED))
			{
				if(MessageBox(hwndDlg, "Really clear all data?", "convolution", MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2|MB_APPLMODAL|MB_SETFOREGROUND)==IDYES)
				{
					// set box array values
					for (int i=0;i<50;i++) g_Filter->farray[i]=0;
					g_Filter->farray[50] = 1;
					g_Filter->farray[24] = 1;
					// enable
					g_Filter->enabled = true;
					g_Filter->wraparound = false;
					g_Filter->twopass = false;
					g_Filter->absolute = false;
					g_Filter->width = 0;
					g_Filter->height = 0;
					g_Filter->updatedraw = true;
					CheckDlgButton(hwndDlg,IDC_CONVOLUTION_CHECK1,BST_CHECKED);
					CheckDlgButton(hwndDlg,IDC_CONVOLUTION_CHECK2,BST_UNCHECKED);
					CheckDlgButton(hwndDlg,IDC_CONVOLUTION_CHECK3,BST_UNCHECKED);
					CheckDlgButton(hwndDlg,IDC_CONVOLUTION_CHECK4,BST_UNCHECKED);
					for(int i=0;i<51;i++)
					{
						hwndEdit = GetDlgItem(hwndDlg,IDC_CONVOLUTION_EDIT1+i);
						_itoa(g_Filter->farray[i],value,10);
						SetWindowText(hwndEdit,value);
					}
				}
				return 0;
			}
			// see if the load button's been clicked
			if ((objectcode == IDC_CONVOLUTION_BUTTON3)&&(objectmessage == BN_CLICKED))
			{
				g_Filter->FileBox.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | 0x02000000 | OFN_HIDEREADONLY;	// 0x02000000 is OFN_DONTADDTORECENT for some reason the compiler is not recognising it.
				if (GetOpenFileName(&g_Filter->FileBox))
				{
					g_Filter->hf = CreateFile(g_Filter->FileBox.lpstrFile, GENERIC_READ, 0, (LPSECURITY_ATTRIBUTES) NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,(HANDLE) NULL);
					if (g_Filter->hf == INVALID_HANDLE_VALUE)
					{
						// display the error
						LPVOID lpMsgBuf;
						FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL);
						MessageBox( NULL, (LPCTSTR)lpMsgBuf, "Unexpected file error.", MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND );
						LocalFree( lpMsgBuf );
						return 0;
					}
					else
					{
						unsigned char *data;
						data = new unsigned char[MAX_FILENAME_SIZE];
						DWORD numbytesread;
						g_Filter->FileBox.lpstrInitialDir = NULL;
						for (int i=0;i<MAX_FILENAME_SIZE;i++) lastszFile[i] = g_Filter->szFile[i];
						if (ReadFile(g_Filter->hf, data, MAX_FILENAME_SIZE, &numbytesread, NULL)==0 && GetLastError()!=ERROR_HANDLE_EOF)
						{
							// display the error
							LPVOID lpMsgBuf;
							FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL);
							MessageBox( NULL, (LPCTSTR)lpMsgBuf, "Unexpected file error.", MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND );
							LocalFree( lpMsgBuf );
						}
						g_Filter->load_config(data,(unsigned int) numbytesread);
						// refresh the dialogue with the new data
						CheckDlgButton(hwndDlg,IDC_CONVOLUTION_CHECK1,g_Filter->enabled);
						CheckDlgButton(hwndDlg,IDC_CONVOLUTION_CHECK2,g_Filter->wraparound);
						CheckDlgButton(hwndDlg,IDC_CONVOLUTION_CHECK3,g_Filter->absolute);
						CheckDlgButton(hwndDlg,IDC_CONVOLUTION_CHECK4,g_Filter->twopass);
						if (g_Filter->farray[50] == 0) g_Filter->farray[50] = 1;
						for(int i=0;i<51;i++)
						{
							hwndEdit = GetDlgItem(hwndDlg,IDC_CONVOLUTION_EDIT1+i);
							_itoa(g_Filter->farray[i],value,10);
							SetWindowText(hwndEdit,value);
						}
						if (CloseHandle(g_Filter->hf)==0)
						{
							// display the error
							LPVOID lpMsgBuf;
							FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL);
							MessageBox( NULL, (LPCTSTR)lpMsgBuf, "Unexpected file error.", MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND );
							LocalFree( lpMsgBuf );
						}
					}
				}
				return 0;
			}
			// see if the save button's been clicked
			if ((objectcode == IDC_CONVOLUTION_BUTTON2)&&(objectmessage == BN_CLICKED))
			{
				g_Filter->FileBox.Flags = 0x02000000 | OFN_HIDEREADONLY;	// 0x02000000 is OFN_DONTADDTORECENT for some reason the compiler is not recognising it.
				if (GetSaveFileName(&(g_Filter->FileBox)))
				{
					g_Filter->hf = CreateFile(g_Filter->FileBox.lpstrFile, GENERIC_WRITE, 0, (LPSECURITY_ATTRIBUTES) NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL,(HANDLE) NULL);
					if ((g_Filter->hf == INVALID_HANDLE_VALUE) && (GetLastError()==ERROR_FILE_EXISTS) && (MessageBox(hwndDlg, "This file already exists. Do you want to overwrite it?", "convolution",  MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2|MB_APPLMODAL|MB_SETFOREGROUND)==IDYES))
						g_Filter->hf = CreateFile(g_Filter->FileBox.lpstrFile, GENERIC_WRITE, 0, (LPSECURITY_ATTRIBUTES) NULL, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL,(HANDLE) NULL);
					if (g_Filter->hf == INVALID_HANDLE_VALUE)
					{
						// display the error
						LPVOID lpMsgBuf;
						FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL);
						MessageBox( NULL, (LPCTSTR)lpMsgBuf, "Unexpected file error.", MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND );
						LocalFree( lpMsgBuf );
					}
					else
					{
						unsigned char *data;
						data = new unsigned char[MAX_FILENAME_SIZE];
						int numbytestowrite = g_Filter->save_config(data);
						DWORD numbyteswritten;
						g_Filter->FileBox.lpstrInitialDir = NULL;
						for (int i=0;i<MAX_FILENAME_SIZE;i++) lastszFile[i] = g_Filter->szFile[i];
						if (WriteFile(g_Filter->hf, data, numbytestowrite, &numbyteswritten, NULL)==0)
						{
							// display the error
							LPVOID lpMsgBuf;
							FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL);
							MessageBox( NULL, (LPCTSTR)lpMsgBuf, "Unexpected file error.", MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND );
							LocalFree( lpMsgBuf );
						}
						if (CloseHandle(g_Filter->hf)==0)
						{
							// display the error
							LPVOID lpMsgBuf;
							FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL);
							MessageBox( NULL, (LPCTSTR)lpMsgBuf, "Unexpected file error.", MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND );
							LocalFree( lpMsgBuf );
						}
					}
				}
				return 0;
			}
			// see if enable checkbox is checked
			if (objectcode == IDC_CONVOLUTION_CHECK1)
			{
				g_Filter->enabled = IsDlgButtonChecked(hwndDlg,IDC_CONVOLUTION_CHECK1)==1;
				g_Filter->updatedraw = true;;
				return 0;
			}
			// see if wraparound checkbox is checked
			if (objectcode == IDC_CONVOLUTION_CHECK2)
			{
				if(IsDlgButtonChecked(hwndDlg,IDC_CONVOLUTION_CHECK2)==1)
				{
					g_Filter->wraparound = true;
					g_Filter->absolute = false;
					CheckDlgButton(hwndDlg,IDC_CONVOLUTION_CHECK3,BST_UNCHECKED);
				}
				else g_Filter->wraparound = false;
				g_Filter->updatedraw = true;;
				return 0;
			}
			// see if absolute checkbox is checked
			if (objectcode == IDC_CONVOLUTION_CHECK3)
			{
				if(IsDlgButtonChecked(hwndDlg,IDC_CONVOLUTION_CHECK3)==1)
				{
					g_Filter->absolute = true;
					g_Filter->wraparound = false;
					CheckDlgButton(hwndDlg,IDC_CONVOLUTION_CHECK2,BST_UNCHECKED);
				}
				else g_Filter->absolute = false;
				g_Filter->updatedraw = true;;
			    return 0;
			}
			// see if twopass checkbox is checked
			if (objectcode == IDC_CONVOLUTION_CHECK4)
			{
				g_Filter->twopass = IsDlgButtonChecked(hwndDlg,IDC_CONVOLUTION_CHECK4)==1;
				g_Filter->updatedraw = true;;
			    return 0;
			}
			//get and put data from the boxes to farray
			{
				int i = objectcode - IDC_CONVOLUTION_EDIT1;
				if (0<=i&&i<51)
				{
					hwndEdit = GetDlgItem(hwndDlg,IDC_CONVOLUTION_EDIT1+i);
					if (objectmessage == EN_CHANGE)
					{
						GetWindowText(hwndEdit,value,16);
						val = atoi(value);
						if (i==50)
						{
							g_Filter->farray[i] = (val==0)?1:val;
						}
						else g_Filter->farray[i] = val;
						g_Filter->updatedraw = true;;
					}
					else if (objectmessage == EN_KILLFOCUS)
					{
						_itoa(g_Filter->farray[i],value,10);
						SetWindowText(hwndEdit,value);
					}
				    return 0;
				}
			}
	}
	return 0;
}

// set up default configuration 
C_CONVOLUTION::C_CONVOLUTION() 
{	
	szFile[0] = NULL;
	if (instances==0) lastszFile[0] = NULL;
	instances++;
	// set box array values
	for (int i=0;i<50;i++) farray[i]=0;
	farray[50] = 1;
	farray[24] = 1;
	// enable
	drawstate = 0;
	enabled = true;
	wraparound = false;
	twopass = false;
	absolute = false;
	width = 0;
	height = 0;
	updatedraw = true;
}

// virtual destructor
C_CONVOLUTION::~C_CONVOLUTION() 
{
	instances--;
	while (drawstate!=0) if (drawstate == 2) deletedraw();
}

_inline void C_CONVOLUTION::deletedraw(void)
{
	delete [](LPBYTE)draw;
	drawstate = 0;
}

// RENDER FUNCTION:
// render should return 0 if it only used framebuffer, or 1 if the new output data is in fbout
// w and h are the-width and height of the screen, in pixels.
// isBeat is 1 if a beat has been detected.
// visdata is in the format of [spectrum:0,wave:1][channel][band].

#define data(xx,yy) framebuffer[c[yy]+min(max(i+xx,0),lastcol)]
//#define scale(out) min(max((long)((out+farray[25])/farray[50]),0),255)
#define bound(xx) min(max(xx,0),255)
int C_CONVOLUTION::render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h)
{
	int ret;
	if ((w!=width)||(h!=height))
	{
		width = w;
		height = h;
		updatedraw = true;
	}
	if (updatedraw)	// if we need to update the draw, do it
	{
		while (true)
		{
			while (drawstate!=0) if (drawstate==2) deletedraw();
			createdraw();
			if (drawstate==2) break;
		}
	}
	ret = draw(framebuffer,fbout,m64farray);
	return ret;
}

char *C_CONVOLUTION::get_desc(void)
{ 
	return MOD_NAME; 
}

// load_/save_config are called when saving and loading presets (.avs files)
#define GET_INT() (data[pos]|(data[pos+1]<<8)|(data[pos+2]<<16)|(data[pos+3]<<24))
void C_CONVOLUTION::load_config(unsigned char *data, int len) // read configuration of max length "len" from data.
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
		// load wraparound toggle
		wraparound=(GET_INT()==1);
		pos+=4; 
	}
	if (len-pos >= 4) 
	{
		// load absolute toggle
		absolute=(GET_INT()==1);
		pos+=4; 
	}
	if (len-pos >= 4) 
	{
		// load twopass toggle
		twopass=(GET_INT()==1);
		pos+=4; 
	}
	for (int i=0;i<51;i++)
	{
		if (len-pos >= 4) 
		{ 
			// load the filter data
			farray[i]=GET_INT(); 
			pos+=4; 
		}
	}
	if (farray[50]==0) farray[50] = 1;
	if (szFile[0] == NULL)
	{
		// load the last file name
		int i = 0;
		for (;len-pos > 0;pos++,i++)
		{
			szFile[i]=data[pos];
			lastszFile[i]=data[pos];
		}
		if (szFile[0] != NULL) FileBox.lpstrInitialDir = NULL;
		szFile[i] = NULL;		// add the terminating null byte 
		lastszFile[i] = NULL;	// add the terminating null byte 
	}
	updatedraw = true;
}

// write configuration to data, return length. config data should not exceed 64k.
#define PUT_INT(y) data[pos]=(y)&255; data[pos+1]=(y>>8)&255; data[pos+2]=(y>>16)&255; data[pos+3]=(y>>24)&255
int  C_CONVOLUTION::save_config(unsigned char *data) 
{
	int pos=0;
	PUT_INT((int)enabled);
	pos+=4;
	PUT_INT((int)wraparound);
	pos+=4;
	PUT_INT((int)absolute);
	pos+=4;
	PUT_INT((int)twopass);
	pos+=4;
	for (int i=0;i<51;i++)
	{
		PUT_INT(farray[i]);
		pos+=4;
	}
	for(int i=0;szFile[i] != NULL;pos++,i++) data[pos]=szFile[i];
	return pos;
}

#define appenddraw(a) { ((LPBYTE)draw)[codelength] = a; codelength++; }

void C_CONVOLUTION::createdraw(void)
{
	unsigned int possum = 0, negsum = 0;	// absolute sums of pos and neg values
	int zerostringl = 0;					// length of the initial run of zeroes
	int numpos = 0;							// number of pos entries
	int numneg = 0;							// number of neg entries
	bool zerostring = true;					// are we still in an initial zero run
	unsigned int fposdata[2][50][3];		// for each pass, will contain the x and y co-ords and the value of the pos data elements. e.g. farray[7*fposdata[pass][i][1]+fposdata[pass][i][0]]=fposdata[pass][i][2]>0
	unsigned int fnegdata[2][50][3];		// for each pass, will contain the x and y co-ords and the absolute value of the neg data elements. e.g. abs(farray[7*fnegdata[pass][i][1]+fnegdata[i][0]])=fnegdata[pass][i][2]>0
	unsigned char usefbout;					// 1 if need to use fbout
	int divisionfactor = farray[50];
	drawstate = 1;
	updatedraw = false;
	for (int i=0;i<50;i++)
	{
		if (farray[i]<0)
		{
			negsum -= farray[i];
			zerostring=false;
			m64farray[i] = (i!=49)?_mm_set1_pi16(((unsigned short) (-farray[i]))):_mm_set1_pi16(((unsigned short) (-256*farray[i])));
		}
		else if (farray[i]>0)
		{
			possum += farray[i];
			zerostring=false;
			m64farray[i] = (i!=49)?_mm_set1_pi16(((unsigned short) (farray[i]))):_mm_set1_pi16(((unsigned short) (256*farray[i])));
		}
		else
		{
			if (zerostring) zerostringl++;
			m64farray[i] = _mm_setzero_si64();
		}
	}
	_mm_empty();
	if (divisionfactor>0)					// no sign swapping necessary
	{
		for (int i=0;i<50;i++)
		{
			if (farray[i]<0)
			{
				if (i / 7 < 7)
				{
					fnegdata[0][numneg][0] = i % 7;
					fnegdata[0][numneg][1] = i / 7;
					fnegdata[0][numneg][2] = -farray[i];
					fnegdata[1][numneg][0] = 6 - i / 7;
					fnegdata[1][numneg][1] = i % 7;
					fnegdata[1][numneg][2] = -farray[i];
					numneg++;
				}
				else
				{
					fnegdata[0][numneg][0] = 0;
					fnegdata[0][numneg][1] = 7;
					fnegdata[0][numneg][2] = -farray[i];
					fnegdata[1][numneg][0] = 0;
					fnegdata[1][numneg][1] = 7;
					fnegdata[1][numneg][2] = -farray[i];
					numneg++;
				}
			}
			else if (farray[i]>0)
			{
				if (i / 7 < 7)
				{
					fposdata[0][numpos][0] = i % 7;
					fposdata[0][numpos][1] = i / 7;
					fposdata[0][numpos][2] = farray[i];
					fposdata[1][numpos][0] = 6 - i / 7;
					fposdata[1][numpos][1] = i % 7;
					fposdata[1][numpos][2] = farray[i];
					numpos++;
				}
				else
				{
					fposdata[0][numpos][0] = 0;
					fposdata[0][numpos][1] = 7;
					fposdata[0][numpos][2] = farray[i];
					fposdata[1][numpos][0] = 0;
					fposdata[1][numpos][1] = 7;
					fposdata[1][numpos][2] = farray[i];
					numpos++;
				}
			}
		}
	}
	else									// do sign swapping
	{
		for (int i=0;i<50;i++)
		{
			if (farray[i]<0)
			{
				if (i / 7 < 7)
				{
					fposdata[0][numpos][0] = i % 7;
					fposdata[0][numpos][1] = i / 7;
					fposdata[0][numpos][2] = -farray[i];
					fposdata[1][numpos][0] = 6 - i / 7;
					fposdata[1][numpos][1] = i % 7;
					fposdata[1][numpos][2] = -farray[i];
					numpos++;
				}
				else
				{
					fposdata[0][numpos][0] = 0;
					fposdata[0][numpos][1] = 7;
					fposdata[0][numpos][2] = -farray[i];
					fposdata[1][numpos][0] = 0;
					fposdata[1][numpos][1] = 7;
					fposdata[1][numpos][2] = -farray[i];
					numpos++;
				}
			}
			else if (farray[i]>0)
			{
				if (i / 7 < 7)
				{
					fnegdata[0][numneg][0] = i % 7;
					fnegdata[0][numneg][1] = i / 7;
					fnegdata[0][numneg][2] = farray[i];
					fnegdata[1][numneg][0] = 6 - i / 7;
					fnegdata[1][numneg][1] = i % 7;
					fnegdata[1][numneg][2] = farray[i];
					numneg++;
				}
				else
				{
					fnegdata[0][numneg][0] = 0;
					fnegdata[0][numneg][1] = 7;
					fnegdata[0][numneg][2] = farray[i];
					fnegdata[1][numneg][0] = 0;
					fnegdata[1][numneg][1] = 7;
					fnegdata[1][numneg][2] = farray[i];
					numneg++;
				}
			}
		}
		divisionfactor=-divisionfactor;
	}
	if (zerostringl<24) usefbout = 1;
	else usefbout = 0;
	draw = (FunctionType) new BYTE[MAX_DRAW_SIZE];
	if (enabled)
	{
		unsigned int iloopstart,jloopstart;	// addresses to loop back to
		codelength = 0;
		// function header (nonstandard stack - see stack.txt)
		appenddraw(0x53)	// push ebx
		appenddraw(0x56)	// push esi
		appenddraw(0x57)	// push edi
		appenddraw(0x55)	// push ebp
		appenddraw(0x8B)	// mov ebp, esp
		appenddraw(0xEC)
		// throughout this code the coments will refference i, j, k and c[.].
		// see prog.txt to see the original non-optimised code to which they (approximiately) refer.
		// also see registers.txt for correspondances between the registers here and the vars in prog.txt.
		// double for loop header j external loop, i internal loop, k internal non reset
		// int j = 0;
		appenddraw(0x33)	// xor ebx, ebx
		appenddraw(0xDB)
		appenddraw(0x8B)	// mov esi, ebx (esi will be permanently used for j)
		appenddraw(0xF3)
		// int *k = (usefbout)?fbout:framebuffer - this var will serve as a pointer to the address to output to
		appenddraw(0x8B)	// mov ebx, [ebp+(usefbout)?24:20]
		appenddraw(0x5D)
		appenddraw((usefbout)?0x18:0x14)
		appenddraw(0x8B)	// mov edx, ebx (edx will be permanently used for k)
		appenddraw(0xD3)
		// set up ecx as pointer to farray
		appenddraw(0x8B)	// mov ecx, [ebp+28] (e.g. mov ecx, m64farray)
		appenddraw(0x4D)
		appenddraw(0x1C)
		// c[0] = framebuffer;
		appenddraw(0x8B)	// mov ebx, [ebp+20]
		appenddraw(0x5D)
		appenddraw(0x14)
		appenddraw(0x53)	// push ebx
		// c[1] = framebuffer;
		appenddraw(0x53)	// push ebx
		// c[2] = framebuffer;
		appenddraw(0x53)	// push ebx
		// c[3] = framebuffer;
		appenddraw(0x53)	// push ebx
		// c[4] = framebuffer+4*width;
		appenddraw(0x81)	// add ebx, 4*width
		appenddraw(0xC3)
		// 4*width needs to begin with the lowest byte
		appenddraw((4*width)&0x000000FF)
		appenddraw(((4*width)&0x0000FF00)>>8)
		appenddraw(((4*width)&0x00FF0000)>>16)
		appenddraw(((4*width)&0xFF000000)>>24)
		appenddraw(0x53)	// push ebx
		// c[5] = framebuffer+8*width;
		appenddraw(0x81)	// add ebx, 4*width
		appenddraw(0xC3)
		// 4*width needs to begin with the lowest byte
		appenddraw((4*width)&0x000000FF)
		appenddraw(((4*width)&0x0000FF00)>>8)
		appenddraw(((4*width)&0x00FF0000)>>16)
		appenddraw(((4*width)&0xFF000000)>>24)
		appenddraw(0x53)	// push ebx
		// c[6] = framebuffer+12*width;
		appenddraw(0x81)	// add ebx, 4*width
		appenddraw(0xC3)
		// 4*width needs to begin with the lowest byte
		appenddraw((4*width)&0x000000FF)
		appenddraw(((4*width)&0x0000FF00)>>8)
		appenddraw(((4*width)&0x00FF0000)>>16)
		appenddraw(((4*width)&0xFF000000)>>24)
		appenddraw(0x53)	// push ebx
		// note: c[6] is at esp, c[5] is at esp+4, c[4] is at esp+8,c[3] is at esp+12,c[2] is at esp+16,c[1] is at esp+20,c[0] is at esp+24
		appenddraw(0x0F)	// pxor	mm7, mm7 - mm7 will be permanently set to 0
		appenddraw(0xEF)
		appenddraw(0xFF)
		// start of j loop
		jloopstart = codelength;
		// int i = 0;
		appenddraw(0x33)	// xor edi, edi - this register will be permanently assigned to i
		appenddraw(0xFF)
		// start of i loop
		for (int im=0;im<7;im++)	// when im=0 this will deal with the i=0 case, im=1, i=1, im=2, i=2, im=3, 2<i<width-3, im=4, i=width-3, im=5, i=width-2, im=6, i=width-1.
		{
			iloopstart = codelength;
			for (int pass=0;pass<(twopass?2:1);pass++)
			{
				int lasty = -1;						// the last f***data[i][1] looked at
				appenddraw(0x0F)	// movq mm0, mm7
				appenddraw(0x7F)
				appenddraw(0xF8)
				if (numneg>0)
				{
					appenddraw(0x0F)	// movq mm1, mm7
					appenddraw(0x7F)
					appenddraw(0xF9)
				}
				// go through the pos values
				for (int i=0; i<numpos; i++)
				{	
					if (fposdata[pass][i][1]==7)	// see if this the bias entry
					{
						appenddraw(0x0F)	// paddusw mm0, [ecx+49*8] (e.g. paddw mm0, m64farray[49])
						appenddraw(0xDD)
						appenddraw(0x81)
						appenddraw(0x88)
						appenddraw(0x01)
						appenddraw(0x00)
						appenddraw(0x00)
					}
					else
					{
						if (lasty!=fposdata[pass][i][1])	//see if we actually need to adjust the value of eax (the start of row address)
						{
							appenddraw(0x8B)	// mov eax, [esp+4*(6-fposdata[pass][i][1])] (e.g. mov eax, c[fposdata[pass][i][1]])
							appenddraw(0x44)
							appenddraw(0x24)
							appenddraw((4*(6-fposdata[pass][i][1]))&0xFF)
							lasty = fposdata[pass][i][1];
						}
						switch (im)
						{
						case 0:
							appenddraw(0x0F)	// movq mm2, [eax+max(4*(fposdata[pass][i][0]-3),0)]
							appenddraw(0x6F)
							appenddraw(0x50)
							appenddraw((max((signed)(4*(fposdata[pass][i][0]-3)),0))&0xFF)
							break;
						case 1:
							appenddraw(0x0F)	// movq mm2, [eax+max(4*(fposdata[pass][i][0]-2),0)]
							appenddraw(0x6F)
							appenddraw(0x50)
							appenddraw((max((signed)(4*(fposdata[pass][i][0]-2)),0))&0xFF)
							break;
						case 2:
							appenddraw(0x0F)	// movq mm2, [eax+max(4*(fposdata[pass][i][0]-1),0)]
							appenddraw(0x6F)
							appenddraw(0x50)
							appenddraw((max((signed)(4*(fposdata[pass][i][0]-1)),0))&0xFF)
							break;
						case 3:
							appenddraw(0x0F)	// movq mm2, [eax+edi+4*(fposdata[pass][i][0]-3)]
							appenddraw(0x6F)
							appenddraw(0x54)
							appenddraw(0x38)
							appenddraw((4*(fposdata[pass][i][0]-3))&0xFF)
							break;
						case 4:
							{
								int xpos;
								appenddraw(0x0F)	// movq mm2, [eax+4*min(fposdata[pass][i][0]-6+width,width-2)]
								appenddraw(0x6F)
								appenddraw(0x90)
								xpos = (4*min((unsigned)(fposdata[pass][i][0]-6+width),(unsigned)(width-2)));
								// xpos needs to begin with the lowest byte
								appenddraw(xpos&0x000000FF)
								appenddraw((xpos&0x0000FF00)>>8)
								appenddraw((xpos&0x00FF0000)>>16)
								appenddraw((xpos&0xFF000000)>>24)
								break;
							}
						case 5:
							{
								int xpos;
								appenddraw(0x0F)	// movq mm2, [eax+4*min(fposdata[pass][i][0]-5+width,width-2)]
								appenddraw(0x6F)
								appenddraw(0x90)
								xpos = (4*min((unsigned)(fposdata[pass][i][0]-5+width),(unsigned)(width-2)));
								// xpos needs to begin with the lowest byte
								appenddraw(xpos&0x000000FF)
								appenddraw((xpos&0x0000FF00)>>8)
								appenddraw((xpos&0x00FF0000)>>16)
								appenddraw((xpos&0xFF000000)>>24)
								break;
							}
						case 6:
							{
								int xpos;
								appenddraw(0x0F)	// movq mm2, [eax+4*min(fposdata[pass][i][0]-4+width,width-2)]
								appenddraw(0x6F)
								appenddraw(0x90)
								xpos = (4*min((unsigned)(fposdata[pass][i][0]-4+width),(unsigned)(width-2)));
								// xpos needs to begin with the lowest byte
								appenddraw(xpos&0x000000FF)
								appenddraw((xpos&0x0000FF00)>>8)
								appenddraw((xpos&0x00FF0000)>>16)
								appenddraw((xpos&0xFF000000)>>24)
								break;
							}
						}
 						appenddraw(0x0F)	// punpcklbw mm2, mm7
						appenddraw(0x60)
						appenddraw(0xD7)
						if (fposdata[pass][i][2]!=1)	// in which case we can just add
						{	// just add
							// see if fposdata[pass][i][2] is a power of 2
							unsigned int j;
							unsigned char k=0;
							for (j=1;j<fposdata[pass][i][2];j<<=1) k++;
							if (j==fposdata[pass][i][2])
							{	// fposdata[pass][i][2]=2^k
	 							appenddraw(0x0F)	// psllw mm2, k
								appenddraw(0x71)
								appenddraw(0xF2)
								appenddraw(k)
							}
							else
							{
	 							appenddraw(0x0F)	// pmullw mm2, [ecx+8*(7*fposdata[pass][i][1]+fposdata[pass][i][0])] (e.g. pmullw mm2, m64farray[7*fposdata[pass][i][1]+fposdata[pass][i][0]])
								appenddraw(0xD5)
								appenddraw(0x91)
								// 8*(7*fposdata[pass][i][1]+fposdata[pass][i][0]) needs to begin with the lowest byte
								appenddraw((8*(7*fposdata[pass][i][1]+fposdata[pass][i][0]))&0x000000FF)
								appenddraw(((8*(7*fposdata[pass][i][1]+fposdata[pass][i][0]))&0x0000FF00)>>8)
								appenddraw(((8*(7*fposdata[pass][i][1]+fposdata[pass][i][0]))&0x00FF0000)>>16)
								appenddraw(((8*(7*fposdata[pass][i][1]+fposdata[pass][i][0]))&0xFF000000)>>24)
							}
						}
						if (possum<256)
						{
							appenddraw(0x0F)	// paddw mm0, mm2
							appenddraw(0xFD)
							appenddraw(0xC2)
						}
						else
						{
							appenddraw(0x0F)	// paddw mm0, mm2
							appenddraw(0xDD)
							appenddraw(0xC2)
						}
					}
				}
				// go through the neg values
				lasty = -1;
				for (int i=0; i<numneg; i++)
				{	
					if (fnegdata[pass][i][1]==7)	// see if this the bias entry
					{
						appenddraw(0x0F)	// paddw mm1, [ecx+49*8] (e.g. paddw mm0, m64farray[49])
						appenddraw(0xDD)
						appenddraw(0x89)
						appenddraw(0x88)
						appenddraw(0x01)
						appenddraw(0x00)
						appenddraw(0x00)
					}
					else
					{
						if (lasty!=fnegdata[pass][i][1])	//see if we actually need to adjust the value of eax (the start of row address)
						{
							appenddraw(0x8B)	// mov eax, [esp+4*(6-fnegdata[pass][i][1])] (e.g. mov eax, c[fnegdata[pass][i][1]])
							appenddraw(0x44)
							appenddraw(0x24)
							appenddraw((4*(6-fnegdata[pass][i][1]))&0xFF)
							lasty = fnegdata[pass][i][1];
						}
						switch (im)
						{
						case 0:
							appenddraw(0x0F)	// movq mm2, [eax+max(4*(fnegdata[pass][i][0]-3),0)]
							appenddraw(0x6F)
							appenddraw(0x50)
							appenddraw((max((signed)(4*(fnegdata[pass][i][0]-3)),0))&0xFF)
							break;
						case 1:
							appenddraw(0x0F)	// movq mm2, [eax+max(4*(fnegdata[pass][i][0]-2),0)]
							appenddraw(0x6F)
							appenddraw(0x50)
							appenddraw((max((signed)(4*(fnegdata[pass][i][0]-2)),0))&0xFF)
							break;
						case 2:
							appenddraw(0x0F)	// movq mm2, [eax+max(4*(fnegdata[pass][i][0]-1),0)]
							appenddraw(0x6F)
							appenddraw(0x50)
							appenddraw((max((signed)(4*(fnegdata[pass][i][0]-1)),0))&0xFF)
							break;
						case 3:
							appenddraw(0x0F)	// movq mm2, [eax+edi+4*(fnegdata[pass][i][0]-3)]
							appenddraw(0x6F)
							appenddraw(0x54)
							appenddraw(0x38)
							appenddraw((4*(fnegdata[pass][i][0]-3))&0xFF)
							break;
						case 4:
							{
								int xpos;
								appenddraw(0x0F)	// movq mm2, [eax+4*min(fnegdata[pass][i][0]-6+width,width-2)]
								appenddraw(0x6F)
								appenddraw(0x90)
								xpos = (4*min((unsigned)(fnegdata[pass][i][0]-6+width),(unsigned)(width-2)));
								// xpos needs to begin with the lowest byte
								appenddraw(xpos&0x000000FF)
								appenddraw((xpos&0x0000FF00)>>8)
								appenddraw((xpos&0x00FF0000)>>16)
								appenddraw((xpos&0xFF000000)>>24)
								break;
							}
						case 5:
							{
								int xpos;
								appenddraw(0x0F)	// movq mm2, [eax+4*min(fnegdata[pass][i][0]-5+width,width-2)]
								appenddraw(0x6F)
								appenddraw(0x90)
								xpos = (4*min((unsigned)(fnegdata[pass][i][0]-5+width),(unsigned)(width-2)));
								// xpos needs to begin with the lowest byte
								appenddraw(xpos&0x000000FF)
								appenddraw((xpos&0x0000FF00)>>8)
								appenddraw((xpos&0x00FF0000)>>16)
								appenddraw((xpos&0xFF000000)>>24)
								break;
							}
						case 6:
							{
								int xpos;
								appenddraw(0x0F)	// movq mm2, [eax+4*min(fnegdata[pass][i][0]-4+width,width-2)]
								appenddraw(0x6F)
								appenddraw(0x90)
								xpos = (4*min((unsigned)(fnegdata[pass][i][0]-4+width),(unsigned)(width-2)));
								// xpos needs to begin with the lowest byte
								appenddraw(xpos&0x000000FF)
								appenddraw((xpos&0x0000FF00)>>8)
								appenddraw((xpos&0x00FF0000)>>16)
								appenddraw((xpos&0xFF000000)>>24)
								break;
							}
						}
 						appenddraw(0x0F)	// punpcklbw mm2, mm7
						appenddraw(0x60)
						appenddraw(0xD7)
						if (fnegdata[pass][i][2]!=1)	// in which case we can just add
						{	// just add
							// see if fnegdata[pass][i][2] is a power of 2
							unsigned int j;
							unsigned char k=0;
							for (j=1;j<fnegdata[pass][i][2];j<<=1) k++;
							if (j==fnegdata[pass][i][2])
							{	// fnegdata[pass][i][2]=2^k
	 							appenddraw(0x0F)	// psllw mm2, k
								appenddraw(0x71)
								appenddraw(0xF2)
								appenddraw(k)
							}
							else
							{
	 							appenddraw(0x0F)	// pmullw mm2, [ecx+8*(7*fnegdata[pass][i][1]+fnegdata[pass][i][0])] (e.g. pmullw mm2, m64farray[7*fnegdata[pass][i][1]+fnegdata[pass][i][0]])
								appenddraw(0xD5)
								appenddraw(0x91)
								// 8*(7*fnegdata[pass][i][1]+fnegdata[pass][i][0]) needs to begin with the lowest byte
								appenddraw((8*(7*fnegdata[pass][i][1]+fnegdata[pass][i][0]))&0x000000FF)
								appenddraw(((8*(7*fnegdata[pass][i][1]+fnegdata[pass][i][0]))&0x0000FF00)>>8)
								appenddraw(((8*(7*fnegdata[pass][i][1]+fnegdata[pass][i][0]))&0x00FF0000)>>16)
								appenddraw(((8*(7*fnegdata[pass][i][1]+fnegdata[pass][i][0]))&0xFF000000)>>24)
							}
						}
						if (negsum<256)
						{
							appenddraw(0x0F)	// paddw mm1, mm2
							appenddraw(0xFD)
							appenddraw(0xCA)
						}
						else
						{
							appenddraw(0x0F)	// paddw mm1, mm2
							appenddraw(0xDD)
							appenddraw(0xCA)
						}
					}					
				}
				// do pos take neg
				if (numneg>0)
				{
					if (absolute)
					{
						appenddraw(0x0F)	// psubsw mm0, mm1
						appenddraw(0xE9)
						appenddraw(0xC1)
						// calculate absolute values
						appenddraw(0x0F)	// psllw mm0, 1
						appenddraw(0x71)
						appenddraw(0xF0)
						appenddraw(0x01)
						appenddraw(0x0F)	// psrlw mm0, 1
						appenddraw(0x71)
						appenddraw(0xD0)
						appenddraw(0x01)
					}
					else
					{
						if (wraparound)
						{
							appenddraw(0x0F)	// psubw mm0, mm1
							appenddraw(0xF9)
							appenddraw(0xC1)
						}
						else
						{
							appenddraw(0x0F)	// psubusw mm0, mm1
							appenddraw(0xD9)
							appenddraw(0xC1)
						}
					}
				}
				if (twopass&&pass==0)
				{
					appenddraw(0x0F)	// movq mm3, mm0
					appenddraw(0x7F)
					appenddraw(0xC3)
				}
			}	// end of pass for
			if (twopass)
			{
				appenddraw(0x0F)	// psubusw mm0, mm3
				appenddraw(0xDD)
				appenddraw(0xC3)
			}
			// scale
			if (divisionfactor!=1)
			{
				int j;
				// see if divisionfactor is a power of 2
				unsigned char k=0;
				for (j=1;j<divisionfactor;j<<=1) k++;
				if (j==divisionfactor)
				{	// divisionfactor=2^k
  					appenddraw(0x0F)	// psrlw mm0, k
					appenddraw(0x71)
					appenddraw(0xD0)
					appenddraw(k)
				}
				else
				{
  					appenddraw(0x0F)	// movq [esp-8], mm0
					appenddraw(0x7F)
					appenddraw(0x44)
					appenddraw(0x24)
					appenddraw(0xF8)
  					appenddraw(0x66)	// mov [esp-10], ax
					appenddraw(0x89)
					appenddraw(0x44)
					appenddraw(0x24)
					appenddraw(0xF6)
  					appenddraw(0x66)	// mov [esp-12], dx
					appenddraw(0x89)
					appenddraw(0x54)
					appenddraw(0x24)
					appenddraw(0xF4)
  					appenddraw(0xBB)	// mov mov ebx, 65536/divisionfactor
					// 65536/divisionfactor needs to begin with the lowest byte
					appenddraw((65536/divisionfactor)&0x000000FF)
					appenddraw(((65536/divisionfactor)&0x0000FF00)>>8)
					appenddraw(((65536/divisionfactor)&0x00FF0000)>>16)
					appenddraw(((65536/divisionfactor)&0xFF000000)>>24)
  					appenddraw(0x66)	// mov dx, bx
					appenddraw(0x8B)
					appenddraw(0xD3)
  					appenddraw(0x66)	// mov ax, [esp-8]
					appenddraw(0x8B)
					appenddraw(0x44)
					appenddraw(0x24)
					appenddraw(0xF8)
  					appenddraw(0x66)	// mul dx
					appenddraw(0xF7)
					appenddraw(0xE2)
  					appenddraw(0x66)	// mov [esp-8], dx
					appenddraw(0x89)
					appenddraw(0x54)
					appenddraw(0x24)
					appenddraw(0xF8)
  					appenddraw(0x66)	// mov dx, bx
					appenddraw(0x8B)
					appenddraw(0xD3)
  					appenddraw(0x66)	// mov ax, [esp-6]
					appenddraw(0x8B)
					appenddraw(0x44)
					appenddraw(0x24)
					appenddraw(0xFA)
  					appenddraw(0x66)	// mul dx
					appenddraw(0xF7)
					appenddraw(0xE2)
  					appenddraw(0x66)	// mov [esp-6], dx
					appenddraw(0x89)
					appenddraw(0x54)
					appenddraw(0x24)
					appenddraw(0xFA)
  					appenddraw(0x66)	// mov dx, bx
					appenddraw(0x8B)
					appenddraw(0xD3)
  					appenddraw(0x66)	// mov ax, [esp-4]
					appenddraw(0x8B)
					appenddraw(0x44)
					appenddraw(0x24)
					appenddraw(0xFC)
  					appenddraw(0x66)	// mul dx
					appenddraw(0xF7)
					appenddraw(0xE2)
  					appenddraw(0x66)	// mov [esp-4], dx
					appenddraw(0x89)
					appenddraw(0x54)
					appenddraw(0x24)
					appenddraw(0xFC)
  					appenddraw(0x66)	// mov dx, [esp-12]
					appenddraw(0x8B)
					appenddraw(0x54)
					appenddraw(0x24)
					appenddraw(0xF4)
  					appenddraw(0x66)	// mov ax, [esp-10]
					appenddraw(0x8B)
					appenddraw(0x44)
					appenddraw(0x24)
					appenddraw(0xF6)
  					appenddraw(0x0F)	// movq mm0, [esp-8]
					appenddraw(0x6F)
					appenddraw(0x44)
					appenddraw(0x24)
					appenddraw(0xF8)
				}
			}
			// output
			appenddraw(0x0F)	// packuswb mm0, mm7
			appenddraw(0x67)
			appenddraw(0xC7)
			appenddraw(0x0F)	// movd [edx], mm0
			appenddraw(0x7E)
			appenddraw(0x02)
			// increase k
			appenddraw(0x83)	// add edx, 4
			appenddraw(0xC2)
			appenddraw(0x04)
			// i "next" block
			if (im<4)
			{
				appenddraw(0x83)	// add edi, 4
				appenddraw(0xC7)
				appenddraw(0x04)
			}
			if (im==3)
			{
				appenddraw(0x81)	// cmp edi, 4*(width-3)
				appenddraw(0xFF)
				// 4*(width-3) needs to begin with the lowest byte
				appenddraw((4*(width-3))&0x000000FF)
				appenddraw(((4*(width-3))&0x0000FF00)>>8)
				appenddraw(((4*(width-3))&0x00FF0000)>>16)
				appenddraw(((4*(width-3))&0xFF000000)>>24)
				// return to i "for". which jump to use depends on codelength
				if (iloopstart-codelength-2<-128)
				{	// can't use short jumps
					int jumpoffset;
					appenddraw(0x0F)	// jb near back to i loop start
					appenddraw(0x82)
					jumpoffset = iloopstart - (codelength + 4);	// codelength + 4 is the index of the next command
					// offset needs to begin with the lowest byte
					appenddraw(jumpoffset&0x000000FF)
					appenddraw((jumpoffset&0x0000FF00)>>8)
					appenddraw((jumpoffset&0x00FF0000)>>16)
					appenddraw((jumpoffset&0xFF000000)>>24)
				}
				else
				{	// can use short jumps.
					appenddraw(0x72)	// jb short back to i loop start
					appenddraw(((char)(iloopstart-(codelength+1))))	// codelength+1 is the indes of the next command 
				}
			}
		}
		// increment the line address array
		appenddraw(0x8B)		// mov ebx, [esp]
		appenddraw(0x1C)
		appenddraw(0x24)
		appenddraw(0x81)		// add ebx, 4*width
		appenddraw(0xC3)
		// 4*width needs to begin with the lowest byte
		appenddraw((4*width)&0x000000FF)
		appenddraw(((4*width)&0x0000FF00)>>8)
		appenddraw(((4*width)&0x00FF0000)>>16)
		appenddraw(((4*width)&0xFF000000)>>24)
		appenddraw(0x8B)		// mov eax, framebuffer
		appenddraw(0x45)
		appenddraw(0x14)
		appenddraw(0x05)		// add eax, 4*width*(height-1)
		// 4*width*(height-1) needs to begin with the lowest byte
		appenddraw((4*width*(height-1))&0x000000FF)
		appenddraw(((4*width*(height-1))&0x0000FF00)>>8)
		appenddraw(((4*width*(height-1))&0x00FF0000)>>16)
		appenddraw(((4*width*(height-1))&0xFF000000)>>24)
		appenddraw(0x3B)		// cmp ebx, eax
		appenddraw(0xD8)
		appenddraw(0x75)		// jne 3 (e.g. to the push epx line)
		appenddraw(0x03)
		appenddraw(0x8B)		// mov ebx, [esp]
		appenddraw(0x1C)
		appenddraw(0x24)
		appenddraw(0x53)		// push ebx
		// note: relative to esp the addresses of c[0]-c[6] are the same.
		// this has effectively done c[0]=c[1]...c[5]=c[6],c[6]+=4*w,c[6]=min(c[6],address of the last row of franebuffer)
		// j next block
		appenddraw(0x46)	// inc esi
		appenddraw(0x81)	// cmp esi, height
		appenddraw(0xFE)
		// height needs to begin with the lowest byte
		appenddraw(height&0x000000FF)
		appenddraw((height&0x0000FF00)>>8)
		appenddraw((height&0x00FF0000)>>16)
		appenddraw((height&0xFF000000)>>24)
		// return to j "for". which jump to use depends on codelength
		if (jloopstart-codelength-2<-128)
		{	// can't use short jumps
			int jumpoffset;
			appenddraw(0x0F)	// jb near back to j loop start
			appenddraw(0x82)
			jumpoffset = jloopstart - (codelength + 4);	// codelength + 4 is the index of the next command
			// offset needs to begin with the lowest byte
			appenddraw(jumpoffset&0x000000FF)
			appenddraw((jumpoffset&0x0000FF00)>>8)
			appenddraw((jumpoffset&0x00FF0000)>>16)
			appenddraw((jumpoffset&0xFF000000)>>24)
		}
		else
		{	// can use short jumps.
			appenddraw(0x72)	// jb short back to j loop start
			appenddraw(((char)(jloopstart-(codelength+1))))	// codelength + 1 is the index of the next command
		}
		// clear mmx state
		appenddraw(0x0F)	// emms
		appenddraw(0x77)
		// function exit code
		appenddraw(0x8B)	// mov esp, ebp
		appenddraw(0xE5)
		appenddraw(0x5D)	// pop ebp
		appenddraw(0x5F)	// pop edi
		appenddraw(0x5E)	// pop esi
		appenddraw(0x5B)	// pop ebx
		appenddraw(0xB8)	// mov eax, usefbout
		appenddraw(usefbout)
		appenddraw(0x00)
		appenddraw(0x00)
		appenddraw(0x00)
		appenddraw(0xC3)	// ret
	}
	else
	{
		((LPBYTE)draw)[0] = 0x33;	// xor eax, eax
		((LPBYTE)draw)[1] = 0xC0;
		((LPBYTE)draw)[2] = 0xC3;	// ret
		codelength = 3;
	}
	drawstate = 2;
}

// export stuff
C_RBASE *R_Convolution(char *desc) // creates a new effect object if desc is NULL, otherwise fills in desc with description
{
	if (desc)
	{
		strcpy(desc,MOD_NAME);
		return NULL;
	}
	return (C_RBASE *) new C_CONVOLUTION();
}
