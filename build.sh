
(rm -rf build && mkdir build && cd build && i686-w64-mingw32-cmake -D CMAKE_TOOLCHAIN_FILE=../CMake-MingWcross-toolchain.txt .. && make)
