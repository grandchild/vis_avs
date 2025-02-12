include(FindPkgConfig)

#[[
This macro searches for include directories of libraries, with 64bit pkg-config
libdir fallback. The intended use is for libraries where the headers are the same for
both 32bit and 64bit, so it makes no difference at compile time.

Only do the fallback when compiling for Linux, because it's the platform that has the
dependency package problem. When cross-compiling from Linux for Windows it would even
introduce Linux include directories into the MinGW build.

Usage:
    pkg_search_include_dirs_with_64bit_fallback(LIBRARY LIB_NAMES SUPPORT)
- LIBRARY: The variable name to store the include directories.
- LIB_NAMES: A ;-list of library names to search for.
- SUPPORT: Description of the feature that depends on the libraries.
]]
macro(pkg_search_include_dirs_with_64bit_fallback LIBRARY LIB_NAMES SUPPORT)
    if(NOT NO_${LIBRARY})
        set(_INCLUDE_DIRS "")
        foreach(lib ${LIB_NAMES})
            pkg_search_module(${LIBRARY}_${lib} ${lib})
            if(NOT ${LIBRARY}_${lib}_FOUND AND LINUX)
                set(PKG_CONFIG_LIBDIR_SAVE $ENV{PKG_CONFIG_LIBDIR})
                set(ENV{PKG_CONFIG_LIBDIR} "/usr/lib/pkgconfig:/usr/lib/x86_64-linux-gnu/pkgconfig")
                pkg_search_module(${LIBRARY}_${lib} ${lib})
                if(NOT ${LIBRARY}_${lib}_FOUND)
                    add_compile_definitions(NO_${LIBRARY})
                    message(WARNING "${lib} headers not found. Building without ${SUPPORT} support.")
                    break()
                else()
                    message(WARNING "Only 64bit ${lib} headers found. Building with ${SUPPORT} support, but 32bit ${lib} is still needed at runtime!")
                endif()
                set(ENV{PKG_CONFIG_LIBDIR} ${PKG_CONFIG_LIBDIR_SAVE})
            endif()
            list(APPEND _INCLUDE_DIRS ${${LIBRARY}_${lib}_INCLUDE_DIRS})
        endforeach()
        list(SORT _INCLUDE_DIRS)
        list(REMOVE_DUPLICATES _INCLUDE_DIRS)
        set(${LIBRARY}_INCLUDE_DIRS ${_INCLUDE_DIRS})
    endif()
endmacro()
