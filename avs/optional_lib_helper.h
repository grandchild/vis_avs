#include "platform.h"

#define LOAD_LIB(NAMESPACE, LIB, FILENAME_SUFFIX)                             \
    library_unload(NAMESPACE::lib##LIB);                                      \
    if ((NAMESPACE::lib##LIB = library_load(#LIB FILENAME_SUFFIX)) == NULL) { \
        NAMESPACE::load_error = "error loading " #LIB FILENAME_SUFFIX;        \
        return false;                                                         \
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
