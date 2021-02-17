cmake_minimum_required(VERSION 3.10)

project(vis_avs VERSION 2.82)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/W4)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "8.0.0")
        message(SEND_ERROR
            "need GCC version >=8, for naked-function support for x86 target")
    endif()
    add_compile_options(
        -g
        -Wno-write-strings  # Don't complain when converting string literals to char*.
        -masm=intel         # Use Intel instead of AT&T inline assembly syntax.
        -march=native       # Convolution-Filter uses inlined mmintrin.h functions, and
                            # a pentium4 arch seems to be roughly the minimum for that.
        # -flto               # Link-time optimization. No performance gain seen so far.
        # -fprofile-generate  # Profile-guided optimization, pass 1
        # -fprofile-use       # Profile-guided optimization, pass 2
        # -Wall -Wextra
        # -save-temps         # Save intermediate assembly files created during
                            # compilation for debugging. The .s files will be output
                            # into the base build directory.
    )
    # C++-specific flags
    set(AVS_CXX_COMPILER_FLAGS
        -fpermissive        # -fpermissive needed for MAKEINTRESOURCE macro, which casts
                            # LPSTR to WORD, a.k.a. char* to short unsigned int.
    )
    # (CMake generator expressions cannot be multiline, hence the CXX_COMPILE_FLAGS
    # variable workaround.)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:${AVS_CXX_COMPILER_FLAGS}>)
else() # TODO: Clang option
    message(SEND_ERROR "unsupported compiler ${CMAKE_CXX_COMPILER_ID}")
endif()

if(WIN32)
    if(NOT CMAKE_CROSSCOMPILING)
        # windows libraries stuff

        if(NOT MSVC)
            # If using Googletest, don't allow it for MSVC because it doesn't work there
        endif()
    endif()
else()
    # link_libraries(#[[...]])
endif()

add_definitions(
    -DAVS_MEGABUF_SUPPORT     # No reason to not enable this. Remove #ifdefs eventually.
    -DNSEEL_LOOPFUNC_SUPPORT  # No reason to not enable this. Remove #ifdefs eventually.
    # -DNO_MMX                # Use (usually slower) C equivalents for MMX assembly.
                              # (Initially enabled for quick porting but should be
                              # removed, now that all MMX sections are ported.)
)

file(GLOB SRC_FILES_NS_EEL
    avs/ns-eel/*.c
)
file(GLOB SRC_FILES_VIS_AVS
    avs/vis_avs/*.cpp
    avs/vis_avs/*.c
    avs/vis_avs/*.rc
)
# This file appears to be an older version of r_contrast.cpp left in the source tree.
list(REMOVE_ITEM SRC_FILES_VIS_AVS "${CMAKE_SOURCE_DIR}/avs/vis_avs/r_colorreplace.cpp")

set(CMAKE_SHARED_LIBRARY_PREFIX "")  # Remove "lib" prefix from output to get original
                                     # filename "vis_avs.dll".
add_library(vis_avs SHARED
    ${SRC_FILES_NS_EEL}
    ${SRC_FILES_VIS_AVS}
    avs/gcc-hacks.c
)

target_link_libraries(vis_avs
    ddraw  # draw.cpp includes ddraw.h
           # https://docs.microsoft.com/en-us/windows/win32/api/ddraw/nf-ddraw-directdrawcreate
    vfw32  # r_avi.cpp includes vfw.h
           # https://docs.microsoft.com/en-us/windows/win32/multimedia/avifile-functions-and-macros
)

if(NOT MSVC)
    # # tests
    # if(WIN32 AND NOT CMAKE_CROSSCOMPILING)
    #     link_libraries(${GTEST_LIB_NAME})
    # else()
    #     link_libraries(gtest)
    # endif()
    # file(GLOB test_src test/*.cpp test/*.hpp)
    # add_executable(test test/test.cpp)
endif()
