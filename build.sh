
if [ ! -d build ]; then
    (rm -rf build && mkdir build && cd build && i686-w64-mingw32-cmake -D CMAKE_TOOLCHAIN_FILE=../CMake-MingWcross-toolchain.txt .. && make)
fi

(cd build \
    && make \
    && cp vis_avs.dll ../../Winamp2/Plugins/ \
    && WINEARCH=win32 WINEPREFIX=~/.wine-i686-w64-mingw32 \
        rlwrap -a -O 'Wine-gdb>' -i \
            --forget-matching "^.{1,2}$|^delete" \
            --histsize 1000000 \
            --history-filename ../winedbg_history \
        winedbg --gdb ../../Winamp2/winamp.exe \
)
