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
    endorse or promote products derived from this software without specific prior
    written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#ifndef _RLIB_H_
#define _RLIB_H_

#include "ape.h"

#define DLLRENDERBASE 16384

class C_RLibrary {
   protected:
    typedef struct {
        C_RBASE* (*rf)(char* desc);
        int is_r2;
    } rfStruct;
    rfStruct* RetrFuncs;

    int NumRetrFuncs;

    typedef struct {
        dlib_t* hDllInstance;
        char* idstring;
        C_RBASE* (*createfunc)(char* desc);
        int is_r2;
    } DLLInfo;

    DLLInfo* DLLFuncs;
    int NumDLLFuncs;

    void add_dofx(void* rf, int has_r2);
    void initfx(void);
    void initdll(void);
    void initbuiltinape(void);

   public:
    C_RLibrary();
    ~C_RLibrary();
    // if which is >= DLLRENDERBASE
    // returns "id" of DLL. which is used to enumerate. str is desc
    // otherwise, returns 1 on success, 0 on error
    C_RBASE* CreateRenderer(int* which, int* has_r2);
    dlib_t* GetRendererInstance(int which, dlib_t* hThisInstance);
    int GetRendererDesc(int which, char* str);
    void add_dll(void*,
                 class C_RBASE*(__cdecl*)(char*),
                 char*,
                 int,
                 void (*set_info)(APEinfo*));
};

#endif  // _RLIB_H_