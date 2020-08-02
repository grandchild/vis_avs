cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0048 NEW) # allow setting VERSION directly in project() command

# project name and version
project(vis_avs VERSION 2.82)

if(MSVC)
    add_compile_options(/W4 /WX /Zc:__cplusplus)
else()
    add_compile_options(-g -Wall -Wextra -masm=intel)
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
    -DNO_MMX                # Avoid as much asm code as possible in the beginning to get
                            # to a successful build first.
    -DAVS_MEGABUF_SUPPORT   # No reason to not enable this -> the #ifdefs should probably
                            # be removed eventually.
)

file(GLOB SRC_FILES_NS_EEL avs/ns-eel/*.c)
file(GLOB SRC_FILES_VIS_AVS avs/vis_avs/*.cpp)

add_library(vis_avs SHARED ${SRC_FILES_NS_EEL} ${SRC_FILES_VIS_AVS})

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
