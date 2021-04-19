# vis_avs - Advanced Visualization Studio

## Modern Toolchain Build Effort

This fork is currently (2021) trying to make AVS build with **MingW-w64 GCC** as well as
a modern **MSVC**. The original source release needs an ancient MSVC 6<sup><sup>
[citation needed] </sup></sup> to build. Consequently, towards the final years of AVS
activity, there were only a perceived handful of people who could still get the code to
compile.

So the primary concern today is getting a **v2.81d**-compliant *vis_avs.dll* version
that can be built with a modern toolchain. Any fixes or improvements to AVS will be held
off until after that goal is achieved.


### Current Status

* 🎉 Builds with MinGW-w64 GCC into a running `vis_avs.dll`, loadable with Winamp.
* 💃 Runs most Winamp2 newpicks! (See below.)
* ❤️ ConvolutionFilter integrated as a builtin-APE thanks to a [donation to free software
  ](https://github.com/tholden/AVSConvolutionFilter) by its author [Tom Holden](
  https://github.com/tholden).
* 🥵 Performance is not up to par with official build (about 20% slower). Looking into
  that...
* 🧨 APE files are crashing AVS or preventing it from starting. Disable these by
  renaming .ape files to some other extension for now.
  * Fyrewurx APE crashes AVS when loaded in a preset.
  * Colormap APE prevents AVS from starting ("Plugin executed illegal operation")

### Building on Linux

First, install a 32bit MingW-w64 GCC (with C++ support) and and a cross-compiler CMake.

Choose your Linux distro, and run the respective commands to install the build dependencies and cross-compile AVS:

<details><summary><em>Fedora / RedHat</em></summary>

```shell
sudo dnf install mingw32-gcc-c++ mingw32-gcc
mkdir -p build
cd build
mingw32-cmake ..
make
```
    
 </details>

<details><summary><em>Archlinux</em></summary>

```shell
sudo pacman -S --needed mingw-w64-gcc mingw-w64-cmake
mkdir -p build
cd build
i686-w64-mingw32-cmake ..
make
```

 </details>


```shell
mkdir build
cd build

# Edit CMake-MingWcross-toolchain.txt with your cross-compile toolchain paths. On many
# Linux distros these are the same or similar and nothing or not much needs to change.
$EDITOR CMake-MingWcross-toolchain.txt

# Use your cross-compile-toolchain cmake here. (The precise name for the cmake binary
# might be different on your Linux distro.)
i686-w64-mingw32-cmake -D CMAKE_TOOLCHAIN_FILE=../CMake-MingWcross-toolchain.txt ..
# in fedora:
# mingw32-cmake -D CMAKE_TOOLCHAIN_FILE=../CMake-MingWcross-toolchain.txt ..

# Compile
make

# Copy vis_avs.dll into a Winamp/Plugins/ folder and the rest of the DLLs mentioned in
# CMake-MingWcross-toolchain.txt into the base Winamp/ folder.
cp vis_avs.dll /my/path/to/Winamp/Plugins/
cp lib*.dll /my/path/to/Winamp/  # <- You only need copy these this one the first time.

# Run Winamp
wine /my/path/to/Winamp/winamp.exe
```


### Conventions

There is room for improvements in the code, but the first step should be replicating the
behavior of v2.81d as well as possible. If going through the code reveals areas of
possible improvements, then mark them with a TODO comment, possibly using a secondary
flag to categorize the suggestion:

| tag | intent |
|-----|--------|
|`// TODO [cleanup]: <comment>`    |cleaner or more readable code is possible here   |
|`// TODO [bugfix]: <comment>`     |more stable or less buggy code is possible here  |
|`// TODO [performance]: <comment>`|the performance of AVS could be better here      |
|`// TODO [feature]: <comment>`    |a new capability of AVS could be implemented here|

Arguably cleanup could happen right away (and sometimes it does), but in the archival
spirit of the project it's only done sparingly up until v2.81d compliance.


<sub>Upstream README below</sub>

---

## Description

Advanced Visualization Studio (AVS), is a music visualization plugin for Winamp. It was designed by Winamp's creator, Justin Frankel. AVS has a customizable design which allows users to create their own visualization effects, or "presets". AVS was made open source software in May 2005, released under a BSD-style license. —[Wikipedia](http://en.wikipedia.org/wiki/Advanced_Visualization_Studio)

## Notes

What follows is a reformatted copy from the original `readme.txt` which came with the source code.

> [DrO](http://forums.winamp.com/member.php?s=&action=getinfo&userid=122037) has updated v2.81b to v2.81d by changing a few files, as he posted [here](http://forums.winamp.com/showthread.php?postid=2054764#post2054764)  
>
> This file you have here is the sourcecode from [nullsoft.com](http://www.nullsoft.com/free/avs/) (v2.81b), with the few files from [vis_avs_changed.zip](http://www.nunzioweb.com/daz/temp/avs/vis_avs_changed.zip) (2.81d) overwritten over v2.81b  >
> Since DrO's download location sais that it's temporary, I've put this file up as a backup for his server and to have a complete file.  
>
> If you have questions or comments, the AVS forums are over [here](http://forums.winamp.com/forumdisplay.php?s=&forumid=85)  
>
> Keep in mind though, that there is currently only little, if any, development going on over there. If you have suggestions, your best chances are to make a start and post your results there with your questions, rather than to ask 'us' to do it, as none of  us are Nullsoft employees.  
>
> You'll also need the Winamp SDK. Search the forums for the lastest version.  
>
> Greetings,  
> 'Warrior of the Light'  
> http://Warrior-of-the-Light.net  
> [email hidden]

## License

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
