cmake_minimum_required(VERSION 3.18)

project(vis_avs VERSION 2.82)

include(CheckIncludeFiles)
include(CheckLinkerFlag)
include(ExternalProject)
include(FindPkgConfig)
include(utils.cmake)

ensure_platform_vars()
ensure_compiler_vars()

file(GLOB SRC_FILES_EEL2
    avs/3rdparty/WDL-EEL2/eel2/nseel-*.c
)
file(GLOB SRC_FILES_AVS_COMMON
    avs/vis_avs/audio.cpp
    avs/vis_avs/avs*.cpp
    avs/vis_avs/blend.cpp
    avs/vis_avs/e_*.cpp
    avs/vis_avs/effect*.cpp
    avs/vis_avs/*.c
    avs/vis_avs/files.cpp
    avs/vis_avs/handles.cpp
    avs/vis_avs/image.cpp
    avs/vis_avs/instance.cpp
    avs/vis_avs/linedraw.cpp
    avs/vis_avs/matrix.cpp
    avs/vis_avs/preset_json_schema.cpp
    avs/vis_avs/render_context.cpp
    avs/vis_avs/r_text.cpp
    avs/vis_avs/r_transition.cpp
    avs/vis_avs/video.cpp
    avs/vis_avs/video_libav.cpp
    avs/platform.c
    avs/uuid.cpp
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
        avs/vis_avs/text_win32.cpp
    )
elseif(LINUX)
    list(APPEND SRC_FILES_AVS_COMMON
        avs/platform_linux.c
        avs/vis_avs/audio_in_linux_pipewire.cpp
        avs/vis_avs/audio_libpipewire.cpp
        avs/3rdparty/pevents.cpp
    )
endif()

if(MSVC)
    add_compile_options(
        /W4                 # Warning level 4 (maximum)
        /wd4458             # Ignore warning about overriding member with local
                            # parameter. Setters should be able to have a parameter
                            # named exactly the same as the member it's setting. The
                            # code style advocates for `this->` everywhere anyway.
        /wd4244             # Ignore warning about type conversions with data loss.
                            # They are simply too many at the moment and obscure other
                            # warnings. TODO[cleanup]: Reenable and fix warnings.
        /wd5051             # Ignore warning that [[fallthrough]] is ignored on C++<17.
                            # It's still useful for readability.
        /MP                 # Multithreaded compilation
        /std:c++14          # Pin to C++14 features
        /Zc:__cplusplus     # Define the actual C++ standard in __cplusplus, instead of
                            # a backwards-compatible "199711L".
                            # https://devblogs.microsoft.com/cppblog/msvc-now-correctly-reports-__cplusplus/
    )
elseif(GCC)
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
        -fvisibility=hidden # Hide all symbols by default, only export API symbols.
        # -flto               # Link-time optimization. No performance gain, but halves
                            # binary size.
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
    WA2_EMBED               # Enabling this shows the window decorations for AVS, at the
                            # cost of making the window resize a bit choppier. (Winamp2
                            # restricted window dimensions to certain multiples).
    # NO_MMX                # Use (usually slower) C equivalents for MMX assembly.
                            # (Initially enabled for quick porting but should be
                            # removed, now that all MMX sections are ported.)
    WFMO                    # Enable `WaitForMultipleObjects()` in pevents.cpp.
    _USE_MATH_DEFINES       # Let math.h declare M_PI.
    SIMD_MODE_X86_SSE       # Use SSE & SSSE3 SIMD optimizations where possible.
                            # Plain C alternatives are provided and used if unset.
)

set(CMAKE_SHARED_LIBRARY_PREFIX "")  # Remove "lib" prefix from output to get original
                                     # filename "vis_avs.dll".

add_library(avs_eel OBJECT ${SRC_FILES_EEL2})
if(GCC)
    target_compile_options(avs_eel PRIVATE
        -masm=att           # EEL2 uses AT&T syntax for inline assembly.
        -w                  # No warnings for code we don't control.
    )
endif()
add_library(avs_common OBJECT ${SRC_FILES_AVS_COMMON})

add_library(libavs SHARED
    $<TARGET_OBJECTS:avs_eel>
    $<TARGET_OBJECTS:avs_common>
)

pkg_search_include_dirs_with_64bit_fallback(FFMPEG "libavcodec;libavformat;libswscale" "video")
if(NOT FFMPEG_INCLUDE_DIRS OR NO_FFMPEG)
    add_compile_definitions(NO_FFMPEG)
else()
    include_directories(${FFMPEG_INCLUDE_DIRS})
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
        $<TARGET_OBJECTS:avs_eel>
        $<TARGET_OBJECTS:avs_common>
        ${SRC_FILES_VIS_AVS}
    )
    target_compile_definitions(vis_avs PUBLIC CAN_TALK_TO_WINAMP)

    if(MSVC)
        check_linker_flag(C "/NXCOMPAT:NO" HAS_DISABLE_NXCOMPAT)
        set(NXCOMPAT_DISABLED_FLAG "/NXCOMPAT:NO")
    elseif(GCC)
        check_linker_flag(C "-Wl,--disable-nxcompat" HAS_DISABLE_NXCOMPAT)
        set(NXCOMPAT_DISABLED_FLAG "-Wl,--disable-nxcompat")
    endif()
    if(HAS_DISABLE_NXCOMPAT)
        add_executable(avs-cli avs/avs-cli.c)
        target_link_libraries(avs-cli libavs)
        target_link_options(avs-cli PUBLIC ${NXCOMPAT_DISABLED_FLAG})
    else()
        message(WARNING "Linker flag --disable-nxcompat not supported."
                        " No standalone Win32 C/C++ binaries possible (libs will work)."
                        " Needs binutils 2.36 or higher.")
    endif()
    target_link_libraries(vis_avs
        ddraw  # draw.cpp includes ddraw.h
               # https://docs.microsoft.com/en-us/windows/win32/api/ddraw/nf-ddraw-directdrawcreate
        rpcrt4 # UUIDs
    )
    target_link_libraries(libavs
        rpcrt4 # UUIDs
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
    if(GCC)
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
elseif(LINUX)
    pkg_search_include_dirs_with_64bit_fallback(PIPEWIRE "libpipewire-0.3" "internal audio")
    if(NOT PIPEWIRE_INCLUDE_DIRS OR NO_PIPEWIRE)
        add_compile_definitions(NO_PIPEWIRE)
    else()
        include_directories(${PIPEWIRE_INCLUDE_DIRS})
    endif()
    pkg_search_module(UUID REQUIRED uuid)
    target_link_libraries(libavs ${UUID_LIBRARIES})
    include_directories(avs_common PRIVATE ${UUID_INCLUDE_DIRS})
    target_link_options(avs_common PUBLIC -Wl,-m elf_i386)
    add_executable(avs-cli avs/avs-cli.c)
    target_link_libraries(avs-cli libavs)
endif()


# Rust
if(WIN32)
    if(MSVC)
        set(RUST_TARGET "i686-pc-windows-msvc")
    else()
        set(RUST_TARGET "i686-pc-windows-gnu")
    endif()
elseif(LINUX)
    set(RUST_TARGET "i686-unknown-linux-gnu")
endif()
find_program(RUSTC rustc)
find_program(CARGO cargo)
if(WIN32)  # Disable experimental Rust build on Windows for now, to make the CI green.
    set(SKIP_RUST ON)
elseif(NOT RUSTC)
    message(WARNING "Rust compiler not found. Rust code skipped.")
    set(SKIP_RUST ON)
elseif(NOT CARGO)
    message(WARNING "Cargo not found. Rust code skipped.")
    set(SKIP_RUST ON)
else()
    execute_process(COMMAND ${RUSTC} --target ${RUST_TARGET} --print target-libdir
        OUTPUT_VARIABLE RUST_LIBDIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT IS_DIRECTORY ${RUST_LIBDIR})
        message(WARNING "Rust toolchain for \"${RUST_TARGET}\" not found. Rust code skipped.")
        set(SKIP_RUST ON)
    endif()
endif()
if(NOT SKIP_RUST)
    ExternalProject_Add(avsrs
        DEPENDS libavs
        SOURCE_DIR "${CMAKE_SOURCE_DIR}"
        CONFIGURE_COMMAND ""  # default configure looks for the toolchain file in parent dir
        BINARY_DIR "${CMAKE_SOURCE_DIR}"
        BUILD_COMMAND ${CMAKE_COMMAND} -E env
            "RUSTFLAGS=-L ${CMAKE_BINARY_DIR} -l avs"
            LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}
            PKG_CONFIG_SYSROOT_DIR=/usr/lib32/
            cargo build
                --target ${RUST_TARGET}
                --release
                --target-dir ${CMAKE_BINARY_DIR}
                --bin avs-cli
        INSTALL_COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_BINARY_DIR}/${RUST_TARGET}/release/avs-cli
            ${CMAKE_BINARY_DIR}/avs
        LOG_BUILD ON
    )
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
