include(FindPkgConfig)

#[[
This macro searches for include directories of libraries, with 64bit PKG_CONFIG_LIBDIR
fallback. The intended use is for _optional_ libraries where the headers are the same
for both 32bit and 64bit & the build environment cannot provide the required 32bit
packages. The libraries are of course still necessary for the feature to work at
runtime.

Only do the fallback when compiling for Linux, because it's the platform that has the
dependency package problem. When cross-compiling from Linux for Windows it would even
introduce Linux include directories into the MinGW build.

Usage:
    pkg_search_include_dirs_with_64bit_fallback(LIBRARY LIB_NAMES SUPPORT)
        LIBRARY:    The name for the include-directories output variable
                    ${LIBRARY}_INCLUDE_DIRS and the NO_${LIBRARY} compiler define.
        LIB_NAMES:  A ;-list of library names to search for.
        SUPPORT:    Description of the feature that depends on the libraries. Used for
                    warning messages.
]]
macro(pkg_search_include_dirs_with_64bit_fallback LIBRARY LIB_NAMES SUPPORT)
    if(NOT NO_${LIBRARY})
        set(_INCLUDE_DIRS "")
        foreach(lib ${LIB_NAMES})
            pkg_search_module(${LIBRARY}_${lib} ${lib})
            if(NOT ${LIBRARY}_${lib}_FOUND)
                if (LINUX)
                    set(PKG_CONFIG_LIBDIR_SAVE $ENV{PKG_CONFIG_LIBDIR})
                    set(ENV{PKG_CONFIG_LIBDIR} "/usr/lib/pkgconfig:/usr/lib/x86_64-linux-gnu/pkgconfig")
                    pkg_search_module(${LIBRARY}_${lib} ${lib})
                    set(ENV{PKG_CONFIG_LIBDIR} ${PKG_CONFIG_LIBDIR_SAVE})
                endif()
                if(NOT ${LIBRARY}_${lib}_FOUND)
                    add_compile_definitions(NO_${LIBRARY})
                    message(WARNING "${lib} headers not found. Building without ${SUPPORT} support.")
                    break()
                else()
                    message(WARNING "Only 64bit ${lib} headers found. Building with ${SUPPORT} support, but 32bit ${lib} is still needed at runtime!")
                endif()
            endif()
            list(APPEND _INCLUDE_DIRS ${${LIBRARY}_${lib}_INCLUDE_DIRS})
        endforeach()
        list(SORT _INCLUDE_DIRS)
        list(REMOVE_DUPLICATES _INCLUDE_DIRS)
        set(${LIBRARY}_INCLUDE_DIRS ${_INCLUDE_DIRS})
    endif()
endmacro()

macro(ensure_platform_vars)
    # Sometimes LINUX is set, but unfortunately on some systems' cmake it's not.
    # Make sure the simple system vars are always set.
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(LINUX ON)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(WIN32 ON)
    endif()
endmacro()