cmake_minimum_required(VERSION 3.10)

project(vis_avs VERSION 2.82)

file(GLOB SRC_FILES_NS_EEL
    avs/ns-eel/*.c
)
file(GLOB SRC_FILES_VIS_AVS
    avs/vis_avs/*.cpp
    avs/vis_avs/*.c
    avs/vis_avs/*.rc
    avs/platform.c
)
set(SRC_FILES_VIS_AVS_TARGET
    avs/platform_win32.c
    libavs.def # The function export list.
)
set(SRC_FILES_VIS_AVS_COMPILER)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(
        /W4                 # Warning level 4 (maximum)
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

if(WIN32)
    add_compile_definitions(
        _WIN32_WINNT=_WIN32_WINNT_VISTA     # Minimum WinNT version (needed for
                                            # GetTickCount64()).
        NOMINMAX                            # Don't define min()/max() in windows.h on
                                            # MSVC. Use our own functions everywhere.
    )
    if(NOT MSVC)
        # googletest here because it doesn't work for MSVC
    endif()
else()
    # link_libraries(#[[...]])
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
add_library(vis_avs SHARED
    ${SRC_FILES_NS_EEL}
    ${SRC_FILES_VIS_AVS}
    ${SRC_FILES_VIS_AVS_TARGET}
    ${SRC_FILES_VIS_AVS_COMPILER}
)

target_link_libraries(vis_avs
    ddraw  # draw.cpp includes ddraw.h
           # https://docs.microsoft.com/en-us/windows/win32/api/ddraw/nf-ddraw-directdrawcreate
    vfw32  # r_avi.cpp includes vfw.h
           # https://docs.microsoft.com/en-us/windows/win32/multimedia/avifile-functions-and-macros
)

if(WIN32)
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
