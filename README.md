# vis_avs - Advanced Visualization Studio

Advanced Visualization Studio (AVS), is a music visualization plugin for Winamp. It was
designed by Winamp's creator, Justin Frankel, among others. AVS has a customizable
design which allows users to create their own visualization effects, or "presets". AVS
was made open source software in May 2005, released under a BSD-style license. —
[Wikipedia](http://en.wikipedia.org/wiki/Advanced_Visualization_Studio)


## Modern Toolchain Port

This fork is a **MingW-w64 GCC 8+** as well as **MSVC 19+** port of AVS.

The goal was to create a **v2.81d**-compliant *vis_avs.dll* version that can be built
with a modern toolchain, which was largely successful. One notable exception being that
APEs (<b>A</b>VS <b>P</b>lugin <b>E</b>ffects) cannot be loaded — so many effects are
integrated as builtin effects instead.


### Current Status

* 🎉 Builds with MinGW-w64 GCC into a running `vis_avs.dll`, loadable with Winamp.
* 💃 Runs most presets (unless they use rare APEs).
* ❤️ Sources of original effects integrated as builtin-APEs, thanks to donations to free
  software by their authors:
  * _ConvolutionFilter_, by [Tom Holden](https://github.com/tholden). Original
    [here](https://github.com/tholden/AVSConvolutionFilter).
  * _TexerII_, by [Steven Wittens](https://acko.net). Original committed to this repo
    [here](https://github.com/grandchild/vis_avs/commit/ddd97ba7).
  * _AVSTrans_, by [TomyLobo](https://github.com/TomyLobo). Original
    [here](https://github.com/TomyLobo/eeltrans). Refactored for this repo.
  * _PictureII_, by [Steven Wittens](https://acko.net).
* 🎂 Former plugin effects rewritten from scratch as builtin-APEs:
  * _Normalise_, originally by [TomyLobo](https://github.com/TomyLobo),
    [here](https://www.deviantart.com/tomylobo/art/Normalise-APE-10334263).
  * _Colormap_, originally by [Steven Wittens](https://acko.net).
  * _AddBorders_, originally by [Goebish](https://github.com/goebish).
  * _Triangle_, originally by [TomyLobo](https://github.com/TomyLobo).
  * _GlobalVariables_, originally by [Semi Essessi](https://github.com/semiessessi).
  * _MultiFilter_, originally by [Semi Essessi](https://github.com/semiessessi).
  * _FramerateLimiter_, originally by [Goebish](https://github.com/goebish).
* 🥵 Performance is generally the same as the official build, but can be a bit slower
  for some effects. A few effects have gained faster SSE3-enabled versions.
* 🧨 APE files are crashing AVS or preventing it from starting. Rename or remove .ape
  files for now.


### The Future

* 🌠 Recreate more APEs
* 🪓 Separate Windows UI code from the renderer
* 📟 Standalone port (probably SDL2-based)
* 🐧 Linux port
* ✅ Automated output testing
* 💾 JSON file format support

These are near-future goals, and most are tracked on the
[issues board](https://github.com/users/grandchild/projects/1). For further development
of AVS itself there are _many_ ideas for improvement and known bugs. Have a look at 
[`wishlist.txt`](wishlist.txt) for a list of issues and feature-requests, both old and
new.


## Building & Running on Linux

### Build

First, install a 32bit MingW-w64 GCC (with C++ support) and and a cross-compiler CMake.

Choose your Linux distro, and run the respective commands to install the build
dependencies and cross-compile AVS:

<details><summary><em>Archlinux / Manjaro</em></summary>

```shell
sudo pacman -S --needed mingw-w64-gcc mingw-w64-cmake
mkdir -p build
cd build
i686-w64-mingw32-cmake ..
make
```

</details>


<details><summary><em>Fedora / RedHat</em></summary>

```shell
sudo dnf install mingw32-gcc-c++ mingw32-gcc
mkdir -p build
cd build
mingw32-cmake ..
make
```

</details>


<details><summary><em>Debian / Ubuntu (& other distros)</em></summary>


```shell
sudo apt install gcc-mingw-w64 g++-mingw-w64 cmake
mkdir -p build
cd build

# Debian- and Ubuntu-based distros don't provide ready-made cross-compiling CMake
# packages, so you'll have to tell CMake about your toolchain.
cmake -D CMAKE_TOOLCHAIN_FILE=../CMake-MingWcross-toolchain.txt ..

make
```

</details>

### Run With Winamp

Once you've compiled `vis_avs.dll`, copy it (and some stdlib DLLs that got introduced by
MinGW) to the Winamp/Plugins folder and run it with Wine:

```bash
cp vis_avs.dll /my/path/to/Winamp/Plugins/

# Not all of these might exist on your system, that's okay, you can ignore errors for
# some of the files.
# You only need to copy these files once, they don't change.
cp /usr/i686-w64-mingw32/bin/lib{gcc_s_dw2-1,ssp-0,stdc++-6,winpthread-1}.dll /my/path/to/Winamp/

# Run Winamp
wine /my/path/to/Winamp/winamp.exe
```

## Building & Running on Windows

### Build

Open the folder with Visual Studio, it should automatically detect the CMake
configurations.

If you don't want to do this, there are some caveats to using CMake itself directly:

* Use `-DCMAKE_GENERATOR_PLATFORM=Win32` to get a 32-bit project. Nothing builds under
  64-bit with the MS compiler due to `__asm` blocks (yet).
* CMake doesn't rebuild the project very well if you don't clean out the generated files
  first. Visual Studio handles this for you.
* Using the CMake GUI is not recommended.

### Run With Winamp

If you properly installed Winamp2 with the installer the project import should pickup
the installation location for you and will copy the output `vis_avs.dll` into the
`Winamp/Plugins` directory for you. You can then:

* Start Winamp
* Debug > Attach to process (`Ctrl`+`Alt`+`P`), search for "winamp"
* Launch AVS by double-clicking the small oscilloscope/spectrum vis on the left of
  Winamp's main window


## Conventions

If going through the code reveals areas of possible improvements, mark them with a TODO
comment, possibly using a secondary flag to categorize the suggestion:

| tag | intent |
|-----|--------|
|`// TODO [cleanup]: <comment>`    |cleaner or more readable code is possible here   |
|`// TODO [bugfix]: <comment>`     |more stable or less buggy code is possible here  |
|`// TODO [performance]: <comment>`|the performance of AVS could be better here      |
|`// TODO [feature]: <comment>`    |a new capability of AVS could be implemented here|


## Notes

Thanks to _Warrior of the Light_ for assembling the source from various edits and patch
versions that floated around soon after the code publication.


## License

BSD-3, see [LICENSE.TXT](LICENSE.TXT).
