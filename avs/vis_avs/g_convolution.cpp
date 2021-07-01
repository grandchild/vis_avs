#include "g__lib.h"
#include "g__defs.h"
#include "c_convolution.h"
#include "resource.h"
#include <windows.h>


int win32_dlgproc_convolution(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
	C_CONVOLUTION* g_Filter = (C_CONVOLUTION*)g_current_render;
	char value[16];
	int val;
	unsigned int objectcode, objectmessage;
	HWND hwndEdit;
	OPENFILENAME file_box;
	HANDLE filehandle;
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
			if ((objectcode == IDC_CONVOLUTION_BUTTON3 || objectcode == IDC_CONVOLUTION_BUTTON2) && objectmessage == BN_CLICKED) {
				// file dialogue initialisation.
				file_box.lStructSize = sizeof(OPENFILENAME);
				file_box.hwndOwner = hwndDlg;
				file_box.hInstance = NULL;
				file_box.lpstrFilter = "Convolution Filter File (*.cff)\0*.cff\0All Files (*.*)\0*.*\0";
				file_box.lpstrCustomFilter = NULL;
				file_box.nMaxCustFilter = 0;
				file_box.nFilterIndex = 1;
				file_box.nMaxFile = sizeof(g_Filter->szFile);
				file_box.lpstrFileTitle = NULL;
				file_box.nMaxFileTitle = 0;
				file_box.lpstrInitialDir = NULL;
				file_box.lpstrFile = g_Filter->szFile;
				file_box.lpstrTitle = NULL;
				file_box.lpfnHook = NULL;
				file_box.lpstrDefExt = "cff";
				// see if the load button's been clicked
				if (objectcode == IDC_CONVOLUTION_BUTTON3)
				{
					file_box.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | 0x02000000 | OFN_HIDEREADONLY;	// 0x02000000 is OFN_DONTADDTORECENT for some reason the compiler is not recognising it.
					if (GetOpenFileName(&file_box))
					{
						filehandle = CreateFile(file_box.lpstrFile, GENERIC_READ, 0, (LPSECURITY_ATTRIBUTES) NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,(HANDLE) NULL);
						if (filehandle == INVALID_HANDLE_VALUE)
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
							file_box.lpstrInitialDir = NULL;
							if (ReadFile(filehandle, data, MAX_FILENAME_SIZE, &numbytesread, NULL)==0 && GetLastError()!=ERROR_HANDLE_EOF)
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
							if (CloseHandle(filehandle)==0)
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
				if (objectcode == IDC_CONVOLUTION_BUTTON2)
				{
					file_box.Flags = 0x02000000 | OFN_HIDEREADONLY;	// 0x02000000 is OFN_DONTADDTORECENT for some reason the compiler is not recognising it.
					if (GetSaveFileName(&(file_box)))
					{
						filehandle = CreateFile(file_box.lpstrFile, GENERIC_WRITE, 0, (LPSECURITY_ATTRIBUTES) NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL,(HANDLE) NULL);
						if ((filehandle == INVALID_HANDLE_VALUE) && (GetLastError()==ERROR_FILE_EXISTS) && (MessageBox(hwndDlg, "This file already exists. Do you want to overwrite it?", "convolution",  MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2|MB_APPLMODAL|MB_SETFOREGROUND)==IDYES))
							filehandle = CreateFile(file_box.lpstrFile, GENERIC_WRITE, 0, (LPSECURITY_ATTRIBUTES) NULL, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL,(HANDLE) NULL);
						if (filehandle == INVALID_HANDLE_VALUE)
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
							file_box.lpstrInitialDir = NULL;
							if (WriteFile(filehandle, data, numbytestowrite, &numbyteswritten, NULL)==0)
							{
								// display the error
								LPVOID lpMsgBuf;
								FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL);
								MessageBox( NULL, (LPCTSTR)lpMsgBuf, "Unexpected file error.", MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND );
								LocalFree( lpMsgBuf );
							}
							if (CloseHandle(filehandle)==0)
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

