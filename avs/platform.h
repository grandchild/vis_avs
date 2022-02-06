#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void lock_t;
typedef void dlib_t;
typedef void signal_t;
typedef void thread_t;

/*
 * OS specifics
 */
#ifdef _WIN32

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

#else

// A no-op here, see above.
#define FORCE_FUNCTION_CAST(x) (x)

#endif  // Compiler specifics

/* Including windows.h on MSVC (not GCC!) would define min/max, but we set NOMINMAX for
 * the project, so we can define them all the same for all platforms. */
int min(int a, int b);
int max(int a, int b);

/* The number of milliseconds since the system was booted. May return 0 on error. */
uint64_t timer_ms();
/* A high-precision microsecond resolution counter for performance measuring. */
uint64_t timer_us();
double timer_us_precision();

lock_t* lock_init();
void lock(lock_t* lock_obj);
bool lock_try(lock_t* lock_obj);
void lock_unlock(lock_t* lock_obj);
void lock_destroy(lock_t* lock_obj);

dlib_t* library_load(char* path);
void* library_get(dlib_t* library, char* func_name);
void library_unload(dlib_t* library);

#define WAIT_INFINITE -1
signal_t* signal_create_single();
signal_t* signal_create_broadcast();
void signal_set(signal_t* signal);
// All wait functions return NULL if timeout was hit or other error occurred.
signal_t* signal_wait(signal_t* signal, int32_t wait_ms);
signal_t* signal_wait_any(signal_t** signals, uint32_t num_signals, int32_t wait_ms);
signal_t* signal_wait_all(signal_t** signals, uint32_t num_signals, int32_t wait_ms);
void signal_destroy(signal_t* signal);

thread_t* thread_create(uint32_t (*func)(void* data), void* data);
// All join functions return false if timeout was hit or other error occurred.
bool thread_join(thread_t* thread, int32_t wait_ms);
bool thread_join_all(thread_t** threads, uint32_t num_threads, int32_t wait_ms);
void thread_destroy(thread_t* thread);

int create_directory(char* path);

#ifdef __cplusplus
}
#endif
