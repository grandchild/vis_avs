cmake_minimum_required(VERSION 3.10)

project(vis_avs VERSION 2.82)

include(CheckIncludeFiles)

file(GLOB SRC_FILES_NS_EEL
    avs/ns-eel/*.c
)
file(GLOB SRC_FILES_AVS_COMMON
    avs/vis_avs/audio.cpp
    avs/vis_avs/avs*.cpp
    avs/vis_avs/e_*.cpp
    avs/vis_avs/effect*.cpp
    avs/vis_avs/*.c
    avs/vis_avs/files.cpp
    avs/vis_avs/handles.cpp
    avs/vis_avs/image.cpp
    avs/vis_avs/instance.cpp
    avs/vis_avs/linedraw.cpp
    avs/vis_avs/matrix.cpp
    avs/vis_avs/render_context.cpp
    # avs/vis_avs/r_text.cpp
    avs/vis_avs/r_transition.cpp
    avs/vis_avs/text_win32.cpp
    avs/vis_avs/video.cpp
    avs/vis_avs/video_libav.cpp
    avs/platform.c
    avs/3rdparty/md_fft.cpp
)

file(GLOB SRC_FILES_VIS_AVS
    avs/vis_avs/g_*.cpp
    avs/vis_avs/bpm.cpp
    avs/vis_avs/cfgwin.cpp
    avs/vis_avs/draw.cpp
    avs/vis_avs/main.cpp
    avs/vis_avs/render.cpp
    avs/vis_avs/undo.cpp
    avs/vis_avs/util.cpp
    avs/vis_avs/wnd.cpp
    avs/vis_avs/*.rc
)
if(WIN32)
    list(APPEND SRC_FILES_AVS_COMMON
        avs/platform_win32.c
        avs/vis_avs/audio_in_windows_wasapi.cpp
    )
elseif(LINUX)
    list(APPEND SRC_FILES_AVS_COMMON
    )
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(
        /W4                 # Warning level 4 (maximum)
        /wd4458             # Ignore warning about overriding member with local
                            # parameter. Setters should be able to have a parameter
                            # named exactly the same as the member it's setting. The
                            # code style advocates for `this->` everywhere anyway.
        /MP                 # Multithreaded compilation
        /std:c++14          # Pin to C++14 features
        /Zc:__cplusplus     # Define the actual C++ standard in __cplusplus, instead of
                            # a backwards-compatible "199711L".
                            # https://devblogs.microsoft.com/cppblog/msvc-now-correctly-reports-__cplusplus/
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "8.0.0")
        message(SEND_ERROR
            "need GCC version >=8, for naked-function support for x86 target")
    endif()
    add_compile_options(
        -g
        -O2
        -m32                # Compile for 32-bit x86.
        -masm=intel         # Use Intel instead of AT&T inline assembly syntax.
        -march=native       # Convolution-Filter uses inlined mmintrin.h functions, and
                            # a pentium4 arch seems to be roughly the minimum for that.
        -fcf-protection=return # Don't emit endbr32/-64 instructions at the beginning of
                               # functions. ("return" value means: not "branch".)
                               # We want clean naked functions.
        # -flto               # Link-time optimization. No performance gain seen so far.
        # -fprofile-generate  # Profile-guided optimization, pass 1
        # -fprofile-use       # Profile-guided optimization, pass 2
        -Wall -Wextra
        -Wno-write-strings  # Don't complain when converting string literals to char*.
        # -fsanitize=address  # Detect memory leaks & stack corruption at runtime.
                            # Doesn't work with some inline asm at the moment.
        # -save-temps         # Save intermediate assembly files created during
                            # compilation for debugging. The .s files will be output
                            # into the base build directory.
    )
    # C++-specific flags
    set(AVS_CXX_COMPILER_FLAGS
        -std=c++14          # Pin to C++14 features
    )
    # (CMake generator expressions cannot be multiline, hence the AVS_CXX_COMPILE_FLAGS
    # variable in a foreach workaround.)
    foreach(FLAG ${AVS_CXX_COMPILER_FLAGS})
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:${FLAG}>)
    endforeach()
else() # TODO: Clang option
    message(SEND_ERROR "unsupported compiler ${CMAKE_CXX_COMPILER_ID}")
endif()

add_compile_definitions(
    AVS_MEGABUF_SUPPORT     # No reason to not enable this. Remove #ifdefs eventually.
    NSEEL_LOOPFUNC_SUPPORT  # No reason to not enable this. Remove #ifdefs eventually.
    WA2_EMBED               # Enabling this shows the window decorations for AVS, at the
                            # cost of making the window resize a bit choppier. (Winamp2
                            # restricted window dimensions to certain multiples).
    # NO_MMX                # Use (usually slower) C equivalents for MMX assembly.
                            # (Initially enabled for quick porting but should be
                            # removed, now that all MMX sections are ported.)
)

set(CMAKE_SHARED_LIBRARY_PREFIX "")  # Remove "lib" prefix from output to get original
                                     # filename "vis_avs.dll".

add_library(avs_common OBJECT
    ${SRC_FILES_NS_EEL}
    ${SRC_FILES_AVS_COMMON}
)

add_library(libavs SHARED $<TARGET_OBJECTS:avs_common>)

check_include_files(
    "libavcodec/avcodec.h;libavformat/avformat.h;libswscale/swscale.h"
    HAVE_FFMPEG
)
if(NOT HAVE_FFMPEG OR NO_FFMPEG)
    add_compile_definitions(NO_FFMPEG)
endif()

if(WIN32)
    target_sources(libavs PRIVATE libavs.def)  # The function export list.
    add_compile_definitions(
        _WIN32_WINNT=_WIN32_WINNT_VISTA     # Minimum WinNT version (needed for
                                            # GetTickCount64()).
        NOMINMAX                            # Don't define min()/max() in windows.h on
                                            # MSVC. Use our own functions everywhere.
    )
    add_library(vis_avs SHARED
        $<TARGET_OBJECTS:avs_common>
        ${SRC_FILES_VIS_AVS}
    )
    target_compile_definitions(vis_avs PUBLIC CAN_TALK_TO_WINAMP)

    target_link_libraries(vis_avs
        ddraw  # draw.cpp includes ddraw.h
               # https://docs.microsoft.com/en-us/windows/win32/api/ddraw/nf-ddraw-directdrawcreate
    )

    get_filename_component(WINAMP_DIR "[HKEY_CURRENT_USER\\SOFTWARE\\Winamp]" ABSOLUTE)
    if(WINAMP_DIR STREQUAL "")
        set(WINAMP_DIR "" CACHE PATH "Winamp installation directory (contains winamp.exe)." FORCE)
    endif()
    if(EXISTS "${WINAMP_DIR}")
        add_custom_command(TARGET vis_avs POST_BUILD
            COMMENT "Copy AVS .dll to Winamp plugins directory"
            COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:vis_avs>" "${WINAMP_DIR}/Plugins"
        )
    else()
        message(WARNING "Winamp installation directory ('${WINAMP_DIR}') not found or not set. Skipped creating debug task.")
    endif()

    #[[
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # These dependent DLLs are introduced by mingw32 and should be next to winamp.exe.
        configure_file(${CROSS_CC_PREFIX}/bin/libgcc_s_dw2-1.dll    . COPYONLY)
        configure_file(${CROSS_CC_PREFIX}/bin/libssp-0.dll          . COPYONLY)
        configure_file(${CROSS_CC_PREFIX}/bin/libstdc++-6.dll       . COPYONLY)
        configure_file(${CROSS_CC_PREFIX}/bin/libwinpthread-1.dll   . COPYONLY)
    endif()
    #]]#

    if(NOT MSVC)
        # googletest here because it doesn't work for MSVC
    endif()
    set(AVS_DYLIB_EXTENSION "dll")
elseif(LINUX)
    target_link_options(libavs PUBLIC -Wl,-m elf_i386)
    set(AVS_DYLIB_EXTENSION "so")
endif()

if(NOT MSVC)
    # # tests
    # if(WIN32 AND NOT CMAKE_CROSSCOMPILING)
    #     link_libraries(${GTEST_LIB_NAME})
    # else()
    #     link_libraries(gtest)
    # endif()
    # file(GLOB test_src test/*.cpp test/*.hpp)
    # add_executable(test test/test.cpp)
    # configure_file(${CROSS_CC_PREFIX}/bin/libgtest.dll . COPYONLY)
endif()
