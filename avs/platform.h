#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void lock_t;
typedef void dlib_t;

/*
 * OS specifics
 */
#ifdef __WIN32

#elif defined __linux__

#include <stdlib.h>

// typedef pthread_mutex_t lock_t;

#endif  // OS specifics

/*
 * Compiler specifics
 */
#ifdef __GNUC__

/*
GCCs -Wcast-function-type is a bit overzealous when casting dynamic library function
pointers (whose signatures _will_ be mismatched most of the time). The warning can be
suppressed by first casting the function pointer to the special case void(*)(). This
macro makes this a bit less cryptic and explicit, while still keeping the warning
enabled for the cases where it might actually catch something.
*/
#define FORCE_FUNCTION_CAST(x) (x)(void (*)())

// These are builtins in MSVC
int min(int a, int b);
int max(int a, int b);

#else

// A no-op here, see above.
#define FORCE_FUNCTION_CAST(x) (x)

#endif  // Compiler specifics

/* Return the number of milliseconds since the system was booted. Return 0 on error. */
uint64_t timer_ms();

lock_t* lock_init();
void lock(lock_t* lock_obj);
bool lock_try(lock_t* lock_obj);
void lock_unlock(lock_t* lock_obj);
void lock_destroy(lock_t* lock_obj);

dlib_t* library_load(char* path);
void* library_get(dlib_t* library, char* func_name);
void library_unload(dlib_t* library);

int create_directory(char* path);

#ifdef __cplusplus
}
#endif
