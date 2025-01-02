#pragma once

#include "platform.h"

#ifdef _WIN32
#define LIB_PREFIX
#elif __linux__
#define LIB_PREFIX "lib"
#endif

#define LOAD_LIB(NAMESPACE, LIB, FILENAME_SUFFIX)                                \
    library_unload(NAMESPACE::lib##LIB);                                         \
    if ((NAMESPACE::lib##LIB = library_load(LIB_PREFIX #LIB FILENAME_SUFFIX))    \
        == NULL) {                                                               \
        NAMESPACE::load_error = "loading " LIB_PREFIX #LIB FILENAME_SUFFIX ": "; \
        NAMESPACE::load_error += library_error();                                \
        return false;                                                            \
    }

#define STR(x) #x

#define LOAD_FUNC(NAMESPACE, LIB, PREFIX, FUNC)                                  \
    if ((NAMESPACE::FUNC =                                                       \
             (decltype(&PREFIX##_##FUNC))library_get(LIB, STR(PREFIX##_##FUNC))) \
        == NULL) {                                                               \
        NAMESPACE::load_error = STR(PREFIX##_##FUNC) " not in " #LIB;            \
        return false;                                                            \
    }
#define DEFINE_FUNC(NAMESPACE, PREFIX, FUNC) decltype(&PREFIX##_##FUNC) NAMESPACE::FUNC
