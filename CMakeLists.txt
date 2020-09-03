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
        # -Wall -Wextra
        -Wno-write-strings
        -fpermissive        # -fpermissive needed for MAKEINTRESOURCE macro, which casts
                            # LPSTR to WORD, a.k.a. char* to short unsigned int
        -masm=intel
    )
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
    -DAVS_MEGABUF_SUPPORT   # No reason to not enable this -> the #ifdefs should
                            # probably be removed eventually.
    # -DNO_MMX              # Use (usually slower) C equivalents for MMX assembly.
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

set(CMAKE_SHARED_LIBRARY_PREFIX "")  # Removes "lib" prefix from output to achieve
                                     # original filename "vis_avs.dll"
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
